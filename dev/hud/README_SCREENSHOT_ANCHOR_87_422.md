# TEST 87.433 — Reuse native shock source at the first frame

Pre-shock phases 02 and 04 had a narrower source filter than the working
phase-06/08 shock. The 87.431 zero-dither step was therefore correct but
never reached the complete native HUD. Remove that separate filter and
run phases 02/04/06/08 through the same shockHudSource(), per-tile save
and Tile::finish() Bayer dither. Do not add a new mask or schedule.

The already-working first-frame paintStep(phase,frame)=0 still owns
initial invisibility. The phase-06+23 shock activation, 30-frame
reveal, phase-02+27 song timing, and keys 8/9 remain unchanged.
The shock's existing palette, position, priority and known-scenery
exclusions remain shared in all four phases. There is deliberately no
special additional protection for unnamed phase-02/04 transition art,
so verify intro visuals locally. The unit test asserts parity across
the four phases and leaves the original Tile::finish tests intact.

Windows build / visual test only. No Actions, no change to STABLE.

# TEST 87.432 — Expand precombat HUD sources, not the zero dither

The user's phase-04 screenshot shows the NEW HERO headers, portraits
and TIME artwork still visible while the health bar is suppressed.
87.431's paintStep correctly sends zero to selected sprites, but
87.417's phase-02/04 source routing excluded everything except
HP strips, tiny clock numeral objects and portrait border blocks.

87.432 adds source-bounded top labels, outer-edge portrait palettes
12/15 and upper-clock artwork to the phase-02/04 mask. It keeps the
forest/scenery veto, 87.417 big introductory logo/title blocks and
scenery palettes excluded. The screenshot and uploaded 87.431 F10
trace establish phase 04 at frame 1363, 14 frames after 02->04
(1349). The zip has no OAM sprite rows because no tag was captured,
so the exact native label tile IDs remain unverified.

To avoid another guessed tile family, the native renderer emits
bounded `XM87.432 PRE_SHOCK_OAM` lines with the actual tile, palette,
dimensions, x/y and mask decision for phases 02 and 04. A failure
to conceal additional glyphs can be diagnosed from those actual
source records instead of again broadening arbitrary priority-7
objects. This telemetry does not depend on an F10 tag.

Do not change phase02+27 audio, phase06+23 shock trigger, 30-frame
dither art, key 8/9, or stage-scene rendering. This checkpoint
requires Windows build and visual validation.

# TEST 87.431 — Force existing step-zero invisibility in the painter

This is NOT another attempt to schedule the concealment with a chooser,
music or stage observer. The native CPS2 OAM draw path now passes
`Timeline::paintStep(phase,frame)` to the existing source-identified HUD
dither. For the precombat ROM phases **02,04,06**, `paintStep` is
**zero immediately**, even if the previous pre-hide event was missed
and `primed` is false. The SDL stun meter uses the same method.

The existing screenshot-verified **song phase02+27** and **shock reveal
phase06+23** are unchanged. When `visualShockActive` becomes true,
`paintStep` delegates directly to the original `step()` and the
same 30-frame dither fade. No change to the HUD source classifier,
transition-wall exclusion, ROM game logic, palette or controller keys.

The value zero removes every source-classified HUD tile and leaves
the original stage framebuffer intact; it does not touch unrelated
scenery. A sprite NOT recognized by the approved source classifier
is outside this dither path and may still require source identification.
Build and native runtime visual inspection are required to confirm.

# TEST 87.430 — Guarantee HUD concealment before the shock

