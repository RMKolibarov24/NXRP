#include "Multiplayer.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <chrono>
#include <utility>

namespace
{
    constexpr std::size_t maxMessage = 4096;
    constexpr std::size_t maxQueued = 65536;

    bool nonblocking(SOCKET socket)
    {
        u_long enabled = 1;
        const int noDelay=1;setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&noDelay),sizeof(noDelay));
        return ioctlsocket(socket, FIONBIO, &enabled) == 0;
    }

    struct Connection
    {
        SOCKET socket = INVALID_SOCKET;
        unsigned generation = 0;
        std::string input;
        std::deque<std::string> pending;
        std::size_t sent = 0, queuedBytes = 0;
    };
}

Multiplayer::~Multiplayer() { stop(); }

void Multiplayer::host(std::uint16_t port)
{
    stop(); stopping_.store(false); status_.store(Status::Listening);
    worker_ = std::thread(&Multiplayer::run, this, true, std::string{}, port);
}

void Multiplayer::join(std::string host, std::uint16_t port)
{
    stop(); stopping_.store(false); status_.store(Status::Connecting);
    worker_ = std::thread(&Multiplayer::run, this, false, std::move(host), port);
}

void Multiplayer::stop()
{
    stopping_.store(true);
    if (worker_.joinable()) worker_.join();
    std::lock_guard<std::mutex> lock(mutex_);
    incoming_.clear(); outgoing_.clear(); incomingBytes_ = outgoingBytes_ = 0;
    connectedMask_.store(0); status_.store(Status::Idle);
}

bool Multiplayer::send(std::string message, int peer)
{
    if (peer < 0 || peer > maxGuests || !(connectedMask() & (1u << peer)) ||
        message.empty() || message.size() > maxMessage || message.find_first_of("\r\n") != std::string::npos) return false;
    message.push_back('\n');
    std::lock_guard<std::mutex> lock(mutex_);
    if (outgoingBytes_ + message.size() > maxQueued || (message.rfind("V ",0)==0 && outgoingBytes_>8192)) return false;
    outgoingBytes_ += message.size();
    outgoing_.push_back({ MultiplayerMessage::Kind::Data, peer, std::move(message), peerGeneration_[peer].load() });
    return true;
}

std::vector<MultiplayerMessage> Multiplayer::receive()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MultiplayerMessage> result;
    result.reserve(incoming_.size());
    while (!incoming_.empty()) { result.push_back(std::move(incoming_.front())); incoming_.pop_front(); }
    incomingBytes_ = 0;
    return result;
}

