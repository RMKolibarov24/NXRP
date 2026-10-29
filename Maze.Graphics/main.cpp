#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>

#include "MazeApplication.h"

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

    MazeApplication application(font);
    application.resize(window.getSize());
    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            application.handleEvent(*event, window);
            if (!window.isOpen()) break;
        }
        if (!window.isOpen()) break;

        application.update(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
        window.clear(sf::Color{ 18, 22, 31 });
        application.draw(window);
        window.display();
    }
}
