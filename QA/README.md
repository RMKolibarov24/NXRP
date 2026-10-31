# QA results

Open `QA_Test_Cases.xlsx`. It starts on `Main Menu` and groups the 78 cases by
feature: `Main Menu`, `Settings`, `Gameplay`, `Victory`, `Generation`,
`Data and Sessions`, and `Setup`. The final `Summary` tab contains totals
and links to each case. UI scenarios appear before supporting native cases.

Each case uses a blue TEST CASE banner, side-by-side metadata and six step
columns. Edit Actual Result and select PASSED, FAILED or NOT RUN from the
Status dropdown. The whole step row turns green, red or amber. Case status
and Summary totals recalculate; a blank or incomplete case stays NOT RUN.

All 63 Native Unit Tests retain their exact adapter names and both Debug/Release
results. Full UI observations remain below each case details panel; the Actual
Result column shows a short result for readability. The introductory menu
display check is split into four observed steps without adding a new case.

Tested source: `44d251e215951517147ecfbb09f292fb42eefa29`, clean at execution.
This QA change adds records only. Run times in the workbook and TRX reports use UTC.
QA review owner: **NKPeshev24**. Commit owner: **RMKolibarov24**.
The workbook was prepared with Codex;
execution methods distinguish native tests, Sky UI observations and controller probes.

The recorded result is **78 distinct cases: 75 Passed, 0 Failed, 3 Not Run**.
The pending cases are the complete manual button sweep, setup on a second clean
machine, and execution in the actual Visual Studio Test Explorer UI.
Native VSTest console execution passed **63/63 in each configuration**.

`Evidence/` contains the two original TRX reports, controller-probe output and
three inspected renders. Controller probes exercised the real application through
synthetic SFML events and offscreen rendering. They are separate exploratory checks,
not additional Native Unit Tests; their temporary harnesses are outside the repository.
The 10x10/50x50 minimum-window board renders were checked in Debug. Screen navigation
and victory probes were run in both configurations.

To repeat the native checks, use **Developer Command Prompt for VS 2026** from the
repository root after preparing dependencies:

```bat
MSBuild NXRP.slnx /p:Configuration=Debug /p:Platform=x64
"%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" build\bin\Debug\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Debug
MSBuild NXRP.slnx /p:Configuration=Release /p:Platform=x64
"%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe" build\bin\Release\Maze.UnitTest.dll /Platform:x64 /Logger:trx /ResultsDirectory:build\TestResults\Release
```

Keep archived evidence unchanged. After a new run, update each affected case's code
version, Actual Result, Status, execution notes, Modified by and Last modified on. The Summary totals
update from the case steps. A blank or incomplete step keeps its case Not Run.

The workbook was checked with the spreadsheet engine: recalculation, failure/blank
propagation, restored test edits, rendering of all eight sheets and XLSX re-import.
The native Excel UI opening check was not completed because app-control approval
timed out.
