# Defense presenter notes

Updated **2 November 2026** for repository snapshot `cf3de4f`. Production code
matches `9fd92aa`; the current QA evidence records its 1 November UTC runs.
The 10-slide editable deck covers the current application, including all added
features. Each slide contains the corresponding notes below in its PowerPoint
speaker-notes pane. Allow about 8–10 minutes plus questions.

## 1. NXRP Maze

Introduce NXRP as a C++17 Windows maze game using SFML 3.1.0. The player chooses 10–50 rows and columns and reaches the green exit. The current version includes persistent dimensions, same-maze restart, elapsed time, successful-move counts, seed replay and zoom. The maintained solution has four normal Visual Studio projects. This presentation describes repository snapshot cf3de4f; its production code matches the 9fd92aa feature-regression source.

Sources: `README.md`, `NXRP.slnx`, `ProjectDefaults.props`.

## 2. Rules and controls

Coordinates are zero-based. Dark cells are walls, light cells are passages and the yellow marker is the player. The green exit is at bottom-right. Each arrow press requests one step; walls and edges block movement without adding a successful move. The current image shows a 50 by 50 grid at 400% zoom, so only the nearby area is visible. The mouse wheel works over the board; + and - also zoom, while 0 or Home fits the whole map. The camera follows the player and clips inside the board area. The HUD keeps the seed, coordinates, elapsed time and moves visible. Escape pauses, and Continue retains the current grid, results and zoom.

Sources: `Maze.Graphics/GameView.cpp`, `Maze.Graphics/MazeViewport.h`, `Maze.Graphics/main.cpp`, `Maze.Logic/Maze.Game.cpp`, `QA/Evidence/FeatureRegression-20261101/Board-zoom-400-minimum.png`.

## 3. Project architecture

Read arrows from a consumer to its dependency. Graphics references Logic and Data. The Native Unit Test DLL also references Logic and Data, and Logic references Data. Only Graphics links SFML. Data owns the model, Logic applies operations and Graphics manages input/display. The camera tests directly include the SFML-free MazeViewport header without adding a Graphics project reference or opening a window. CMake prepares SFML only; add and build application files through the maintained Visual Studio projects.

Sources: `NXRP.slnx`, `Maze.Graphics/Maze.Graphics.vcxproj`, `Maze.Logic/Maze.Logic.vcxproj`, `Maze.UnitTest/Maze.UnitTest.vcxproj`, `Maze.Graphics/MazeViewport.h`, `dependencies/CMakeLists.txt`.

## 4. Data and memory ownership

MazeGrid owns one contiguous array through std::unique_ptr<MazeCell[]>. The row-major index is row * cols + col. The constructor validates dimensions before allocation, and at checks both coordinates. Copying is disabled. Moving transfers ownership and empties the source; replacement and destruction release the old array. GameState keeps the owned grid, player, phase and statistics. selectedSize and selectedSeed describe the next game, while mazeSeed records the current generated layout. Statistics store fractional steady-clock time and an unsigned 64-bit move count. The camera belongs to the view rather than the game rules.

Sources: `Maze.Data/MazeGrid.h`, `Maze.Data/MazeGrid.cpp`, `Maze.Data/GameState.h`, `Maze.Data/GameStatistics.h`, `Maze.Graphics/MazeViewport.h`.

## 5. Randomized depth-first generation

Generation begins with walls and a stack containing (0,0). Rooms use even row and column indices. For the top room, collect unvisited neighbors two cells away. Choose one randomly, open the separating wall and destination, and push it. Pop at a dead end. Every newly opened room joins the connected carved area. After DFS finishes, connect the last even-indexed room to bottom-right and mark Exit. For 10 by 10, the extension joins (8,8) to (9,9). Time and auxiliary stack space are O(rows * cols). New Game chooses a fixed seed or obtains random entropy once, then records the same seed used by mt19937. Matching size and seed replay a layout on the same generator/toolchain.

