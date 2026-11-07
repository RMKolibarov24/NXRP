# Refuge upgrade validation

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
evidence will be appended after creating the final archive.

Lighting uses screen-space depth ray marching, not hardware DXR/path tracing.
Offscreen reflections are therefore unavailable. Basketball uses a fixed throw
arc and bounded upstairs court. These are documented in PLAY.md.
