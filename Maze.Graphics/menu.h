#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <string>

#include "MazeSize.h"

enum class MenuAction
{
    None,
    Continue,
    NewGame,
    RestartMaze,
    HowToPlay,
    Settings,
    ApplySettings,
    Back,
    Exit
};

class Menu
{
public:
    explicit Menu(const sf::Font& font);

    void resize(sf::Vector2u size);
    void setHasActiveGame(bool active);
    void setCanRestart(bool available);
    void showMain();
    void showHowToPlay();
    void showSettings(const MazeSize& selectedSize);
    void showVictory(const MazeSize& completedSize);
    void showMessage(const std::string& message);
    MazeSize draftSize() const noexcept;

    MenuAction handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(sf::Vector2f mousePosition);
    void draw(sf::RenderTarget& target) const;

private:
    enum class Screen { Main, HowToPlay, Settings, Victory };

    struct Button
    {
        sf::RectangleShape shape;
        sf::Text label;
        MenuAction action;
        bool enabled = true;

        Button(const sf::Font& font, const std::string& caption, MenuAction action);
        void setBounds(sf::Vector2f position, sf::Vector2f size);
        bool hit(sf::Vector2f point) const;
        void update(sf::Vector2f mousePosition);
        void draw(sf::RenderTarget& target) const;
    };

    void refreshSettings();

    const sf::Font& font_;
    Screen screen_ = Screen::Main;
    sf::Vector2u viewport_{ 900u, 600u };
    MazeSize draftSize_{ MIN_MAZE_SIZE, MIN_MAZE_SIZE };
    bool hasActiveGame_ = false;
    bool canRestart_ = false;
    std::string notice_;

    sf::Text title_;
    sf::Text subtitle_;
    sf::Text rules_;
    sf::Text message_;
    sf::Text victorySummary_;
    sf::Text rowName_;
    sf::Text colName_;
    sf::Text rowValue_;
    sf::Text colValue_;
    sf::Text sizeSummary_;
    std::array<Button, 6> mainButtons_;
    std::array<Button, 4> sizeButtons_;
    Button backButton_;
    Button applyButton_;
};
