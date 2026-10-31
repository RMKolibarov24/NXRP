# NXRP Maze

A Windows maze game built with C++17 and SFML 3.1.0. Choose a grid from 10 to
50 rows and columns, move with the arrow keys, and reach the green exit.

![The 10 by 10 game board](QA/Evidence/Board-10-minimum.png)

## Requirements

- Windows and Visual Studio 2026 with **Desktop development with C++**, the
  **MSVC v145 x64** toolset, a Windows SDK and the C++ native unit test tools.
- Git and CMake **4.2 or later** for the Visual Studio 18 2026 dependency generator.
- Internet access for the first SFML download.

The repository contains four standard Visual Studio projects. Open
**NXRP.slnx**, rather than the generated SFML solution inside `build/`.
CMake prepares the third-party library only.

## First setup

From **Command Prompt** in your checkout, run:

```bat
cd /d C:\Users\rafat\source\repos\NXRP
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Prepare-Dependencies.ps1
```

Replace the checkout path if yours differs. The script downloads and builds
static SFML graphics/window/system libraries for Debug and Release, then creates
configuration-specific property sheets in `build/dependencies/`. It leaves
the four application projects under Visual Studio's control. Run it again after
moving the checkout or deleting `build/`.

## Build and run in Visual Studio

1. Open **NXRP.slnx** with **File > Open > Project/Solution**.
2. Select **Debug / x64**.
3. Right-click **Maze.Graphics > Set as Startup Project**.
4. Choose **Build > Build Solution**, then **F5** or **Ctrl+F5**.

The executable is `build/bin/Debug/Maze.Graphics.exe`. Release builds go to
`build/bin/Release/`. Each build copies `assets/` beside the executable. The
font path starts at the actual executable directory, so another working
directory does not break resource loading.

For command-line builds, use **Developer Command Prompt for VS 2026**:

```bat
MSBuild NXRP.slnx /p:Configuration=Debug /p:Platform=x64
build\bin\Debug\Maze.Graphics.exe
```

## Controls and rules

| Control or menu item | Behavior |
| --- | --- |
| Arrow keys | Move one cell per press. Walls and outer boundaries block movement. |
| Escape during play | Pause and open the main menu. |
| Continue | Resume the same maze and position. Available only for an unfinished session. |
| New Game | Generate a maze using the selected dimensions and start at `(0, 0)`. Replaces any previous session. |
| How to Play | Show the rules. Back or Escape returns to the main menu. |
| Settings | Use the separate row/column minus/plus controls, then **Save size**. Back or Escape discards the draft. |
| Exit or window close | Release the current maze and close the application. |

The yellow player starts at the top-left passage. The green exit is at
`(rows - 1, columns - 1)`. Reaching it opens the victory page, where **New Game**
starts another maze and **Main menu** returns to the menu. Continue is disabled
after victory.

Settings apply to the **next New Game**. Continue keeps the existing grid.
Settings persist between launches in `%LOCALAPPDATA%\NXRP\settings.txt` for the
current Windows user. **Save size** writes both dimensions. Missing, unreadable
or malformed preferences use 10 by 10. If saving fails, the size still applies
to New Game for this launch, and the menu explains that it could not save it.
The file contains two integers: rows followed by columns. Both must be 10–50,
with no extra data other than whitespace. Closing the game and deleting this
specific file restores the defaults on the next launch. The maze and player
position remain in memory only.

The board fits the window, including a 50 by 50 maze at
the minimum supported window size of 640 by 560.

## Projects and files

| Project or path | Purpose |
| --- | --- |
| `Maze.Data/` | Static library containing grid ownership, cells, dimensions, positions and game state. |
| `Maze.Logic/` | Static library containing validation, generation, movement and session operations. Depends on Data. |
| `Maze.Graphics/` | SFML application, event loop, menu pages, controller, board rendering and resource paths. Depends on Logic and Data. |
| `Maze.UnitTest/` | Microsoft Native Unit Test DLL using the real Data and Logic code. |
| `ProjectDefaults.props` | Shared C++17 compiler settings and output directories. |
| `dependencies/` | CMake setup for SFML 3.1.0 and the generated property-sheet template. |
| `assets/fonts/` | Bundled Source Sans 3 Semibold font and its license. |
| `QA/` | Feature-based Excel cases and archived regression evidence. |
| `docs/` | Technical documentation, algorithm diagrams and defense presentation. |

Add source files with **Add > New Item / Existing Item** in the relevant project.
Include the resulting `.vcxproj` and `.vcxproj.filters` changes in your commit.
Build products, dependency downloads and `.vs/` caches stay ignored.

## Native unit tests

Build the solution, then open **Test > Test Explorer > Run All**. The 75 tests
belong to `MazeSizeTests`, `MazeGridTests`, `GameStateTests`,
`MazeMovementTests`, `MazeGenerationTests`, `MazeSessionTests` and `MazeSettingsTests`.

`Maze.UnitTest` is a **Native Unit Test Project** created in Visual Studio. It
uses `CppUnitTest.h`, `TEST_CLASS`, `TEST_METHOD` and `Assert`. Its output is a
DLL for the native adapter to load. Run the game project with F5, and run tests
through Test Explorer or VSTest.

To repeat both configurations in **Developer Command Prompt for VS 2026**:

```bat
MSBuild NXRP.slnx /p:Configuration=Debug /p:Platform=x64
"%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" build\bin\Debug\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Debug
MSBuild NXRP.slnx /p:Configuration=Release /p:Platform=x64
"%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" build\bin\Release\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Release
```

The suite checks ownership and bounds, movement and victory, session replacement,
preference round-trips and malformed/unwritable files,
fixed-seed repeatability and independently verified reachability for all 1,681
allowed size pairs. [QA results](QA/README.md) distinguish CLI runs, UI observations
and checks that remain unrun. [Final validation](docs/VALIDATION.md) records the
documentation-stage rebuild and subsequent feature checks.

## Documentation

- [Technical documentation](docs/TECHNICAL.md): architecture, data and memory,
  generation, movement, session lifecycle and test design.
- [Algorithm diagrams](docs/ALGORITHMS.md): generation and movement flowcharts.
- [Defense presentation](docs/NXRP_Defense.pptx): 10 editable slides with speaker notes,
  describing the completed base project at `44d251e`, before preference persistence.
- [Presenter notes](docs/PRESENTATION_NOTES.md): slide notes and a short demonstration route.

## Troubleshooting

| Symptom | Action |
| --- | --- |
| Missing `NXRP.SFML.Debug.props` or SFML headers/libraries | Run the dependency preparation script for the chosen configuration. |
| v145 toolset or native test framework missing | Modify the Visual Studio installation to include the C++ desktop toolchain and native test tools. |
| Font fails to load at startup | Rebuild Maze.Graphics and keep the copied `assets/fonts/SourceSans3-Semibold.ttf` beside the executable. The console prints the failed path. |
| Size applies only to this launch | Check write access to `%LOCALAPPDATA%\NXRP\`. The game keeps the chosen size in memory when saving fails. |
| Tests do not appear | Build Maze.UnitTest in x64, check the C++ native test adapter, then reopen Test Explorer. The VSTest command above provides a separate diagnostic. |
| `Set-Location` is not recognized | That command belongs to PowerShell. In Command Prompt use `cd /d` as shown above. |

## Team

RMKolibarov24, HAMihaylov24, PDKapralev24 and NKPeshev24.
See the [technical documentation](docs/TECHNICAL.md#team-responsibilities) for
the confirmed development responsibilities.
