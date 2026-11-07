#pragma once

#include "GameView.h"
#include "Soundscape.h"
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
    void endNetwork();

    bool mouseCaptured_ = false;
    Soundscape soundscape_;
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
