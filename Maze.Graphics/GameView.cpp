#include "GameView.h"
#include "Maze.Game.h"

#include <algorithm>
#include <cmath>

namespace
{
    const sf::Color wallColor{ 54, 65, 84 };
    const sf::Color passageColor{ 212, 220, 230 };
    const sf::Color exitColor{ 73, 190, 126 };
    const sf::Color playerColor{ 255, 196, 74 };
    const sf::Color textColor{ 235, 240, 248 };
    const sf::Color mutedColor{ 151, 164, 184 };

    void centerText(sf::Text& text, sf::Vector2f center)
    {
        const auto bounds = text.getLocalBounds();
        text.setOrigin({ std::round(bounds.position.x + bounds.size.x / 2.f),
            std::round(bounds.position.y + bounds.size.y / 2.f) });
        text.setPosition({ std::round(center.x), std::round(center.y) });
    }
}

GameView::GameView(const sf::Font& font)
    : title_(font, "MAZE", 30), controls_(font, "", 18),
      legend_(font, "", 17), position_(font, "", 22), message_(font, "", 20),
      cellLabel_(font, "", 16)
{
    title_.setFillColor(textColor);
    controls_.setFillColor(mutedColor);
    legend_.setFillColor(textColor);
    position_.setFillColor(textColor);
    cellLabel_.setFillColor({ 18, 22, 31 });
    reset();
}

void GameView::resize(sf::Vector2u size)
{
    if (size.x != 0 && size.y != 0) viewport_ = size;
}

void GameView::reset()
{
    notice_ = "Find the green exit.";
}

void GameView::handleEvent(const sf::Event& event, GameState& game)
{
    const auto* key = event.getIf<sf::Event::KeyPressed>();
    if (!key || game.phase != GamePhase::Playing) return;

    MoveDirection direction;
    switch (key->code)
    {
    case sf::Keyboard::Key::Up:    direction = MoveDirection::Up; break;
    case sf::Keyboard::Key::Down:  direction = MoveDirection::Down; break;
    case sf::Keyboard::Key::Left:  direction = MoveDirection::Left; break;
    case sf::Keyboard::Key::Right: direction = MoveDirection::Right; break;
    default: return;
    }

    switch (tryMove(game, direction))
    {
    case MoveResult::Blocked: notice_ = "Blocked: wall or maze boundary."; break;
    case MoveResult::Moved:   notice_ = "Move successful."; break;
    case MoveResult::Won:     notice_ = "You reached the exit! Esc returns to the menu."; break;
    }
}

void GameView::draw(sf::RenderTarget& target, const GameState& game)
{
    const float width = static_cast<float>(viewport_.x);
    const float height = static_cast<float>(viewport_.y);
    title_.setPosition({ 24.f, 14.f });
    controls_.setString(std::to_string(game.maze.size().rows) + " x "
        + std::to_string(game.maze.size().cols) + "  |  Arrow keys: move  |  Esc: menu");
    controls_.setPosition({ 24.f, 56.f });
    target.draw(title_);
    target.draw(controls_);

    const sf::Color legendColors[] = { wallColor, passageColor, exitColor, playerColor };
    const char* captions[] = { "Wall", "Passage", "Exit", "You" };
    for (int index = 0; index < 4; ++index)
    {
        const float x = 24.f + static_cast<float>(index) * 112.f;
        sf::RectangleShape swatch({ 12.f, 12.f });
        swatch.setPosition({ x, 88.f });
        swatch.setFillColor(legendColors[index]);
        target.draw(swatch);
        legend_.setString(captions[index]);
        legend_.setPosition({ x + 19.f, 81.f });
        target.draw(legend_);
    }

    position_.setString("Row " + std::to_string(game.player.row)
        + "  |  Column " + std::to_string(game.player.col));
    position_.setPosition({ 24.f, height - 86.f });
    target.draw(position_);
    const bool won = game.phase == GamePhase::Won;
    message_.setString(won ? "You reached the exit! Esc returns to the menu." : notice_);
    message_.setFillColor(won ? exitColor : mutedColor);
    message_.setPosition({ 24.f, height - 50.f });
    target.draw(message_);

    if (game.maze.empty()) return;
    const auto& size = game.maze.size();
    const float rows = static_cast<float>(size.rows);
    const float cols = static_cast<float>(size.cols);
    const float availableHeight = std::max(0.f, height - 220.f);
    const float cellSize = std::floor(std::min(std::max(0.f, width - 48.f) / cols,
        availableHeight / rows));
    if (cellSize < 2.f) return;
    const sf::Vector2f boardSize{ cellSize * cols, cellSize * rows };
    const sf::Vector2f origin{ std::floor((width - boardSize.x) / 2.f),
        112.f + std::floor((availableHeight - boardSize.y) / 2.f) };
    sf::RectangleShape border(boardSize);
    border.setPosition(origin);
    border.setFillColor({ 18, 22, 31 });
    border.setOutlineThickness(2.f);
    border.setOutlineColor(won ? exitColor : sf::Color{ 75, 91, 115 });
    target.draw(border);

    // One batch keeps large grids inexpensive to draw.
    cells_.resize(game.maze.cellCount() * 6);
    std::size_t vertex = 0;
    for (int row = 0; row < size.rows; ++row)
    {
        for (int col = 0; col < size.cols; ++col)
        {
            const auto cell = game.maze.at(row, col);
            const auto color = cell == MazeCell::Exit ? exitColor
                : cell == MazeCell::Passage ? passageColor : wallColor;
            const float x = origin.x + static_cast<float>(col) * cellSize;
            const float y = origin.y + static_cast<float>(row) * cellSize;
            const float edge = cellSize - 1.f;
            cells_[vertex++] = { { x, y }, color };
            cells_[vertex++] = { { x + edge, y }, color };
            cells_[vertex++] = { { x, y + edge }, color };
            cells_[vertex++] = { { x, y + edge }, color };
            cells_[vertex++] = { { x + edge, y }, color };
            cells_[vertex++] = { { x + edge, y + edge }, color };
        }
    }
    target.draw(cells_);

    if (cellSize >= 20.f)
    {
        cellLabel_.setString("S");
        centerText(cellLabel_, origin + sf::Vector2f{ cellSize / 2.f, cellSize / 2.f });
        target.draw(cellLabel_);
        cellLabel_.setString("E");
        centerText(cellLabel_, origin + sf::Vector2f{
            (static_cast<float>(size.cols) - 0.5f) * cellSize,
            (static_cast<float>(size.rows) - 0.5f) * cellSize });
        target.draw(cellLabel_);
    }

    if (game.maze.contains(game.player))
    {
        const float radius = std::max(1.f, std::floor(cellSize * 0.28f));
        sf::CircleShape player(radius, 20);
        player.setFillColor(playerColor);
        player.setOutlineColor(sf::Color::White);
        player.setOutlineThickness(1.f);
        player.setPosition({ origin.x + static_cast<float>(game.player.col) * cellSize
                + std::floor(cellSize / 2.f - radius),
            origin.y + static_cast<float>(game.player.row) * cellSize
                + std::floor(cellSize / 2.f - radius) });
        target.draw(player);
    }
}
