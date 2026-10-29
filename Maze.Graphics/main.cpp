#include <SFML/Graphics.hpp>
#include <exception>
#include <filesystem>
#include <iostream>

#include "GameView.h"
#include "Maze.Game.h"
#include "Maze.Validation.h"
#include "menu.h"

int main(int, char* argv[])
{
    sf::RenderWindow window(sf::VideoMode({ 900u, 600u }), "NXRP - Maze");
    window.setMinimumSize(sf::Vector2u{ 640u, 560u });
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    const auto fontPath = std::filesystem::absolute(argv[0]).parent_path()
        / "assets" / "fonts" / "SourceSans3-Semibold.ttf";
    sf::Font font;
    if (!font.openFromFile(fontPath))
    {
        std::cerr << "Could not load the font: " << fontPath << '\n';
        return 1;
    }
    font.setSmooth(false);

    GameState game;
    Menu menu(font);
    GameView gameView(font);
    bool showingGame = false;
    const auto canContinue = [&game] {
        return game.hasActiveGame()
            && (game.phase == GamePhase::Playing || game.phase == GamePhase::Paused);
    };
    const auto resize = [&window, &menu, &gameView](sf::Vector2u size)
    {
        if (size.x == 0 || size.y == 0) return;
        window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, {
            static_cast<float>(size.x), static_cast<float>(size.y)
        })));
        menu.resize(size);
        gameView.resize(size);
    };
    resize(window.getSize());

    while (window.isOpen())
    {
        menu.setHasActiveGame(canContinue());
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
                break;
            }
            if (const auto* resized = event->getIf<sf::Event::Resized>())
                resize(resized->size);

            if (showingGame)
            {
                const auto* key = event->getIf<sf::Event::KeyPressed>();
                if (key && key->code == sf::Keyboard::Key::Escape)
                {
                    if (game.phase == GamePhase::Playing) game.phase = GamePhase::Paused;
                    showingGame = false;
                    menu.setHasActiveGame(canContinue());
                    menu.showMain();
                    window.setTitle("NXRP - Maze");
                }
                else gameView.handleEvent(*event, game);
                continue;
            }

            switch (menu.handleEvent(*event, window))
            {
            case MenuAction::HowToPlay:
                menu.showHowToPlay();
                break;
            case MenuAction::Settings:
                menu.showSettings(game.selectedSize);
                break;
            case MenuAction::Back:
                menu.showMain();
                break;
            case MenuAction::ApplySettings:
                if (isValidMazeSize(menu.draftSize()))
                {
                    game.selectedSize = menu.draftSize();
                    menu.showMain();
                    menu.showMessage("Saved size: " + std::to_string(game.selectedSize.rows)
                        + " x " + std::to_string(game.selectedSize.cols) + ". Demo uses 10 x 10.");
                }
                break;
            case MenuAction::NewGame:
                try
                {
                    const MazeSize selectedSize = game.selectedSize;
                    startGame(game, createDemoMaze());
                    // Keep the next-game preference separate from this fixed demo's size.
                    game.selectedSize = selectedSize;
                    gameView.reset();
                    showingGame = true;
                    window.setTitle("NXRP - Fixed 10x10 demo");
                }
                catch (const std::exception& error)
                {
                    menu.showMessage("Could not start the demo maze.");
                    std::cerr << "Starting the demo failed: " << error.what() << '\n';
                }
                break;
            case MenuAction::Continue:
                if (canContinue())
                {
                    game.phase = GamePhase::Playing;
                    gameView.reset();
                    showingGame = true;
                    window.setTitle("NXRP - Fixed 10x10 demo");
                }
                break;
            case MenuAction::Exit:
                window.close();
                break;
            case MenuAction::None:
                break;
            }
            if (!window.isOpen()) break;
        }
        if (!window.isOpen()) break;

        window.clear(sf::Color{ 18, 22, 31 });
        if (showingGame) gameView.draw(window, game);
        else
        {
            menu.update(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
            menu.draw(window);
        }
        window.display();
    }
}
