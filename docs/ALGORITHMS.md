# Algorithm diagrams

These Mermaid flowcharts describe the implemented functions and render directly
on GitHub. The defense presentation also contains an editable generation diagram.
The movement and controller diagrams below also describe the earlier grid-based
interface; the first-person systems are documented in the README.

## Maze generation

Implementation: `Maze.Logic/Maze.Generation.cpp`, `generateMaze(size, seed)`.

```mermaid
flowchart TD
    A(["generateMaze(size, seed)"]) --> B["Validate dimensions and allocate walls"]
    B --> C["Initialize mt19937<br/>Open and push (0, 0)"]
    C --> D{"Stack empty?"}
    D -- No --> E["Read top room<br/>Find unvisited rooms two cells away"]
    E --> F{"Any candidates?"}
    F -- Yes --> G["Choose a random candidate<br/>Open wall and destination<br/>Push destination"]
    G --> D
    F -- No --> H["Pop room to backtrack"]
    H --> D
    D -- Yes --> K["Open seeded shortcuts between connected passages"]
    K --> I["Extend last room to bottom-right<br/>Mark bottom-right Exit"]
    I --> J(["Return owned MazeGrid"])
```

Invalid dimensions throw before allocation. Unvisited rooms still contain Wall.
Every stack entry has even row and column indices. A completed grid contains
one Exit at `(rows - 1, cols - 1)`. A seeded selection of walls is opened after
the initial spanning tree to create loops and alternate escape routes. Time and auxiliary stack space are
O(rows * cols).

For 10 by 10, the last room is `(8, 8)`. The extension opens `(9, 8)` and `(9, 9)`
and marks `(9, 9)` Exit. For 11 by 11, the last room is already `(10, 10)`, which
becomes Exit without an extra corridor.

## Movement

Implementation: `Maze.Logic/Maze.Game.cpp`, `tryMove`.

```mermaid
flowchart TD
    A(["tryMove(game, direction)"]) --> B{"Playing, current cell walkable<br/>and exit not already reached?"}
    B -- No --> X(["Return Blocked<br/>Keep state unchanged"])
    B -- Yes --> C{"Known direction?"}
    C -- No --> X
    C -- Yes --> D["Compute one-cell target"]
    D --> E{"Target in bounds<br/>and Passage or Exit?"}
    E -- No --> X
    E -- Yes --> F["Update player position<br/>Increment successful moves with saturation"]
    F --> G{"At bottom-right Exit?"}
    G -- No --> H(["Return Moved"])
    G -- Yes --> I["Set phase Won"]
    I --> J(["Return Won"])
```

All checks that can reject a move precede the position update. Movement takes
O(1) time and uses no grid allocation. The winning step counts once. Time is
accounted for by the controller before this operation, as shown below.

## New Game ownership

```mermaid
sequenceDiagram
    participant UI as MazeApplication
    participant Gen as generateMaze
    participant Rules as startGame
    participant State as GameState
    UI->>UI: Choose selectedSeed or obtain one random seed
    UI->>Gen: selectedSize, usedSeed
    alt Generation succeeds
        Gen-->>UI: prepared MazeGrid
        UI->>Rules: game, move(prepared), usedSeed
        Rules->>Rules: Validate complete layout
        alt Valid layout
            Rules->>State: Move grid, record seed, reset player/statistics, set Playing
            State->>State: Release previous array
            Rules-->>UI: Started
            UI->>UI: Reset feedback and fit the board
        else Validation error
            Rules-->>UI: Exception
            UI->>UI: Show error, preserve old session
        end
    else Generation error
        Gen-->>UI: Exception
        UI->>UI: Show error, preserve old session
    end
```

The rvalue reference parameter alone does not transfer ownership. startGame
performs the move only after layout validation succeeds.
The optional seed choice distinguishes random mode from fixed seed zero.
The controller obtains random entropy once and records the exact seed used.

## Game phases and restart

Implementation: `Maze.Logic/Maze.Game.cpp`, `Maze.Graphics/MazeApplication.cpp`.

```mermaid
stateDiagram-v2
    [*] --> Menu
    Menu --> Playing: New Game succeeds
    Playing --> Paused: Escape
    Paused --> Playing: Continue
    Playing --> Won: Enter bottom-right Exit
    Playing --> Playing: Restart
    Paused --> Playing: Restart or New Game
    Won --> Playing: Restart or New Game
    Playing --> Menu: Exit releases the grid
    Paused --> Menu: Exit releases the grid
    Won --> Menu: Exit releases the grid
```

