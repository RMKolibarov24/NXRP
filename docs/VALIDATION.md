# Final local validation

Current handover: **2 November 2026**, source `cf3de4f`. The latest recorded
regression passed **109/109 Native tests in Debug and Release / x64**. The
expanded QA workbook records **139 cases: 136 Passed, 0 Failed, 3 Not Run**.
See the [current QA guide](../QA/README.md) for methods, evidence and pending
manual/environment checks. This documentation update changes no application
or Native Unit Test source.

The original base-project validation below records `44d251e`. Feature-specific
checks and the final handover checks follow it and retain the earlier evidence.

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

The 29 October regression evidence at the same source baseline originally
recorded **78 distinct cases: 75 Passed,
0 Failed and 3 Not Run**. The documentation-stage native runs above provide a
fresh check. The [QA guide](../QA/README.md) now describes the expanded workbook;
its archived GUI observations and the original evidence files remain intact.

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
  Seed entry, timer, move count, same-layout restart and zoom were outside that
  earlier interface; the later feature checks below cover their implementation.

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

## Maze zoom validation

Date: **1 November 2026**. Tested source: `83791e9` plus the maze-zoom
working-tree changes, before their manual commit.

| Configuration | Solution build | Native tests | Added viewport tests |
| --- | --- | --- | --- |
| Debug / x64 | Passed | 109 passed, 0 failed | 8 passed |
| Release / x64 | Passed | 109 passed, 0 failed | 8 passed |

MazeViewportTests exercises zoom limits, whole-grid fit including the border,
player visibility at the center and all corners, rectangular grids, resizing,
fit reset, invalid/tiny areas and following real movement through victory.
The camera calculations use no SFML types; the Native Unit Test project keeps
its existing Data/Logic references without linking the graphics executable.

Separate rendering/controller probes used the actual GameView and
MazeApplication in both configurations. Each checked 10 by 10 at 900 by 600
and 50 by 50 at the 640 by 560 minimum, with exact timestamps, isolated
preference files, hidden windows and independent BFS routes. They verified:

- Wheel and keyboard zoom stay between 100% and 400%. Numpad keys work;
  0, numpad 0 and Home restore fit. Horizontal, zero/nonfinite and off-board
  wheel events leave the view unchanged.
- The player stays visible while moving to the exit at maximum zoom. Resizing
  preserves the zoom level. The complete grid and its border remain visible
  at fit, including rectangular grids covered by the native tests.
- Pixel comparisons show the zoomed board stays inside its own area without
  overwriting the HUD. GameView restores the caller's view, viewport and
  scissor; a reused render target clears and redraws correctly after zooming.
- Continue retains zoom after a pause, Help, Settings and resize. Those menu
  pages ignore zoom input. New Game and Restart, including the Victory action,
  restore fit. Pause duration remains excluded from elapsed time.
- Zoom leaves the cell allocation/layout, player, seed and move count intact.

Fit, maximum zoom, tracked middle/exit positions, continued play at minimum
size, enlarged-window play, Help and Victory renders were reviewed. Controls
and Help fit the minimum window. These synthetic-event/render checks do not
replace the outstanding manual QA sweep or actual Test Explorer UI execution.

## Current feature regression and QA update

Executed **1 November 2026 UTC**, with the workbook review completed on
**2 November 2026, Europe/Sofia**. Tested clean source:
`9fd92aa1a9cb2f664ef20868901deb7806aaa762` on `dev`.
This QA update changes no production source or Native Unit Test implementation.

| Configuration | Solution build | Native tests | Feature probes |
| --- | --- | --- | --- |
| Debug / x64 | Passed | 109 passed, 0 failed | All five feature groups passed |
| Release / x64 | Passed | 109 passed, 0 failed | All five feature groups passed |

Fresh runs cover the complete current Native suite, including preferences,
restart, game statistics, seed replay and viewport geometry. The private
controller probes were compiled against the current production sources.
Older probe click coordinates were adjusted for the six-item menu and seed
field. No production behavior was changed to accommodate the probes.

Both configurations exercised preference save/reload in separate processes,
fallback/save failure paths, same-maze restart, pause/resume timing, successful
move counts, seed entry/replay and bounded zoom. The route checks use an
independent BFS. Graphics checks cover 10 by 10 at 900 by 600 and 50 by 50 at
640 by 560, resize, Victory actions, player tracking and view/scissor restoration.
All solution/probe build logs contain no compiler warning or error entries.

The workbook retains its eight tabs and reference-style case panels. It now
records **139 cases: 136 Passed, 0 Failed, 3 Not Run**. These comprise 109 current
Native cases, 15 current Controller cases and 15 archived GUI/manual cases.
The archived GUI cases preserve their original `44d251e` source, dates, full
observations, outcomes and cell positions. Existing Native cases retain their
IDs, creation metadata, test data and expectations with refreshed results.

Workbook checks verified all 109 exact Native adapter names and recorded
durations, 139 native internal links, dropdowns, formulas, failed/blank/partial
status propagation and XLSX re-import. Temporary edits were restored. Views
across all eight tabs and nine current minimum-window game renders were reviewed.
The workbook has no formula errors in the checked engine.

Fresh reports, feature logs, renders, source/evidence hashes and execution
windows are under [FeatureRegression-20261101](../QA/Evidence/FeatureRegression-20261101/Regression.json).
Earlier evidence remains unchanged. QA-013, QA-014 and QA-015 remain Not Run;
native Excel UI opening and system-clipboard paste remain unverified. These
console/synthetic-event checks do not replace actual manual or Test Explorer
UI execution. QA preparation is assigned to NKPeshev24's manual commit.

## Current documentation and defense handover

Date: **2 November 2026**, source `cf3de4f8ef06311237cffbd027a040b0f8fee0b2`.
Production and Native test sources still match the hashes recorded by the
`9fd92aa` feature regression. This step refreshes the README, technical guide,
algorithm diagrams, presenter notes and actual editable PowerPoint file.

The presentation retains ten slides, the reference slide size and Arial theme.
Its architecture, memory-ownership and generation diagrams remain editable.
The three tables on slides 6, 8 and 10 remain native PowerPoint tables. Current
regression renders show zoom, seed Settings and the complete Victory controls.
Every slide has updated speaker notes that match the Markdown presenter notes.

File checks passed package integrity, declared slide size/count, font policy,
native-table presence and import into the presentation engine. Every slide was
rendered again from the validated final file and reviewed individually against
the original. No geometry or font-policy findings were reported. These checks
do not establish native PowerPoint UI behavior.

The documentation now covers preference persistence, current/selected seeds,
same-maze restart, elapsed time, accepted-move counts and camera clipping.
The added phase, time and camera diagrams were cross-checked against the actual
controller and logic. File/resource references and relative documentation links
resolve. The maintenance guide explains which project owns each kind of feature
and keeps CMake limited to SFML preparation.

A private probe generated the **10 by 10, seed-zero** demo with the real Logic
library, found an independent BFS route and followed it through tryMove. The
route reaches `(9,9)` with **42 successful moves**. A boundary request adds no
move, and repeating the route after Restart succeeds with the same allocation
and seed. The presenter notes list all 42 presses in grouped directions. Exact
layouts remain dependent on the generator and standard-library toolchain.

Existing QA workbook/evidence files and original validation reports are
unchanged. The three pending manual/environment cases and the native Excel,
PowerPoint and clipboard UI limits still apply. Temporary deck builders,
validation receipts, previews and the demo probe stay outside the repository.
