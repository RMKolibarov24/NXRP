# The Hedge

Launch `Play.cmd` or `Maze.Graphics.exe`. Windows x64 and an OpenGL-capable
graphics driver are required. Extract the entire archive before playing.

WASD moves, mouse looks, Shift runs, Space jumps, Ctrl/C crouches, E interacts, Escape pauses. Follow the
entrance inscription, find the three engraved stones, then decipher the gate.
Running draws the hunter. Closed refuge doors block you: E opens or closes them.

Houses have colourful two-floor interiors and act as checkpoints. They do not
protect you from the hunter: it can force doors and climb the stairs. Walk up
the east-side stairs to the games court. G picks up or throws a nearby basketball;
face the hoop before throwing. K kicks the football in the direction you face.
Goals and baskets reset the ball and update shared scores. Balls stay upstairs.

Downstairs, face the slot-cabinet handle at close range and press E to pull. The monster can hear the reels.
Three matching reel symbols are a jackpot; play is free. E at the bar seats you and lifts a glass to drink, restoring breath. E drinks again after the cooldown; Q or Space stands up. Clinking glass attracts the hunter. Closed doors only delay it. F collects keeper notes
inside refuges; J cycles collected notes. Puzzle clues and notes survive captures.
Restart clears the run, sports scores, slots and notes.

T toggles screen-space ray-traced lighting, B toggles camera motion, M mutes.
Lighting traces the visible depth buffer for reflections and contact shading;
offscreen geometry cannot contribute to those reflections. A compatible driver
is needed, with an automatic fallback if framebuffer support is unavailable.

For co-op, all players need this same build. Multiplayer > Host listens on TCP
24816. Guests join the host address. On one computer use 127.0.0.1:24816; across
the internet use a configured TCP tunnel or port forwarding. The host controls
shared doors, puzzles, balls, slots and the hunter. Host pause pauses the world.

Asset attribution is included in assets/models/README.md,
assets/textures/README.md and assets/fonts/README.md.

The hands close around carried basketballs, open with throw follow-through, and
reach to the model's actual lever knob during its pull. Hold interactions briefly
lock walking; the bar has a visible stool and a specific sit/drink prompt.
All co-op players need this protocol-10 build for seated stance and handle actors.