void Multiplayer::run(bool hosting, std::string host, std::uint16_t port)
{
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { status_.store(Status::Error); return; }
    SOCKET listener = INVALID_SOCKET;
    std::array<Connection, 4> peers{};
    bool failed = false;
    auto emit = [&](MultiplayerMessage::Kind kind, int peer, std::string message = {})
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (kind == MultiplayerMessage::Kind::Data && incomingBytes_ + message.size() > maxQueued) return false;
        incomingBytes_ += message.size();
        incoming_.push_back({ kind, peer, std::move(message), peers[peer].generation });
        return true;
    };
    auto drop = [&](int slot)
    {
        auto& connection = peers[slot];
        if (connection.socket == INVALID_SOCKET) return;
        closesocket(connection.socket); connection.socket = INVALID_SOCKET;
        connection.input.clear(); connection.pending.clear(); connection.sent = connection.queuedBytes = 0;
        connectedMask_.fetch_and(~(1u << slot));
        if (hosting) emit(MultiplayerMessage::Kind::Left, slot);
    };

    if (hosting)
    {
        listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in address{}; address.sin_family = AF_INET; address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (listener == INVALID_SOCKET || !nonblocking(listener) ||
            bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || listen(listener, maxGuests + 1) != 0)
            failed = true;
    }
    else
    {
        addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP;
        addrinfo* addresses = nullptr;
        const auto portText = std::to_string(port);
        if (getaddrinfo(host.c_str(), portText.c_str(), &hints, &addresses) != 0) failed = true;
        bool connected = false;
        for (auto* address = addresses; address && !connected && !stopping_.load(); address = address->ai_next)
        {
            SOCKET peer = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
            if (peer == INVALID_SOCKET) continue;
            if (!nonblocking(peer)) { closesocket(peer); continue; }
            if (connect(peer, address->ai_addr, static_cast<int>(address->ai_addrlen)) == 0) connected = true;
            else if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
                while (!stopping_.load() && std::chrono::steady_clock::now() < deadline)
                {
                    fd_set writable, errors; FD_ZERO(&writable); FD_ZERO(&errors);
                    FD_SET(peer, &writable); FD_SET(peer, &errors); timeval timeout{ 0, 50000 };
                    if (select(0, nullptr, &writable, &errors, &timeout) > 0)
                    {
                        int error = 0, length = sizeof(error);
                        connected = !FD_ISSET(peer, &errors) &&
                            getsockopt(peer, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length) == 0 && error == 0;
                        break;
                    }
                }
            }
            if (connected) peers[0].socket = peer;
            else closesocket(peer);
        }
        if (addresses) freeaddrinfo(addresses);
        if (!connected && !stopping_.load()) failed = true;
        if (connected)
        {
            peers[0].generation = ++peerGeneration_[0];
            connectedMask_.store(1u); status_.store(Status::Connected);
        }
    }

    while (!failed && !stopping_.load())
    {
        std::deque<MultiplayerMessage> outgoing;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outgoing.swap(outgoing_); outgoingBytes_ = 0;
        }
        for (auto& message : outgoing)
        {
            auto& peer = peers[message.peer];
            if (peer.socket == INVALID_SOCKET || message.generation != peer.generation) continue;
            // Speech is disposable. Keep it from filling a slow peer's queue
            // ahead of player/world updates or causing a gameplay disconnect.
            if(message.text.rfind("V ",0)==0 && peer.queuedBytes>4096)continue;
            if (peer.queuedBytes + message.text.size() > maxQueued) { drop(message.peer); continue; }
            peer.queuedBytes += message.text.size(); peer.pending.push_back(std::move(message.text));
        }

        fd_set readers, writers; FD_ZERO(&readers); FD_ZERO(&writers);
        if (listener != INVALID_SOCKET) FD_SET(listener, &readers);
        for (auto& peer : peers) if (peer.socket != INVALID_SOCKET)
        {
            FD_SET(peer.socket, &readers);
            if (!peer.pending.empty()) FD_SET(peer.socket, &writers);
        }
        if (!hosting && peers[0].socket == INVALID_SOCKET) break;
        timeval timeout{ 0, 20000 };
        const int available = select(0, &readers, &writers, nullptr, &timeout);
        if (available == SOCKET_ERROR) { failed = true; break; }

        if (listener != INVALID_SOCKET && FD_ISSET(listener, &readers))
        {
            SOCKET socket = accept(listener, nullptr, nullptr);
            if (socket != INVALID_SOCKET)
            {
                int slot = 1; while (slot <= maxGuests && peers[slot].socket != INVALID_SOCKET) ++slot;
                if (slot > maxGuests) { ::send(socket, "F\n", 2, 0); closesocket(socket); }
                else if (!nonblocking(socket)) closesocket(socket);
                else
                {
                    peers[slot].socket = socket; peers[slot].generation = ++peerGeneration_[slot];
                    emit(MultiplayerMessage::Kind::Joined, slot);
                    connectedMask_.fetch_or(1u << slot);
                }
            }
        }
        for (int slot = 0; slot <= maxGuests; ++slot)
        {
            auto& peer = peers[slot];
            if (peer.socket == INVALID_SOCKET) continue;
            bool disconnected = false;
            if (FD_ISSET(peer.socket, &readers))
            {
                char bytes[2048]; const int count = recv(peer.socket, bytes, sizeof(bytes), 0);
                if (count <= 0) disconnected = count == 0 || WSAGetLastError() != WSAEWOULDBLOCK;
                else
                {
                    peer.input.append(bytes, static_cast<std::size_t>(count));
                    std::size_t newline;
                    while ((newline = peer.input.find('\n')) != std::string::npos)
                    {
                        if (newline == 0 || newline > maxMessage ||
                            !emit(MultiplayerMessage::Kind::Data, slot, peer.input.substr(0, newline)))
                        { disconnected = true; break; }
                        peer.input.erase(0, newline + 1);
                    }
                    if (peer.input.size() > maxMessage) disconnected = true;
                }
            }
            if (!disconnected && !peer.pending.empty() && FD_ISSET(peer.socket, &writers))
            {
                const auto& message = peer.pending.front();
                const int count = ::send(peer.socket, message.data() + peer.sent, static_cast<int>(message.size() - peer.sent), 0);
                if (count > 0)
                {
                    peer.sent += static_cast<std::size_t>(count);
                    if (peer.sent == message.size())
                    { peer.queuedBytes -= message.size(); peer.pending.pop_front(); peer.sent = 0; }
                }
                else if (count == 0 || WSAGetLastError() != WSAEWOULDBLOCK) disconnected = true;
            }
            if (disconnected) drop(slot);
        }
        if (hosting) status_.store(connectedMask() ? Status::Connected : Status::Listening);
    }
    for (int slot = 0; slot <= maxGuests; ++slot) drop(slot);
    if (listener != INVALID_SOCKET) closesocket(listener);
    WSACleanup();
    if (!stopping_.load()) status_.store(failed ? Status::Error : Status::Disconnected);
}
