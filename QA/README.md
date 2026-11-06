# QA results

Open `QA_Test_Cases.xlsx`. The eight existing tabs and reference-style case
panels are retained: `Main Menu`, `Settings`, `Gameplay`, `Victory`,
`Generation`, `Data and Sessions`, `Setup`, and `Summary`.

The workbook records **139 distinct cases: 136 Passed, 0 Failed, 3 Not Run**.
The Summary separates the execution sources:

| Group | Cases | Passed | Failed | Not Run | Tested source |
| --- | ---: | ---: | ---: | ---: | --- |
| Native | 109 | 109 | 0 | 0 | `9fd92aa`, Debug and Release / x64 |
| Controller | 15 | 15 | 0 | 0 | `9fd92aa`, synthetic events and render checks in both configurations |
| GUI | 15 | 12 | 0 | 3 | Archived `44d251e` observations and pending manual checks |

Current tested commit: `9fd92aa1a9cb2f664ef20868901deb7806aaa762`.
The working tree was clean when execution started. Runs took place on
**1 November 2026 UTC**; the workbook review completed on **2 November,
Europe/Sofia**. Run timestamps in the workbook and reports use UTC.
QA and manual commit owner: **NKPeshev24**. Preparation and checks used Codex.
This change updates QA records and validation notes without changing production
code or the Native Unit Test project.

## Using the workbook

Each case has a blue banner, side-by-side metadata and six step columns.
Edit Actual Result and choose PASSED, FAILED or NOT RUN in the Status dropdown.
Whole-row colors, case status and Summary totals recalculate. A blank or
partially completed case stays NOT RUN; a failed step makes its case FAILED.
Summary case IDs link to their existing or newly appended case panels.

All 109 Native cases use exact adapter names and separate Debug/Release steps,
including actual runner durations. The existing 63 Native case IDs, creation
metadata, test data and expectations remain in place; their execution results
now reference the fresh runs. Another 46 Native cases and 15 Controller cases
cover preferences, restart, statistics, seed replay and zoom.

The original 15 GUI cases retain their complete observations, dates, source
version, outcomes and positions. They describe the earlier interface, including
its five-item menu. Current six-item menu, Help, Settings and Victory behavior
is recorded in the Controller cases. Archived GUI results are not claimed as
new observations of the current version.

## Current evidence

[Regression provenance](Evidence/FeatureRegression-20261101/Regression.json)
records the source revision, execution windows, methods, class counts and
SHA-256 hashes of evidence, tested sources and private probe sources.

- [Native Debug report](Evidence/FeatureRegression-20261101/Native-Debug.trx)
  and [Native Release report](Evidence/FeatureRegression-20261101/Native-Release.trx):
  **109/109 passed in each configuration**.
- [Debug build](Evidence/FeatureRegression-20261101/Build-Debug.txt) and
  [Release build](Evidence/FeatureRegression-20261101/Build-Release.txt).
- Preferences, Restart, Statistics, Seed and Zoom text logs in the same folder,
  with one result file per feature and configuration.
- Nine reviewed Debug renders at the 640 by 560 minimum, including the menu,
  Help, valid/invalid seed settings, fitted/zoomed/tracked boards, statistics
  and Victory.

The private probes compile the actual Graphics sources and link Data/Logic.
They use synthetic SFML events, hidden windows, isolated preference files,
independent BFS routes and offscreen rendering. Statistics checks inject exact
monotonic timestamps. They cover 10 by 10 at 900 by 600 and 50 by 50 at
640 by 560, including resize and session transitions. Preference checks also
use separate processes to verify loading saved dimensions and failure paths.
These probes are separate from Native Unit Tests and manual UI execution.
Temporary harnesses and workbook builders stay outside the repository.

## Archived evidence and pending checks

The original files directly under `Evidence/` are unchanged. They retain the
29 October results at `44d251e`: 63/63 Native tests per configuration,
controller logs and three inspected renders. The original workbook recorded
78 cases: 75 Passed, 0 Failed and 3 Not Run.

The following remain **Not Run**:

- **QA-013:** complete manual hover/click sweep across all pages and controls.
- **QA-014:** setup and build on a second clean Windows computer.
- **QA-015:** execution in the actual Visual Studio Test Explorer UI.

Opening the file in the native Excel UI and a system-clipboard paste check
remain unverified. Workbook checks used the spreadsheet engine: recalculation,
failed/blank/partial status propagation, restored temporary edits, reviewed
renders across all eight sheets, native internal links and XLSX re-import.

## Repeat native regression

Use **Developer Command Prompt for VS 2026** from the repository root after
preparing dependencies:

```bat
MSBuild NXRP.slnx /p:Configuration=Debug /p:Platform=x64
"%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" build\bin\Debug\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Debug
MSBuild NXRP.slnx /p:Configuration=Release /p:Platform=x64
"%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" build\bin\Release\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Release
```

Keep archived evidence unchanged. For a new run, update each affected case's
code version, Actual Result, Status, execution notes, Modified by and Last
modified on. Record the actual execution method and add fresh evidence.
