#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Game.h"
#include "Maze.Generation.h"

#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        std::vector<MazeCell> ReadCells(const MazeGrid& grid)
        {
            std::vector<MazeCell> result;
            for (int row = 0; row < grid.size().rows; ++row)
                for (int col = 0; col < grid.size().cols; ++col)
                    result.push_back(grid.at(row, col));
            return result;
        }
    }

    TEST_CLASS(MazeSeedTests)
    {
    public:
        TEST_METHOD(EmptyInputUsesRandomMode)
        {
            std::optional<std::uint32_t> seed = 42;
            Assert::IsTrue(parseMazeSeed("", seed));
            Assert::IsFalse(seed.has_value());
        }

        TEST_METHOD(AcceptsZeroMaximumAndDecimalValues)
        {
            std::optional<std::uint32_t> seed;
            Assert::IsTrue(parseMazeSeed("0", seed));
            Assert::IsTrue(seed.has_value() && *seed == 0U);
            Assert::IsTrue(parseMazeSeed("4294967295", seed));
            Assert::IsTrue(seed == std::numeric_limits<std::uint32_t>::max());
            Assert::IsTrue(parseMazeSeed("0000000042", seed));
            Assert::IsTrue(seed == 42U);
        }

        TEST_METHOD(RejectsSignsWhitespaceAndNonnumericInput)
        {
            for (const auto text : { "-1", "+1", " 42", "42 ", "4.2", "1e3", "abc", "1\n2", " " })
            {
                std::optional<std::uint32_t> seed = 77;
                Assert::IsFalse(parseMazeSeed(text, seed));
                Assert::IsTrue(seed == 77U);
            }
        }

        TEST_METHOD(RejectsOverflowAndOverlongInput)
        {
            for (const auto text : { "4294967296", "9999999999", "00000000000", "12345678901234567890" })
            {
                std::optional<std::uint32_t> seed = 77;
                Assert::IsFalse(parseMazeSeed(text, seed));
                Assert::IsTrue(seed == 77U);
            }
        }

        TEST_METHOD(SeededSessionsRecordUsedSeedAndRepeatLayout)
        {
            for (const auto size : { MazeSize(10, 10), MazeSize(50, 50), MazeSize(12, 37) })
            {
                for (const auto seed : { 0U, 42U, std::numeric_limits<std::uint32_t>::max() })
                {
                    GameState game;
                    game.selectedSeed = 77;
                    startGame(game, generateMaze(size, seed), seed);
                    Assert::IsTrue(game.mazeSeed == seed);
                    Assert::IsTrue(game.selectedSeed == 77U);
                    Assert::IsTrue(ReadCells(game.maze) == ReadCells(generateMaze(size, *game.mazeSeed)));
                }
            }
        }

        TEST_METHOD(PlainPreparedSessionClearsOldSeed)
        {
            GameState game;
            game.selectedSeed = 77;
            startGame(game, generateMaze({ 10, 10 }, 42), 42);
            startGame(game, createDemoMaze());
            Assert::IsFalse(game.mazeSeed.has_value());
            Assert::IsTrue(game.selectedSeed == 77U);
        }

        TEST_METHOD(RestartPreservesCurrentAndSelectedSeeds)
        {
            for (const auto phase : { GamePhase::Playing, GamePhase::Paused, GamePhase::Won })
            {
                GameState game;
                startGame(game, generateMaze({ 10, 10 }, 0), 0);
                game.selectedSeed = 42;
                game.phase = phase;
                const auto* storage = &game.maze.at(0, 0);
                Assert::IsTrue(restartGame(game));
                Assert::IsTrue(game.mazeSeed == 0U && game.selectedSeed == 42U);
                Assert::IsTrue(storage == &game.maze.at(0, 0));
            }
        }

        TEST_METHOD(EndGameClearsCurrentSeedAndKeepsChoice)
        {
            GameState game;
            startGame(game, generateMaze({ 10, 10 }, 42), 42);
            game.selectedSeed = 0;
            endGame(game);
            Assert::IsFalse(game.mazeSeed.has_value());
            Assert::IsTrue(game.selectedSeed == 0U);
            endGame(game);
            Assert::IsTrue(game.selectedSeed == 0U);
        }

        TEST_METHOD(InvalidReplacementPreservesBothSeeds)
        {
            GameState game;
            startGame(game, generateMaze({ 10, 10 }, 42), 42);
            game.selectedSeed = 0;
            const auto* storage = &game.maze.at(0, 0);
            MazeGrid invalid;
            Assert::ExpectException<std::invalid_argument>([&] { startGame(game, std::move(invalid), 99); });
            Assert::IsTrue(game.mazeSeed == 42U && game.selectedSeed == 0U);
            Assert::IsTrue(storage == &game.maze.at(0, 0));
        }

        TEST_METHOD(GeneratedRandomSeedCanReproduceAMaze)
        {
            const auto seed = generateMazeSeed();
            GameState game;
            startGame(game, generateMaze({ 10, 50 }, seed), seed);
            Assert::IsTrue(game.mazeSeed == seed);
            Assert::IsTrue(ReadCells(game.maze) == ReadCells(generateMaze({ 10, 50 }, *game.mazeSeed)));
        }
    };
}
