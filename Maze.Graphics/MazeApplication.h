#pragma once

#include "GameView.h"
#include "Soundscape.h"
#include "VoiceChat.h"
#include "Multiplayer.h"
#include "menu.h"

#include <filesystem>
#include <chrono>

// Owns the current game and routes events between gameplay and menu pages.
class MazeApplication
{
public:
    using Clock = std::chrono::steady_clock;

    explicit MazeApplication(const sf::Font& font);
    MazeApplication(const sf::Font& font, std::filesystem::path settingsPath,
        Clock::time_point now = Clock::now());

    void resize(sf::Vector2u size);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window,
        Clock::time_point now = Clock::now());
    void update(sf::RenderWindow& window, Clock::time_point now = Clock::now());
    void draw(sf::RenderTarget& target);
    const GameState& gameState() const noexcept;

private:
    bool canContinue() const noexcept;
    bool canRestart() const noexcept;
    void showMainMenu();
    void beginNewGame();
    void restartCurrentGame();
    void updateTime(Clock::time_point now);
    void updateNetwork(float seconds);
    void sendPlayer();
    void receiveVoice(const std::string& message,int peer);
    void transmitVoice(VoicePacket packet);
    void playVoice(const VoicePacket& packet);
    void updateVoice(sf::RenderWindow& window);
    void endNetwork();

    bool mouseCaptured_ = false;
    Soundscape soundscape_;
    VoiceChat voice_;
    bool voiceEnabled_=true;
    unsigned mutedVoices_=0,voiceGeneration_=0;
    std::array<std::uint32_t,4> voiceSequence_{};
    std::array<bool,4> voiceSequenceSeen_{};
    std::array<float,4> voiceTokens_{{6.f,6.f,6.f,6.f}},voiceSpeaking_{};
    std::array<unsigned,4> voiceReceived_{};
    Multiplayer network_;
    enum class NetworkRole { Solo, Host, Guest } networkRole_ = NetworkRole::Solo;
    Multiplayer::Status networkStatus_ = Multiplayer::Status::Idle;
    bool networkPaused_ = false;
    std::uint32_t networkGeneration_ = 0, joinedGeneration_ = 0;
    int networkSlot_ = 0;
    float networkSendClock_ = 0.f, networkWaitClock_ = 0.f;
    GameState game_;
    std::filesystem::path settingsPath_;
    Clock::time_point previousTick_;
    Clock::time_point previousFrame_;
    Menu menu_;
    GameView gameView_;
};
