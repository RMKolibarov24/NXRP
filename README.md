# NXRP: The Hedge

A first-person maze game for Windows, built in C++17 with SFML and OpenGL. Explore a seeded hedge maze, solve the stone ritual and gate inscription, and reach the gate while a creature follows the passages. The hedge opens temporary passages and closes them again when they are clear. Lit safe houses offer refuge and checkpoints.

## Play

- **W / S** move forward and back; **A / D** strafe.
- **Mouse** or **Left / Right arrows** turn; **Shift** runs while breath lasts. Breath recovers when you stop running.
- **E** reads the inscription near the entrance. Depending on the seed, touch the stones in the shown sequence or toggle stones whose numbers add up to the target. A wrong sequence resets its progress; in the balance puzzle `*` marks lit stones. Their numbers are recorded as you find them.
- Each stone bears a number. At the gate, follow its inscription to transform the stone numbers into a code. Enter it with **1-9** (top row or numpad), then **Enter**; **Backspace** edits. Step through the gate when it opens. The creature patrols, watches ahead, follows what it sees or hears, and investigates your last known location. Running carries farther through the hedge, though several layers muffle the sound. If it catches you, you return to the entrance and keep your puzzle progress.
- Enter a lit **safe house** to set your personal checkpoint and recover breath faster. The creature cannot enter. **M** toggles sound; **B** toggles camera motion. **Escape** pauses; **R** restarts the current maze. Choose a size from 10 to 100 and an optional decimal seed in Settings; hold Shift while clicking +/- to adjust by ten. The default for new settings is 30×30; saved dimensions are respected. A blank seed produces a new random maze; the same size and seed reproduce the layout and puzzles with the same game version and toolchain.

## Four-player co-op

All players need the same game build. The host chooses the maze size and seed in Settings, then selects **Multiplayer → Host game**. The host's game listens on local TCP port **24816**. With an authenticated ngrok agent, run `ngrok tcp 24816` on the host and share the resulting `tcp://...:port` address privately. Each friend chooses **Multiplayer**, pastes that address into **Host address**, and selects **Join game**. For a test on the same computer, use `127.0.0.1:24816` without ngrok. The host accepts up to three guests, for four players total; there is no game password. Older builds are not compatible with this version.

The host controls the shared maze, creature, puzzle progress and shifting walls. Any player can interact with the stones or gate and reach the exit for a shared win. The creature can pursue and catch any player. The host's pause pauses the shared world; guests can pause locally. A host restart resets the shared run. If a guest disconnects, the others can continue and another guest can fill the open slot. If the host disconnects, guests return to the menu. Public TCP endpoints on ngrok's free plan currently require a verified payment method; see [ngrok's TCP endpoint documentation](https://ngrok.com/docs/gateway/endpoints/tcp).

For a repeatable local integration check, build `tools/CoopSmoke.vcxproj` in Release/x64 after preparing SFML, then run `build\bin\Release\CoopSmoke.exe` from the repository root. It exercises the host/join menu, four-player loopback TCP, room capacity, controls, shared puzzles, safe shifts, individual captures, rejoining and shared victory. Test port 24817 must be available.

The flashlight flickers more when the creature is close, irregular seeded lightning briefly lights the hedge, and pockets of thicker fog and darkness obscure the passages. Creature breathing, pursuit growls, wind, footsteps, shifting foliage, and delayed thunder provide sound cues when an audio device is available; headphones reveal whether nearby creature and hedge movement is to the left or right. A depth-buffered OpenGL scene caches nearby hedge and foliage geometry in chunks and uses shaders for wind, flashlight and fog, with a fixed-function fallback for older drivers. Visibility is selected on the CPU. The creature uses procedural movement, while co-op survivors use baked animation from a rigged model. Asset sources are listed under `assets/models` and `assets/textures`.

## Build

Windows, Visual Studio 2026 with Desktop development with C++, the MSVC v145 x64 toolset, a Windows SDK, Git and CMake 4.2 or later are required. From the repository root, run:

```powershell
.\Prepare-Dependencies.ps1
```

Open `NXRP.slnx`, select Debug or Release and x64, set `Maze.Graphics` as the startup project, then build and run. The script downloads and builds SFML 3.1.0. Assets are copied beside the executable at build time.

Settings persist the maze dimensions under `%LOCALAPPDATA%\NXRP\settings.txt`; seed selection lasts for the current launch. The current seed is visible in the game and on the victory screen.

## Project structure

`Maze.Data` owns the grid and session state, `Maze.Logic` generates seeded mazes and maintains session rules, and `Maze.Graphics` contains the SFML application and first-person OpenGL view. `Maze.UnitTest` covers grid and session rules and seeded puzzle placement. The archived QA evidence predates the first-person systems.
