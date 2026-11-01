#include "menu.h"
#include "Maze.Validation.h"
#include "GameStatisticsText.h"
#include "Maze.Generation.h"
#include <SFML/Window/Clipboard.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace
{
    const sf::Color textColor{ 235, 240, 248 };
    const sf::Color mutedColor{ 151, 164, 184 };

    void centerText(sf::Text& text, sf::Vector2f position)
    {
        const auto bounds = text.getLocalBounds();
        text.setOrigin({
            std::round(bounds.position.x + bounds.size.x / 2.f),
            std::round(bounds.position.y + bounds.size.y / 2.f)
        });
        text.setPosition({ std::round(position.x), std::round(position.y) });
    }

    std::string wrapText(const std::string& text, const sf::Font& font,
        unsigned int characterSize, float maximumWidth)
    {
        sf::Text measure(font, "", characterSize);
        std::istringstream paragraphs(text);
        std::string paragraph;
        std::string result;
        bool firstParagraph = true;
        while (std::getline(paragraphs, paragraph))
        {
            if (!firstParagraph) result += '\n';
            firstParagraph = false;
            std::istringstream words(paragraph);
            std::string word;
            std::string line;
            while (words >> word)
            {
                const auto candidate = line.empty() ? word : line + " " + word;
                measure.setString(candidate);
                if (!line.empty() && measure.getLocalBounds().size.x > maximumWidth)
                {
                    result += line + '\n';
                    line = word;
                }
                else line = candidate;
            }
            result += line;
        }
        return result;
    }
}

Menu::Button::Button(const sf::Font& font, const std::string& caption, MenuAction value)
    : label(font, caption, 24), action(value)
{
    shape.setOutlineThickness(1.f);
    update({ -1.f, -1.f });
}

void Menu::Button::setBounds(sf::Vector2f position, sf::Vector2f size)
{
    shape.setPosition({ std::round(position.x), std::round(position.y) });
    shape.setSize(size);
    centerText(label, shape.getPosition() + size / 2.f);
}

bool Menu::Button::hit(sf::Vector2f point) const
{
    return enabled && shape.getGlobalBounds().contains(point);
}

void Menu::Button::update(sf::Vector2f mousePosition)
{
    const bool hovered = hit(mousePosition);
    shape.setFillColor(!enabled ? sf::Color{ 30, 37, 49 }
        : hovered ? sf::Color{ 76, 137, 225 } : sf::Color{ 45, 90, 170 });
    shape.setOutlineColor(!enabled ? sf::Color{ 45, 54, 70 }
        : hovered ? sf::Color{ 140, 185, 248 } : sf::Color{ 73, 114, 178 });
    label.setFillColor(enabled ? textColor : sf::Color{ 113, 127, 148 });
}

void Menu::Button::draw(sf::RenderTarget& target) const
{
    target.draw(shape);
    target.draw(label);
}

Menu::Menu(const sf::Font& font)
    : font_(font), title_(font, "", 42), subtitle_(font, "", 19),
      rules_(font, "", 20), message_(font, "", 18), victorySummary_(font, "", 24),
      victoryStatistics_(font, "", 22),
      victorySeed_(font, "", 20),
      rowName_(font, "Rows", 24), colName_(font, "Columns", 24),
      rowValue_(font, "", 28), colValue_(font, "", 28), sizeSummary_(font, "", 19),
      seedLabel_(font, "Seed", 24), seedValue_(font, "", 22),
      mainButtons_{
          Button(font, "Continue", MenuAction::Continue),
          Button(font, "New Game", MenuAction::NewGame),
          Button(font, "Restart maze", MenuAction::RestartMaze),
          Button(font, "How to Play", MenuAction::HowToPlay),
          Button(font, "Settings", MenuAction::Settings),
          Button(font, "Exit", MenuAction::Exit)
      },
      sizeButtons_{
          Button(font, "-", MenuAction::None), Button(font, "+", MenuAction::None),
          Button(font, "-", MenuAction::None), Button(font, "+", MenuAction::None)
      },
      backButton_(font, "Back", MenuAction::Back),
      applyButton_(font, "Apply", MenuAction::ApplySettings)
{
    for (auto* text : { &title_, &rules_, &victorySummary_, &rowName_, &colName_, &rowValue_, &colValue_, &seedLabel_, &seedValue_ })
        text->setFillColor(textColor);
    for (auto* text : { &subtitle_, &message_, &sizeSummary_, &victoryStatistics_, &victorySeed_ })
        text->setFillColor(mutedColor);
    rules_.setLineSpacing(1.3f);
    seedField_.setFillColor({ 30, 37, 49 });
    seedField_.setOutlineThickness(1.f);
    showMain();
}

