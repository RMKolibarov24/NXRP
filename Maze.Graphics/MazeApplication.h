#pragma once

#include "GameView.h"
#include "menu.h"

// Owns the current game and routes events between gameplay and menu pages.
class MazeApplication
{
public:
    explicit MazeApplication(const sf::Font& font);

    void resize(sf::Vector2u size);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Vector2f mousePosition);
    void draw(sf::RenderTarget& target);
    const GameState& gameState() const noexcept;

private:
    bool canContinue() const noexcept;
    void showMainMenu();
    void beginNewGame();

    GameState game_;
    Menu menu_;
    GameView gameView_;
};
