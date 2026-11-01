#include "MazeApplication.h"
#include "Maze.Game.h"
#include "Maze.Generation.h"
#include "Maze.Settings.h"
#include "Maze.Validation.h"
#include "SettingsPath.h"

#include <exception>
#include <iostream>
#include <utility>

MazeApplication::MazeApplication(const sf::Font& font)
    : MazeApplication(font, settingsFilePath())
{
}

MazeApplication::MazeApplication(const sf::Font& font, std::filesystem::path settingsPath,
    Clock::time_point now)
    : settingsPath_(std::move(settingsPath)), previousTick_(now), menu_(font), gameView_(font)
{
    game_.selectedSize = loadMazeSizePreferences(settingsPath_);
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

bool MazeApplication::canRestart() const noexcept
{
    return canContinue() || (game_.hasActiveGame() && game_.phase == GamePhase::Won);
}

void MazeApplication::showMainMenu()
{
    if (game_.phase == GamePhase::Playing) game_.phase = GamePhase::Paused;
    menu_.setHasActiveGame(canContinue());
    menu_.setCanRestart(canRestart());
    menu_.showMain();
}

void MazeApplication::beginNewGame()
{
    try
    {
        const auto seed = game_.selectedSeed ? *game_.selectedSeed : generateMazeSeed();
        startGame(game_, generateMaze(game_.selectedSize, seed), seed);
        gameView_.reset();
    }
    catch (const std::exception& error)
    {
        menu_.showMessage("Could not start a new maze. Please try again.");
        std::cerr << "Starting a new maze failed: " << error.what() << '\n';
    }
}

void MazeApplication::restartCurrentGame()
{
    if (restartGame(game_)) gameView_.reset();
}

void MazeApplication::updateTime(Clock::time_point now)
{
    if (now <= previousTick_) return;
    advanceGameTime(game_, now - previousTick_);
    previousTick_ = now;
}

void MazeApplication::handleEvent(const sf::Event& event, sf::RenderWindow& window,
    Clock::time_point now)
{
    // Account for time in the old phase before an event pauses, resumes or wins.
    updateTime(now);
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
        else if (key && key->code == sf::Keyboard::Key::R) restartCurrentGame();
        else
        {
            gameView_.handleEvent(event, game_);
            if (game_.phase == GamePhase::Won)
            {
                menu_.setHasActiveGame(false);
                menu_.setCanRestart(canRestart());
                menu_.showVictory(game_.maze.size(), game_.statistics, game_.mazeSeed);
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
        menu_.showSettings(game_.selectedSize, game_.selectedSeed);
        break;
    case MenuAction::Back:
        showMainMenu();
        break;
    case MenuAction::ApplySettings:
    {
        std::optional<std::uint32_t> seed;
        if (isValidMazeSize(menu_.draftSize()) && menu_.draftSeed(seed))
        {
            game_.selectedSize = menu_.draftSize();
            game_.selectedSeed = seed;
            const bool saved = saveMazeSizePreferences(settingsPath_, game_.selectedSize);
            showMainMenu();
            if (saved)
            {
                menu_.showMessage("Saved size: " + std::to_string(game_.selectedSize.rows)
                    + " x " + std::to_string(game_.selectedSize.cols) + ". Next seed: "
                    + (seed ? std::to_string(*seed) : "random") + ".");
            }
            else
            {
                menu_.showMessage("Size set for this launch only. Could not save preferences.");
                std::cerr << "Could not save maze size preferences to: " << settingsPath_ << '\n';
            }
        }
        break;
    }
    case MenuAction::NewGame:
        beginNewGame();
        break;
    case MenuAction::RestartMaze:
        restartCurrentGame();
        break;
    case MenuAction::Continue:
        if (canContinue())
        {
            game_.phase = GamePhase::Playing;
            gameView_.resetFeedback();
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

void MazeApplication::update(sf::Vector2f mousePosition, Clock::time_point now)
{
    updateTime(now);
    if (game_.phase != GamePhase::Playing) menu_.update(mousePosition);
}

void MazeApplication::draw(sf::RenderTarget& target)
{
    if (game_.phase == GamePhase::Playing) gameView_.draw(target, game_);
    else menu_.draw(target);
}
