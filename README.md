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

## Ryu Stage visibility audit — TEST 87.324

Ryu Stage uses custom host stage ID **6**, backed by ROM-native stage 05.
Like Sigma and Akuma, adding a menu row or renderer alone is insufficient.
The six original PSD-derived PNG layers and coordinate manifest are
tracked under `_art/ryustage/extracted/` on the **same branch** as
`_art/ryustage/ryustage.psd` and `audio/music/ryustage.ogg`.
The extractor has cached source/extractor SHA256 markers; an unchanged
PSD should never cause PNG rewrites or a binary build-cache invalidation.
Original source bounds may be outside 768×471; no pixels are clipped
during extraction.

**All three required native gates admit ID 6:** the camera observer in
`xmvsf_stage86_49_camera.h`, the CPS2 OBJ capture whitelist and the
native palette exception in `cps_draw.cpp`. An SDL painter that
requires `xm77SpriteValid` cannot display custom art until those gates
are populated. Read `RYU_GATE`, `RYU_READY`, `RYU_ASSETS_MISSING`
and `RYU_DRAW_FAILED` in the live TEST log; never diagnose a blank
custom stage by rewriting parallax coordinates first.

## Ryu Stage 87.325 — PNG ground and native roof parallax

The user's new `_art/ryustage/ryustage.png` is now the **only Ground**.
Its original **768×471 RGBA** is loaded without scaling, recropping or
regenerating it from Photoshop. The 384×224 native view begins centered
at X=192 of the PNG when the real stage-05 ROM camera X=296. Ground
follows the native camera X **one authored pixel per camera pixel**,
bounded by the 768px image's full width. At camera Y=256 its bottom
source row is anchored to viewport Y=224; during super-jumps the native
vertical camera moves the floor down/off-screen. Neither player nor
combat collision/camera registers are modified.

The current 768×471 PSD has exactly five named parallax roles:
Sky Bottom, Sky Top, Roof3, Roof2, and Roof. All retain original
Photoshop dimensions and X/Y coordinates, including overflow beyond
the PSD canvas. Its unnamed opaque Layer 2 construction raster does
**not** render or cover the skies. The obsolete prior PSD Ground PNG
remains historical generated data and is not used. The extraction
manifest records only the five active roles; the edited Roof2
texture was regenerated while the four unchanged named textures
were retained. PSD/extractor SHA warm caching remains enabled.

**Render order, back to front:** Sky Top -> Sky Bottom -> Roof3 ->
Roof2 -> Roof -> `ryustage.png` Ground -> original HUD ->
native fighter/FX and flashes. Ground is above *all* parallax,
not underneath any roof.

All X rates use the ROM camera displacement relative to Ground at
100%: Roof **45%**, Roof2 **27%**, Roof3 **13%**, so the foremost
Roof moves visibly yet slowly as the native camera scrolls. Roof3
tracks Ground's Y at **100%**; Roof2 moves at **75%** Y and Roof
at **50%** Y, creating visible relative vertical motion as the
camera rises. Sky Top and Bottom tile left/right respectively and
retain Ground-locked vertical position.

On Windows TEST 87.325, use the established `RYU_GATE` and
`RYU_READY` logs plus `RYU_GROUND_PNG_MISSING`,
`RYU_GROUND_BOUNDS_BAD`, `RYU_CAMERA`, and
`RYU_DRAW_FAILED` to confirm the image and native camera.
The mandatory camera/OBJ-capture/palette stage ID 6 gates remain
unchanged. Actual gameplay validation requires local
`BUILD_TEST.bat` and `RUN_TEST.bat`; a source check is not a
claim that the Windows renderer was run.

## Ryu Stage 87.326 — corner-accurate camera and uniform Y motion

The user's TEST 87.325 capture contains a complete native FBNeo
`FB1 FS1` save state, inputs, and F10 fighter-location observations.
The checkpoint's CPS2 `CpsRamFF` bytes at `FF524C..FF524D`
are **FF C0** (signed -64). The established native stage-05 camera
observer adds **64**, so the **real camera X is 0** at the left arena
corner. The F10 data independently shows fighter X ranges extending
near both ends of the 768-pixel native world (X=21 through X=749).
These are native ROM observations, not a new camera approximation.

87.325 incorrectly used:
`groundCrop = clamp(192 + (nativeCameraX - 296), 0, 384)`.
This is equivalent to `clamp(nativeCameraX - 104, 0, 384)`.
It held the ground image still as native X traveled **0 through 104**,
then stopped at crop **280** when native X reached **384**. That
accounts for the left-side hitch and the missing right-side corner.

**87.326 uses exactly**
`groundCrop = clamp(nativeCameraX, 0, 384)`.
It maps left camera X=0 to source crop 0, center X=296 to crop 296,
and right camera X=384 to source crop 384. The `ryustage.png`
ground remains the original 768×471 image without scaling or
reindexing, and native fighter/camera/collision RAM is unchanged.

