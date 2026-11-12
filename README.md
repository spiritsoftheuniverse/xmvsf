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

## Custom stage visibility and asset-delivery requirement (Sigma / Guile)

**Do not declare a new custom stage ready just because its menu option,
renderer and music code have been committed.** Before reporting success:

1. Verify the actual PNG and OGG exist **in the same remote branch as the
   stage code** using GitHub's tree, not just the local working directory.
   Files left untracked, hidden by GitHub Desktop filters, ignored by a local
   exclude rule or pushed to another branch will not appear in a fresh TEST
   checkout. A missing PNG necessarily results in the original native stage
   appearing; it is **not a successful art integration**.
2. Sigma's preferred source is `_art/sigmastage.png`, with its music at
   `audio/music/sigmastage.ogg`. The Sigma renderer also accepts common
   sigma filename variants and discovers Sigma-named PNGs or a PNG within a
   Sigma-named subdirectory of `_art/`. Do not rename or rescale source art
   just to make the loader work. Guile's distinct PSD-extraction contract
   remains `_art/guilestage/extracted/manifest.tsv` with the original
   source in `_art/guilestage/guilestage.psd`.
3. If a custom stage shows native scenery, inspect the **actual runtime**
   diagnostic log before changing camera math. Sigma logs
   `SIGMA_GATE`, `SIGMA_ART_MISSING`, `SIGMA_READY` and
   `SIGMA_DRAW_FAILED`. The gate exposes selected/resolved IDs, native
   stage 05 activation and sprite capture. An asset-missing error means
   the source file was not found; native scene fallback is intentional.
   For Guile, inspect `GUILE_ASSETS_MISSING` and
   `GUILE_FIGHTER_UPLOAD_FAILED` before altering its camera.
4. In PowerShell on the Windows checkout, verify the files and their
   committed tracking status (both should return an entry):

   ```powershell
   Get-ChildItem F:\xmvsf\_art -Recurse -File | Where-Object Name -Match 'sigma'
   Get-ChildItem F:\xmvsf\audio\music -File | Where-Object Name -Match 'sigma'
   git -C F:\xmvsf ls-files _art audio/music | Select-String 'sigma'
   ```

5. Render smoke-test **the actual local Windows TEST build** in Training
   and Versus: stage art (not native fallback), correct native camera X/Y,
   stage audio, source-color pulse, HUD/fighter ordering, and returning to
   the menu. Do not infer display success from static source checks or
   from a GitHub commit. Use only the existing local build/test scripts.

**Confirmed Sigma native culling root cause (TEST 87.316):** the CPS2 draw
path in `dev/stable/buildtree/src/burn/drv/capcom/cps_draw.cpp` still
whitelisted ONLY IDs 1=Wily, 2=Final Destination, and 3=Guile for
`xm77CaptureSprites()`. Sigma is ID 4, so `xm77SpriteValid` was
unconditionally cleared to zero and `XMSigma::paint()` refused to draw
even when `_art/sigmastage/sigmastage.png` existed and loaded correctly.
The same missing ID in `xm8657BeginNativeGrade()` omitted Sigma from
the background palette preservation exception. **When adding a custom
stage, update the CPS2 native capture whitelist AND native palette
exception, alongside the stage picker, early stage05 selector,
camera observer, WASAPI preload and SDL painter.** A loader-only fix
cannot overcome missing fighter/FX capture. Verify a running TEST build
before calling either stage visible.

**Historical failure:** Guile Stage and then Sigma Stage were selectable
while their authored scenery did not display. The policy is to first
verify the committed asset, runtime lookup path, and native sprite/camera
gate. No stage-specific workaround may replace the game's ROM camera or
reset verified combat behavior to hide a missing-asset problem.

## Repository tracking and local TEST retention

Git is **source control, not the build archive**. The repository deliberately
tracks authored artwork/audio (including Sigma and Final Destination FDB video),
native C++ sources, Wily runtime assets, and the protected Stable build. It
must not track the numbered `runtime/stable/XMVSF_Test_87_*.exe` outputs,
`dev/stable/buildtree/fbneo.exe`, generated hitbox/diagnostic logs, or
`local_rom_dump/` decrypted CPS2 bytes. They are build or machine output,
not new source. Existing replay evidence under `test/` remains in Git for
this cleanup; the test directory is ignored for new locally generated sessions.
Do not remove source files or replay captures as part of Git performance work.

**Pull safety:** The cleanup commit removes previously tracked binaries from
the Git tree. A normal pull may **delete those tracked files from the local
working checkout**. To retain earlier TEST executables, back them up to a
directory outside `F:\xmvsf` BEFORE pulling the cleanup commit, e.g.:

```powershell
$archive = 'F:\xmvsf_local_test_archive'
New-Item -ItemType Directory -Force -Path $archive | Out-Null
Copy-Item 'F:\xmvsf\runtime\stable\XMVSF_Test_87_*.exe' $archive
```

Only numbered TEST builds are removed from version control; **do not delete
or overwrite `runtime/stable/XMVSF_Stable.exe`**. Running
`BUILD_TEST.bat` regenerates the current TEST executable locally using
the warm build cache. Keeping history as-is means older Git objects remain
reachable; this is **not** a destructive rewrite of branches, Git history,
the Stable build, or past replay recordings.

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
