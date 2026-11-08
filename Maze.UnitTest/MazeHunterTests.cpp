#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Generation.h"
#include "Maze.Hunter.h"
#include "Maze.Safehouse.h"
#include "Maze.Acoustics.h"
#include "Maze.Puzzle.h"
#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    TEST_CLASS(MazeHunterTests)
    {
    public:
        TEST_METHOD(HiddenPlayerMovementCannotSteerRememberedSearch)
        {
            MazeGrid maze({12,12});maze.fill(MazeCell::Passage);
            HunterState first;first.cell={5,5};first.previous={5,4};first.lastKnown={5,7};
            first.searchGoal=first.lastKnown;first.mode=HunterMode::Search;first.memory=12;first.random=7;
            auto second=first;
            // Both silent players are hidden behind an upper floor. Their actual
            // coordinates differ each step; identical remembered searches must result.
            for(int step=0;step<18;++step)
            {
                const HunterTarget a{{2+step%3,3},false,false,0,false,false,2.f};
                const HunterTarget b{{8,7+step%3},false,false,0,false,false,2.f};
                advanceHunterGroup(maze,first,&a,1);advanceHunterGroup(maze,second,&b,1);
                Assert::AreEqual(first.cell.row,second.cell.row);Assert::AreEqual(first.cell.col,second.cell.col);
                Assert::IsTrue(first.mode==second.mode);
                Assert::AreEqual(5,first.lastKnown.row);Assert::AreEqual(7,first.lastKnown.col);
            }
        }
        TEST_METHOD(SearchLingersThenChecksAreaAndEventuallyGivesUp)
        {
            MazeGrid maze({12,12});maze.fill(MazeCell::Passage);
            HunterState hunter;hunter.cell={5,5};hunter.lastKnown={5,5};
            hunter.searchGoal={5,5};hunter.memory=7;hunter.mode=HunterMode::Search;
            hunter.previous={5,4};hunter.random=42;
            advanceHunterGroup(maze,hunter,nullptr,0);
            Assert::IsTrue(hunter.mode==HunterMode::Search && hunter.searchingArea);
            Assert::AreEqual(5,hunter.cell.col);Assert::AreEqual(5,hunter.cell.row);
            advanceHunterGroup(maze,hunter,nullptr,0);
            Assert::AreEqual(5,hunter.cell.col);Assert::AreEqual(5,hunter.cell.row);
            advanceHunterGroup(maze,hunter,nullptr,0);
            Assert::IsTrue(hunter.cell.col!=5 || hunter.cell.row!=5);
            for(int step=0;step<20;++step)advanceHunterGroup(maze,hunter,nullptr,0);
            Assert::IsTrue(hunter.mode==HunterMode::Patrol && hunter.memory==0);
            Assert::AreEqual(5,hunter.lastKnown.row);Assert::AreEqual(5,hunter.lastKnown.col);
        }
        TEST_METHOD(OnlyObservedJunctionExitsAreLearnedAndBiasSearch)
        {
            MazeGrid maze({12,12});maze.fill(MazeCell::Passage);
            HunterState hunter;hunter.cell={4,1};hunter.previous={4,0};
            HunterTarget target{{4,4},false,false};
            advanceHunterGroup(maze,hunter,&target,1);
            target.cell={4,5};advanceHunterGroup(maze,hunter,&target,1);
            Assert::IsTrue(hunter.routes[0].junction.row==4 && hunter.routes[0].junction.col==4);
            Assert::AreEqual(5,hunter.routes[0].exit.col);
            const unsigned confidence=hunter.routes[0].confidence;
            target.audibleOnly=true;target.cell={4,6};
            advanceHunterGroup(maze,hunter,&target,1);
            Assert::AreEqual(confidence,hunter.routes[0].confidence);
            for(const auto& route:hunter.routes)Assert::IsFalse(route.junction.col==5 && route.confidence>0);
            hunter.routes[0].confidence=5;
            int chosen=0;
            for(unsigned seed=1;seed<=100;++seed)
            {
                auto probe=hunter;probe.cell=probe.lastKnown=probe.searchGoal={4,4};
                probe.previous={4,3};probe.searchingArea=true;probe.linger=0;probe.memory=5;probe.random=seed;
                advanceHunterGroup(maze,probe,nullptr,0);
                chosen+=probe.cell.row==4 && probe.cell.col==5;
            }
            Assert::IsTrue(chosen>60 && chosen<100); // learned, but still escapable
        }
        TEST_METHOD(FloorsHideSilentPlayersAndMuffleTheirFootsteps)
        {
            MazeGrid maze({12,12});maze.fill(MazeCell::Passage);
            HunterState hunter;hunter.cell={5,5};hunter.previous={5,4};
            HunterTarget upstairs{{5,6},false,false,0,false,false,2.f};
            Assert::AreEqual(0,senseHunterTarget(maze,hunter,upstairs));
            upstairs.moving=true;
            Assert::AreEqual(0,senseHunterTarget(maze,hunter,upstairs));
            upstairs.running=true;
            Assert::AreEqual(1,senseHunterTarget(maze,hunter,upstairs));
            advanceHunterGroup(maze,hunter,&upstairs,1);
            Assert::IsTrue(hunter.mode==HunterMode::Search);
            Assert::AreEqual(2.f,hunter.lastKnownElevation);
        }
        TEST_METHOD(ClosedDoorsMuffleAudioAndBlockSight)
        {
            MazeGrid maze({12,12});maze.fill(MazeCell::Passage);
            const std::vector<MazePosition> houses{{5,5}};
            const MazePosition outside{5,2},inside{5,5};
            const auto clear=acousticTransmission(maze,outside,inside,houses,0);
            const auto closed=acousticTransmission(maze,outside,inside,houses,1u);
            Assert::IsTrue(closed.gain<clear.gain && closed.clarity<clear.clarity);
            Assert::IsTrue(acousticReachable(maze,outside,inside,3,houses,0));
            Assert::IsFalse(acousticReachable(maze,outside,inside,3,houses,1u));
            HunterState hunter;hunter.cell=outside;hunter.previous={5,1};hunter.random=7;
            const HunterTarget quiet{inside,false,false};
            advanceHunterGroup(maze,hunter,&quiet,1,houses,1u);
            Assert::IsTrue(hunter.mode==HunterMode::Patrol);
            hunter.cell=outside;hunter.previous={5,1};
            advanceHunterGroup(maze,hunter,&quiet,1,houses,0);
            Assert::IsTrue(hunter.mode==HunterMode::Chase);
        }
        TEST_METHOD(FoliageLayersMuffleHighFrequencies)
        {
            MazeGrid maze({10,10});maze.fill(MazeCell::Passage);
            const auto clear=acousticTransmission(maze,{4,1},{4,7});
            maze.at(4,3)=MazeCell::Wall;
            const auto one=acousticTransmission(maze,{4,1},{4,7});
            maze.at(4,5)=MazeCell::Wall;
            const auto two=acousticTransmission(maze,{4,1},{4,7});
            Assert::IsTrue(clear.gain>one.gain && one.gain>two.gain);
            Assert::IsTrue(clear.clarity>one.clarity && one.clarity>two.clarity);
        }
        TEST_METHOD(DistantHunterRelocationStaysOutOfViewAndRefuges)
        {
            MazeGrid maze({30,30}); maze.fill(MazeCell::Passage);
            const HunterTarget player{ {2,2}, true, true };
            const std::vector<MazePosition> houses{ {15,15} };
            const std::array<MazePosition,3> stones{ MazePosition{10,10}, MazePosition{12,12}, MazePosition{14,14} };
            HunterState first; first.cell={29,29};
            HunterState replay=first;
            Assert::IsTrue(relocateDistantHunter(maze,first,&player,1,stones,houses,42,1));
            Assert::IsTrue(relocateDistantHunter(maze,replay,&player,1,stones,houses,42,1));
            Assert::AreEqual(first.cell.row,replay.cell.row); Assert::AreEqual(first.cell.col,replay.cell.col);
            const int dr=first.cell.row-player.cell.row, dc=first.cell.col-player.cell.col;
            Assert::IsTrue(dr*dr+dc*dc >= 14*14 && dr*dr+dc*dc <= 22*22);
            Assert::IsTrue(safehouseAt(first.cell,houses)<0);
            Assert::IsFalse(relocateDistantHunter(maze,first,&player,1,stones,houses,42,2));
        }
        TEST_METHOD(HunterEntersRefugesAndLearnsFromRepeatedDetections)
        {
            MazeGrid maze({10,10});
            for (int col=0;col<10;++col) maze.at(4,col)=MazeCell::Passage;
            const std::vector<MazePosition> houses{{4,5}};
            HunterState hunter; hunter.cell={4,2}; hunter.previous={4,1}; hunter.random=7;
            const HunterTarget player{{4,5},true,true};
            bool entered=false;
            for (int step=0;step<8;++step)
            {
                advanceHunterGroup(maze,hunter,&player,1,houses);
                entered=entered || safehouseAt(hunter.cell,houses)>=0;
            }
            Assert::IsTrue(entered && hunter.vigilance>0);
            Assert::IsTrue(hunter.mode==HunterMode::Chase);
            Assert::IsFalse(refugeEdgeAllowed({1,4},{2,4},houses));
            Assert::IsTrue(refugeEdgeAllowed({1,5},{2,5},houses));
        }
        TEST_METHOD(CoopHunterSeesEitherPlayerAndPrioritizesSightOverNoise)
        {
            MazeGrid maze({10, 10});
            for (int col = 1; col <= 7; ++col) maze.at(4, col) = MazeCell::Passage;
            HunterState hunter; hunter.cell = {4,4}; hunter.previous = {4,3}; hunter.random = 7;
            const HunterTarget nearbyRunner{ {4,2}, true, true };
            const HunterTarget visiblePlayer{ {4,7}, false, false };
            advanceHunterCoop(maze, hunter, nearbyRunner, visiblePlayer);
            Assert::IsTrue(hunter.mode == HunterMode::Chase);
            Assert::AreEqual(7, hunter.lastKnown.col);
            Assert::AreEqual(5, hunter.cell.col);
        }

        TEST_METHOD(GroupHunterDetectsTheFourthPlayer)
        {
            MazeGrid maze({10, 10});
            for (int col = 1; col <= 8; ++col) maze.at(4, col) = MazeCell::Passage;
            HunterState hunter; hunter.cell = {4,4}; hunter.previous = {4,3}; hunter.random = 7;
            const HunterTarget targets[] = { {{4,1}, false, false}, {{4,2}, false, false},
                {{4,1}, false, false}, {{4,8}, false, false} };
            advanceHunterGroup(maze, hunter, targets, 4);
            Assert::IsTrue(hunter.mode == HunterMode::Chase);
            Assert::AreEqual(8, hunter.lastKnown.col);
            Assert::AreEqual(5, hunter.cell.col);
        }

        TEST_METHOD(DetectsSightAndSearchesLastKnownPosition)
        {
            MazeGrid maze({10, 10});
            for (int col = 1; col <= 6; ++col) maze.at(1, col) = MazeCell::Passage;
            maze.at(4, 2) = MazeCell::Passage;
            HunterState hunter; hunter.cell = { 1,1 }; hunter.random = 7;
            advanceHunter(maze, hunter, { 1,6 }, false, false);
            Assert::IsTrue(hunter.mode == HunterMode::Chase);
            Assert::AreEqual(2, hunter.cell.col);
            advanceHunter(maze, hunter, { 4,2 }, false, false);
            Assert::IsTrue(hunter.mode == HunterMode::Search);
            Assert::AreEqual(3, hunter.cell.col);
            Assert::AreEqual(6, hunter.lastKnown.col);
        }

        TEST_METHOD(HedgeBlocksSightButRunningCanBeHeard)
        {
            MazeGrid maze({10, 10});
            maze.at(1,1) = maze.at(1,2) = maze.at(1,3) = maze.at(1,5) = MazeCell::Passage;
            HunterState quiet; quiet.cell = {1,1}; quiet.random = 7;
            advanceHunter(maze, quiet, {1,5}, false, false);
            Assert::IsTrue(quiet.mode == HunterMode::Patrol);
            HunterState loud; loud.cell = {1,1}; loud.random = 7;
            advanceHunter(maze, loud, {1,5}, true, true);
            Assert::IsTrue(loud.mode == HunterMode::Search);
            Assert::AreEqual(5, loud.lastKnown.col);
            maze.at(1,4) = MazeCell::Passage;
            advanceHunter(maze, loud, {1,5}, false, false);
            Assert::IsTrue(loud.mode == HunterMode::Chase);
        }

        TEST_METHOD(DenseHedgeAttenuatesFootstepsAndPassagesCarryThemAroundCorners)
        {
            MazeGrid maze({10, 10});
            maze.at(1,1) = maze.at(1,5) = MazeCell::Passage;
            HunterState behindWalls; behindWalls.cell = {1,1}; behindWalls.random = 7;
            advanceHunter(maze, behindWalls, {1,5}, true, true);
            Assert::IsTrue(behindWalls.mode == HunterMode::Patrol);
            maze.at(2,1) = maze.at(3,1) = maze.at(3,2) = maze.at(3,3) = MazeCell::Passage;
            HunterState aroundBend; aroundBend.cell = {1,1}; aroundBend.random = 7;
            advanceHunter(maze, aroundBend, {3,3}, true, true);
            Assert::IsTrue(aroundBend.mode == HunterMode::Search);
            Assert::AreEqual(3, aroundBend.lastKnown.row);
            Assert::AreEqual(3, aroundBend.lastKnown.col);
            HunterState walking; walking.cell = {1,1}; walking.random = 7;
            advanceHunter(maze, walking, {3,3}, true, false);
            Assert::IsTrue(walking.mode == HunterMode::Patrol);
        }

        TEST_METHOD(CreatureSeesAheadAndNoticesPlayerVeryCloseBehind)
        {
            MazeGrid maze({10, 10});
            for (int col = 1; col <= 7; ++col) maze.at(1,col) = MazeCell::Passage;
            HunterState ahead; ahead.cell = {1,4}; ahead.previous = {1,3}; ahead.random = 7;
            advanceHunter(maze, ahead, {1,6}, false, false);
            Assert::IsTrue(ahead.mode == HunterMode::Chase);
            HunterState behind; behind.cell = {1,4}; behind.previous = {1,3}; behind.random = 7;
            advanceHunter(maze, behind, {1,1}, false, false);
            Assert::IsTrue(behind.mode == HunterMode::Patrol);
            HunterState close; close.cell = {1,4}; close.previous = {1,3}; close.random = 7;
            advanceHunter(maze, close, {1,3}, false, false);
            Assert::IsTrue(close.mode == HunterMode::Chase);
            HunterState running; running.cell = {1,4}; running.previous = {1,3}; running.random = 7;
            advanceHunter(maze, running, {1,1}, true, true);
            Assert::IsTrue(running.mode == HunterMode::Search);
        }

        TEST_METHOD(SpawnAvoidsStonesAndVariesAfterCapture)
        {
            bool varied = false;
            for (std::uint32_t seed = 0; seed < 32; ++seed)
            {
                const auto maze = generateMaze({10,10}, seed);
                const auto puzzle = generateMazePuzzle(maze, seed);
                const auto first = makeHunter(maze, seed, 0, puzzle.stones, {-1,-1});
                const auto replay = makeHunter(maze, seed, 0, puzzle.stones, {-1,-1});
                Assert::IsTrue(first.cell.row == replay.cell.row && first.cell.col == replay.cell.col);
                Assert::IsTrue(maze.at(first.cell.row, first.cell.col) == MazeCell::Passage);
                Assert::IsTrue(first.cell.row + first.cell.col >= 5);
                Assert::IsTrue((9-first.cell.row) + (9-first.cell.col) >= 3);
                for (const auto stone : puzzle.stones)
                    Assert::IsFalse(first.cell.row == stone.row && first.cell.col == stone.col);
                for (std::uint32_t encounter = 1; encounter < 5; ++encounter)
                {
                    const auto next = makeHunter(maze, seed, encounter, puzzle.stones, {-1,-1});
                    varied = varied || next.cell.row != first.cell.row || next.cell.col != first.cell.col;
                }
            }
            Assert::IsTrue(varied);
        }

        TEST_METHOD(PatrolIsSeededAndStaysOnPassages)
        {
            auto maze = generateMaze({20, 20}, 42);
            HunterState first, second;
            first.cell = second.cell = {0,0}; first.random = second.random = 1234;
            for (int step = 0; step < 100; ++step)
            {
                advanceHunter(maze, first, {19,19}, false, false);
                advanceHunter(maze, second, {19,19}, false, false);
                Assert::AreEqual(first.cell.row, second.cell.row);
                Assert::AreEqual(first.cell.col, second.cell.col);
                Assert::IsTrue(maze.contains(first.cell));
                Assert::IsTrue(maze.at(first.cell.row, first.cell.col) != MazeCell::Wall);
            }
        }
    };
}
