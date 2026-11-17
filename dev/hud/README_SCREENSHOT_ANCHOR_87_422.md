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
