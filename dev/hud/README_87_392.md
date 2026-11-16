# TEST 87.395 — Handoff-anchored grouped HUD shock

Branch: `feature/hud-shock-reveal`.

The native CPS2 HUD (time, portraits, health and upper status decoration)
and separately composited stun bars share one emulator-frame timeline.
This changes the *appearance only*, not game RAM, ROM, inputs, or round clock.

## Timing corrected from 87.394 capture

The user's Sigma-stage TEST 87.394 capture recorded:
- Screen handoff native status **02** at frame ~1–2;
- Round-introduction status **06** around frame 182;
- Fight/combat status **08** around frame 362.

TEST 87.394 started the HUD effect at **08**, too late.
TEST 87.395 now arms when the match screen FIRST enters phase **02**.
The effect is never restarted by phase **04, 06, or 08**.
Intermediate phases 03/05/07 do not discard the schedule.
For direct game snapshots that first arrive in the later phase, the first
observed 04/06/08 frame is a fallback anchor.

- Phase 02 frame: HUD group hidden at once (do not cover the arena).
- 210 emulated frames after that handoff: begin the reveal.
- Next 30 emulated frames: Bayer 8×8 dither reveals the group.
- During reveal: white blowout shifts to cyan/electric blue, then native
  palette; TWO additional diagonal, converging specular sweeps run from
  the portrait edges toward the timer with white-hot centers and a soft
  blue corona. This is separately animated from the dither coverage.
- After 30 frames: exact normal HUD pixels, no ongoing shine or processing.

Paused paints do not consume frames. KO/results/selection are unaffected.
Version 3 stays clean. Regular and custom stages use the same native source
HUD pixel and SDL stun paths; original CPS depth is restored wherever the
HUD is hidden. The effect does not tint native stage backgrounds or sprites.

## Local verification

Use the existing Windows `BUILD_TEST.bat` and `RUN_TEST.bat`; both target
TEST 87.395. No GitHub Actions; preserve Stable and all earlier TEST builds.

The standalone `dev\hud\RUN_TEST_HUD_392.bat` is retained under its
original file path, but its regression now covers the 87.395 phase-02
clock, transitions, 210+30 frame schedule, the specular beams and the
native depth restoration. Run it locally if desired.

For visual acceptance, confirm the HUD is hidden on **the first combat
screen frame**, not the later FIGHT cue. On Sigma, the dither/shine should
occur around frame 212–241 rather than around frame 572–601, based on the
uploaded phase trace. Confirm both players' bars and portraits, the timer,
and stun bars, across both normal and custom stages.

Runtime markers: `XM87.395 HUD_SHOCK_HANDOFF` and
`XM87.395 HUD_SHOCK_OBJECT`.

Not locally compiled or visually accepted in this environment.
