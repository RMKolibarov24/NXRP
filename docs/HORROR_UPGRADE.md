# Horror and refuge upgrade

The Hedge now combines a dark hunted maze with colourful two-floor refuges.

## Delivered systems

- Animated, collidable refuge doors shared in four-player co-op. The hunter
  cannot enter; it investigates occupied refuges and rattles/slams their doors.
- Framebuffer lighting with depth-ray-marched reflections, contact occlusion and
  volumetric fog. T toggles it; unsupported framebuffer drivers fall back.
- Fear-driven grain, grade, distortion, breathing, heartbeat and directional
  whispers. Seeded lightning, fog and creature pressure support exploration.
- Original collectible keeper field notes, F to collect and J to reread, retained
  after captures and cleared on restart.
- Larger protected refuges, carved hunter bypasses, stairs, upstairs galleries,
  warm lighting, colourful trim, festoon bulbs and physical furniture.
- Physical purple slot cabinets with red pull handles. E while facing the handle
  starts shared staggered reels and lever animation; matching three is a free-play
  jackpot. Each machine has independent state and cooldown.
- Upstairs basketball pickup/throw, football kicking, gravity, rebounds, court
  bounds, hoop/goal detection, shared scores and automatic ball reset.
- Protocol 7 synchronizes player elevation, doors, sports and slot state. Older
  builds are rejected; all co-op players must use this build.

## Verification

Release Graphics and CoopSmoke build with installed VS2022 v143 override.
139 unit tests pass, including refuge placement, hunter safety and soundscape.
Expanded TCP smoke covers four players, capacity, reconnects, both puzzles,
individual captures, shared victory, doors, actual stair movement and gallery
transitions, exclusive basketball possession, full throwing arc into the hoop,
football goals at both ends, synchronized scores, disconnected carrier release,
slot handle pulls, shared symbols/cooldown and upstairs interaction rejection.
Rendered ground/upstairs views were inspected under build/refuge-*.png.

The packager includes assets, app-local MSVC runtime, launch command, play guide,
third-party notices and a SHA256 manifest. Runtime package checks and final
commit/launch evidence are recorded in docs/RELEASE_VALIDATION.md.

## Technical boundaries

Lighting is screen-space ray tracing, not DXR/hardware path tracing. Reflections
cannot include offscreen geometry. Sports use a compact upstairs court with a
fixed basketball throw arc in the facing direction. Slots have free play and
no purchases, currency, payouts or real-world prizes. 10x10 mazes fit one larger
refuge; larger maps contain multiple cabins.

## Research and original direction

Outlast's vulnerable escape and Labyrinthine's cooperative maze informed the
pursuit/exploration loop. RE7 developers describe environmental exploration,
fear and puzzles as core pillars; keeper notes and contrasting refuge interiors
supply original environmental history. No assets from those games were copied.

- https://redbarrelsgames.com/games/outlast/
- https://www.valkogames.com/
- https://news.capcomusa.com/lets/browse/resident-evil-7-developers-discuss-the-new-evolution-of-horror

## Delivery

Push to the existing PR #6 head, aleksandarkolev-st/NXRP feature/the-hedge,
remote fork. Do not push upstream origin. Package the verified Release build and
launch it visibly for the user; leave it running for play.
