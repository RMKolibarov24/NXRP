#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ 900u, 600u }),
        "NXRP - Maze"
    );

    window.setFramerateLimit(60);

    sf::RectangleShape player({ 32.f, 32.f });
    player.setPosition({ 50.f, 50.f });
    player.setFillColor(sf::Color::Cyan);

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        if (!window.isOpen())
        {
            break;
        }

        window.clear(sf::Color{ 25, 25, 30 });
        window.draw(player);
        window.display();
    }

    return 0;
}