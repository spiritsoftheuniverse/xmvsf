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
