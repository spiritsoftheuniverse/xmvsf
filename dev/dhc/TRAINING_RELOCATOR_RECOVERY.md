# Training relocator — recovered 87.110–87.135 work

Branch: `training-relocator` (created from `main` 724d9d40, 2026-11-10).
No changes to the Stable executable, build scripts, CI, or `main`.

## Gesture and retained side-switch

During live Training combat, three independent Start press edges within 1000 ms
(with no more than 450 ms between presses) queue one safe post-frame reload.
The direction held on the third edge selects the preset:

| Gesture | Preset | Point P1 X | Dummy P2 X | Camera visible X |
| --- | --- | ---: | ---: | ---: |
| Start x3, neutral | NORMAL | 462* | 514* | 296* |
| Right+Start x3 | RIGHT | 568 | 725 | 384 |
| Left+Start x3 | LEFT | 217 | 46 | 0 |

*NORMAL restores the actual recorded match-start actor X/Y and recenters the
camera. Values are the observed normal match-start stage-05 coordinates, not
new forced actor assignments. RIGHT retains the original stage-05 placement.
LEFT restores the final user-provided 2026-11-03 fixed native coordinates,
including Y=472 for both point fighters.

The existing single direction+Start side-switch may also act during the
three presses. That is accepted and is not a reason to reject the reset:
the third-edge reload restores the chosen snapshot and positions.

## Native details and recovery

- `xmvsf_stage87_110_restart.h`: three independent Start edges, pending
  NORMAL/RIGHT/LEFT choice, release latch; obsolete mirrored LEFT target corrected.
- `src/intf/input/sdl/inp_sdl2.cpp`: previously present mapped-controller Start
  detector (virtual input 0x87), direction input from that same controller;
  existing Stage18 side-switch integration retained, no extra polling.
- `xmvsf_stage87_110_reload.h`: post-frame `BurnStateLoad`, then native
  stage-05 position/facing/camera writes and camera verification.
- `xmvsf_stage87_111_corner.h`: historical right and center coordinates,
  fixed final left target. Stage-05 camera is native signed word at
  `FF524C` plus 64; left visible X=0 requires native word -64
  (`0xFFC0`), not zero or the old 192 bound.
- `xmvsf_stage87_135_roster_snapshot.h`: native IDs at fighter
  record `+0x52`; P1 point `FF4000`, P2 point `FF4400`,
  P1 reserve `FF4800`, P2 reserve `FF4C00`.
  Prior helper matched and, for native tags, reconstructed the point/reserve
  order from a first-frame save. It previously had no call path from reset.
- `xmvsf_stage87_102_training_lab.h`: now archives each successfully
  captured first-live-frame state by full four-character roster.
  During triple-Start reload, current live IDs are read **before** load.
  The helper prefers the current match first-frame snapshot, then matching
  cached roster(s), and can copy native point/reserve records when tagged.
  If no matching four-character state is available, the reset refuses
  rather than silently switching back to an older team.

The source supports a first-frame snapshot for a team that has been seen
in the current or previous match; it does **not** construct arbitrary new
fighters from ID bytes alone. If a completely unrelated roster has no
corresponding first-frame save, the safe outcome is a logged refusal.

## Verification when using native Windows TEST

Use the existing `BUILD_TEST.bat` and `RUN_TEST.bat`; do not touch Stable
or enable GitHub Actions. Verify directly in stage 05:

1. Neutral Start x3: default point positions and centered camera; confirm
   `CENTER_CAMERA_SYNC` and `TRAINING_TRIPLE_START positioned=1`.
2. Right+Start x3: P1=568, P2=725, camera=384.
3. Left+Start x3: P1=217, P2=46, both Y=472, camera=0; follow
   `CAMERA_VERIFY` for 8 frames to detect ROM camera re-clamping.
4. Use either native tag to change the active point character on a side;
   triple-Start again. Check `ROSTER_VERIFIED` for all four IDs.
5. Switch to a different team, start a new match and repeat. The current
   first-frame snapshot takes precedence over older archived copies.
6. Disconnect/reconnect a pad, then repeat with its mapped Start; ensure
   no repeat-trigger from held buttons. Check normal side-switch still works.

Code review has verified integration paths and constants, **not** Windows
compilation or native gameplay. Corner target and camera verification are
limited to measured native stage 05; other stages remain fail-closed rather
than reusing guessed geometry.

## TEST 87.268: input registration repair

The triple-Start edge detector now samples once in `SDLinpStart()`,
immediately after the existing SDL controller update. Previously the
detector ran only when the driver explicitly requested the virtual
`0x4087`/P2 Start code; nonvirtual keyboard and legacy joystick mappings
could skip it entirely. The new path uses the same Stage13 saved or
menu-selected controller mapping used for gameplay, with a direct FBNeo
`GameInp` binding fallback. The fallback caches action indices, and does
not repeatedly scan every control each frame. The normal single
direction+Start side switch is untouched.

Human triple-tap allowance: maximum 750 ms between taps and 2200 ms
from first to third, requiring an actual release between taps.
Only the **third press's** direction selects LEFT, RIGHT or NORMAL.

`RUN_TEST.bat` starts in the `FBNeo` directory. Tap/trigger events,
snapshot refusal reasons, and reset outcomes are appended to
`FBNeo/xmvsf_training_relocator.log`. Diagnostic examples:
`START_EDGE player=1 tap=1`, `START_TRIGGER player=1 preset=LEFT`,
`RESTART_BEGIN`, `TRAINING_TRIPLE_START ... positioned=1`.
If taps appear but a matching first-frame roster is missing, the log
contains a specific `RESTART_REFUSED reason=...` entry. No full
controller poll or input dump is recorded every frame.

The branch was source-reviewed only; Windows native build and controller
behavior still require running `BUILD_TEST.bat` then `RUN_TEST.bat`.
