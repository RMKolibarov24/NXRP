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

## Same-maze restart validation

Date: **31 October 2026**. Tested source: `88b5c29` plus the same-maze restart
working-tree changes, before their manual commit.

| Configuration | Solution build | Native tests | Added restart tests |
| --- | --- | --- | --- |
| Debug / x64 | Passed | 81 passed, 0 failed | 6 passed |
| Release / x64 | Passed | 81 passed, 0 failed | 6 passed |

The added MazeSessionTests cases cover restart from Playing, Paused and Won,
repeated completion/restart, rejection without a started session, preservation
of every grid cell and its allocation, and preservation of the next-game size.
Minimum, maximum and rectangular grids retain their dimensions and layout.

Controller probes ran against the real application code in both configurations,
using isolated settings files and hidden windows. Each configuration checked a
10 by 10 maze at 900 by 600 and a 50 by 50 maze at 640 by 560. Independent BFS
routes exercised repeated victory transitions through real arrow-key events.
The probes confirmed:

- R restarts during play, from the paused main menu and on the Victory page.
- Restart maze works from the main menu, after returning from victory, and
  directly on the Victory page.
- Restart keeps the cell allocation and complete layout, resets position/phase
  and movement feedback, and leaves saved next-game preferences unchanged.
- Continue and Restart maze are unavailable before the first game. After
  victory, Continue remains unavailable while Restart maze is available.
- R on Help and Settings leaves the paused game and Settings draft intact.
  Cancelling the draft leaves the saved preference file unchanged.
- New Game still uses the selected dimensions. All three victory buttons route
  to their intended actions, and Exit releases the session.

Main-menu, Help, restarted-board and Victory renders were reviewed, including
disabled controls, restart hover state and the minimum-window layout. These
synthetic-event and render checks remain distinct from the outstanding manual
QA sweep and actual Test Explorer UI execution.

## Game statistics validation

Date: **31 October 2026**. Tested source: `f5baa28` plus the game-statistics
working-tree changes, before their manual commit.

| Configuration | Solution build | Native tests | Added statistics tests |
| --- | --- | --- | --- |
| Debug / x64 | Passed | 91 passed, 0 failed | 10 passed |
| Release / x64 | Passed | 91 passed, 0 failed | 10 passed |

GameStatisticsTests covers zero initialization, accepted and blocked movement,
the winning step, fractional elapsed time, inactive/empty sessions, nonpositive
durations, reset operations, rejection preservation and numeric saturation.
Existing state, movement and session tests also check the added fields.

Controller checks injected exact monotonic timestamps through the real
MazeApplication and used synthetic mouse/key events with hidden windows. Both
configurations checked a 10 by 10 maze at 900 by 600 and a 50 by 50 maze at
640 by 560. No waiting or real-time tolerances were needed. The checks verified:

- Time records the interval before Escape, remains unchanged across the main
  menu/Help/Settings, and excludes the entire paused interval on Continue.
- Subsecond time survives pause/resume. Repeated or earlier timestamps neither
  add time nor shift the clock anchor backwards.
- Blocked requests do not add moves. Legal steps and the final exit step do.
- An independent BFS route reaches victory with the exact route-length move
  count and expected accumulated time. Later updates, keys and resize events
  leave those results frozen. Before/after victory renders match pixel for pixel.
- New Game, Restart and Exit clear both statistics. Continue preserves them.
  The saved dimension file remains unchanged by statistics and cancelled drafts.

The shared text formatter was checked at zero, fractional seconds, a minute
boundary and one hour. The updated game display and Victory page were rendered
and reviewed at both sizes. These engine/controller checks do not claim completion
of the outstanding manual QA sweep or native Test Explorer UI check.

## Seed replay validation

Date: **1 November 2026**. Tested source: `df931d6` plus the seed-replay
working-tree changes, before their manual commit.

| Configuration | Solution build | Native tests | Added seed tests |
| --- | --- | --- | --- |
| Debug / x64 | Passed | 101 passed, 0 failed | 10 passed |
| Release / x64 | Passed | 101 passed, 0 failed | 10 passed |

MazeSeedTests covers empty/random input, zero, maximum uint32, leading zeros,
signs, whitespace, non-digits, overflow and excessive length. It also covers
recording the used seed, repeated layouts, clearing stale fixture metadata,
restart/end/replacement behavior and replaying a recorded random seed. The
generator/toolchain and dimensions remain part of the replay contract.

Controller probes used real application code, synthetic input, exact timestamps,
isolated dimension files and hidden windows in both configurations. They checked
10 by 10 at 900 by 600 and 50 by 50 at the 640 by 560 minimum. Each probe verified:

- Random New Game records a seed that reproduces the complete grid.
- Overflow, negative, nonnumeric and long invalid input cannot apply or write
  settings. Keyboard entry, Backspace, Delete, focus changes and Enter work.
- Cancel and Escape discard seed drafts. Editing preserves the paused layout,
  allocation, player, current seed and statistics. Continue retains the old maze.
- Seed zero is a fixed seed. Repeated New Game actions reproduce every cell.
- Maximum seed 4294967295 works with changed row/column selections, including
  rectangular grids. New Game resets statistics and the resulting seed remains
  visible on victory after following an independent BFS route.
- Clearing the field restores random mode. Restart retains the current fixed
  layout/seed even when the next-game choice changes. Close clears current seed
  metadata. Fresh construction restores saved dimensions with a random seed choice.

Settings in random, invalid and maximum-seed states, the zero-seed game display,
and maximum-seed Victory renders were reviewed at both sizes. Long invalid text
stays within the input box. Clipboard paste support was built, while system
clipboard contents were not changed for these checks. These probes do not replace
the outstanding manual QA sweep or actual Test Explorer UI execution.
