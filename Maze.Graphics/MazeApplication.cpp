#include "MazeApplication.h"
#include "Maze.Game.h"
#include "Maze.Generation.h"
#include "Maze.Validation.h"

#include <exception>
#include <iostream>

MazeApplication::MazeApplication(const sf::Font& font)
    : menu_(font), gameView_(font)
{
    showMainMenu();
}

void MazeApplication::resize(sf::Vector2u size)
{
    menu_.resize(size);
    gameView_.resize(size);
}

const GameState& MazeApplication::gameState() const noexcept
{
    return game_;
}

bool MazeApplication::canContinue() const noexcept
{
    return game_.hasActiveGame()
        && (game_.phase == GamePhase::Playing || game_.phase == GamePhase::Paused);
}

void MazeApplication::showMainMenu()
{
    if (game_.phase == GamePhase::Playing) game_.phase = GamePhase::Paused;
    menu_.setHasActiveGame(canContinue());
    menu_.showMain();
}

void MazeApplication::beginNewGame()
{
    try
    {
        startGame(game_, generateMaze(game_.selectedSize));
        gameView_.reset();
    }
    catch (const std::exception& error)
    {
        menu_.showMessage("Could not start a new maze. Please try again.");
        std::cerr << "Starting a new maze failed: " << error.what() << '\n';
    }
}

void MazeApplication::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (event.is<sf::Event::Closed>())
    {
        endGame(game_);
        window.close();
        return;
    }
    if (const auto* resized = event.getIf<sf::Event::Resized>())
    {
        if (resized->size.x != 0 && resized->size.y != 0)
        {
            window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, {
                static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)
            })));
            resize(resized->size);
        }
        return;
    }

    if (game_.phase == GamePhase::Playing)
    {
        const auto* key = event.getIf<sf::Event::KeyPressed>();
        if (key && key->code == sf::Keyboard::Key::Escape) showMainMenu();
        else
        {
            gameView_.handleEvent(event, game_);
            if (game_.phase == GamePhase::Won)
            {
                menu_.setHasActiveGame(false);
                menu_.showVictory(game_.maze.size());
            }
        }
        return;
    }

    switch (menu_.handleEvent(event, window))
    {
    case MenuAction::HowToPlay:
        menu_.showHowToPlay();
        break;
    case MenuAction::Settings:
        menu_.showSettings(game_.selectedSize);
        break;
    case MenuAction::Back:
        showMainMenu();
        break;
    case MenuAction::ApplySettings:
        if (isValidMazeSize(menu_.draftSize()))
        {
            game_.selectedSize = menu_.draftSize();
            showMainMenu();
            menu_.showMessage("Saved size: " + std::to_string(game_.selectedSize.rows)
                + " x " + std::to_string(game_.selectedSize.cols) + ". Applies to New Game.");
        }
        break;
    case MenuAction::NewGame:
        beginNewGame();
        break;
    case MenuAction::Continue:
        if (canContinue())
        {
            game_.phase = GamePhase::Playing;
            gameView_.reset();
        }
        break;
    case MenuAction::Exit:
        endGame(game_);
        window.close();
        break;
    case MenuAction::None:
        break;
    }
}

void MazeApplication::update(sf::Vector2f mousePosition)
{
    if (game_.phase != GamePhase::Playing) menu_.update(mousePosition);
}

void MazeApplication::draw(sf::RenderTarget& target)
{
    if (game_.phase == GamePhase::Playing) gameView_.draw(target, game_);
    else menu_.draw(target);
}
