#pragma once

#include "GameView.h"
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
    void update(sf::Vector2f mousePosition, Clock::time_point now = Clock::now());
    void draw(sf::RenderTarget& target);
    const GameState& gameState() const noexcept;

private:
    bool canContinue() const noexcept;
    bool canRestart() const noexcept;
    void showMainMenu();
    void beginNewGame();
    void restartCurrentGame();
    void updateTime(Clock::time_point now);

    GameState game_;
    std::filesystem::path settingsPath_;
    Clock::time_point previousTick_;
    Menu menu_;
    GameView gameView_;
};
