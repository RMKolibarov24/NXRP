# Physical interaction animation fixes

## Changes

- Bar now advertises E sit-and-drink, with a usable counter interaction region,
  sourced stool and stable seat facing the counter. Q/Space stands, movement
  is locked while seated, and capture/restart clears the seat/drink action.
- Source tumbler moves with the sipping hand; counter height, seat distance and
  eye height were corrected after render review so the view clears the counter.
- Hands anchor to the basketball in world space, ease into holding, curl around
  it and follow through on release. Held height accounts for jump/crouch.
- Open/grip poses use the downloaded character's original skeleton, weights,
  topology and textures. Runtime interpolates the baked finger deformation;
  no replacement hand geometry is authored.
- Slot interaction aligns to the accessible lever side. Reach precedes pull;
  the right palm tracks the actual source knob position through pull/return,
  then withdraws. Lever and contact use the same InteractionPose.h functions.
  Walking/jumping is briefly locked during contact.
- Protocol 10 shares seated stance and slot actor, so other players' pulls
  cannot trigger your own hands. All players need the same build.

## Verification

Release Graphics and CoopSmoke compile with VS2022 v143. Unit suite 140/140
passes. Expanded real TCP smoke verifies guest sitting, movement lock, standing,
seated stance propagation, bar benefit/cooldowns, source grip mesh deformation,
source asset loading and lever-knob trajectory. Existing four-player puzzles,
sports, captures, stairs and forced-door tests pass. Rendered basketball hold,
seated drinking and physical lever contact were inspected and framing corrected.
A release/follow-through render check also exercises an actual basketball throw.

Source/licensing: assets/models/REFUGE_SOURCES.md and assets/models/README.md.
Latest package/PR/launch receipts are in RELEASE_VALIDATION.md.