Menu here means the empty initial/endGame phase. Menu pages and game phases are
separate: Help/Settings leave an unfinished session Paused, while returning to
the menu after victory keeps Won. Continue requires an unfinished nonempty game.
Exit then closes the window. Restart resets position/statistics and fits the
view without allocating or changing cells, current seed or next-game choices.
New Game generates and validates a replacement. Both reset the session's results.

## Elapsed time before an event or update

Implementation: `MazeApplication::updateTime` and `advanceGameTime`.

```mermaid
flowchart TD
    A["Read injected or steady-clock timestamp"] --> B{"Later than previousTick?"}
    B -- No --> F["Apply event or update the page"]
    B -- Yes --> C["Compute positive elapsed duration"]
    C --> D{"Old phase Playing<br/>and maze nonempty?"}
    D -- Yes --> E["Add duration to statistics<br/>Saturate at maximum"]
    D -- No --> G["Keep elapsed time unchanged"]
    E --> H["Set previousTick to the new timestamp"]
    G --> H
    H --> F
```

This ordering includes active time up to Escape or the winning move. It excludes
the paused interval before Continue. Equal/earlier timestamps do not move the
clock anchor backwards. The model retains fractional time; only display text
uses total minutes and seconds. Logic takes a duration and never reads a clock.

## Zoom and player tracking

Implementation: `Maze.Graphics/MazeViewport.h` and `Maze.Graphics/GameView.cpp`.

```mermaid
flowchart TD
    A["Playing input: wheel over board or +/-<br/>0 or Home selects fit"] --> B["Clamp zoom level to 1 through 4"]
    B --> C{"Valid dimensions<br/>and finite area larger than border padding?"}
    C -- No --> X["Skip board drawing"]
    C -- Yes --> D["Compute fitted integer cell size<br/>Reserve two pixels on each side"]
    D --> E{"Fitted cell at least two pixels?"}
    E -- No --> X
    E -- Yes --> F["Multiply by zoom level<br/>Compute world dimensions"]
    F --> G{"Finite world dimensions?"}
    G -- No --> X
    G -- Yes --> H["For each axis: center if it fits<br/>otherwise follow player and clamp to edges"]
    H --> I["Save caller view<br/>Set board viewport and scissor"]
    I --> J["Draw grid and player in world coordinates"]
    J --> K["Restore caller view"]
```

The board clips to its own area and leaves the HUD in pixel coordinates.
Continue and resize retain zoom; New Game and Restart return to fit. Zoom does
not change model cells, seed, position or successful-move count. Invalid or
overflowed geometry returns no drawable layout before GameView changes its view.

## Ray-traced lighting and fear

Implementation: `Maze.Graphics/GameView.cpp`, `assets/shaders/post.frag`.

```mermaid
flowchart TD
    A["Prepare scene FBO with colour and depth textures"] --> B["Render hedge, refuges, doors and creature"]
    B --> C["Linearize depth and rebuild view position and normal"]
    C --> D["Ray-march screen-space reflections"]
    C --> E["Sample contact ambient occlusion"]
    C --> F["March volumetric fog toward the flashlight"]
    D --> G["Cold grade, fear warp, chromatic edges, grain and vignette"]
    E --> G
    F --> G
    G --> H["Composite full-screen quad, then draw the HUD"]
```

The pass renders at the window resolution and only samples geometry already
visible in the depth buffer, so it cannot reflect objects behind the camera.
If framebuffer object entry points are unavailable the renderer draws directly,
and **T** disables the expensive ray marches without disabling the grade.

Fear is a smoothed 0..1 value built from creature proximity, darkness, pursuit
and safehouse shelter. It feeds camera sway, flashlight interference, heartbeat
rate, the post shader and the creature's door rattles. Safehouse doors are four
per refuge; the host stores their open state in one bit mask appended to the
version-9 world snapshot. Guests animate the received mask locally. A closed
door blocks the threshold, an open door does not, and the creature may slam an
open door. The hunter routes through doorway centres, strikes closed doors
three times to force them, and follows continuous stair waypoints to pursue
upstairs. Captures compare interpolated XYZ height. Repeated detections raise
vigilance, hearing range and search memory. Slots and the bar create temporary
noise targets; crouching reduces sound and detection range.
