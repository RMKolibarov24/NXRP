#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Generation.h"
#include "Maze.Puzzle.h"
#include "Maze.Safehouse.h"
#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    TEST_CLASS(MazePuzzleTests)
    {
    public:
        TEST_METHOD(SeededStonesAreReachableAndReproducible)
        {
            const MazeSize sizes[] = { {10, 10}, {11, 12}, {30, 25}, {50, 50} };
            std::set<std::string> answers;
            std::set<int> rules;
            for (const auto size : sizes) for (std::uint32_t seed = 0; seed < 32; ++seed)
            {
                const auto maze = generateMaze(size, seed);
                const auto puzzle = generateMazePuzzle(maze, seed);
                const auto replay = generateMazePuzzle(maze, seed);
                std::vector<bool> reached(maze.cellCount(), false);
                std::queue<MazePosition> pending;
                pending.push({ 0, 0 }); reached[0] = true;
                constexpr MazePosition steps[] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
                while (!pending.empty())
                {
                    const auto p = pending.front(); pending.pop();
                    for (auto d : steps)
                    {
                        const MazePosition q{ p.row + d.row, p.col + d.col };
                        if (!maze.contains(q) || maze.at(q.row, q.col) != MazeCell::Passage) continue;
                        const auto index = static_cast<std::size_t>(q.row * size.cols + q.col);
                        if (!reached[index]) { reached[index] = true; pending.push(q); }
                    }
                }
                std::set<int> order, numbers, positions;
                std::string answer;
                for (int i = 0; i < 3; ++i)
                {
                    const auto p = puzzle.stones[i], r = replay.stones[i];
                    Assert::IsTrue(maze.contains(p));
                    Assert::IsTrue(maze.at(p.row, p.col) == MazeCell::Passage);
                    Assert::IsTrue(safehouseAt(p, generateSafehouses(size, seed)) < 0);
                    Assert::IsTrue(reached[static_cast<std::size_t>(p.row * size.cols + p.col)]);
                    Assert::IsTrue(p.row == r.row && p.col == r.col);
                    Assert::AreEqual(puzzle.order[i], replay.order[i]);
                    Assert::AreEqual(puzzle.numbers[i], replay.numbers[i]);
                    Assert::IsTrue(puzzle.order[i] >= 0 && puzzle.order[i] < 3);
                    Assert::IsTrue(puzzle.numbers[i] >= 1 && puzzle.numbers[i] <= 9);
                    order.insert(puzzle.order[i]); numbers.insert(puzzle.numbers[i]);
                    positions.insert(p.row * size.cols + p.col);
                }
                Assert::AreEqual(static_cast<std::size_t>(3), order.size());
                Assert::AreEqual(static_cast<std::size_t>(3), numbers.size());
                Assert::AreEqual(static_cast<std::size_t>(3), positions.size());
                Assert::IsTrue(puzzle.gateRule == replay.gateRule);
                Assert::IsTrue(puzzle.stoneKind == replay.stoneKind);
                Assert::AreEqual(puzzle.balanceTarget, replay.balanceTarget);
                Assert::IsTrue(gateCode(puzzle) == gateCode(replay));
                for (auto digit : gateCode(puzzle)) answer += static_cast<char>('0' + digit);
                answers.insert(answer); rules.insert(static_cast<int>(puzzle.gateRule));
            }
            Assert::IsTrue(answers.size() >= 80);
            Assert::AreEqual(static_cast<std::size_t>(5), rules.size());
        }

        TEST_METHOD(BalanceTargetsHaveExactlyOneTwoStoneAnswerAcrossSeeds)
        {
            std::set<int> targets;
            std::set<unsigned> answers;
            for (std::uint32_t seed = 1; seed < 256; seed += 2)
            {
                const auto puzzle = generateMazePuzzle(generateMaze({10, 10}, seed), seed);
                Assert::IsTrue(puzzle.stoneKind == StonePuzzleKind::Balance);
                unsigned answer = 0; int matches = 0;
                for (unsigned mask = 1; mask < 8; ++mask)
                    if (stoneBalance(puzzle, mask) == puzzle.balanceTarget) { answer = mask; ++matches; }
                Assert::AreEqual(1, matches);
                Assert::IsTrue(answer == 3u || answer == 5u || answer == 6u);
                answers.insert(answer); targets.insert(puzzle.balanceTarget);
            }
            Assert::AreEqual(static_cast<std::size_t>(3), answers.size());
            Assert::IsTrue(targets.size() >= 8);
        }

        TEST_METHOD(SequenceAndBalanceUseDifferentInteractionRules)
        {
            MazePuzzle puzzle; puzzle.order = { 2, 0, 1 }; puzzle.numbers = { 1, 2, 4 };
            int progress = 0; unsigned active = 0;
            Assert::IsTrue(touchStone(puzzle, 2, progress, active) == StoneTouchResult::Changed);
            Assert::AreEqual(1, progress); Assert::AreEqual(4u, active);
            Assert::IsTrue(touchStone(puzzle, 1, progress, active) == StoneTouchResult::Reset);
            Assert::AreEqual(0, progress); Assert::AreEqual(0u, active);
            for (int stone : puzzle.order) touchStone(puzzle, stone, progress, active);
            Assert::AreEqual(3, progress); Assert::AreEqual(7u, active);
            Assert::IsTrue(touchStone(puzzle, 2, progress, active) == StoneTouchResult::AlreadySolved);

            puzzle.stoneKind = StonePuzzleKind::Balance; puzzle.balanceTarget = 5;
            progress = 0; active = 0;
            Assert::IsTrue(touchStone(puzzle, 0, progress, active) == StoneTouchResult::Changed);
            Assert::AreEqual(1, stoneBalance(puzzle, active));
            Assert::IsTrue(touchStone(puzzle, 0, progress, active) == StoneTouchResult::Changed);
            Assert::AreEqual(0u, active);
            Assert::IsTrue(touchStone(puzzle, 2, progress, active) == StoneTouchResult::Changed);
            Assert::IsTrue(touchStone(puzzle, 0, progress, active) == StoneTouchResult::Solved);
            Assert::AreEqual(3, progress); Assert::AreEqual(5u, active);
            Assert::IsTrue(touchStone(puzzle, 1, progress, active) == StoneTouchResult::AlreadySolved);
            Assert::IsTrue(touchStone(puzzle, -1, progress, active) == StoneTouchResult::Invalid);
            Assert::AreEqual(5u, active);
        }

        TEST_METHOD(GateInscriptionsProduceDistinctCodes)
        {
            MazePuzzle puzzle;
            puzzle.order = { 2, 0, 1 }; puzzle.numbers = { 1, 3, 4 };
            puzzle.gateRule = GateRule::Forward;
            const std::array<int, 3> forward{ 4, 1, 3 };
            Assert::IsTrue(gateCode(puzzle) == forward);
            puzzle.gateRule = GateRule::Reverse;
            const std::array<int, 3> reverse{ 3, 1, 4 };
            Assert::IsTrue(gateCode(puzzle) == reverse);
            puzzle.gateRule = GateRule::Advance;
            const std::array<int, 3> advance{ 5, 2, 4 };
            Assert::IsTrue(gateCode(puzzle) == advance);
            puzzle.gateRule = GateRule::Rotate;
            const std::array<int, 3> rotate{ 1, 3, 4 };
            Assert::IsTrue(gateCode(puzzle) == rotate);
            puzzle.gateRule = GateRule::Complement;
            const std::array<int, 3> complement{ 6, 9, 7 };
            Assert::IsTrue(gateCode(puzzle) == complement);
            puzzle.gateRule = GateRule::Advance;
            puzzle.numbers[2] = 9;
            Assert::AreEqual(1, gateCode(puzzle)[0]);
        }

        TEST_METHOD(RejectsMazeWithoutEnoughReachablePassages)
        {
            MazeGrid maze({ 10, 10 });
            maze.at(0, 0) = MazeCell::Passage;
            maze.at(8, 8) = MazeCell::Passage;
            maze.at(8, 9) = MazeCell::Passage;
            maze.at(9, 8) = MazeCell::Passage;
            maze.at(9, 9) = MazeCell::Exit;
            Assert::ExpectException<std::invalid_argument>([&maze] { generateMazePuzzle(maze, 1); });
        }
    };
}
