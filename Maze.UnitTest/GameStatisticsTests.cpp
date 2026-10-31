#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Game.h"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::chrono_literals;

namespace MazeUnitTest
{
    namespace
    {
        MazeGrid OpenMaze()
        {
            MazeGrid maze({ 10, 10 });
            maze.fill(MazeCell::Passage);
            maze.at(9, 9) = MazeCell::Exit;
            return maze;
        }

        GameState PlayingGame()
        {
            GameState game;
            startGame(game, OpenMaze());
            return game;
        }

        void AssertZero(const GameState& game)
        {
            Assert::IsTrue(game.statistics.elapsed == 0s);
            Assert::IsTrue(game.statistics.successfulMoves == 0);
        }
    }

    TEST_CLASS(GameStatisticsTests)
    {
    public:
        TEST_METHOD(NewSessionsStartAtZero)
        {
            GameState game;
            AssertZero(game);
            game.statistics.elapsed = 12s;
            game.statistics.successfulMoves = 5;
            startGame(game, OpenMaze());
            AssertZero(game);
        }

        TEST_METHOD(OnlySuccessfulMovesCount)
        {
            auto game = PlayingGame();
            game.maze.at(0, 1) = MazeCell::Wall;
            Assert::IsTrue(tryMove(game, MoveDirection::Up) == MoveResult::Blocked);
            Assert::IsTrue(tryMove(game, MoveDirection::Left) == MoveResult::Blocked);
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Blocked);
            Assert::IsTrue(tryMove(game, static_cast<MoveDirection>(99)) == MoveResult::Blocked);
            AssertZero(game);
            Assert::IsTrue(tryMove(game, MoveDirection::Down) == MoveResult::Moved);
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
            Assert::IsTrue(game.statistics.successfulMoves == 2);
            Assert::IsTrue(game.statistics.elapsed == 0s);
        }

        TEST_METHOD(WinningMoveCountsAndTimeStops)
        {
            auto game = PlayingGame();
            game.player = { 9, 8 };
            advanceGameTime(game, 1250ms);
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Won);
            Assert::IsTrue(game.statistics.successfulMoves == 1);
            advanceGameTime(game, 100s);
            Assert::IsTrue(tryMove(game, MoveDirection::Left) == MoveResult::Blocked);
            Assert::IsTrue(game.statistics.successfulMoves == 1);
            Assert::IsTrue(game.statistics.elapsed == 1250ms);
        }

        TEST_METHOD(AccumulatesSubsecondTime)
        {
            auto game = PlayingGame();
            advanceGameTime(game, 250ms);
            advanceGameTime(game, 750ms);
            advanceGameTime(game, 1s);
            Assert::IsTrue(game.statistics.elapsed == 2s);
            Assert::IsTrue(game.statistics.successfulMoves == 0);
        }

        TEST_METHOD(IgnoresInactiveOrEmptyGames)
        {
            for (const auto phase : { GamePhase::Menu, GamePhase::Paused, GamePhase::Won, static_cast<GamePhase>(99) })
            {
                auto game = PlayingGame();
                game.statistics.elapsed = 1s;
                game.phase = phase;
                advanceGameTime(game, 5s);
                Assert::IsTrue(game.statistics.elapsed == 1s);
                Assert::IsTrue(game.statistics.successfulMoves == 0);
            }
            GameState empty;
            empty.phase = GamePhase::Playing;
            advanceGameTime(empty, 5s);
            AssertZero(empty);
        }

        TEST_METHOD(NonpositiveTimeIsIgnored)
        {
            auto game = PlayingGame();
            advanceGameTime(game, 125ms);
            advanceGameTime(game, -1s);
            advanceGameTime(game, 0s);
            Assert::IsTrue(game.statistics.elapsed == 125ms);
        }

        TEST_METHOD(RestartResetsStatisticsAndPreservesNextSize)
        {
            for (const auto phase : { GamePhase::Playing, GamePhase::Paused, GamePhase::Won })
            {
                auto game = PlayingGame();
                advanceGameTime(game, 2s);
                tryMove(game, MoveDirection::Right);
                game.phase = phase;
                game.selectedSize = MazeSize(50, 37);
                const auto* storage = &game.maze.at(0, 0);
                Assert::IsTrue(restartGame(game));
                AssertZero(game);
                Assert::IsTrue(storage == &game.maze.at(0, 0));
                Assert::AreEqual(50, game.selectedSize.rows);
                Assert::AreEqual(37, game.selectedSize.cols);
            }
        }

        TEST_METHOD(ReplacementAndEndGameResetStatistics)
        {
            auto game = PlayingGame();
            advanceGameTime(game, 2s);
            tryMove(game, MoveDirection::Right);
            startGame(game, OpenMaze());
            AssertZero(game);
            advanceGameTime(game, 3s);
            tryMove(game, MoveDirection::Down);
            endGame(game);
            AssertZero(game);
            endGame(game);
            AssertZero(game);
        }

        TEST_METHOD(RejectedOperationsPreserveStatistics)
        {
            auto game = PlayingGame();
            advanceGameTime(game, 2s);
            tryMove(game, MoveDirection::Right);
            MazeGrid empty;
            Assert::ExpectException<std::invalid_argument>([&] { startGame(game, std::move(empty)); });
            Assert::IsTrue(game.statistics.elapsed == 2s && game.statistics.successfulMoves == 1);
            game.phase = GamePhase::Menu;
            Assert::IsFalse(restartGame(game));
            Assert::IsTrue(tryMove(game, MoveDirection::Down) == MoveResult::Blocked);
            Assert::IsTrue(game.statistics.elapsed == 2s && game.statistics.successfulMoves == 1);
        }

        TEST_METHOD(TimeAndMovesSaturateInsteadOfWrapping)
        {
            auto game = PlayingGame();
            using Duration = std::chrono::steady_clock::duration;
            game.statistics.elapsed = Duration::max() - 10ms;
            advanceGameTime(game, 20ms);
            advanceGameTime(game, 1s);
            Assert::IsTrue(game.statistics.elapsed == Duration::max());
            game.statistics.successfulMoves = std::numeric_limits<std::uint64_t>::max() - 1;
            tryMove(game, MoveDirection::Right);
            tryMove(game, MoveDirection::Right);
            Assert::IsTrue(game.statistics.successfulMoves == std::numeric_limits<std::uint64_t>::max());
        }
    };
}
