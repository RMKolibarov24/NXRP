#include "pch.h"
#include "CppUnitTest.h"
#include "GameState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    TEST_CLASS(GameStateTests)
    {
    public:
        TEST_METHOD(DefaultsToMenuWithNoActiveGame)
        {
            const GameState game;
            Assert::IsTrue(game.phase == GamePhase::Menu);
            Assert::IsFalse(game.hasActiveGame());
            Assert::IsTrue(game.maze.empty());
            Assert::AreEqual(std::size_t{ 0 }, game.maze.cellCount());
            Assert::AreEqual(DEFAULT_MAZE_SIZE, game.selectedSize.rows);
            Assert::AreEqual(DEFAULT_MAZE_SIZE, game.selectedSize.cols);
            Assert::AreEqual(0, game.player.row);
            Assert::AreEqual(0, game.player.col);
            Assert::IsTrue(game.statistics.elapsed == std::chrono::steady_clock::duration::zero());
            Assert::IsTrue(game.statistics.successfulMoves == 0);
            Assert::IsFalse(game.selectedSeed.has_value());
            Assert::IsFalse(game.mazeSeed.has_value());
        }

        TEST_METHOD(ChangingSelectedSizeDoesNotAllocateAMaze)
        {
            GameState game;
            game.selectedSize = MazeSize(50, 10);
            Assert::AreEqual(50, game.selectedSize.rows);
            Assert::AreEqual(10, game.selectedSize.cols);
            Assert::IsFalse(game.hasActiveGame());
            Assert::IsTrue(game.maze.empty());
            Assert::AreEqual(std::size_t{ 0 }, game.maze.cellCount());
            Assert::AreEqual(0, game.player.row);
            Assert::AreEqual(0, game.player.col);
            Assert::IsTrue(game.phase == GamePhase::Menu);
        }

        TEST_METHOD(ActiveGameReflectsOwnedMazeData)
        {
            GameState game;
            game.maze = MazeGrid(MazeSize(10, 10));
            Assert::IsTrue(game.hasActiveGame());

            game.maze = MazeGrid();
            Assert::IsFalse(game.hasActiveGame());
            Assert::IsTrue(game.maze.empty());
        }
    };
}
