# The Hedge

Launch `Play.cmd` or `Maze.Graphics.exe`. Windows x64 and an OpenGL-capable
graphics driver are required. Extract the entire archive before playing.

WASD moves, mouse looks, Shift runs, E interacts, Escape pauses. Follow the
entrance inscription, find the three engraved stones, then decipher the gate.
Running draws the hunter. Closed refuge doors block you: E opens or closes them.

Refuges are protected checkpoints with colourful two-floor interiors. Walk up
the east-side stairs to the games court. G picks up or throws a nearby basketball;
face the hoop before throwing. K kicks the football in the direction you face.
Goals and baskets reset the ball and update shared scores. Balls stay upstairs.

Downstairs, face the red slot-cabinet handle at close range and press E to pull.
Three matching reel symbols are a jackpot; play is free. F collects keeper notes
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
