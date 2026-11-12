# XMVSF — Native Game, Verifiable Replays, and Preservation

This repository develops a native Windows X-Men vs. Street Fighter
application with its own SDL boot menu, Training, Versus, Arcade,
Tournament rules, DHC research tools, and full-state replay testing.

## Read first: native camera and custom-stage contract

**Before editing, importing, or rendering ANY custom level**, read
`dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_49_camera.h`
and `dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_49_state.h`
(the existing native camera capture/observer implementation), then
`dev/dhc/FINAL_DESTINATION_274.md` for the current custom-stage integration.
The game's own camera model is authoritative—not character midpoint estimates,
a fixed SDL viewport coordinate, or artwork-driven camera substitutions.

- **Custom levels must behave like normal native levels.** The scrolling
  arena and stationary world geometry are owned by the game's camera.
  Foreground/platform/art must use **both native horizontal and vertical
  camera coordinates**, not follow either player's jump or keep its floor
  pinned to the viewport. Only parallax back layers may use different
  camera-relative rates.
- **Native stage-05 verified register contract:** native camera X =
  signed 16-bit word at `0xFF524C` plus 64; native camera Y =
  signed 16-bit word at `0xFF5250`. Super-jump observations carry Y
  approximately **256 → 0**. This is camera motion, not a fighter Y
  adjustment. For a bottom-registered world asset at ground camera Y=256,
  its screen top is `224 - worldImageHeight + (256 - cameraY)` in native
  384×224 coordinates. As the camera rises, the fixed world floor moves
  down/off-screen rather than moving upward with the fighter.
- **Stage-specific verification is mandatory.** Do not extrapolate the
  stage-05 RAM addresses, vertical range, or camera origin to another
  native stage without its own recorded camera contract. Preserve game
  physics, camera state, and stage collision; do not write RAM to imitate
  scenery movement.
- Preserve the original transparent stage asset, use
  **nearest-neighbor** sampling for pixel art, and keep scene →
  platform → native fighters/FX → existing HUD depth.
- Maintain camera X/Y evidence in the existing native diagnostic log.
  Read native movement first and verify idle, jump, super jump, and
  landing before claiming a new level behaves like a native level.
  Local incremental `BUILD_TEST.bat` and `RUN_TEST.bat` remain the
  only build/test entrypoints; no GitHub Actions or new build gates.

This rule applies to Final Destination, Wily Factory, and any future
custom level. If another archived camera project is used for comparison,
the in-repository native observer and ROM camera capture are still the
required source of truth for the active TEST branch.

## Our data-model manifesto

**[READ THE REPLAY DATA MODEL MANIFESTO](REPLAY_DATA_MODEL_MANIFESTO.md)**

It is the **authoritative contract** for native F9 recording, session
boundaries, host-only decisions, emulator snapshots, input segments,
manifest sealing, legacy recovery, fast-forward, and failure reporting.
Future implementations must obey it instead of creating incompatible
one-off recorders or runners.

The nonnegotiable commitments:

- **Truth over approximation.** Capture exact native input bytes,
  actual host state, verified save-state checkpoints, and monotonic
  host-event order. Never infer unseen menu transitions from ROM data.
- **One F9, one user-visible recording.** Keep the red `RECORDING`
  counter visible in menu and gameplay, green `SAVED` confirmation
  after successful finalization, amber `ERROR` when it fails.
- **A recording stays a recording through resets.** Returning to the
  custom menu, loading a Training snapshot, rematching, or starting
  another ROM session must not silently end capture.
- **Recording completion is explicit.** `SEG` + `DONE` records
  commit individual native input segments. `FINAL` seals the
  complete recording, including frame/session/segment totals and stop
  reason. Normal application Quit auto-finalizes active F9 capture.
- **Unfinished evidence is still evidence.** Old `XM8761R1`
  recordings and complete-segment v2 manifests without `FINAL`
  remain readable without being mislabeled as finished.
- **Fast-forward does not skip emulator frames.** It changes rendering
  cadence, not host events, CPS2 CPU timing, or DHC inputs.
- **Preserve the working machine.** Never reset the user's warm build
  tree, delete original recordings, replace verified DHC behavior,
  or overwrite the Stable executable because a TEST build fails.

## The three permanent launchers

`BUILD_TEST.bat` — build only the CURRENT TEST using the existing
incremental MSYS2 toolchain. Complete compiler/linker transcripts
append to `test/build_logs/BUILD_TEST.log`.

`RUN_TEST.bat` — start the real game and record with **F9**.
New recordings live under `test/87.###/replays/session_XXXX/`
with a permanent sibling `F9_status.log`.

`RUN_REPLAY.bat` — run the latest sealed recording, or an
explicitly selected legacy/unsealed session, with adjustable
high-speed replay (default 32 emulated frames per presentation batch).
It retains discovery of older TEST-version recordings.

The launcher filenames never change; version numbers belong inside
the scripts and diagnostics, not in new launcher names.

## Handoff and verification

- [Recording, replay, and fast-forward instructions](REPLAY.md)
- [Authoritative schema, lifecycle, integrity and compatibility rules](REPLAY_DATA_MODEL_MANIFESTO.md)
- [Original v2 architecture and remaining limitations](dev/dhc/REPLAY_V2_HOST_TRANSITIONS.md)
- [Native C++11 replay round-trip and regression tests](dev/dhc/tests/test_dhc_host_replay_164.cpp)

The real uploaded TEST 87.166 capture contained **3,407 frames in eight
complete segments**, with matching native checkpoints, but no recording
seal or latest pointer. It is the concrete regression case for normal
application-exit finalization and lossless unsealed-prefix replay.

**Passing isolated tests is not a claim that native Windows gameplay
has already passed.** When a build or playback fails, preserve its
diagnostic files and repair the current source in place.
