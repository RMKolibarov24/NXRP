#pragma once

#include <SFML/Graphics.hpp>
#include <string>

#include "GameState.h"

class GameView
{
public:
    explicit GameView(const sf::Font& font);

    void resize(sf::Vector2u size);
    void reset();
    void handleEvent(const sf::Event& event, GameState& game);
    void draw(sf::RenderTarget& target, const GameState& game);

private:
    sf::Vector2u viewport_{ 900u, 600u };
    std::string notice_;
    sf::Text title_;
    sf::Text seed_;
    sf::Text controls_;
    sf::Text legend_;
    sf::Text position_;
    sf::Text statistics_;
    sf::Text message_;
    sf::Text cellLabel_;
    sf::VertexArray cells_{ sf::PrimitiveType::Triangles };
};