**No vertical parallax whatsoever.** Ground, both skies, and all
three Roof layers share the same native Y shift:
`224 - 471 + layerSourceY + (256 - nativeCameraY)`.
Roof3 is no longer the only fully ground-locked layer; Roof and Roof2
now also move at 100% native camera Y. Horizontal rates remain Roof
45%, Roof2 27%, Roof3 13%, and both skies keep their opposite looping
motions. Horizontal parallax follows **unclamped nativeCameraX**
relative to the previously established X=296 art-registration point;
it does not freeze when Ground reaches a source-image edge.
The unchanged render order remains skies -> Roof3 -> Roof2 -> Roof ->
PNG Ground -> native HUD/fighter layers.

Only the existing local `BUILD_TEST.bat` and `RUN_TEST.bat` are
used for native validation. Source math and original checkpoint were
inspected; actual Windows TEST 87.326 playback is not claimed.

## Tournament air-combo medium velocity protection — TEST 87.327

**Evidence:** the user's original `87.326.zip` contained tournament mode
enabled (`tournament_rules.tsv`), plus 2,640 records from the
`storm_like_flight_events.tsv` native post-CPU hit/velocity observer.
It recorded **29** Tournament host writes to victim velocity:
**17** from Ryu's aerial MK (`1A02`, confirmed physical button 16),
**6** from a light air attack (`1800`, button 1), and only **6**
from a confirmed HK (`1A04`, button 32). At native frame 6092,
Ryu's connected MK entered `FS_STORM_HIT_CONFIRMED_PENDING`;
at frame 6101, the same pending hit wrote vertical velocity from
`FFF97D00` (upward) to `0000B000` (downward). This is an observed
host modification, not just normal ROM gravity.

**Root cause:** `xmvsf_stage87_86_storm_button_guard.h` registered
EVERY normal air/ground hit during an ongoing flying-screen phase as
eligible for delayed Storm knockdown. Its earlier heavy-only/medium
exclusions were bypassed entirely by the `fsConditionalPending`
path. Shared animation codes additionally made action-only heavy
classification unsafe.

**87.327 fix:** queue a new Tournament knockdown ONLY for a confirmed
native normal HP/HK: an actual single-button input edge captured at
the native hit onset **and** an airborne or grounded heavy action.
A retained button/action identity is checked again at the deferred
native substate-2 velocity writer. The separate legacy severe-finisher
writer now requires identical confirmed heavy identity both at latch
and write time. Ambiguous input fails closed. Light and medium attacks
continue to update the existing connected-hit combo ledger (including
breaking HP->HK chains), but NEVER receive host velocity, knockback,
animation-pair, or flying-screen overrides.

An offline dry-run of the 87.326 recorded hit classifications shows
**59** previously eligible FS hit confirmations: **26** light air hits,
**26** MK hits, **1** grounded MP hit, **6** verified HK hits. With the
new strong-only predicate, **53** undesired candidates are suppressed
and the **6** HK candidates remain eligible. This is a source/recording
analysis, **not** a claim of a native Windows replay pass. No Stable
changes, new GitHub Actions, or added build gates.

For native verification, launch TEST 87.327 with Tournament ON and
inspect the existing `storm_like_flight_events.tsv`: any event with
`velocity_written=1` must originate from a previously confirmed
HP/HK normal with matching native action; MK/MP/LP/LK must remain
native, including during active FS. The deferred event now reports
the originating HP/HK tag, not the unrelated buttons held on the
later native substate-2 frame. Compare against Tournament OFF
and preserve original F9 recordings.

## Verified versus-screen M68K checkpoint — TEST 87.324

The original uploaded **87.320** F9 recording starts with a
358,352-byte FBNeo `FB1 FS1` checkpoint, followed by **148 native
input frames**. Parsing the `CpsRamFF` save-state block with the
CPS2 RAM byte-swap shows `FF5104=00`, `FF5105=04`,
so the authentic big-endian 68000 game-step **word is 0004**.
At that exact checkpoint `FF5000=00`, `FF5180=07` and
`FF51A0=03` (P1 done, P2 selecting).
The old verse cue read `FF5104` as one byte and got 00, so the
`step==4` detector could not arm. The corrected detector reads the
full word; there is no hard-coded half-second shift or late status-only
fallback.

`RUN_REPLAY.bat` with the original 87.320 F9 session emits
`test/87.324/replay_results/versus_m68k_handoff.tsv` containing
the exact movie frame, native frame, 68000 PC/PPC, ROM opcode,
game-step word, selection records, phase and stage. Only that
native trace can confirm the exact SCREEN handoff frame at runtime;
a checkpoint alone cannot reveal the next 148 executed M68K states.
Keep the original recording untouched. No GitHub builds or extra
build gates are introduced.

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