void Menu::resize(sf::Vector2u size)
{
    if (size.x == 0 || size.y == 0) return;
    viewport_ = size;
    const float width = static_cast<float>(size.x);
    const float height = static_cast<float>(size.y);
    const float center = std::floor(width / 2.f);
    title_.setFillColor(screen_ == Screen::Victory ? sf::Color{ 73, 190, 126 } : textColor);
    centerText(title_, { center, 56.f });
    centerText(subtitle_, { center, 103.f });

    const float buttonWidth = std::min(320.f, width - 80.f);
    constexpr float buttonStep = 54.f;
    const float mainHeight = buttonStep * static_cast<float>(mainButtons_.size() - 1) + 48.f;
    const float top = std::max(144.f, (height - mainHeight) / 2.f + 16.f);
    for (std::size_t index = 0; index < mainButtons_.size(); ++index)
        mainButtons_[index].setBounds({ center - buttonWidth / 2.f, top + buttonStep * static_cast<float>(index) },
            { buttonWidth, 48.f });

    message_.setString(wrapText(notice_, font_, 18, width - 80.f));
    centerText(message_, { center, height - 45.f });
    centerText(victorySummary_, { center, std::floor(height / 2.f - 100.f) });
    centerText(victoryStatistics_, { center, std::floor(height / 2.f - 64.f) });
    centerText(victorySeed_, { center, std::floor(height / 2.f - 138.f) });
    rules_.setString(wrapText(
        "Start in the top-left corner.\n"
        "Reach the green exit at the bottom-right.\n"
        "Arrow keys move one cell at a time.\n"
        "Walls and maze edges block movement.\n"
        "Esc opens the menu; Continue resumes the same game.\n"
        "R restarts the same maze at the start.\n"
        "Choose size and an optional seed in Settings.\n"
        "After winning: Restart maze, New Game or Main menu.", font_, 20, width - 112.f));
    rules_.setOrigin({ 0.f, 0.f });
    rules_.setPosition({ std::round((width - rules_.getLocalBounds().size.x) / 2.f), 155.f });

    const float rowTop = std::floor(height / 2.f - 120.f);
    const float colTop = rowTop + 76.f;
    rowName_.setPosition({ center - 190.f, rowTop + 8.f });
    colName_.setPosition({ center - 190.f, colTop + 8.f });
    sizeButtons_[0].setBounds({ center - 40.f, rowTop }, { 48.f, 48.f });
    sizeButtons_[1].setBounds({ center + 100.f, rowTop }, { 48.f, 48.f });
    sizeButtons_[2].setBounds({ center - 40.f, colTop }, { 48.f, 48.f });
    sizeButtons_[3].setBounds({ center + 100.f, colTop }, { 48.f, 48.f });
    seedLabel_.setPosition({ center - 190.f, std::floor(height / 2.f + 40.f) });
    seedField_.setPosition({ center - 40.f, std::floor(height / 2.f + 32.f) });
    seedField_.setSize({ 220.f, 44.f });
    refreshSettings();

    backButton_.label.setString(screen_ == Screen::Settings ? "Cancel"
        : screen_ == Screen::Victory ? "Main menu" : "Back");
    if (screen_ == Screen::Settings)
    {
        backButton_.setBounds({ center - 160.f, height - 108.f }, { 146.f, 48.f });
        applyButton_.setBounds({ center + 14.f, height - 108.f }, { 146.f, 48.f });
    }
    else if (screen_ == Screen::Victory)
    {
        mainButtons_[2].setBounds({ center - buttonWidth / 2.f, std::floor(height / 2.f - 44.f) },
            { buttonWidth, 48.f });
        mainButtons_[1].setBounds({ center - buttonWidth / 2.f, std::floor(height / 2.f + 16.f) },
            { buttonWidth, 48.f });
        backButton_.setBounds({ center - buttonWidth / 2.f, std::floor(height / 2.f + 76.f) },
            { buttonWidth, 48.f });
    }
    else backButton_.setBounds({ center - 100.f, height - 108.f }, { 200.f, 48.f });
}

void Menu::setHasActiveGame(bool active)
{
    hasActiveGame_ = active;
    mainButtons_[0].enabled = active;
}

void Menu::setCanRestart(bool available)
{
    canRestart_ = available;
    mainButtons_[2].enabled = available;
}

