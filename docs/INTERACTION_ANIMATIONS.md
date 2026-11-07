# Physical interaction animation fixes

## Runtime skeletal arms

The primary arms use the unsimplified downloaded MakeHuman mesh and skin with
the original finger bones and vertex weights. Two-bone IK places the wrist and
elbow without stretching arm segments. Each finger joint rotates at runtime;
the old open/grip vertex morph remains only as a fallback. Idle arms have
restrained camera-turn lag. Object contact overrides that lag.

Basketball throws keep host-owned possession for a 0.28-second wind-up. The ball
and hands draw back together, then release; follow-through begins at the actual
release position and returns smoothly while fingers open. Duplicate throws
cannot restart wind-up. Loss of possession/upper-floor eligibility cancels it.
Protocol 12 includes the bounded wind-up state so co-op players share its phase.
All players must use the same new build.

Smoke checks cover original high-resolution rig loading, finger deformation,
finite singular/unreachable IK solutions, preserved arm lengths, reachable
wrist targets, malformed rig headers, shared TCP wind-up, delayed physical
release, hoop scoring and rendered hold/wind-up/release/drink/lever poses.

## Slot motion refinement

The sourced lever now uses quintic easing through reach, pull, hold and return,
with a small damped spring motion. The palm tracks the knob until it returns;
fingers close before the pull and open before withdrawal, with wrist pitch
following the lever. Contact and movement lock last 1.95 seconds.

Reels start after the physical pull and decelerate to staggered exact results.
A shader scrolls between the existing source artwork tiles on the downloaded
curved meshes, replacing rapid symbol swaps and positional jitter. No new model
geometry was made. Systems without shader support retain eased symbol changes.

Release Graphics and CoopSmoke builds pass. The real four-player TCP smoke
passes, including sampled phase-boundary velocity continuity, grip timing,
monotonic reel travel, exact stopping results, and rendered interaction checks.
The broader realism, voice and poker work remains in progress.

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
