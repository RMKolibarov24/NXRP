#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct MultiplayerMessage
{
    enum class Kind { Joined, Data, Left } kind = Kind::Data;
    int peer = 0; // Host assigns guest slots 1-3. A guest receives from slot 0.
    std::string text;
    unsigned generation = 0;
};

// The host accepts up to three TCP peers on a worker thread. Messages are
// bounded and ordered per peer; game-state validation belongs to the caller.
class Multiplayer
{
public:
    enum class Status { Idle, Listening, Connecting, Connected, Disconnected, Error };
    static constexpr std::uint16_t hostPort = 24816;
    static constexpr int maxGuests = 3;

    Multiplayer() = default;
    ~Multiplayer();
    Multiplayer(const Multiplayer&) = delete;
    Multiplayer& operator=(const Multiplayer&) = delete;

    void host(std::uint16_t port = hostPort);
    void join(std::string host, std::uint16_t port);
    void stop();
    Status status() const noexcept { return status_.load(); }
    bool send(std::string message, int peer = 0);
    std::vector<MultiplayerMessage> receive();
    unsigned connectedMask() const noexcept { return connectedMask_.load(); }

private:
    void run(bool hosting, std::string host, std::uint16_t port);

    std::atomic<bool> stopping_{ false };
    std::atomic<Status> status_{ Status::Idle };
    std::atomic<unsigned> connectedMask_{ 0 };
    std::array<std::atomic<unsigned>, 4> peerGeneration_{};
    std::thread worker_;
    std::mutex mutex_;
    std::deque<MultiplayerMessage> incoming_, outgoing_;
    std::size_t incomingBytes_ = 0, outgoingBytes_ = 0;
};
