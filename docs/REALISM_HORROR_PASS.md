# Active realism/horror pass

The full user scope remains active. Earlier releases are checkpoints, not proof
that the complete pass is finished. Delivery requires verification, a fresh
package, fork PR #6 updates and visible launch. Never author new model geometry.

## Requirements and authoritative status

1. Higher-quality downloaded hands, runtime skeletal animation and IK, finger
   contact, weighted arms and throwing anticipation/follow-through: **unfinished**.
   Existing hands use the downloaded MakeHuman source with baked grip morphs.
   They still need full-resolution source extraction, runtime skinning and IK.
2. Roughness/normal maps, softer shadows, wet reflections and readable fog:
   **unfinished**. Existing screen-space reflection/contact shading is not proof
   of material maps or soft shadows. Source suitable maps and render-verify.
3. Layered breathing, uneven steps, distant knocks and positional growls, with
   wall/door muffling and silence: implemented acoustic transmission/filtering,
   uneven footfall timing, paired knocks and ambient hush. Quantitative unit
   checks pass. Final listening/device and render review still required.
4. Fair unpredictable pursuit, lost trails, door lingering, learned escape
   routes, breach warnings: implemented bounded last-evidence searches, two
   paused search beats, local weighted exit checks, observed junction learning,
   floor-aware senses and evidence-based interior pursuit/door testing. Learning
   persists through captures but resets on a new run. New unit and TCP checks
   cover hidden-player invariance, floor concealment, learning bias, search
   expiry, stair pursuit/descent, captures and forced entry. Final play review
   still required.
5. Downloaded abandoned belongings, damaged shelters, footprints/scratched signs,
   connected notes and distinct sections: **unfinished**. Current keeper notes
   and scenery do not meet this full requirement. Source models and expand the
   environmental story and distinct maze identities.
6. Meaningful bar/slot noise, crouch concealment, running/breath risks, temporary
   refuge with multiple exits and warnings: existing systems plus acoustic/floor
   sensing now connect these risks. Final balance/play review remains required.

## Added user scope

- Slot improvements: independent checkpoint 5767a15, receipt da236b1, pushed to
  fork feature/the-hedge feeding PR #6. Smooth reach/grip/pull/return/release,
  wrist follow and decelerating source-artwork reels. Exact committed-source
  builds and real four-player smoke pass; packaged and launched visibly.
  See INTERACTION_ANIMATIONS.md and RELEASE_VALIDATION.md.
- Proximity voice: implemented V push-to-talk, F8 global toggle and F1-F4 peer
  mute. Windows waveIn opens only while focused eligible PTT is held; no files
  are recorded. Pause/focus loss, disconnect, generation changes and mute clear
  queues. Host authenticates actors, validates generations/replays/rate, relays
  within 12 units; playback applies pan, distance, wall/door/floor filtering.
  Audible speech generates hunter noise. Bounded queues drop speech ahead of
  gameplay congestion. Two 20ms output buffers and four-frame peer queues bound
  latency. Protocol 11 requires all co-op players to update together.
  Synthetic real-TCP tests cover codec/parser bounds, identity spoofing, stale
  generation/replay/rate rejection, no self echo, mute and range. All fixtures
  disable audio device I/O: **actual microphone-to-remote-speaker testing remains
  unverified**. Test that manually with two players before final voice sign-off.
- Poker: source pack downloaded for evaluation under build/asset-sources.
  Primary license verification, conversion and playable co-op poker remain
  **unfinished**. Asset acquisition alone does not constitute poker gameplay.

## Verification checkpoint

Release Graphics and CoopSmoke build with installed VS2022 v143. The unit suite
passes 147/147, including quantitative high-frequency loss at equal gain,
closed-door sight/hearing, hidden upstairs motion independence, bounded search
expiry, observed route learning and reproducible weighted search choices.
Expanded real four-player TCP smoke passes after isolating other players during
its single-target upstairs test. It checks quiet upstairs concealment, actual
noise acquisition, continuous stair pursuit and descent, and protocol voice rate
limits with device I/O disabled. Re-run latest smoke after any subsequent edits.

Build: MSBuild NXRP.slnx /p:PlatformToolset=v143 /p:Configuration=Release
/p:Platform=x64 /m /v:minimal. Build tools/CoopSmoke.vcxproj similarly, then run
build/bin/Release/CoopSmoke.exe from the repository root (TCP 24817).
Graphics build copies assets; CoopSmoke does not. Unit runner is VS vstest.console.

## Next work

1. Preserve/checkpoint current acoustic, fair-AI and voice code after final smoke.
2. Extract unsimplified source arms and original rig; implement runtime skeletal
   skinning, two-bone IK, shaped finger contact and weighted throwing phases.
3. Download/attribute material maps; implement normal/roughness-driven lighting,
   softer shadows and restrained wet reflections, then inspect renders.
4. Download licensed story props and build connected notes/distinct maze sections.
5. Implement proper playable co-op poker with host-authoritative actions and
   private hole cards; ship source licenses/credits for imported props/cards.
6. Full audit against every requirement, real audio/voice/play review, fresh
   package, PR update and launch. Do not mark the goal complete prematurely.

Voice API references: Microsoft waveInOpen and Devices and Data Types:
https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveinopen
https://learn.microsoft.com/en-us/windows/win32/multimedia/devices-and-data-types
