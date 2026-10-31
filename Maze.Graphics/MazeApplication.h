#pragma once

#include "GameView.h"
#include "menu.h"

#include <filesystem>

// Owns the current game and routes events between gameplay and menu pages.
class MazeApplication
{
public:
    explicit MazeApplication(const sf::Font& font);
    MazeApplication(const sf::Font& font, std::filesystem::path settingsPath);

    void resize(sf::Vector2u size);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Vector2f mousePosition);
    void draw(sf::RenderTarget& target);
    const GameState& gameState() const noexcept;

private:
    bool canContinue() const noexcept;
    bool canRestart() const noexcept;
    void showMainMenu();
    void beginNewGame();
    void restartCurrentGame();

    GameState game_;
    std::filesystem::path settingsPath_;
    Menu menu_;
    GameView gameView_;
};
