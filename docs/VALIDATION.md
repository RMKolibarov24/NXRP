# Final local validation

The original base-project validation below records `44d251e`. Feature-specific
checks appear at the end and do not replace its archived evidence.

Validation date: **31 October 2026**.
Source baseline: `44d251e215951517147ecfbb09f292fb42eefa29` on `dev`.
The documentation and QA additions do not change production code or native tests.

## Build and native tests

Environment: Windows, Visual Studio 2026 Community, MSVC v145, x64,
C++17 and CMake 4.3.2. The existing dependency preparation supplied static
SFML 3.1.0. The check rebuilt the entire solution, rather than relying only on
previous binaries, then ran its native test DLL through VSTest console.

| Configuration | Rebuild | Native tests | UTC execution window | Evidence |
| --- | --- | --- | --- | --- |
| Debug / x64 | Passed | 63 passed, 0 failed | 08:05:29–08:05:35 | [Native Debug report](validation/Native-Debug.trx) |
| Release / x64 | Passed | 63 passed, 0 failed | 08:05:35–08:05:40 | [Native Release report](validation/Native-Release.trx) |

Both rebuild logs contained no compiler warning or error entries. The copied
font in each executable's `assets/fonts/` matched the repository font's SHA-256
hash. The maintained project sources contain no remaining `TODO`, `FIXME`,
`sfml_test`, Maze.Tests or GoogleTest references. The development-only
`createDemoMaze()` fixture remains available to native tests; New Game calls
the random generator.

The full repeatable build/VSTest commands are in the [README](../README.md#native-unit-tests).
Generated build products and temporary verification tools remain outside tracked
source files.

## QA workbook

The archived [QA guide](../QA/README.md) describes the 29 October regression
evidence at the same source baseline. It records **78 distinct cases: 75 Passed,
0 Failed and 3 Not Run**. The documentation-stage native runs above provide a
fresh check, while the archived workbook retains its original observations,
dates and evidence files.

The workbook now follows the supplied reference layout, with feature tabs,
separate case banners, side-by-side metadata and step rows. Engine validation
checked status dropdowns, whole-row colors, formula propagation, restored test
edits, 78 internal case-index links, all-sheet rendering and XLSX re-import.
Native cases preserve exact adapter names and separate Debug/Release results.

## Documentation and presentation

The documentation was checked against the maintained project files and production
source. Relative documentation links and referenced source paths resolve.
The generation and movement diagrams follow the implemented validation and
mutation order. The 10-slide presentation includes editable text, diagrams and
tables, actual archived game renders, and speaker notes for every slide.

PPTX validation checked package integrity, slide count, slide size, font policy,
native tables and import into the presentation engine. Every exported slide was
rendered again after re-import and visually reviewed. These are file/engine
checks; the presentation was not opened in the native PowerPoint UI.

## Remaining checks and limits

- **QA-013:** full manual hover/click sweep across all pages and controls.
- **QA-014:** first-time setup on a second clean Windows computer.
- **QA-015:** test execution in the actual Visual Studio Test Explorer UI.
- Opening the workbook in the native Excel UI remains unverified.
- At this base-project snapshot, preferences and the session lasted only in
  memory. The preference-persistence check below records the subsequent change.
  Seed entry, timer, move count, same-layout restart and zoom remain outside the
  implemented interface.

The documentation does not claim that the pending manual checks passed.

## Persistent dimension preferences validation

Date: **31 October 2026**. Tested source: `292dd1b` plus the preference-persistence
working-tree changes, before their manual commit.

| Configuration | Solution build | Native tests | New settings tests |
| --- | --- | --- | --- |
| Debug / x64 | Passed | 75 passed, 0 failed | 12 passed |
| Release / x64 | Passed | 75 passed, 0 failed | 12 passed |

The new Native Unit Test class covers loading/saving real isolated files,
rectangular and boundary dimensions, whitespace, malformed/overflowed input,
extra data, Unicode paths, automatic parent creation, replacement and write
failures. Test files live in directories created by the fixture and never use
the player's actual preferences.

Five controller-probe modes ran as separate processes in both configurations.
They exercised the real MazeApplication, menu and storage code through synthetic
SFML events:

- Save dimensions, then start a maze using those dimensions.
- Save a different next-game size while paused. Continue preserves the old
  allocation, every cell and the player's moved position.
- Cancel and Escape discard drafts without changing the saved file.
- A second process loads the saved size and uses it for New Game, while leaving
  Continue unavailable until a game starts.
- Invalid stored dimensions fall back without rewriting the input. A blocked
  storage path or disabled persistence retains the size in memory and allows
  New Game after the save failure.

Saved/reloaded settings and the failure message were rendered and reviewed,
including the 640 by 560 minimum window. These controller probes use hidden
windows and offscreen renders; they do not replace the outstanding manual QA
sweep or Test Explorer UI check. The original workbook, presentation and native
reports above retain their base-project versions and results.
