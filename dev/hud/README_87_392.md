# TEST 87.418 — Native wall-opening timing (HUD visuals FROZEN)

The 87.417 screenshot confirmed that HUD layout, disappearance and
reveal now look correct. **Do not modify the HUD source classifier,
shock colors/dither, or stage artwork** while calibrating timing.

The supplied F10 M68K log shows FF5000 native state transitions
00->02 (PC F1E4), 02->04 (F20C), 04->06 (F2BE),
and 06->08 (F304), with an approximately 103-frame closed-box
state 04. The song was previously requested at the earlier
FF5104 chooser step 0004->0008, which can precede this stage reveal
by a variable amount of loading time.

TEST 87.418 starts the **preloaded one-shot Versus OGG** at the
native 04->06 wall-opening edge and anchors the existing
120-frame concealment + 30-frame HUD reveal to that same exact
game frame. Earlier 0004->0008 chooser handoff only primes
concement (as before), but does NOT start the song or the delay.
First audible PCM is now read-only lag telemetry,
`XM87.418 VS_PCM_OBSERVED`, never a second clock that drifts.
The 87.417 stage imagery mask, key 8, key 9 and game physics are
unchanged. Native phase 08 still stops the VS song as before.

This implementation is frozen for unrelated stage work after
the user confirms song/fade synchronization in their Windows build.

# TEST 87.417 — Protect native VS transition logo tiles

The user supplied a native 384x224 capture of the VS transition: logo
tile boxes were disappearing only across the top of the screen,
exposing the background forest. The 87.416 broad priority-7 classifier
treated the top row of repeating transition tiles as HUD.

In phases **02/04**, conceal ONLY verified native HP strip, verified
portrait border and central timer-digit sources. Preserve every other
priority-7 intro/logo/forest object, independent of whether it is
small, upper-screen, or a high ROM tile. In combat phases **06/08**,
hide and reveal the complete top HUD through ONE original shock clock.
The key-8 stun renderer and key-9 native fighter cutout are untouched.

The provided 87.416 F10 OAM files contained no sprite rows, and its
probe files cover only combat phase 08. Therefore screenshot/visual
validation is still required for the VS transition; the test guards
the precise phase-vs-size regression without claiming the live OAM
signature is known.

# TEST 87.416 — Unified HUD shock; freeze after visual acceptance

TEST 87.416 is **shock-only**. It does not change key 8, key 9,
native OBJ HUD-only capture, stage painters, or match controls.

During the VS-to-combat transition the existing 120-frame hold and
30-frame common dither clock now cover the **entire native priority-7
top HUD**, including portrait frames, hero/point labels and name cells,
plus the priority-0 palette-1 center clock graphics. It reuses the
earlier 87.393 whole-priority-7 source treatment with the later
ROM stage-05 scenery veto (including 109xxx and palettes 25+),
scene exclusions and top-screen bounds. Its source predicate is separate
from the **unchanged** key-9 HUD-only native capture predicate.

Acceptance lock: when the user confirms that all top HUD contents
are hidden and reveal in unison without scenery damage, treat this
implementation and its regression tests as FROZEN. Never modify the
shock timeline, classifier, or compositor during unrelated stage or
controller work. Require explicit user authorization for any future
shock change. Do not claim visual acceptance from source checks.



Branch: `feature/hud-shock-reveal`. Version 4 only; Version 3 untouched.

## TEST 87.415 follow-up: portrait/point/time sources

The previously verified portrait-border tiles 89113..89115 are now
classified as HUD through the opening shock, alongside 87808..89999
upper HUD sheets restricted to portrait and clock source footprints.
The stage-05 tree/forest sprites in the 109xxx tile family remain
excluded. Unmatched top-band priority-0/7 objects are now recorded in both
the original and custom-stage OBJ passes as `XM87.415 HUD_SOURCE_ESCAPE`
with `capture=0/1` and exact OAM metadata. The first actual HUD tile
processed in custom capture also emits `XM87.415 HUD_SHOCK_CAPTURE_TILED`,
distinguishing missing source classification from an inactive shock.

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