USER-APPROVED TIMING IS FROZEN: the Versus song still starts at native
phase 02+27; the HUD reveal still activates at native 04->06+23
(F1475 in the user's corresponding F6 capture), completing its
existing 30-frame reveal afterward. No code inside either timing
gate was changed.

Problem: older pre-hide initialization relied on a chooser-edge
callback, which can be missed or superseded by visual song anchoring.
Before that callback the timeline is not primed and step() returns
30 (HUD fully visible). 87.430 primes concealment on the first
observed native match step 0008 during phases 02/04/06, independent
of audio, loading and shock activation. It does NOT start the song.

All 87.417 ROM-source-specific pre-fight masking stays intact to
protect the Versus logo boxes and the forest; the full priority7
classifier remains scoped to phases 06/08. SDL stun HUD uses the
same primed timeline. The change neither broadens tile filters nor
adds framebuffer rectangles. Runtime log marker
`XM87.430 HUD_EARLY_PRIME` identifies when early concealment armed.

Local HUD regression test asserts pre-song invisibility, unchanged
song phase gate, unchanged 04->06+23 activation, and no rearm after
combat. Local Windows visual validation is required to verify the
observed early HUD components are source-classified; no Windows
build was run remotely.

# TEST 87.428 — Explicit shock activation at screenshot F1475

The user's F6 screenshot `XMVSF_native_20261117_185307_f000001475_000001.png`
records F=1475, status FF5000=06, step FF5104=0008, stage FF5113=05,
WRPC 00F2BE, last FF5000 EDGE 04>06 at frame 1452.
This identifies the desired shock FADE START 23 emulated frames into
native phase 06.

The previous 87.427 fixed 130-frame hold after the song made the
shock start too early. TEST 87.428 replaces that song-to-reveal delay
for captured Versus scenes with an independent read-only native
phase gate: **04>06 entry plus 23 frames**.
Until that event, the HUD stays concealed; starting at that frame,
the APPROVED 30-frame dither reveal runs without further waiting
(unless HUD sources first appear late, retaining the anti-pop safeguard).

**Do not alter the song start.** It remains the previously approved
phase 02+27 M68K event. The 130-frame DelayFrames constant remains
only for the legacy no-captured-anchor PCM compatibility path,
never the normal HUD reveal. Stage graphics, HUD tile classification,
dither art, FX, controllers, and keys 8/9 are unchanged.

Regression tests verify exact F1452->F1475 offset, no premature reveal
even if song elapsed 130f, one-shot triggering, stage-load invariance,
unchanged music clock and 30-frame full reveal at frame 1504.
Binary markers require the old 87.426 SONG cue plus BOTH new
87.428 native shock and HUD fade signals. Local Windows build only.

# TEST 87.427 — Advance shock reveal by 20 frames; DO NOT move song

The user confirmed the musical anchor is correct and specifically requested
the start of the shock insertion 20 frames earlier, without moving the song
back. Keep the **song request at phase 02 + 27 frames**, as confirmed by
F10/F6 evidence. Only shorten the HUD's concealed hold **150 -> 130 frames**
at 60 fps. Keep the **same 30-frame reveal** and existing late-HUD-source
safeguard. Do not change the approved HUD source classifier, draw path,
palette, game physics, stage art, controller or key 8/9 behavior.

Using captured phase entry F1651 and song anchor F1678, the nominal fade
starts F1808 (previously F1828) and becomes fully visible F1837
(previously F1857), provided HUD objects are already drawable.
Unit tests cover the 130-frame global hold and exactly unchanged anchor.
Version advances to 87.427. No GitHub Actions or remote build.

# TEST 87.426 — New screenshot anchor at phase 02 + 27 frames

User screenshot `XMVSF_native_20261117_090817_f000001678_000002.png`
shows frame **1678**, FF5000 status **02**, FF5104 step **0008**,
FF5113 stage **05**, ROM status writer PPC **00F1E4**,
and FF5000 **00->02** at frame **1651**.

Therefore the song and HUD-delay cue is at **phase-02 entry +27 frames**
(instead of the 87.424 frame-zero cue). The requested increased delay
**remains 150 frames (2.5 seconds at 60 fps)**, with the original
30-frame reveal: nominal first fade frame **1828**, complete **1857**.
The late-first-HUD-source safeguard may defer the *visible* reveal,
but never resets the song or initial anchor clock.

Only the visual-anchor offset, corresponding HUD/audio diagnostics,
unit-test expectations, docs, local build scripts and version changed.
Approved native HUD classification, actual rendering, keys 8/9,
audio decoding and gameplay remain untouched. The screen image proves
the current requested placement within this match; repeat-load visual
consistency is not yet verified.

# TEST 87.424 — Phase-02 frame ZERO and +0.5-second HUD delay

At user request, adjust **only** the stage-start timing from TEST 87.423:
- VS song request + HUD shock clock at the **first observed native 00->02
  frame** (offset `0`, instead of the prior phase-02 +35 frames)
- Shock hold from **120** to **150 frames** (0.5 seconds added at 60fps)
- Original **30-frame** synchronized dither reveal unchanged
- Preserve the previously accepted 87.417 native HUD classifier, stage
  rendering and the late-HUD-source visibility safeguard
- Keep keys 8/9, F7/F6 M68K diagnostics and audio decoder unchanged

On the F10 screenshot's example timeline, the 00->02 state write was at
frame 1969, making this the shared song/clock start; the first dither
frame is nominally frame 2119, the 30th is frame 2148. A late first
HUD source may postpone the *visible* fade to avoid an instant pop-in,
but never re-arms the song or initial delay.

Source tests assert the exact frame-zero trigger, one-shot behavior,
150-frame hold, 30-frame fade, PCM non-restart, and late-source guard.
Windows runtime playback has not yet been validated.

# TEST 87.422 — Song/HUD delay at user-selected M68K screenshot position

The user's native F6 PNG `XMVSF_native_20261117_075620_f000002004_000001.png`
was inspected. It shows the stage and fighters already visible, with:
F=00002004, FF5000 phase 02, FF5104 step 0008, FF5113 stage 09,
WRPC 00F1E4, EDGE 00->02 AT=00001969.
Therefore the captured placement occurs exactly 35 emulated frames after
the native 00->02 transition, **before** either 02->04 or 04->06.
This is a single-session target, not proof of identical wall movement
in all stage-load conditions.

TEST 87.422 uses that directly measured `00->02 plus 35 frames` as
its event. Loading before entering phase 02 can vary; the count is
from observed emulated frames *after the status entry*, not from the
earlier chooser-to-match handoff. On this event, the already-preloaded
one-shot VS music is requested and the existing 120-frame concealment
clock begins together. The existing 30-frame HUD dither then begins
at phase-entry+155 frames. The independent audio PCM first-packet edge
is diagnostic only (lag), never a second trigger that can slip the fade.
The original 87.417 late-HUD-source safeguard remains intact: if
native HUD artwork first becomes drawable only *after* the 120f
deadline, the visible dither begins on its next frame rather than
popping into view fully formed. That safeguard never restarts the
song or the initial 120-frame delay.
If phase02 exits before the measured position, it fails closed.

Important: no modifications to the approved 87.417 visual HUD
classifiers/actual sprite overlays, key 8/9, stage art or controllers.
The existing F7 M68K overlay from 87.421 stays available.

The screen capture identifies a desired PLACEMENT, but does not alone
prove it is the first visible opening frame in every match. User
validation with varying load lengths is necessary before freezing
the audio timing.
