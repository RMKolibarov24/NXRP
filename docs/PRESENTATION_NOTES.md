# Defense presenter notes

The 10-slide presentation describes the implemented C++17 Windows application.
Each slide also contains these notes in its PowerPoint speaker-notes pane.
Allow about 6–8 minutes plus questions. All named sources are repository files.

## 1. NXRP Maze

Introduce NXRP as a Windows maze game. The player chooses dimensions, follows passages and reaches the exit. Explain that the presentation covers the implemented version, including its test evidence and remaining checks. The maintained solution contains four standard Visual Studio projects.

Sources: `README.md`, `NXRP.slnx`, `ProjectDefaults.props`.

## 2. Rules and controls

Explain the colors using the actual board render: dark walls, light passages, green exit and yellow player. Coordinates are zero-based. The arrow keys request one move per press. Show a blocked move before a legal one during the demo. The image is an archived controller render at the 640 by 560 minimum window size, not a newly executed manual test.

Sources: `Maze.Graphics/GameView.cpp`, `Maze.Graphics/main.cpp`, `Maze.Logic/Maze.Game.cpp`, `QA/Evidence/Board-10-minimum.png`.

## 3. Project architecture

Read dependency arrows from the consumer to the dependency. Graphics references Logic and Data. Native tests also reference Logic and Data. Logic references Data. Only Graphics depends on SFML. Data owns storage, Logic applies rules, and Graphics handles input and display. CMake builds SFML only. Explain that the diagram shows the four maintained projects, with the third-party dependency mentioned separately.

Sources: `NXRP.slnx`, `Maze.Graphics/Maze.Graphics.vcxproj`, `Maze.Logic/Maze.Logic.vcxproj`, `Maze.UnitTest/Maze.UnitTest.vcxproj`, `dependencies/CMakeLists.txt`.

## 4. Data and memory ownership

MazeGrid uses a dynamic array through std::unique_ptr<MazeCell[]>. Show that row-major indexing gives constant-time cell access. The constructor validates before allocation and fills the array with walls. at checks both coordinates. Copying is disabled to prevent accidental ownership duplication. A move empties the source. Replacement releases the old array, and the destructor performs cleanup automatically. selectedSize is a future-game preference, while the actual maze keeps its own dimensions.

Sources: `Maze.Data/MazeGrid.h`, `Maze.Data/MazeGrid.cpp`, `Maze.Data/GameState.h`, `Maze.Data/MazeSize.h`.

## 5. Randomized depth-first generation

Walk through the diagram from the initialized wall grid. Rooms lie at even row and column indices. The current room examines unvisited rooms two cells away. A random choice opens both the wall between rooms and the destination, then pushes the destination. A dead end pops the stack. Every new room joins the connected carved area. When DFS ends, the last room connects to the bottom-right exit. For 10 by 10 the last room is (8, 8), and the extension reaches (9, 9). Time and stack space are O(rows * cols). The seeded API reproduces a layout on the same generator and toolchain.

Sources: `Maze.Logic/Maze.Generation.cpp`, `docs/ALGORITHMS.md`, `Maze.UnitTest/MazeGenerationTests.cpp`.

## 6. Movement and session lifecycle

tryMove returns Blocked, Moved or Won. Rejected requests leave the game unchanged. The controller switches to the victory page when movement sets Won. Continue requires both a nonempty grid and an unfinished phase, so a completed grid cannot be resumed. New Game prepares a replacement before transferring it into GameState. startGame checks the start cell, the single exit and known cell values. It does not run a reachability search itself. The generator and independent tests establish reachability. Exit and window close call endGame before closing.

Sources: `Maze.Logic/Maze.Game.cpp`, `Maze.Graphics/MazeApplication.cpp`, `Maze.Data/GameState.h`.

## 7. Build and resources

Use Windows with the Visual Studio 2026 C++ desktop workload, MSVC v145, a Windows SDK, Git and CMake 4.2 or later. The generated dependency property sheets belong under ignored build/dependencies. The four Maze projects remain normal Visual Studio projects. ProjectDefaults.props selects C++17. Read the exact Command Prompt commands in README for setup and VSTest. Build copies assets, including the bundled Source Sans 3 font and license. The Windows module path resolves the font even if the working directory changes. A font-load failure reports the path and exits cleanly.

Sources: `README.md`, `Prepare-Dependencies.ps1`, `ProjectDefaults.props`, `Maze.Graphics/main.cpp`, `Maze.Graphics/ExecutablePath.cpp`, `Maze.Graphics/Maze.Graphics.vcxproj`.

## 8. Tests and QA evidence

The final documentation-stage rebuild ran on 31 October 2026 in both x64 configurations. Both VSTest console runs passed all 63 tests. Six test classes cover dimensions, array ownership, GameState, movement, generation and session lifecycle. The independent one-cell BFS checks all walkable cells and all 1,681 valid dimension pairs. The archived QA workbook describes the 29 October source run at baseline 44d251e and contains 78 distinct cases. Its count includes 63 native cases, each with separate configuration steps, plus 15 GUI/manual scenarios. A successful VSTest run does not confirm Test Explorer UI execution.

Sources: `docs/VALIDATION.md`, `Maze.UnitTest/MazeGenerationTests.cpp`, `QA/README.md`, `QA/Evidence/Native-Debug.trx`, `QA/Evidence/Native-Release.trx`.

## 9. Demonstration and remaining checks

For a short live demo, show the five menu choices, enter Settings and save a rectangular size, then start a random maze. Show a boundary rejection and a legal move. Press Esc, change next-game dimensions, then Continue to demonstrate preservation of the current board. Start another game to apply the new size. Explain the victory controls using the archived victory render if a complete route would take too long. The pending QA items remain Not Run. The application currently stores preferences only in memory, and has no exposed seed input, timer, move counter, same-layout restart or zoom. The workbook and presentation passed engine checks, while native Excel and PowerPoint UI behavior has not been verified.

Sources: `QA/README.md`, `QA/Evidence/Victory-minimum.png`, `Maze.Graphics/MazeApplication.cpp`, `docs/VALIDATION.md`.

## 10. Team responsibilities

Introduce the four confirmed usernames and explain the areas shown. These are responsibility areas, not claims that every commit or test execution came from the listed account. The QA workbook keeps its actual execution methods and metadata. Be ready to explain why unique_ptr owns the array, why Data and Logic are independent of SFML, how even dimensions connect to the exit, and how the BFS tests differ from the DFS generator.

Sources: `docs/TECHNICAL.md#team-responsibilities`, `QA/README.md`.

## Short demonstration route

1. Launch Maze.Graphics and show the five menu options.
2. Set 10 rows and 12 columns, save, then start New Game.
3. Press Up at the start to show boundary rejection, then try a visible legal passage.
4. Press Esc, change the next-game dimensions, and Continue to show that the old maze remains.
5. Choose New Game to apply the new dimensions.
6. Explain the victory page using its archived image if the live route would take too long.
7. Open Test Explorer to run the suite when demonstrating locally. Record the actual result before marking its QA case Passed.

## Questions to prepare

- Why does one contiguous array still represent a two-dimensional grid?
- What happens to the old array when a new game replaces it?
- How does generation connect an even-sized grid to the bottom-right exit?
- Why use an independent BFS in tests instead of repeating the generator?
- Why can a nonempty completed grid still have Continue disabled?