Sources: `Maze.Logic/Maze.Generation.cpp`, `Maze.Graphics/MazeApplication.cpp`, `docs/ALGORITHMS.md`, `Maze.UnitTest/MazeGenerationTests.cpp`, `Maze.UnitTest/MazeSeedTests.cpp`.

## 6. Movement and session lifecycle

tryMove returns Blocked, Moved or Won. A legal step increments successful moves, including the final exit step. The controller accounts for elapsed time in the old phase before applying an event, so Escape includes active time up to pausing and Continue excludes the paused interval. New Game generates and validates a replacement before ownership moves; an error preserves the old session. Restart performs no allocation or regeneration: it keeps every cell and current seed, resets player/statistics and fits the board. It also preserves next-game choices. R is inactive on Help and Settings. A completed grid can restart but cannot Continue. Exit releases the grid and clears current results/seed. Only dimensions persist on disk.

Sources: `Maze.Logic/Maze.Game.cpp`, `Maze.Graphics/MazeApplication.cpp`, `Maze.Data/GameState.h`, `Maze.Graphics/GameStatisticsText.cpp`.

## 7. Setup, settings and replay

First run Prepare-Dependencies.ps1 as described in README. CMake builds the third-party SFML libraries; NXRP.slnx still contains four standard Visual Studio projects. Use the C++ desktop workload, MSVC v145 and x64, select Maze.Graphics as startup project and press F5. Build copies assets beside the executable; executableDirectory resolves the Source Sans 3 font independently of the working directory. Settings apply to the next New Game. Apply persists only rows and columns in the current user's LOCALAPPDATA/NXRP/settings.txt; a save failure retains the choices for this launch. A blank Seed means random mode. Zero is valid, and 4294967295 is the maximum. Invalid input disables Apply. Cancel/Escape discards drafts. The seed choice lasts for the launch; restart uses the current maze and seed.

Sources: `README.md`, `Prepare-Dependencies.ps1`, `Maze.Graphics/ExecutablePath.cpp`, `Maze.Graphics/SettingsPath.cpp`, `Maze.Logic/Maze.Settings.cpp`, `Maze.Logic/Maze.Generation.cpp`, `Maze.Graphics/menu.cpp`, `QA/Evidence/FeatureRegression-20261101/Settings-seed-minimum.png`.

## 8. Tests and QA evidence

The latest feature regression at source 9fd92aa ran on 1 November 2026 UTC and is recorded in the cf3de4f QA update. Both x64 builds and VSTest console runs passed all 109 Native tests across ten classes. Independent one-cell BFS verifies reachability for all 1,681 valid size pairs and follows routes through the real movement API. Further cases cover ownership, file failures, restart, precise time, seed handling and camera geometry. Five private controller/render probe groups also passed in Debug and Release. The workbook records 139 distinct cases: 109 current Native cases, 15 current Controller cases and 15 archived GUI/manual cases. The total is 136 Passed, 0 Failed and 3 Not Run. Archived GUI observations retain their 44d251e source and dates. Console success does not establish Test Explorer UI execution.

Sources: `QA/README.md`, `QA/Evidence/FeatureRegression-20261101/Regression.json`, `QA/Evidence/FeatureRegression-20261101/Native-Debug.trx`, `QA/Evidence/FeatureRegression-20261101/Native-Release.trx`, `Maze.UnitTest/MazeGenerationTests.cpp`, `docs/VALIDATION.md`.

## 9. Demonstration and remaining checks

Use a 10 by 10 maze with seed 0. Show a blocked Up request, one legal move, zoom and fit. Pause, change next-game dimensions, then Continue to show that the current maze and results remain. R resets this current maze and returns the camera to fit. The verified route in these presenter notes reaches the exit in 42 successful moves from (0,0) and works again after Restart on the current MSVC generator. The screenshot is a real offscreen SFML Victory render from the current regression; its displayed time and move count belong to that recorded 50 by 50 run, not the short demo. Restart maze, New Game and Main menu have distinct effects. The full manual hover/click sweep, second clean-machine setup and actual Test Explorer UI execution remain Not Run. Native Excel/PowerPoint opening and system-clipboard paste remain unverified.

