# NXRP Maze

Учебен C++20 проект за лабиринт. Отвори **`NXRP.slnx`** с Visual Studio 2026.

## Стартиране

1. Отвори `NXRP.slnx` чрез **File → Open → Project/Solution**.
2. Избери **Debug / x64**.
3. Задай **Maze.Graphics → Set as Startup Project**.
4. Използвай **Build → Build Solution**, след това **F5** или **Ctrl+F5**.

Приложението е `build/bin/Debug/Maze.Graphics.exe`. Ресурсите от `assets/`
се копират до него при build.

The main menu has Continue, New Game, How to Play, Settings, and Exit.
New Game generates a random maze at the size saved in Settings. Press an arrow key to move one cell;
walls and boundaries block movement. The display shows the current row/column,
blocked-move feedback, and victory at the green exit. Esc returns to the menu,
and Continue resumes a paused game. Continue is disabled before a game and after
victory. Exit closes the window.

Settings saves separate row/column preferences from 10 to 50. Changes apply to
New Game; Continue keeps the existing maze and player position. The generator
supports both even and odd dimensions, including 10x10 and 50x50. The renderer
fits the current maze when the window is resized.

## Проекти

| Проект | Тип | Отговорност |
| --- | --- | --- |
| Maze.Data | Static Library | Данни и размери |
| Maze.Logic | Static Library | Проверки и игрови правила |
| Maze.Graphics | Application | SFML прозорец, вход и рисуване |
| Maze.UnitTest | Native Unit Test Project — DLL | Microsoft Unit Testing Framework for C++ |

Новите `.cpp` и `.h` се добавят през **Add → New Item / Existing Item**
в съответния проект. Комитват се и променените `.vcxproj`/`.filters` файлове.

## Подготовка на нов компютър

Необходими са Visual Studio 2026 с **Desktop development with C++**, MSVC v145,
Windows SDK, Git и CMake 4.2+. От PowerShell в корена на репото изпълни:

```powershell
.\Prepare-Dependencies.ps1
```

Скриптът подготвя само SFML **3.1.0** за Debug и Release. За първото изтегляне
е нужен интернет. Повтори подготовката, ако преместиш репото или изтриеш `build/`.
Microsoft Native Unit Test Framework идва с Visual Studio; не се изтегля от CMake.

## Тестове

`Maze.UnitTest` е създаден от шаблона **Native Unit Test Project** на Visual Studio.
Използва `CppUnitTest.h`, `TEST_CLASS`, `TEST_METHOD` и `Assert` от
`Microsoft::VisualStudio::CppUnitTestFramework`.

Проектът е свързан с Maze.Data и Maze.Logic и се компилира в
`build/bin/Debug/Maze.UnitTest.dll` или Release варианта. Запазени са PCH файловете
от шаблона. Няма собствен `main()` и DLL файлът не се стартира с F5.

След build отвори **Test → Test Explorer** и избери **Run All**.
The suite contains 26 native tests grouped as `MazeSizeTests`, `MazeGridTests`,
and `GameStateTests`. They cover the 10/50 size boundaries, invalid dimensions,
rectangular grids, wall initialization, checked access, fill, move ownership,
and the initial game state. They use the real Data/Logic implementations and
do not open an SFML window.

To run them from **Developer Command Prompt for VS 2026** after building:

```bat
vstest.console.exe build\bin\Debug\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Debug
```

For Release, build **Release / x64** and replace both `Debug` paths with `Release`.
The `.trx` result files stay under the ignored `build/` directory.

Генерираните файлове под `build/` и `.vs/` са игнорирани от Git.

## Data model

`MazeGrid` stores a two-dimensional grid in one dynamically allocated cell array,
owned by `std::unique_ptr<MazeCell[]>`. The grid releases its memory automatically.
It cannot be copied; moving it transfers ownership and leaves the source empty.

Each dimension must be in the inclusive range 10..50. Invalid grid construction
throws `std::invalid_argument` before allocation. New cells are all `MazeCell::Wall`.
`at(row, col)` throws `std::out_of_range` outside the grid, including an empty grid.
`MazeSize` values can still hold invalid input for validation without allocating.

`GameState` starts in the menu with an empty grid, a 10x10 selected size and a
player position of (0,0). The GUI updates selectedSize through Settings. Gameplay
uses the actual maze dimensions; selectedSize remains the next-game preference.

## Gameplay logic

`Maze.Game.h` provides `startGame`, `tryMove` and `hasWon` without SFML or user I/O.
Starting takes ownership of a prepared maze, resets the player to (0,0), and
enters Playing. The start must be a passage, with a single exit at the bottom-right.
Invalid layouts throw before changing the existing game or the supplied grid.
Movement uses the four directions, respects walls and boundaries, and reports
Blocked, Moved or Won. Blocked requests leave the game unchanged; reaching the
exit sets the Won phase. `GameView` draws cells and the player from the data
model and routes arrow-key events through `tryMove`. `createDemoMaze()` remains
a fixed development fixture; New Game uses the random generator.

`Maze.Generation.h` provides `generateMaze(size)` and `generateMaze(size, seed)`.
The generator uses iterative randomized depth-first search: it visits rooms
two cells apart, opens the connecting wall, and backtracks using a stack. All
rooms connect to (0,0). A short extension joins the last room to the bottom-right
exit when a dimension is even. Every walkable cell is reachable from the start.
The seeded overload reproduces a map with the same generator/toolchain; the GUI
obtains a seed from `std::random_device`. Random maps can occasionally repeat.
