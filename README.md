# NXRP Maze

Учебен C++20 проект за лабиринт. Отвори **`NXRP.slnx`** с Visual Studio 2026.

## Стартиране

1. Отвори `NXRP.slnx` чрез **File → Open → Project/Solution**.
2. Избери **Debug / x64**.
3. Задай **Maze.Graphics → Set as Startup Project**.
4. Използвай **Build → Build Solution**, след това **F5** или **Ctrl+F5**.

Приложението е `build/bin/Debug/Maze.Graphics.exe`. Ресурсите от `assets/`
се копират до него при build. Засега прозорецът има демонстрационен бутон
New Game, който променя заглавието; още няма лабиринт и движение.

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
Засега няма реални тестови методи; празният шаблонен `TestMethod1` е премахнат.

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
player position of (0,0). No game or maze-generation behavior is wired into the GUI yet.
