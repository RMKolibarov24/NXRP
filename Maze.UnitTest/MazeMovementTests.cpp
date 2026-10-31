#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Game.h"

#include <climits>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        GameState MakeOpenGame()
        {
            MazeGrid maze(MazeSize(10, 50));
            maze.fill(MazeCell::Passage);
            maze.at(9, 49) = MazeCell::Exit;
            GameState game;
            startGame(game, std::move(maze));
            return game;
        }

        std::vector<MazeCell> ReadCells(const MazeGrid& maze)
        {
            std::vector<MazeCell> result;
            for (int row = 0; row < maze.size().rows; ++row)
                for (int col = 0; col < maze.size().cols; ++col)
                    result.push_back(maze.at(row, col));
            return result;
        }

        void AssertPosition(MazePosition expected, MazePosition actual)
        {
            Assert::AreEqual(expected.row, actual.row, L"Unexpected player row.");
            Assert::AreEqual(expected.col, actual.col, L"Unexpected player column.");
        }

        void AssertBlockedPreservesState(GameState& game, MoveDirection direction)
        {
            const auto player = game.player;
            const auto phase = game.phase;
            const auto statistics = game.statistics;
            const auto selected = game.selectedSize;
            const auto size = game.maze.size();
            const auto count = game.maze.cellCount();
            const auto* storage = game.maze.empty() ? nullptr : &game.maze.at(0, 0);
            const auto cells = ReadCells(game.maze);

            Assert::IsTrue(tryMove(game, direction) == MoveResult::Blocked, L"Expected a blocked move.");
            AssertPosition(player, game.player);
            Assert::IsTrue(game.phase == phase, L"Blocked movement changed the game phase.");
            Assert::IsTrue(game.statistics.elapsed == statistics.elapsed);
            Assert::IsTrue(game.statistics.successfulMoves == statistics.successfulMoves);
            Assert::AreEqual(selected.rows, game.selectedSize.rows);
            Assert::AreEqual(selected.cols, game.selectedSize.cols);
            Assert::AreEqual(size.rows, game.maze.size().rows);
            Assert::AreEqual(size.cols, game.maze.size().cols);
            Assert::AreEqual(count, game.maze.cellCount());
            Assert::IsTrue(storage == (game.maze.empty() ? nullptr : &game.maze.at(0, 0)),
                L"Blocked movement replaced the maze allocation.");
            Assert::IsTrue(cells == ReadCells(game.maze), L"Blocked movement changed maze cells.");
        }

        void AssertSuccessfulMove(MoveDirection direction, MazePosition destination)
        {
            auto game = MakeOpenGame();
            game.player = { 5, 5 };
            const auto cells = ReadCells(game.maze);
            Assert::IsTrue(tryMove(game, direction) == MoveResult::Moved);
            AssertPosition(destination, game.player);
            Assert::IsTrue(game.phase == GamePhase::Playing);
            Assert::IsFalse(hasWon(game));
            Assert::AreEqual(10, game.selectedSize.rows);
            Assert::AreEqual(50, game.selectedSize.cols);
            Assert::IsTrue(cells == ReadCells(game.maze), L"Movement changed maze topology.");
        }
    }

    TEST_CLASS(MazeMovementTests)
    {
    public:
        TEST_METHOD(StartsPreparedMazeAtTopLeft)
        {
            const auto game = MakeOpenGame();
            AssertPosition({ 0, 0 }, game.player);
            Assert::IsTrue(game.phase == GamePhase::Playing);
            Assert::IsTrue(game.hasActiveGame());
            Assert::IsFalse(hasWon(game));
            Assert::AreEqual(10, game.maze.size().rows);
            Assert::AreEqual(50, game.maze.size().cols);
            Assert::IsTrue(game.maze.at(9, 49) == MazeCell::Exit);
        }

        TEST_METHOD(MovesUp)
        {
            AssertSuccessfulMove(MoveDirection::Up, { 4, 5 });
        }

        TEST_METHOD(MovesDown)
        {
            AssertSuccessfulMove(MoveDirection::Down, { 6, 5 });
        }

        TEST_METHOD(MovesLeft)
        {
            AssertSuccessfulMove(MoveDirection::Left, { 5, 4 });
        }

        TEST_METHOD(MovesRight)
        {
            AssertSuccessfulMove(MoveDirection::Right, { 5, 6 });
        }

        TEST_METHOD(BlocksWallsInEveryDirection)
        {
            auto game = MakeOpenGame();
            game.player = { 5, 5 };
            game.maze.at(4, 5) = MazeCell::Wall;
            game.maze.at(6, 5) = MazeCell::Wall;
            game.maze.at(5, 4) = MazeCell::Wall;
            game.maze.at(5, 6) = MazeCell::Wall;
            for (const auto direction : { MoveDirection::Up, MoveDirection::Down,
                MoveDirection::Left, MoveDirection::Right })
                AssertBlockedPreservesState(game, direction);
        }

        TEST_METHOD(BlocksTopBoundary)
        {
            auto game = MakeOpenGame();
            game.player = { 0, 10 };
            AssertBlockedPreservesState(game, MoveDirection::Up);
        }

        TEST_METHOD(BlocksBottomBoundary)
        {
            auto game = MakeOpenGame();
            game.player = { 9, 10 };
            AssertBlockedPreservesState(game, MoveDirection::Down);
        }

        TEST_METHOD(BlocksLeftBoundary)
        {
            auto game = MakeOpenGame();
            game.player = { 5, 0 };
            AssertBlockedPreservesState(game, MoveDirection::Left);
        }

        TEST_METHOD(BlocksRightBoundary)
        {
            auto game = MakeOpenGame();
            game.player = { 5, 49 };
            AssertBlockedPreservesState(game, MoveDirection::Right);
        }

        TEST_METHOD(BlocksInvalidPlayerPositions)
        {
            auto game = MakeOpenGame();
            const MazePosition positions[] = { { -1, 0 }, { 0, -1 }, { 10, 0 }, { 0, 50 },
                { INT_MIN, 0 }, { INT_MAX, 0 }, { 0, INT_MIN }, { 0, INT_MAX }, { 4, 5 } };
            game.maze.at(4, 5) = MazeCell::Wall;
            for (const auto position : positions)
            {
                game.player = position;
                Assert::IsFalse(hasWon(game));
                for (const auto direction : { MoveDirection::Up, MoveDirection::Down,
                    MoveDirection::Left, MoveDirection::Right })
                    AssertBlockedPreservesState(game, direction);
            }
        }

        TEST_METHOD(BlocksMovesOutsidePlayingPhase)
        {
            auto game = MakeOpenGame();
            game.player = { 5, 5 };
            for (const auto phase : { GamePhase::Menu, GamePhase::Paused, GamePhase::Won })
            {
                game.phase = phase;
                for (const auto direction : { MoveDirection::Up, MoveDirection::Down,
                    MoveDirection::Left, MoveDirection::Right })
                    AssertBlockedPreservesState(game, direction);
            }
        }

        TEST_METHOD(BlocksMovesWithoutMaze)
        {
            GameState game;
            game.phase = GamePhase::Playing;
            Assert::IsFalse(hasWon(game));
            for (const auto direction : { MoveDirection::Up, MoveDirection::Down,
                MoveDirection::Left, MoveDirection::Right })
                AssertBlockedPreservesState(game, direction);
        }

        TEST_METHOD(BlocksUnknownDirection)
        {
            auto game = MakeOpenGame();
            AssertBlockedPreservesState(game, static_cast<MoveDirection>(99));
        }

        TEST_METHOD(DoesNotWinBeforeReachingExit)
        {
            auto game = MakeOpenGame();
            game.player = { 9, 48 };
            Assert::IsFalse(hasWon(game));
            Assert::IsTrue(tryMove(game, MoveDirection::Left) == MoveResult::Moved);
            AssertPosition({ 9, 47 }, game.player);
            Assert::IsFalse(hasWon(game));
            Assert::IsTrue(game.phase == GamePhase::Playing);
        }

        TEST_METHOD(WinsWhenEnteringExit)
        {
            auto game = MakeOpenGame();
            game.player = { 9, 48 };
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Won);
            AssertPosition({ 9, 49 }, game.player);
            Assert::IsTrue(game.phase == GamePhase::Won);
            Assert::IsTrue(hasWon(game));
        }

        TEST_METHOD(DoesNotWinAtUnmarkedCorner)
        {
            auto game = MakeOpenGame();
            game.maze.at(9, 49) = MazeCell::Passage;
            game.player = { 9, 48 };
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
            AssertPosition({ 9, 49 }, game.player);
            Assert::IsFalse(hasWon(game));
            Assert::IsTrue(game.phase == GamePhase::Playing);
        }

        TEST_METHOD(DoesNotWinAtMisplacedExit)
        {
            auto game = MakeOpenGame();
            game.maze.at(5, 6) = MazeCell::Exit;
            game.player = { 5, 5 };
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
            AssertPosition({ 5, 6 }, game.player);
            Assert::IsFalse(hasWon(game));
            Assert::IsTrue(game.phase == GamePhase::Playing);
        }

        TEST_METHOD(BlocksMovementAfterVictory)
        {
            auto game = MakeOpenGame();
            game.player = { 9, 48 };
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Won);
            for (const auto direction : { MoveDirection::Up, MoveDirection::Down,
                MoveDirection::Left, MoveDirection::Right })
                AssertBlockedPreservesState(game, direction);
            Assert::IsTrue(hasWon(game));
        }

        TEST_METHOD(DemoCanBeCompletedWithLegalMoves)
        {
            GameState game;
            startGame(game, createDemoMaze());
            const auto cells = ReadCells(game.maze);
            const struct { MoveDirection direction; int count; } route[] = {
                { MoveDirection::Right, 3 }, { MoveDirection::Down, 2 }, { MoveDirection::Left, 2 },
                { MoveDirection::Down, 2 }, { MoveDirection::Right, 6 }, { MoveDirection::Down, 2 },
                { MoveDirection::Left, 4 }, { MoveDirection::Down, 2 }, { MoveDirection::Right, 6 },
                { MoveDirection::Down, 1 }
            };
            int moves = 0;
            for (const auto segment : route)
            {
                for (int step = 0; step < segment.count; ++step)
                {
                    ++moves;
                    const auto expected = moves == 30 ? MoveResult::Won : MoveResult::Moved;
                    Assert::IsTrue(tryMove(game, segment.direction) == expected);
                }
            }
            AssertPosition({ 9, 9 }, game.player);
            Assert::IsTrue(hasWon(game));
            Assert::IsTrue(game.phase == GamePhase::Won);
            Assert::IsTrue(cells == ReadCells(game.maze));
        }
    };
}
