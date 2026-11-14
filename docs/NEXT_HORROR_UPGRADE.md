# Downloaded-prop horror upgrade

## Implemented

- Downloaded CC BY 4.0 Pillar Slots assembly, original separate lever and source
  curved reel strips. The lever animates around its source pivot; columns cycle
  original artwork, stop at staggered times and show the shared result.
- Downloaded CC0 bar-counter module. E drinks, restores breath and reduces fear;
  individual cooldowns and benefits synchronize correctly for co-op guests.
- CC0 Innerscene/MakeHuman anatomical hands/forearms selected from original
  character triangles, with original skin atlas, first-person sway and reaching/
  drinking movement. No new replacement prop or hand geometry was authored.
- Separate slot pull/motor/click/result and glass-clink sounds. More textured,
  spatial monster rasp/growls; pause/mute fades sound output.
- Space jumps, Ctrl/C crouches. Stance and jump height synchronize in protocol 9.
  Crouch lowers speed, camera, avatar height and hearing/vision signals.
- Seeded irregular passage-opening/closing delays with player-overlap guards.
- Monster doorway routing, three-strike forced doors, continuous stairs/gallery
  pursuit and height-aware captures. Repeated detections increase vigilance,
  hearing radius and search persistence. Bar and slot noise attracts investigation.

## Verification

140 unit tests pass, including distinct audio cues, pause fade and deterministic
block rendering. Expanded four-player TCP smoke passes joins/capacity, puzzles,
reconnect/captures, refuge door collisions, forced entry, actual stair movement,
upstairs hunter ascent/descent/capture, shared sports, per-player bar restoration/
cooldowns, shared slots, stances, asset loading and active screen-space tracing.
Ground, gallery and spinning-machine frames were rendered and reviewed. Source
hands are anatomically detailed but angular in this game's legacy lighting;
this is not a photorealistic hand-rendering claim.

Source, licenses and conversion changes: assets/models/REFUGE_SOURCES.md and
assets/models/README.md. All new prop/hand geometry is adapted from downloaded
assets. Conversion scripts preserve source topology and texture coordinates.

## Delivery

Final package checks, PR #6 head update and launch evidence will be recorded in
docs/RELEASE_VALIDATION.md. Use remote fork, branch feature/the-hedge; never push
upstream origin. Existing earlier release archives remain untouched.