void Menu::showMain()
{
    screen_ = Screen::Main;
    title_.setString("NXRP MAZE");
    subtitle_.setString("Find your way out.");
    mainButtons_[0].enabled = hasActiveGame_;
    mainButtons_[2].enabled = canRestart_;
    if (hasActiveGame_) notice_ = "An active game is available.";
    else if (canRestart_) notice_ = "Maze complete. Restart it or choose New Game.";
    else notice_ = "No active game to continue.";
    resize(viewport_);
}

void Menu::showHowToPlay()
{
    screen_ = Screen::HowToPlay;
    title_.setString("How to Play");
    subtitle_.setString("The goal and controls");
    resize(viewport_);
}

void Menu::showSettings(const MazeSize& selectedSize, std::optional<std::uint32_t> seed)
{
    if (!isValidMazeSize(selectedSize))
        throw std::invalid_argument("Settings require rows and columns between 10 and 50.");
    screen_ = Screen::Settings;
    draftSize_ = selectedSize;
    draftSeedText_ = seed ? std::to_string(*seed) : "";
    seedFocused_ = true;
    title_.setString("Settings");
    subtitle_.setString("Choose size and seed for your next game.");
    resize(viewport_);
}

void Menu::showMessage(const std::string& message)
{
    notice_ = message;
    resize(viewport_);
}

void Menu::showVictory(const MazeSize& completedSize, const GameStatistics& statistics,
    std::optional<std::uint32_t> seed)
{
    if (!isValidMazeSize(completedSize))
        throw std::invalid_argument("Victory requires valid completed maze dimensions.");
    screen_ = Screen::Victory;
    title_.setString("You escaped!");
    subtitle_.setString("The maze is complete.");
    victorySummary_.setString("Maze completed: " + std::to_string(completedSize.rows)
        + " x " + std::to_string(completedSize.cols));
    victoryStatistics_.setString(gameStatisticsText(statistics));
    victorySeed_.setString(seed ? "Seed " + std::to_string(*seed) : "Seed unavailable");
    notice_ = "R restarts this maze. Esc returns to the main menu.";
    resize(viewport_);
}

MazeSize Menu::draftSize() const noexcept
{
    return draftSize_;
}

bool Menu::draftSeed(std::optional<std::uint32_t>& seed) const noexcept
{
    return parseMazeSeed(draftSeedText_, seed);
}

void Menu::refreshSettings()
{
    rowValue_.setString(std::to_string(draftSize_.rows));
    colValue_.setString(std::to_string(draftSize_.cols));
    const float center = std::floor(static_cast<float>(viewport_.x) / 2.f);
    const float rowTop = std::floor(static_cast<float>(viewport_.y) / 2.f - 120.f);
    centerText(rowValue_, { center + 54.f, rowTop + 24.f });
    centerText(colValue_, { center + 54.f, rowTop + 100.f });
    std::optional<std::uint32_t> seed;
    const bool validSeed = draftSeed(seed);
    applyButton_.enabled = validSeed;
    seedField_.setOutlineColor(!validSeed ? sf::Color{ 236, 106, 106 }
        : seedFocused_ ? sf::Color{ 140, 185, 248 } : sf::Color{ 75, 91, 115 });
    auto visibleSeed = draftSeedText_;
    seedValue_.setString(draftSeedText_.empty() ? "Random" : visibleSeed);
    while (!visibleSeed.empty() && seedValue_.getLocalBounds().size.x > seedField_.getSize().x - 24.f)
    {
        visibleSeed.erase(0, 1);
        seedValue_.setString("..." + visibleSeed);
    }
    seedValue_.setPosition(seedField_.getPosition() + sf::Vector2f{ 12.f, 8.f });
    sizeSummary_.setFillColor(validSeed ? mutedColor : sf::Color{ 236, 106, 106 });
    sizeSummary_.setString(std::string(validSeed ? "Rows/cols: 10-50. Blank seed = random."
        : "Seed: 0-4294967295, or leave blank.")
        + "\nSize persists. Seed applies for this launch.");
    centerText(sizeSummary_, { center, std::floor(static_cast<float>(viewport_.y) / 2.f + 112.f) });
    sizeButtons_[0].enabled = draftSize_.rows > MIN_MAZE_SIZE;
    sizeButtons_[1].enabled = draftSize_.rows < MAX_MAZE_SIZE;
    sizeButtons_[2].enabled = draftSize_.cols > MIN_MAZE_SIZE;
    sizeButtons_[3].enabled = draftSize_.cols < MAX_MAZE_SIZE;
}

