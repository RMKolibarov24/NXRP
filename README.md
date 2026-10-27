# NXRP Maze

Учебен C++20 проект за лабиринт. Отвори **`NXRP.slnx`** с Visual Studio 2026.

## Стартиране

1. Отвори `NXRP.slnx` чрез **File → Open → Project/Solution**.
2. Избери **Debug / x64**.
3. Задай **Maze.Graphics → Set as Startup Project**.
4. Използвай **Build → Build Solution**, след това **F5** или **Ctrl+F5**.

Приложението е `build/bin/Debug/Maze.Graphics.exe`. Ресурсите от `assets/`
се копират до него при build.

The main menu has Continue, New Game, How to Play, Settings, and Exit. Continue
is disabled without an active game. How to Play shows the maze rules. Settings
lets you save or cancel row/column values from 10 to 50. Esc returns from those
screens to the menu. Exit closes the window. New Game returns a menu action
and currently reports that maze creation is unavailable; it does not start a game.

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
player position of (0,0). The GUI updates selectedSize through Settings. Maze
generation and playable-game controls are not connected yet.
