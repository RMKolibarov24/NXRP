#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Game.h"
#include "Maze.Generation.h"

#include <stdexcept>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        MazeGrid PreparedMaze(const MazeSize& size)
        {
            MazeGrid maze(size);
            maze.fill(MazeCell::Passage);
            maze.at(size.rows - 1, size.cols - 1) = MazeCell::Exit;
            return maze;
        }

        std::vector<MazeCell> ReadCells(const MazeGrid& maze)
        {
            std::vector<MazeCell> result;
            for (int row = 0; row < maze.size().rows; ++row)
                for (int col = 0; col < maze.size().cols; ++col)
                    result.push_back(maze.at(row, col));
            return result;
        }

        void AssertEnded(const GameState& game)
        {
            Assert::IsTrue(game.maze.empty());
            Assert::IsFalse(game.hasActiveGame());
            Assert::AreEqual(std::size_t{ 0 }, game.maze.cellCount());
            Assert::AreEqual(0, game.maze.size().rows);
            Assert::AreEqual(0, game.maze.size().cols);
            Assert::AreEqual(0, game.player.row);
            Assert::AreEqual(0, game.player.col);
            Assert::IsTrue(game.phase == GamePhase::Menu);
            Assert::IsFalse(hasWon(game));
        }
    }

    TEST_CLASS(MazeSessionTests)
    {
    public:
        TEST_METHOD(EndGameReleasesMazeAndResetsState)
        {
            GameState game;
            startGame(game, PreparedMaze({ 10, 10 }));
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
            game.selectedSize = MazeSize(50, 50);
            endGame(game);
            AssertEnded(game);
            Assert::AreEqual(50, game.selectedSize.rows);
            Assert::AreEqual(50, game.selectedSize.cols);
        }

        TEST_METHOD(EndGameIsSafeWhenAlreadyEmpty)
        {
            GameState game;
            endGame(game);
            endGame(game);
            AssertEnded(game);
            Assert::AreEqual(10, game.selectedSize.rows);
            Assert::AreEqual(10, game.selectedSize.cols);
        }

        TEST_METHOD(EndGameHandlesPausedAndWonGames)
        {
            for (const auto phase : { GamePhase::Paused, GamePhase::Won })
            {
                GameState game;
                startGame(game, PreparedMaze({ 50, 50 }));
                game.phase = phase;
                game.player = phase == GamePhase::Won ? MazePosition{ 49, 49 } : MazePosition{ 0, 1 };
                endGame(game);
                AssertEnded(game);
                Assert::AreEqual(50, game.selectedSize.rows);
                Assert::AreEqual(50, game.selectedSize.cols);
            }
        }

        TEST_METHOD(RepeatedStartsReplaceSmallAndLargeMazes)
        {
            GameState game;
            for (int iteration = 0; iteration < 32; ++iteration)
            {
                const int dimension = iteration % 2 == 0 ? 10 : 50;
                startGame(game, PreparedMaze({ dimension, dimension }));
                Assert::AreEqual(dimension, game.maze.size().rows);
                Assert::AreEqual(dimension, game.maze.size().cols);
                Assert::AreEqual(dimension, game.selectedSize.rows);
                Assert::AreEqual(dimension, game.selectedSize.cols);
                Assert::AreEqual(0, game.player.row);
                Assert::AreEqual(0, game.player.col);
                Assert::IsTrue(game.phase == GamePhase::Playing);
                Assert::IsTrue(game.maze.at(dimension - 1, dimension - 1) == MazeCell::Exit);
                Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
                game.phase = GamePhase::Paused;
            }
            endGame(game);
            AssertEnded(game);
        }

        TEST_METHOD(InvalidReplacementPreservesExistingSession)
        {
            GameState game;
            startGame(game, PreparedMaze({ 10, 10 }));
            game.player = { 0, 1 };
            game.phase = GamePhase::Paused;
            game.selectedSize = MazeSize(50, 50);
            const auto* existingStorage = &game.maze.at(0, 0);
            const auto existingCells = ReadCells(game.maze);

            for (int scenario = 0; scenario < 5; ++scenario)
            {
                MazeGrid candidate;
                if (scenario != 0) candidate = PreparedMaze({ 50, 50 });
                if (scenario == 1) candidate.at(0, 0) = MazeCell::Wall;
                if (scenario == 2) candidate.at(49, 49) = MazeCell::Passage;
                if (scenario == 3) candidate.at(1, 1) = MazeCell::Exit;
                if (scenario == 4) candidate.at(1, 1) = static_cast<MazeCell>(99);
                const auto* candidateStorage = candidate.empty() ? nullptr : &candidate.at(0, 0);
                const auto candidateCells = ReadCells(candidate);
                const auto candidateSize = candidate.size();

                Assert::ExpectException<std::invalid_argument>([&game, &candidate] {
                    startGame(game, std::move(candidate));
                });
                Assert::IsTrue(existingStorage == &game.maze.at(0, 0));
                Assert::IsTrue(existingCells == ReadCells(game.maze));
                Assert::AreEqual(10, game.maze.size().rows);
                Assert::AreEqual(10, game.maze.size().cols);
                Assert::AreEqual(0, game.player.row);
                Assert::AreEqual(1, game.player.col);
                Assert::IsTrue(game.phase == GamePhase::Paused);
                Assert::AreEqual(50, game.selectedSize.rows);
                Assert::AreEqual(50, game.selectedSize.cols);
                Assert::IsTrue(candidateStorage == (candidate.empty() ? nullptr : &candidate.at(0, 0)));
                Assert::AreEqual(candidateSize.rows, candidate.size().rows);
                Assert::AreEqual(candidateSize.cols, candidate.size().cols);
                Assert::IsTrue(candidateCells == ReadCells(candidate));
            }
        }

        TEST_METHOD(CanStartAgainAfterEndGame)
        {
            GameState game;
            startGame(game, PreparedMaze({ 50, 50 }));
            endGame(game);
            AssertEnded(game);
            startGame(game, PreparedMaze({ 10, 50 }));
            Assert::IsTrue(game.hasActiveGame());
            Assert::IsTrue(game.phase == GamePhase::Playing);
            Assert::AreEqual(10, game.maze.size().rows);
            Assert::AreEqual(50, game.maze.size().cols);
            Assert::AreEqual(0, game.player.row);
            Assert::AreEqual(0, game.player.col);
        }

        TEST_METHOD(RestartPlayingGameKeepsLayoutAndAllocation)
        {
            for (const auto size : { MazeSize(10, 10), MazeSize(50, 50), MazeSize(10, 50) })
            {
                GameState game;
                startGame(game, generateMaze(size, 42));
                auto moved = tryMove(game, MoveDirection::Right);
                if (moved == MoveResult::Blocked) moved = tryMove(game, MoveDirection::Down);
                Assert::IsTrue(moved == MoveResult::Moved);
                const auto* storage = &game.maze.at(0, 0);
                const auto cells = ReadCells(game.maze);

                Assert::IsTrue(restartGame(game));
                Assert::IsTrue(storage == &game.maze.at(0, 0));
                Assert::IsTrue(cells == ReadCells(game.maze));
                Assert::AreEqual(size.rows, game.maze.size().rows);
                Assert::AreEqual(size.cols, game.maze.size().cols);
                Assert::AreEqual(0, game.player.row);
                Assert::AreEqual(0, game.player.col);
                Assert::IsTrue(game.phase == GamePhase::Playing);
            }
        }

        TEST_METHOD(RestartPausedGameResetsPositionAndEntersPlaying)
        {
            GameState game;
            startGame(game, PreparedMaze({ 12, 37 }));
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
            Assert::IsTrue(tryMove(game, MoveDirection::Down) == MoveResult::Moved);
            game.phase = GamePhase::Paused;

            Assert::IsTrue(restartGame(game));
            Assert::AreEqual(0, game.player.row);
            Assert::AreEqual(0, game.player.col);
            Assert::IsTrue(game.phase == GamePhase::Playing);
            Assert::IsTrue(tryMove(game, MoveDirection::Right) == MoveResult::Moved);
        }

        TEST_METHOD(RestartWonGameCanBePlayedAgain)
        {
            GameState game;
            startGame(game, PreparedMaze({ 10, 10 }));
            for (int round = 0; round < 2; ++round)
            {
                for (int i = 0; i < 9; ++i) tryMove(game, MoveDirection::Right);
                for (int i = 0; i < 9; ++i) tryMove(game, MoveDirection::Down);
                Assert::IsTrue(hasWon(game));
                Assert::IsTrue(game.phase == GamePhase::Won);
                Assert::IsTrue(restartGame(game));
                Assert::IsFalse(hasWon(game));
                Assert::AreEqual(0, game.player.row);
                Assert::AreEqual(0, game.player.col);
                Assert::IsTrue(game.phase == GamePhase::Playing);
            }
        }

        TEST_METHOD(RestartPreservesNextGameSize)
        {
            GameState game;
            startGame(game, PreparedMaze({ 10, 10 }));
            game.selectedSize = MazeSize(50, 37);
            game.phase = GamePhase::Paused;
            Assert::IsTrue(restartGame(game));
            Assert::AreEqual(50, game.selectedSize.rows);
            Assert::AreEqual(37, game.selectedSize.cols);
            Assert::AreEqual(10, game.maze.size().rows);
            Assert::AreEqual(10, game.maze.size().cols);
        }

        TEST_METHOD(RestartWithoutSessionChangesNothing)
        {
            for (const auto phase : { GamePhase::Menu, GamePhase::Playing, GamePhase::Paused, GamePhase::Won })
            {
                GameState game;
                game.phase = phase;
                game.player = { 7, 9 };
                game.selectedSize = MazeSize(50, 10);
                Assert::IsFalse(restartGame(game));
                Assert::IsTrue(game.maze.empty());
                Assert::AreEqual(7, game.player.row);
                Assert::AreEqual(9, game.player.col);
                Assert::AreEqual(50, game.selectedSize.rows);
                Assert::AreEqual(10, game.selectedSize.cols);
                Assert::IsTrue(game.phase == phase);
            }
        }

        TEST_METHOD(RestartRejectsUnstartedOrUnknownPhase)
        {
            for (const auto phase : { GamePhase::Menu, static_cast<GamePhase>(99) })
            {
                GameState game;
                game.maze = PreparedMaze({ 10, 10 });
                game.phase = phase;
                game.player = { 1, 1 };
                game.selectedSize = MazeSize(20, 30);
                const auto* storage = &game.maze.at(0, 0);
                const auto cells = ReadCells(game.maze);
                Assert::IsFalse(restartGame(game));
                Assert::IsTrue(game.phase == phase);
                Assert::AreEqual(1, game.player.row);
                Assert::AreEqual(1, game.player.col);
                Assert::AreEqual(20, game.selectedSize.rows);
                Assert::AreEqual(30, game.selectedSize.cols);
                Assert::IsTrue(storage == &game.maze.at(0, 0));
                Assert::IsTrue(cells == ReadCells(game.maze));
            }
        }
    };
}
