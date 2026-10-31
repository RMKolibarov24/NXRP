# Algorithm diagrams

These Mermaid flowcharts describe the implemented functions and render directly
on GitHub. The defense presentation also contains an editable generation diagram.

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
    D -- Yes --> I["Extend last room to bottom-right<br/>Mark bottom-right Exit"]
    I --> J(["Return owned MazeGrid"])
```

Invalid dimensions throw before allocation. Unvisited rooms still contain Wall.
Every stack entry has even row and column indices. A completed grid contains
one Exit at `(rows - 1, cols - 1)`. Time and auxiliary stack space are
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
    E -- Yes --> F["Update player position"]
    F --> G{"At bottom-right Exit?"}
    G -- No --> H(["Return Moved"])
    G -- Yes --> I["Set phase Won"]
    I --> J(["Return Won"])
```

All checks that can reject a move precede the position update. Movement takes
O(1) time and uses no grid allocation.

## New Game ownership

```mermaid
sequenceDiagram
    participant UI as MazeApplication
    participant Gen as generateMaze
    participant Rules as startGame
    participant State as GameState
    UI->>Gen: selectedSize
    alt Generation succeeds
        Gen-->>UI: prepared MazeGrid
        UI->>Rules: game, move(prepared)
        Rules->>Rules: Validate complete layout
        alt Valid layout
            Rules->>State: Move in grid, reset position, set Playing
            State->>State: Release previous array
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