MenuAction Menu::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (screen_ == Screen::Settings && seedFocused_)
    {
        if (const auto* text = event.getIf<sf::Event::TextEntered>())
        {
            if (text->unicode == U'\b')
            {
                if (!draftSeedText_.empty()) draftSeedText_.pop_back();
            }
            else if (text->unicode >= 32 && text->unicode != 127 && draftSeedText_.size() < 32)
                draftSeedText_ += text->unicode < 127 ? static_cast<char>(text->unicode) : '?';
            refreshSettings();
            return MenuAction::None;
        }
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (screen_ == Screen::Settings)
        {
            if (key->code == sf::Keyboard::Key::Enter && applyButton_.enabled)
                return MenuAction::ApplySettings;
            if (seedFocused_ && key->code == sf::Keyboard::Key::Delete)
                draftSeedText_.clear();
            if (seedFocused_ && key->control && key->code == sf::Keyboard::Key::V)
            {
                draftSeedText_.clear();
                for (const auto code : sf::Clipboard::getString())
                {
                    if (draftSeedText_.size() == 32) break;
                    draftSeedText_ += code >= 32 && code < 127 ? static_cast<char>(code) : '?';
                }
            }
            refreshSettings();
        }
        if (key->code == sf::Keyboard::Key::R && canRestart_
            && (screen_ == Screen::Main || screen_ == Screen::Victory))
            return MenuAction::RestartMaze;
        if (key->code == sf::Keyboard::Key::Escape && screen_ != Screen::Main)
            return MenuAction::Back;
    }
    const auto* click = event.getIf<sf::Event::MouseButtonPressed>();
    if (!click || click->button != sf::Mouse::Button::Left) return MenuAction::None;
    const auto point = window.mapPixelToCoords(click->position);

    if (screen_ == Screen::Main)
    {
        for (const auto& button : mainButtons_)
            if (button.hit(point)) return button.action;
    }
    else
    {
        if (backButton_.hit(point)) return MenuAction::Back;
        if (screen_ == Screen::Victory && mainButtons_[1].hit(point)) return MenuAction::NewGame;
        if (screen_ == Screen::Victory && mainButtons_[2].hit(point)) return MenuAction::RestartMaze;
        if (screen_ == Screen::Settings)
        {
            seedFocused_ = seedField_.getGlobalBounds().contains(point);
            if (applyButton_.hit(point)) return MenuAction::ApplySettings;
            if (sizeButtons_[0].hit(point)) --draftSize_.rows;
            else if (sizeButtons_[1].hit(point)) ++draftSize_.rows;
            else if (sizeButtons_[2].hit(point)) --draftSize_.cols;
            else if (sizeButtons_[3].hit(point)) ++draftSize_.cols;
            refreshSettings();
        }
    }
    return MenuAction::None;
}

void Menu::update(sf::Vector2f mousePosition)
{
    if (screen_ == Screen::Main)
    {
        for (auto& button : mainButtons_) button.update(mousePosition);
    }
    else
    {
        backButton_.update(mousePosition);
        if (screen_ == Screen::Victory)
        {
            mainButtons_[1].update(mousePosition);
            mainButtons_[2].update(mousePosition);
        }
        if (screen_ == Screen::Settings)
        {
            applyButton_.update(mousePosition);
            for (auto& button : sizeButtons_) button.update(mousePosition);
        }
    }
}

void Menu::draw(sf::RenderTarget& target) const
{
    target.draw(title_);
    target.draw(subtitle_);
    if (screen_ == Screen::Main)
    {
        for (const auto& button : mainButtons_) button.draw(target);
        target.draw(message_);
    }
    else
    {
        backButton_.draw(target);
        if (screen_ == Screen::HowToPlay) target.draw(rules_);
        else if (screen_ == Screen::Victory)
        {
            target.draw(victorySummary_);
            target.draw(victoryStatistics_);
            target.draw(victorySeed_);
            mainButtons_[1].draw(target);
            mainButtons_[2].draw(target);
            target.draw(message_);
        }
        else
        {
            target.draw(rowName_);
            target.draw(colName_);
            target.draw(rowValue_);
            target.draw(colValue_);
            target.draw(sizeSummary_);
            target.draw(seedLabel_);
            target.draw(seedField_);
            target.draw(seedValue_);
            for (const auto& button : sizeButtons_) button.draw(target);
            applyButton_.draw(target);
        }
    }
}
