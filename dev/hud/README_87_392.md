# TEST 87.393 — Match-start grouped HUD shock

Branch: `feature/hud-shock-reveal` (branched from main TEST 87.391).

The timer digits, native P1/P2 life bars, portraits and related top HUD objects
are sampled *per source CPS2 tile*, including the original priority-0 digit
atlas. The separately composed 61×6 stun bars use the same clock.

- Stage intro: CPS2 phase 06 **hides the whole HUD immediately**, without
  counting down the delay while the match is not yet playable.
- Real match start: the first combat phase 08 begins the **210 emulated-frame
  hold** (3.5 s at 60 FPS). A direct phase 08 match entry also arms the
  effect. Phase 06→08 starts the combat clock only once.
- Reveal: the next **30 emulated frames** (0.5 s at 60 FPS) use an 8×8 ordered
  Bayer dither, with white overexposure → electric blue → untouched original
  RGB. The HUD is fully normal on the thirtieth frame.
- Normal combat continues normally after completion. KO/results/selection are
  not obstructed. Reloads and new round openings may re-arm. Paused/duplicate
  paints do not advance the effect. Frame skipping uses the current emulated
  frame count rather than wall time.
- Cosmetic, Version 4 only: Version 3 remains clean. No ROM writes, gameplay
  timing edits, HUD rectangle fills, new stage-filter exclusions or SDL
  renderer handoffs.
- Match-phase HUD object coverage includes all small native priority-7
  top-of-screen objects (bar frames, labels, portrait surrounds), plus
  source-confirmed long HP strips and timer digits. Verified stage scenery,
  tall stage objects, and intro walls are explicitly excluded.
- Original CPS depth and the pixels underneath hidden HUD tiles are restored
  before the next painter pass; all current custom-stage paths use the same
  sprite capture and stun overlay hooks.

## Local checks

Run `dev\hud\RUN_TEST_HUD_392.bat` on Windows with the existing MSYS2/UCRT64
`g++` installation. It builds a standalone test under `%TEMP%` and checks
the 210+30-frame schedule, phase transitions, classification bounds,
ordered-dither coverage, native-pixel/depth restoration and the stun layer.

For visual verification on a **locally built TEST 87.393** using existing
`BUILD_TEST.bat` / `RUN_TEST.bat` (no GitHub Actions), inspect:

1. Intro→FIGHT: hide native timer, portraits, life and stun as one HUD group.
2. At 3.5 s: dither 30 actual game frames, from white to blue to normal.
3. Check that both the arena and fighters behind hidden HUD areas are intact.
4. Repeat on a stock stage and at least two custom stages (e.g. Wily and
   Sigma) to confirm consistent depth and no forest-stage regressions.
5. Try Training, Versus, pause, round transition, tag-in, and Game Version 3.

This branch has **not** been locally compiled or visually accepted here.
A received 87.392 runtime capture confirmed the user was in Version 4,
but the first implementation armed during the lengthy stage intro and did
not cover every small HUD decoration. TEST 87.393 corrects both, and emits
XM87.393 HUD_SHOCK_HIDE / HUD_SHOCK_COMBAT / HUD_SHOCK_OBJECT markers.
