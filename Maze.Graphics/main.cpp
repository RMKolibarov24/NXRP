#include <SFML/Graphics.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>

int main(int, char* argv[])
{
    sf::RenderWindow window(
        sf::VideoMode({ 900u, 600u }), "NXRP - Maze"
    );
    window.setFramerateLimit(60);

    // Resolve resources from the executable directory, not the working directory.
    sf::Font font;
    const auto fontPath = std::filesystem::absolute(argv[0]).parent_path()
        / "assets" / "fonts" / "SourceSans3-Semibold.ttf";

    if (!font.openFromFile(fontPath))
    {
        std::cerr << "Could not load the font: " << fontPath << '\n';
        return 1;
    }
    font.setSmooth(false);

    // Create the button background
    sf::RectangleShape button({ 260.f, 70.f });
    button.setPosition({ 320.f, 265.f });

    // Create and center the label
    sf::Text label(font, "New Game", 28);
    label.setFillColor(sf::Color::White);

    const auto bounds = label.getLocalBounds();
    // Keep glyphs on whole pixels instead of sampling between texture pixels.
    label.setOrigin({
        std::round(bounds.position.x + bounds.size.x / 2.f),
        std::round(bounds.position.y + bounds.size.y / 2.f)
    });
    label.setPosition(
        button.getPosition() + button.getSize() / 2.f
    );

    const auto resizeLayout = [&window, &button, &label](sf::Vector2u size)
    {
        if (size.x == 0 || size.y == 0)
        {
            return;
        }

        // Keep one view unit equal to one window pixel when resizing.
        const sf::Vector2f viewSize{
            static_cast<float>(size.x), static_cast<float>(size.y)
        };
        window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, viewSize)));
        button.setPosition({
            std::round((viewSize.x - button.getSize().x) / 2.f),
            std::round((viewSize.y - button.getSize().y) / 2.f)
        });
        label.setPosition(button.getPosition() + button.getSize() / 2.f);
    };
    resizeLayout(window.getSize());

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                resizeLayout(resized->size);
            }

            if (const auto* click =
                event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (click->button == sf::Mouse::Button::Left)
                {
                    const auto position =
                        window.mapPixelToCoords(click->position);

                    if (button.getGlobalBounds().contains(position))
                    {
                        // Part 1 keeps the existing demo; gameplay comes later.
                        window.setTitle("New Game selected!");
                    }
                }
            }
        }

        if (!window.isOpen())
        {
            break;
        }

        // Change the color when the mouse is over the button
        const auto mousePosition = window.mapPixelToCoords(
            sf::Mouse::getPosition(window)
        );

        const bool hovered =
            button.getGlobalBounds().contains(mousePosition);

        button.setFillColor(
            hovered
            ? sf::Color{ 80, 140, 230 }
            : sf::Color{ 45, 90, 170 }
        );

        window.clear(sf::Color{ 25, 25, 30 });
        window.draw(button);
        window.draw(label);
        window.display();
    }
}
