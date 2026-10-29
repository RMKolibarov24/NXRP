#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Game.h"
#include "Maze.Generation.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        constexpr std::uint32_t fixedSeeds[] = { 0, 1, 42, 12345, 0x80000000U, UINT32_MAX };
        const MazeSize invalidSizes[] = {
            { 9, 10 }, { 51, 10 }, { 10, 9 }, { 10, 51 }, { 0, 10 }, { 10, 0 },
            { -1, -1 }, { INT_MIN, 10 }, { INT_MAX, 10 }, { 10, INT_MIN }, { 10, INT_MAX }
        };

        std::wstring Context(const MazeSize& size, std::uint32_t seed)
        {
            return L"rows=" + std::to_wstring(size.rows) + L", cols=" + std::to_wstring(size.cols)
                + L", seed=" + std::to_wstring(seed);
        }

        std::vector<MazeCell> ReadCells(const MazeGrid& maze)
        {
            std::vector<MazeCell> result;
            for (int row = 0; row < maze.size().rows; ++row)
                for (int col = 0; col < maze.size().cols; ++col)
                    result.push_back(maze.at(row, col));
            return result;
        }

        struct RouteStep
        {
            MoveDirection direction;
            MazePosition destination;
        };

        std::vector<RouteStep> AssertConnectedMaze(const MazeGrid& maze, const MazeSize& size,
            const wchar_t* context)
        {
            Assert::IsFalse(maze.empty(), context);
            Assert::AreEqual(size.rows, maze.size().rows, context);
            Assert::AreEqual(size.cols, maze.size().cols, context);
            const int total = size.rows * size.cols;
            Assert::AreEqual(static_cast<std::size_t>(total), maze.cellCount(), context);
            int exits = 0, walls = 0, walkable = 0;
            for (const auto cell : ReadCells(maze))
            {
                Assert::IsTrue(cell == MazeCell::Wall || cell == MazeCell::Passage || cell == MazeCell::Exit, context);
                exits += cell == MazeCell::Exit;
                walls += cell == MazeCell::Wall;
                walkable += cell != MazeCell::Wall;
            }
            Assert::AreEqual(1, exits, context);
            Assert::IsTrue(walls > 0, context);
            Assert::IsTrue(maze.at(0, 0) == MazeCell::Passage, context);
            Assert::IsTrue(maze.at(size.rows - 1, size.cols - 1) == MazeCell::Exit, context);

            // This independent BFS reads one-cell neighbors. It does not call
            // tryMove or reuse the generator's two-cell DFS carving rules.
            const struct { int row, col; MoveDirection direction; } steps[] = {
                { -1, 0, MoveDirection::Up }, { 1, 0, MoveDirection::Down },
                { 0, -1, MoveDirection::Left }, { 0, 1, MoveDirection::Right }
            };
            std::vector<int> parent(static_cast<std::size_t>(total), -1);
            std::vector<MoveDirection> arrival(static_cast<std::size_t>(total));
            std::queue<int> pending;
            parent[0] = 0;
            pending.push(0);
            int reached = 0;
            while (!pending.empty())
            {
                const int current = pending.front();
                pending.pop();
                ++reached;
                for (const auto step : steps)
                {
                    const int row = current / size.cols + step.row;
                    const int col = current % size.cols + step.col;
                    if (row < 0 || row >= size.rows || col < 0 || col >= size.cols) continue;
                    const int next = row * size.cols + col;
                    if (parent[next] != -1 || maze.at(row, col) == MazeCell::Wall) continue;
                    parent[next] = current;
                    arrival[next] = step.direction;
                    pending.push(next);
                }
            }
            Assert::IsTrue(parent[total - 1] != -1, context);
            Assert::AreEqual(walkable, reached, context);
            std::vector<RouteStep> route;
            for (int cell = total - 1; cell != 0; cell = parent[cell])
                route.push_back({ arrival[cell], { cell / size.cols, cell % size.cols } });
            std::reverse(route.begin(), route.end());
            return route;
        }

        void AssertSizeForFixedSeeds(const MazeSize& size)
        {
            for (const auto seed : fixedSeeds)
            {
                const auto context = Context(size, seed);
                const auto maze = generateMaze(size, seed);
                (void)AssertConnectedMaze(maze, size, context.c_str());
            }
        }
    }

    TEST_CLASS(MazeGenerationTests)
    {
    public:
        TEST_METHOD(GeneratesMinimumSizeWithReachableExit)
        {
            AssertSizeForFixedSeeds({ 10, 10 });
        }

        TEST_METHOD(GeneratesIntermediateSizeWithReachableExit)
        {
            AssertSizeForFixedSeeds({ 25, 37 });
        }

        TEST_METHOD(GeneratesMaximumSizeWithReachableExit)
        {
            AssertSizeForFixedSeeds({ 50, 50 });
        }

        TEST_METHOD(SupportsEvenAndOddRectangles)
        {
            const MazeSize sizes[] = { { 10, 50 }, { 50, 10 }, { 11, 12 },
                { 12, 11 }, { 49, 50 }, { 50, 49 } };
            for (const auto& size : sizes) AssertSizeForFixedSeeds(size);
        }

        TEST_METHOD(ReproducesMapsForFixedSeeds)
        {
            for (const MazeSize size : { MazeSize(10, 10), MazeSize(25, 37), MazeSize(50, 50) })
            {
                for (const auto seed : fixedSeeds)
                {
                    const auto context = Context(size, seed);
                    const auto first = generateMaze(size, seed);
                    const auto repeated = generateMaze(size, seed);
                    Assert::AreEqual(first.size().rows, repeated.size().rows, context.c_str());
                    Assert::AreEqual(first.size().cols, repeated.size().cols, context.c_str());
                    Assert::IsTrue(ReadCells(first) == ReadCells(repeated), context.c_str());
                }
            }
        }

        TEST_METHOD(RejectsInvalidSizesWithSeed)
        {
            for (const auto& size : invalidSizes)
            {
                const auto context = Context(size, 42);
                Assert::ExpectException<std::invalid_argument>([size] {
                    (void)generateMaze(size, 42);
                }, context.c_str());
            }
        }

        TEST_METHOD(RejectsInvalidSizesWithoutSeed)
        {
            for (const auto& size : invalidSizes)
            {
                const auto context = Context(size, 0);
                Assert::ExpectException<std::invalid_argument>([size] {
                    (void)generateMaze(size);
                }, context.c_str());
            }
        }

        TEST_METHOD(AllAllowedDimensionsHaveReachableExit)
        {
            for (int rows = 10; rows <= 50; ++rows)
            {
                for (int cols = 10; cols <= 50; ++cols)
                {
                    const MazeSize size(rows, cols);
                    const auto seed = static_cast<std::uint32_t>(rows * 100 + cols);
                    const auto context = Context(size, seed);
                    const auto maze = generateMaze(size, seed);
                    (void)AssertConnectedMaze(maze, size, context.c_str());
                }
            }
        }

        TEST_METHOD(GeneratedMazesCanBePlayedToVictory)
        {
            const MazeSize sizes[] = { { 10, 10 }, { 25, 37 }, { 50, 50 },
                { 10, 50 }, { 50, 10 }, { 11, 12 } };
            for (const auto& size : sizes)
            {
                for (const auto seed : fixedSeeds)
                {
                    const auto context = Context(size, seed);
                    auto maze = generateMaze(size, seed);
                    const auto original = ReadCells(maze);
                    const auto route = AssertConnectedMaze(maze, size, context.c_str());
                    GameState game;
                    startGame(game, std::move(maze));
                    Assert::AreEqual(0, game.player.row, context.c_str());
                    Assert::AreEqual(0, game.player.col, context.c_str());
                    Assert::IsFalse(hasWon(game), context.c_str());
                    for (std::size_t index = 0; index < route.size(); ++index)
                    {
                        const auto expected = index + 1 == route.size() ? MoveResult::Won : MoveResult::Moved;
                        Assert::IsTrue(tryMove(game, route[index].direction) == expected, context.c_str());
                        Assert::AreEqual(route[index].destination.row, game.player.row, context.c_str());
                        Assert::AreEqual(route[index].destination.col, game.player.col, context.c_str());
                    }
                    Assert::IsTrue(hasWon(game), context.c_str());
                    Assert::IsTrue(game.phase == GamePhase::Won, context.c_str());
                    Assert::AreEqual(size.rows - 1, game.player.row, context.c_str());
                    Assert::AreEqual(size.cols - 1, game.player.col, context.c_str());
                    Assert::IsTrue(original == ReadCells(game.maze), context.c_str());
                }
            }
        }

        TEST_METHOD(UnseededMazesAreValid)
        {
            for (const MazeSize size : { MazeSize(10, 10), MazeSize(25, 37), MazeSize(50, 50) })
            {
                const auto context = Context(size, 0);
                const auto maze = generateMaze(size);
                (void)AssertConnectedMaze(maze, size, context.c_str());
            }
        }

        TEST_METHOD(KnownFixedSeedsShowVariation)
        {
            std::set<std::vector<MazeCell>> layouts;
            for (const auto seed : fixedSeeds)
                layouts.insert(ReadCells(generateMaze(MazeSize(10, 10), seed)));
            // This fixed sample must demonstrate variation; individual maps may repeat.
            Assert::IsTrue(layouts.size() > 1, L"All sampled fixed seeds produced the same layout.");
        }
    };
}