## Akuma Stage A/B visual contract (TEST 87.319)

**Six original raster assets**, all under `_art/sf3akuma/`, are used directly:

| Layer / depth | A, initially visible | B, revealed below A |
|---|---|---|
| 1. Stage (front of scenery) | `sf3akuma1_a1.png` | `sf3akuma1_b.png` |
| 2. Moon | `sf3akuma1_a2.png` | `sf3akuma1_b2.png` |
| 3. Clouds (farthest) | `sf3akuma1_a3.png` | `sf3akuma1_b3.png` |

Stage images retain their **768-pixel original world width**. Their
height may now exceed 471 pixels to add sky above the frame (committed A1
is 768×512 and B1 is 768×526). **Anchor
the ACTUAL bottom pixel of each A1/B stage texture at native ground
camera Y=256**, so added upper rows extend UP, never shift the floor
or get resized/cropped; the previous 471-pixel sky could leave a 9px
gap when native camera Y reaches zero. The updated moon A2/B2 source is
**768×471** for both textures, and also aligns by its bottom edge; the
renderer accepts old moon dimensions without stretching. The cloud
A3/B3 images keep their independent original dimensions and vertical
center relative to the baseline 768×471 world. The moon uses
55% horizontal parallax with an additional **280-pixel LEFT** world
offset, placing its disc at the left side of the viewport when
fighters/camera are centered. The ground stays at 100% native camera
speed. Both A/B moon phases retain the same displacement.

Render A3/B3 clouds behind the moon and ground at **50% presentation
scale**, centered within the 768×471 world and translated **down by
30% of 471 pixels**. The original source PNGs remain untouched.
Repeat them horizontally with a steady **rightward 2px per 3 emulated
frames (40px/sec)** and 30% native horizontal camera parallax.
Apply the same slow 2% sinusoidal size breathing to both cloud phases,
with seamless tiles recalculated at their current display widths.
Both phases retain the native camera's vertical motion. Never change
source padding, remap fighters or use stage art as camera origin.

Add a subtle 1-native-pixel horizontal foliage wiggle using narrow
24-pixel source strips with staggered sine phases. Precompute separate
alpha masks for green leaves (upper canopy) and green grass (bottom
rows). Overlay the animated masks only for the ground A/B pairs; do
not move rocks, trunks, floor geometry, camera, or character placement.
Rebuild the foliage mask pixels only on dither-step changes, not each
rendered frame.

During the 2700-frame A→B ordered-dither interval, draw B underneath
the corresponding A image at the same depth. **B starts VERY GREEN**
at first reveal, then returns gradually to its **normal original
red/orange authored colors** as A disappears. Do not darken B to 60%
or apply global texture brightness modulation.

Compute a strongly green-shifted RGB image at load, blending B's
luminance-derived green grade with the corresponding A pixels in
WORLD coordinates. For ground/moon, align world positions by their
bottom pixel; for differing cloud sizes, wrap horizontal pixel sampling.
When A is transparent, still derive a strong green tone from B itself:
otherwise red sky or foliage pops through the first dither holes.
Never recolor source PNGs or alter B's original alpha. Interpolate from
this green grade to unmodified B RGB across the same 64 ordered-dither
steps, reaching the true authored B at completion. Update streaming
textures only when steps change; keep native fighter/FX and stun HUD
above all scenery layers.

**Transition is time-based, not KO-based.** Start the clock at first native
combat phase 08. Keep A entirely visible for the first **600 emulated
frames** (10 seconds at 60 fps). Then reveal each B layer **behind its
matching A at the same Z depth**, and remove A over the next **2700
emulated frames** (approximately 45 seconds), finishing at combat frame
3300. Use world-anchored **ordered alpha dithering**, not crossfade or
an instant swap. Keep depth order clouds → moon → stage → native stun HUD
→ native fighter/FX, so the two stages cannot overdraw fighters during
the transition. Renderer must not depend on first character death,
team KO, or damage meter state; timer pauses with the emulation.

The authored soundtrack is **`audio/music/sf3akumastage.wav`**,
committed in the concurrent `audio update.` revision. The existing
low-priority WASAPI worker loads it using the native 48k PCM WAV reader
(with supported 16-bit conversion). Common Akuma OGG filenames are
optional fallbacks. Akuma scenery is independent of audio load state;
a missing/unsupported track logs `AKUMA_MUSIC_PRELOAD MISSING`.
Never silently substitute FD, Sigma, or Guile music.

To avoid the earlier Guile and Sigma culling regressions, native stage
05 admission for Akuma ID **5** must be added consistently to the
stage picker, selector, camera observer, **CPS2 fighter/FX capture gate**,
palette exception, SDL compositor, native BGM suppression, and WASAPI
stage audio. Preserve all other custom stages and Stable. Verify actual
visual display and audio on the local Windows TEST build before claiming
live verification.

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
