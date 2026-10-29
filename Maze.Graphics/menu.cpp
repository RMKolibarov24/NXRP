#include "menu.h"
#include "Maze.Validation.h"

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
      rowName_(font, "Rows", 24), colName_(font, "Columns", 24),
      rowValue_(font, "", 28), colValue_(font, "", 28), sizeSummary_(font, "", 19),
      mainButtons_{
          Button(font, "Continue", MenuAction::Continue),
          Button(font, "New Game", MenuAction::NewGame),
          Button(font, "How to Play", MenuAction::HowToPlay),
          Button(font, "Settings", MenuAction::Settings),
          Button(font, "Exit", MenuAction::Exit)
      },
      sizeButtons_{
          Button(font, "-", MenuAction::None), Button(font, "+", MenuAction::None),
          Button(font, "-", MenuAction::None), Button(font, "+", MenuAction::None)
      },
      backButton_(font, "Back", MenuAction::Back),
      applyButton_(font, "Save size", MenuAction::ApplySettings)
{
    for (auto* text : { &title_, &rules_, &victorySummary_, &rowName_, &colName_, &rowValue_, &colValue_ })
        text->setFillColor(textColor);
    for (auto* text : { &subtitle_, &message_, &sizeSummary_ })
        text->setFillColor(mutedColor);
    rules_.setLineSpacing(1.3f);
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
    const float top = std::max(140.f, (height - 288.f) / 2.f + 20.f);
    for (std::size_t index = 0; index < mainButtons_.size(); ++index)
        mainButtons_[index].setBounds({ center - buttonWidth / 2.f, top + 60.f * static_cast<float>(index) },
            { buttonWidth, 48.f });

    message_.setString(wrapText(notice_, font_, 18, width - 80.f));
    centerText(message_, { center, height - 45.f });
    centerText(victorySummary_, { center, std::floor(height / 2.f - 100.f) });
    rules_.setString(wrapText(
        "Start in the top-left corner.\n"
        "Reach the green exit at the bottom-right.\n"
        "Arrow keys move one cell at a time.\n"
        "Walls and maze edges block movement.\n"
        "Esc opens the menu; Continue resumes the same game.\n"
        "Choose rows and columns (10-50) in Settings.\n"
        "After winning, choose New Game or Main menu.", font_, 20, width - 112.f));
    rules_.setOrigin({ 0.f, 0.f });
    rules_.setPosition({ std::round((width - rules_.getLocalBounds().size.x) / 2.f), 155.f });

    const float rowTop = std::floor(height / 2.f - 80.f);
    const float colTop = rowTop + 76.f;
    rowName_.setPosition({ center - 190.f, rowTop + 8.f });
    colName_.setPosition({ center - 190.f, colTop + 8.f });
    sizeButtons_[0].setBounds({ center - 40.f, rowTop }, { 48.f, 48.f });
    sizeButtons_[1].setBounds({ center + 100.f, rowTop }, { 48.f, 48.f });
    sizeButtons_[2].setBounds({ center - 40.f, colTop }, { 48.f, 48.f });
    sizeButtons_[3].setBounds({ center + 100.f, colTop }, { 48.f, 48.f });
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
        mainButtons_[1].setBounds({ center - buttonWidth / 2.f, std::floor(height / 2.f - 24.f) },
            { buttonWidth, 48.f });
        backButton_.setBounds({ center - buttonWidth / 2.f, std::floor(height / 2.f + 36.f) },
            { buttonWidth, 48.f });
    }
    else backButton_.setBounds({ center - 100.f, height - 108.f }, { 200.f, 48.f });
}

void Menu::setHasActiveGame(bool active)
{
    hasActiveGame_ = active;
    mainButtons_[0].enabled = active;
}

void Menu::showMain()
{
    screen_ = Screen::Main;
    title_.setString("NXRP MAZE");
    subtitle_.setString("Find your way out.");
    mainButtons_[0].enabled = hasActiveGame_;
    notice_ = hasActiveGame_ ? "An active game is available." : "No active game to continue.";
    resize(viewport_);
}

void Menu::showHowToPlay()
{
    screen_ = Screen::HowToPlay;
    title_.setString("How to Play");
    subtitle_.setString("The goal and controls");
    resize(viewport_);
}

void Menu::showSettings(const MazeSize& selectedSize)
{
    if (!isValidMazeSize(selectedSize))
        throw std::invalid_argument("Settings require rows and columns between 10 and 50.");
    screen_ = Screen::Settings;
    draftSize_ = selectedSize;
    title_.setString("Settings");
    subtitle_.setString("Choose the size for your next game.");
    resize(viewport_);
}

void Menu::showMessage(const std::string& message)
{
    notice_ = message;
    resize(viewport_);
}

void Menu::showVictory(const MazeSize& completedSize)
{
    if (!isValidMazeSize(completedSize))
        throw std::invalid_argument("Victory requires valid completed maze dimensions.");
    screen_ = Screen::Victory;
    title_.setString("You escaped!");
    subtitle_.setString("The maze is complete.");
    victorySummary_.setString("Maze completed: " + std::to_string(completedSize.rows)
        + " x " + std::to_string(completedSize.cols));
    notice_ = "Esc returns to the main menu.";
    resize(viewport_);
}

MazeSize Menu::draftSize() const noexcept
{
    return draftSize_;
}

void Menu::refreshSettings()
{
    rowValue_.setString(std::to_string(draftSize_.rows));
    colValue_.setString(std::to_string(draftSize_.cols));
    const float center = std::floor(static_cast<float>(viewport_.x) / 2.f);
    const float rowTop = std::floor(static_cast<float>(viewport_.y) / 2.f - 80.f);
    centerText(rowValue_, { center + 54.f, rowTop + 24.f });
    centerText(colValue_, { center + 54.f, rowTop + 100.f });
    sizeSummary_.setString("Allowed range: 10-50 for each dimension");
    centerText(sizeSummary_, { center, rowTop + 167.f });
    sizeButtons_[0].enabled = draftSize_.rows > MIN_MAZE_SIZE;
    sizeButtons_[1].enabled = draftSize_.rows < MAX_MAZE_SIZE;
    sizeButtons_[2].enabled = draftSize_.cols > MIN_MAZE_SIZE;
    sizeButtons_[3].enabled = draftSize_.cols < MAX_MAZE_SIZE;
}

MenuAction Menu::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
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
        if (screen_ == Screen::Settings)
        {
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
        if (screen_ == Screen::Victory) mainButtons_[1].update(mousePosition);
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
            mainButtons_[1].draw(target);
            target.draw(message_);
        }
        else
        {
            target.draw(rowName_);
            target.draw(colName_);
            target.draw(rowValue_);
            target.draw(colValue_);
            target.draw(sizeSummary_);
            for (const auto& button : sizeButtons_) button.draw(target);
            applyButton_.draw(target);
        }
    }
}
