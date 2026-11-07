# Refuge upgrade validation

## Downloaded-prop horror upgrade (current)

Release Graphics and CoopSmoke builds pass with VS2022 v143 override. Unit suite:
140/140 pass. Expanded real TCP smoke passes protocol-9 stances, guest bar
restoration without host restoration, independent drink cooldowns, forced-door
entry, upstairs ascent/descent/pursuit and height-aware captures. Original sports,
puzzles, reconnects, room capacity and shared victory checks also pass.

Required imported cabinet/lever/bar/reel/hand meshes and textures load. Renderer
reports `Screen-space ray tracing active: 1`. Ground, gallery and spin frames
reviewed under build/refuge-*.png. A fixed-camera test changes settled source reel
states and verifies visible pixel differences within the model area, excluding
the HUD. Audio checks cover separate slot/drink cues, pause fade and continuous
block equivalence. Sound synthesis and device routing are retained; these tests
do not constitute a subjective headphone listening assessment.

Downloaded-model credits/licenses are included with the assets. No new prop or
hand geometry was authored; source geometry was selected, converted, textured
and animated. Hand rendering retains the source's angular visual style.

Final artifact and launch receipts for this upgrade are appended below. Earlier
artifact evidence in this document is historical and identifies the previous
release, not the current upgrade.

Verified on Windows x64 with installed VS2022 BuildTools v143 (project toolset
overridden on the MSBuild command line).

- Release Maze.Graphics and CoopSmoke builds pass.
- Maze.UnitTest: 139/139 pass. Latest TRX is under ignored build/test-results.
- CoopSmoke passes real four-player TCP joins, capacity, puzzles, doors,
  captures, reconnect and victory. Added checks cover actual upstairs movement,
  ball possession and throwing, a complete basket trajectory, goals at both ends,
  synchronized scores, disconnected carrier release and shared slot handle pulls.
- Ground/upstairs renders inspected: warm cabins, coloured trim/lights, physical
  cabinets, reel glyphs, moving handles, marked court, balls, goals and hoop/net.
- RC1 package manifest: 39 files checked, zero SHA256 mismatches.

The desktop automation native pipe is unavailable after retry and session reset.
The alternate desktop connection reports the same missing native pipe. This
prevents a final interactive desktop screenshot check; rendered game-frame
evidence and integration tests remain available. Package process/window launch
evidence is recorded below.

Lighting uses screen-space depth ray marching, not hardware DXR/path tracing.
Offscreen reflections are therefore unavailable. Basketball uses a fixed throw
arc and bounded upstairs court. These are documented in PLAY.md.

## Final artifact and launch

`build/packages/TheHedge-Windows-x64.zip`, 27,792,464 bytes. SHA256:
`2C94C42DFC0E441D63974F43573B5751F6133DB2BE4F9837A083A7FC661F1301`.
Extracted independently to `build/package-verification/TheHedge-Windows-x64`:
all 40 manifest entries match their SHA256 hashes. VERSION.txt records source
commit `a27b1c2db7abc3748f36d0d66488068e763cf61c` and co-op protocol 7.

Launched the extracted Maze.Graphics.exe with its package working directory.
Observed process 19476, responding True, title `NXRP - The Hedge`, nonzero
MainWindowHandle 5313588, with executable path inside the extracted package.
Left running for the user. Native desktop screenshots remain unavailable due
to the missing automation helper, so no interactive screenshot claim is made.

GitHub PR #6 independently confirms head repository aleksandarkolev-st/NXRP,
branch feature/the-hedge, upgraded source commit a27b1c2. Follow-up validation
documentation does not alter the packaged game binary.

Final renderer probe reports: Screen-space ray tracing active: 1. Expanded four-player regression passes after this renderer check.