Verified key sequence from (0,0), 10 by 10, seed 0: Right 8, Down 2, Left 6, Down 2, Left 2, Down 4, Right 2, Up 2, Right 2, Up 2, Right 2, Down 4, Right 2, Down 1, Right 1. Press R first if demonstration moves have already been made. Tap once for each listed press; the final move is number 42.

Sources: `docs/PRESENTATION_NOTES.md`, `Maze.Graphics/MazeApplication.cpp`, `Maze.Logic/Maze.Generation.cpp`, `QA/README.md`, `QA/Evidence/FeatureRegression-20261101/Victory-minimum.png`.

## 10. Team responsibilities

Use the four confirmed usernames. RMKolibarov24 covers solution/dependency integration and handover documentation. HAMihaylov24 covers the data/rule/generation/session area and seed replay. PDKapralev24 covers the SFML interface, rendering and zoom. NKPeshev24 covers Native test coverage and QA review. These are responsibility areas; Git authorship and individual execution records are separate. Be ready to explain unique_ptr ownership, the SFML boundary, the even-size exit extension, the independent BFS oracle, fixed-seed replay, pause timing, zoom clipping and why Restart differs from New Game.

Sources: `docs/TECHNICAL.md#team-responsibilities`, `QA/README.md`.

## Short demonstration

1. Launch Maze.Graphics and show the six menu options. Continue and Restart maze start disabled.
2. In Settings, choose **10 rows, 10 columns, seed 0**, then Apply and New Game.
3. Press Up to show a boundary rejection, then Right once. The first request adds no move; the second adds one.
4. Zoom with + or the wheel, then fit with 0. Time and move count remain visible.
5. Press Escape. Choose different next-game dimensions in Settings and Apply, then Continue. The current maze is still 10 by 10 with seed 0 and its existing results.
6. Press R. The same current grid returns to (0,0), with zero time/moves and fit view. The changed dimensions still apply only to the next New Game.
7. Follow the route below to show Victory, frozen results and the three return actions. Restart repeats the current grid; New Game uses the next-game choices.
8. Optionally close and reopen to show saved dimensions. The seed choice returns to random mode. Run Native tests through Test Explorer during a local demonstration, and record the actual result before marking QA-015 Passed.

## Verified seed-zero route

Start at `(0,0)` in a **10 by 10 maze generated with seed 0**. Use R immediately
before the route if any demonstration moves were made. Key repeat is disabled,
so tap once for each listed press. Do not include the blocked Up request in
the successful-move count.

| Order | Arrow key | Presses |
| ---: | --- | ---: |
| 1 | Right | 8 |
| 2 | Down | 2 |
| 3 | Left | 6 |
| 4 | Down | 2 |
| 5 | Left | 2 |
| 6 | Down | 4 |
| 7 | Right | 2 |
| 8 | Up | 2 |
| 9 | Right | 2 |
| 10 | Up | 2 |
| 11 | Right | 2 |
| 12 | Down | 4 |
| 13 | Right | 2 |
| 14 | Down | 1 |
| 15 | Right | 1 |

The final move reaches `(9,9)` with **42 successful moves**. A private
probe generated the grid using the real Logic code, found an independent BFS
route, followed every step through tryMove, and repeated it after restart.
This sequence applies to the current generator/MSVC toolchain; another
standard-library implementation can produce a different fixed-seed layout.

## Questions to prepare

- How does a contiguous array represent a two-dimensional grid, and who releases it?
- Why do Data and Logic stay independent of SFML?
- How does DFS connect even dimensions to the bottom-right exit?
- How does independent BFS testing differ from the generation algorithm?
- Why is seed zero different from an empty seed choice?
- How do next-game choices differ from the current grid and current seed?
- Why do pause and victory stop time, and which move counts as the winning step?
- What does Restart preserve that New Game replaces?
- Why does zoom belong in Graphics, and how does clipping protect the HUD?
