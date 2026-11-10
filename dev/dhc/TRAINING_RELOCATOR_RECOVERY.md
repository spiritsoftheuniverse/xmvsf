# Training relocator — recovered 87.110–87.135 work

Branch: `training-relocator` (created from `main` 724d9d40, 2026-11-10).
No changes to the Stable executable, GitHub Actions, or `main`. TEST build/launch scripts are versioned.

## Gesture and retained side-switch

During live Training combat, three independent Start press edges within 2200 ms
(with no more than 750 ms between presses) queue one safe post-frame reload.
The direction held on the third edge selects the preset:

| Gesture | Preset | Point P1 X | Dummy P2 X | Camera visible X |
| --- | --- | ---: | ---: | ---: |
| Start x3, neutral | NORMAL | 462* | 514* | 296* |
| Right+Start x3 | RIGHT | 689 | 747 | 384 |
| Left+Start x3 | LEFT | 79 | 21 | 0 |

*NORMAL restores the actual recorded match-start actor X/Y and recenters the
camera. Values are the observed normal match-start stage-05 coordinates, not
new forced actor assignments. Both side presets use native ground Y=472.
RIGHT is mirrored from the new LEFT reference, rather than independently
tuned; thus the reference coordinates always sum to 768 per fighter.

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

## TEST 87.269: native P1/P2 joystick Start fix and synchronized diagnostics

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

The previous fallback erroneously accepted only joystick codes at or above
`0x4200`, excluding the `0x4000–0x41FF` native P1/P2 bindings altogether.
TEST 87.269 reads those actual FBNeo-bound joystick codes too. The detector
writes `INPUT_MONITOR_ON` at startup, `INPUT_GATE` on combat-gate changes,
`INPUT_RISE` on observed physical Start edges (even if combat is not ready),
and `START_EDGE` / `START_TRIGGER` when a valid triple-press is counted.
This distinguishes detection failures from missing snapshots / reset refusals.

`RUN_TEST.bat` no longer writes to hardcoded `test/87.235`. All gameplay
trace/replay roots use `test/87.269`, and the relocator's own log is inside
`test/87.269/observer/training_relocator.log`, with a current TEST version
session marker. The actual game input mapping log and stage18 side-switch
log are copied into that observer folder after the game exits. Older
`87.181`/ `87.186` strings in legacy *schema* names still identify their
instrumentation stages, not the current TEST executable.

`RESTART_REFUSED reason=...` reports snapshot/roster/reload failures after
a recognized triple-press. No full controller poll or per-frame dump is
recorded. The warm object cache remains intact; no build preflights added.

The branch was source-reviewed only; Windows native build and controller
behavior still require running `BUILD_TEST.bat` then `RUN_TEST.bat`.

## TEST 87.270: first-live-frame snapshot capture restored

The user-provided `87.269.zip` proves controller registration is working.
`training_relocator.log` contains repeated successful `START_TRIGGER` for
NORMAL/LEFT/RIGHT, followed each time by
`RESTART_REFUSED reason=no_match_start_snapshot`.
`observer_health_test.log` proves combat starts at frame 1446.

Source investigation found the actual blocker: `XM87102Lab::Capture::frame`
had NO call site in the completed-frame `run.cpp` path. Consequently
`Capture.saved` was always false and both triple-Start resets and Training
KO-landing restores lacked the expected first-frame state. TEST 87.270
calls the existing capture function **once per completed real emulation
frame**, immediately after stage18/Training HUD state is sampled and
before either reset handler, preserving the first-live-frame one-shot
save logic inside `Capture::frame`.

The same versioned observer log now records
`MATCH_START_CAPTURE frame=... saved=1 path=...` (or saved=0 for a
genuine filesystem/savestate failure). Expect this event when Training
first enters live combat; then `START_TRIGGER`, `RESTART_BEGIN`, and
`TRAINING_TRIPLE_START positioned=1` on a successful third press.
The supplied F9 `.fs` snapshot was inspected offline: it has valid
`FB1 FS1` layout, the expected `CpsRamFF` block, and four valid
fighter IDs, supporting the existing cache parser format.

No new build gates, GitHub Actions, cache invalidation, or Stable changes.
The patch is source-reviewed only; a native Windows build and live gameplay
test remain necessary.

## TEST 87.271: mirrored taunt reference positions

The new `87.270.zip` contains `observer/training_side_switch.log` with
the two final marker events:

- First RIGHT marker: P1=689, P2=747, 58 px apart
- Second LEFT marker: P1=79, P2=21, 58 px apart

Both are already perfectly mirrored: 689+79=768; 747+21=768.
The LEFT pair is the canonical preset; RIGHT uses `768-leftX` per
fighter, giving 689/747 and keeping future adjustments symmetrical.
Both side presets use ground Y=472; LEFT visible camera X=0 and RIGHT=384.
NORMAL/center preserves the original first-frame actor positions and
existing camera-centering logic unchanged.

Important: 87.270 `training_relocator.log` confirms snapshot creation
(`MATCH_START_CAPTURE saved=1`) and all six observed Start triples.
The `TRAINING_TRIPLE_START` results, however, reported
`positioned=0`. The old stage-05-only camera/actor validation did not
write the corner coordinates. The zip does not include an exact
stage/phase/camera measurement explaining which validation was false.
87.271 now writes `POSITION_APPLIED` or `POSITION_REFUSED` (stage,
phase, pointer, both actors, and original X) directly into the *versioned*
relocator log. It deliberately keeps the stage-05-only mutation gate,
rather than applying unverified stage-05 camera writes to every stage.
Placement success therefore remains to be established by native play,
not assumed from correct triple-Start registration.

Only the current TEST source/launcher version is advanced. Stable, cached
build objects and the existing local build procedure are unchanged.
