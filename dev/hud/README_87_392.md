# TEST 87.399 — Corrected HUD concealment, real timing, and binary proof

Branch: `feature/hud-shock-reveal`. Version 4 only; Version 3 untouched.

## TEST 87.414 follow-up: transition and name source scope

The prior 87.413 all-small-priority-7 transition fallback was NOT
source-verified and could mask native forest/intro trees. TEST 87.414
removes it during phases 02/04, retains verified HP/time, and adds
upper-HUD candidate glyphs only within original Stage87.35 name bounds
and HUD ROM tile families. The Wolverine-sized 11x2 multi-tile name
now has an explicit regression; stage trees and mid-screen objects have
negative regression cases. Captures print bounded
`XM87.414 SHOCK_NAME_ESCAPE_CANDIDATE` records for any remaining name
source not covered, with priority, tile, palette, bounding box and
phase so real exceptions can be fixed without speculative broad filters.

## Root causes of the repeated 87.397–87.398 regression

1. Previous `Timeline::step()` selected
   `max(versusSongStart+delay, firstPhase06or08Frame+6)`.
   In the actual VS-to-fight timeline, the latter usually won. Changing
   delay from 180 to 120 frames **had no visible effect**.
2. Previous native HUD classifier disabled *all* masking during phase
   02/04 to avoid forest/intro trees accidentally appearing. The correct
   approach is not broad priority-7 removal in those scenes, but only
   source-verified HUD objects, so actual HP strips/timer can be invisible
   from the handoff without deleting surrounding VS scene artwork.
3. Previous timeline could complete its 30-frame reveal before ANY
   genuine HUD artwork was first drawn, causing already-visible HUD
   on its first combat scene frame.

## Corrected design

- On the existing M68K chooser-to-Versus edge:
  `XM87392::timeline().prepareForSong(frame)` marks the HUD as
  initially **invisible**. It does NOT start the delay.
- When WASAPI actually outputs the first `versusscreen.ogg` PCM,
  the atomic audio-start event triggers `startWithSong()`.
  **120 emulated game frames (2 seconds)** after that event, the
  original 30-frame white-electric-blue materialization begins.
  There is **NO phase-06/08 +6 frame override**.
- During phase 02/04, only known ROM HUD objects may be masked:
  priority-7 verified HP sheet 88417–88579 and 89417/89418
  at y=20/32, palette 2/4; and verified priority-0 timer atlas
  87808–87826, palette 1, size 2x2. Unknown small priority-7
  art, large transition logos, forest scenery and fighters are
  not eligible. During actual fight status 06/08, retain the
  established broader HUD grouping.
- The first time a genuine drawable HUD source is encountered is
  tracked independently of the scene phase. If the music+120+30
  sequence has already expired *offscreen*, **that first HUD source
  frame is hidden**, and the 30-frame reveal starts immediately on
  the next emulated frame. This is a source-availability guard, not
  another six-frame scene delay.
- The SDL stun bar participates in first-source recognition on
  custom stages. No added ROM writes, gameplay changes, blackout
  rectangles, or altered stage render order.

Important physical limit: HUD pixels cannot materialize before the
native ROM exposes their source objects to the renderer. If they
first appear after the song's absolute reveal deadline, the code
shows a first hidden frame and starts the effect at the first possible
actual source frame. Its `HUD_SHOCK_FIRST_SOURCE` diagnostic
makes this lateness explicit rather than silently ignoring the delay.

## Local test and proof

The normal Windows `BUILD_TEST.bat` / `RUN_TEST.bat` now target
`XMVSF_Test_87_399.exe`. The local HUD regression runner remains
`dev/hud/RUN_TEST_HUD_392.bat` for backward compatibility; its
tests now verify:
- Initial concealment even before the VS audio is decoded;
- 120+30 exact audio clock without the old phase override;
- Source-qualified 02/04 masking rejects unknown forest/intro
  tiles while matching both HP strips and the timer;
- The first genuine HUD source is fully invisible if the song
  reveal previously finished while the sprite was not drawable;
- Scene reset, Version 3, white glint, Z restoration and stun alpha.

**New build protection:** `dev/dhc/BUILD_TEST.sh` checks the
compiled native EXE itself for the new, literal
`XM87.399 HUD_SHOCK_RUNTIME_SOURCE` signature. If the HUD source was
not recompiled/linked, the builder now **fails without publishing**
a TEST executable. This is a single fast byte-string check
before the existing atomic TEST publish, not a full regression
suite or a build-cache reset. Stable and old TESTs stay intact.

Runtime markers:
`XM87.399 HUD_SHOCK_RUNTIME_SOURCE`,
`XM87.399 HUD_SHOCK_PRIMED`,
`XM87.399 HUD_SHOCK_SONG_START`,
`XM87.399 HUD_SHOCK_FIRST_SOURCE`,
and `XM87.399 HUD_SHOCK_OBJECT`.

Source integrated, but this environment cannot run the full native
MSYS2/Windows game build or visually accept the result.
