#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>

#include "GameState.h"
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
    const auto resize = [&window, &menu](sf::Vector2u size)
    {
        if (size.x == 0 || size.y == 0) return;
        window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, {
            static_cast<float>(size.x), static_cast<float>(size.y)
        })));
        menu.resize(size);
    };
    resize(window.getSize());

    while (window.isOpen())
    {
        menu.setHasActiveGame(game.hasActiveGame());
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
                break;
            }
            if (const auto* resized = event->getIf<sf::Event::Resized>())
                resize(resized->size);

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
                        + " x " + std::to_string(game.selectedSize.cols) + ".");
                }
                break;
            case MenuAction::NewGame:
                // The controller receives a request; no game is created yet.
                menu.showMessage("Maze creation is not available yet.");
                break;
            case MenuAction::Continue:
                menu.showMessage("No playable game is available.");
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

        menu.update(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
        window.clear(sf::Color{ 18, 22, 31 });
        menu.draw(window);
        window.display();
    }
}
