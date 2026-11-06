# Stage 87.203: live stage-capture status in your existing MSYS2 terminal

Run in the ALREADY OPEN MSYS2 UCRT64 window:

    cd /f/xmvsf
    bash dev/stage_capture/WATCH_CAPTURE.sh

The watcher follows test/87.203/stage_capture/status.log, and accepts an alternate path as its first argument. It opens no new window, does not change game inputs, and does not alter Stable or GitHub Actions. Ctrl+C stops the watcher without stopping the game. NO_COLOR=1 disables terminal colors.

PART_DONE is blue (one independently verified part); LEVEL_DONE green (the entire level independently certified); INCOMPLETE yellow; ERROR red. CAPTURE_START and PROGRESS are neutral. The saved file stays plain UTF-8 with explicit event names.

A separate Windows game process cannot inject output into an arbitrary pre-existing MSYS window without a receiver. This follower is the receiver; it can run while the game is launched elsewhere.

The offline checker is:

    python dev/stage_capture/VERIFY_CAPTURE_STATUS.py --root test/87.203/stage_capture

It checks a stage builder's capture_manifest.json and SHA-256 artifact receipts. Five parts are required: scroll1, scroll2, scroll3, sprites, animation. Green also requires capture_closed=true, coverage_complete=true, replay_sealed=true, unresolved_gaps=[]. The builder must independently prove coverage and animation loops. The checker cannot infer unseen tiles or untriggered animations from artwork, and must never fabricate green.

The read-only native recorder is now enabled by default in RUN_TEST.bat ON THIS CAPTURE BRANCH only (XM87203_STAGE_CAPTURE_ENABLE=0 disables it). RUN_REPLAY.bat leaves capture disabled unless explicitly enabled. The new TEST binary is XMVSF_Test_87_203.exe; existing Stable is untouched.
Native files: frames.tsv, raster.bin (per-slice register snapshots), maps.bin (changing 0x4000 scroll banks), row_scroll.bin (per-row scroll banks), palettes.bin (raw palette RAM changes), oam.bin (buffered original objects). Status shows STAGE_BEGIN and PROGRESS every 300 frames. It never emits PART_DONE/LEVEL_DONE until the offline verifier has certified built artifacts, sealed F9 replay and camera/animation coverage. Final stage reconstruction, decoded tile pixels and effective palette DMA validation are next steps; this is a read-only capture milestone, not a completed automatic builder. Files under test/ remain local and git-ignored.

## Correct source ROM — Marvel vs. Capcom 1, not XMVSF

The Stage44 Wily donor exporter already proved the exact FBNeo driver `mvscur1`.
Stage87.203 reads **`mvscur1`** by default via `RUN_TEST.bat`; optional
`RUN_TEST.bat mvsc` selects the base Marvel vs. Capcom ROM. The ROM must
exist locally in FBNeo's configured ROM search paths; no ROM files are
committed or copied by this tool. `RUN_TEST.bat xmvsfu` explicitly enters
the separate XMVSF/DHC TEST path, with stage capture disabled by default.
The runner does not launch XMVSF when MVC1 ROM loading fails.

Recorder eligibility is the native CPS2 driver prefix `mvsc`, not `xmvsf`.
The old XMVSF FF5001/FF5112 phase/stage offsets have been **removed** from
capture gating because they are not verified MVC1 addresses. Hardware
snapshots start with the MVC1 driver and preserve raw scroll registers,
raster data, tilemaps, palette and OAM. Source stage IDs remain marked
**unverified** until the MVC1 CPU stage selector is proved from executed
code or matching memory traces. Do not report a complete level based on
an XMVSF memory address or on recording simply stopping.

The original Stage44 `XM44_CAPTURE` donor exporter remains disabled by
default to prevent duplicate huge pixel exports. The Wily compositor
remains the *destination* engine, not the donor source. Full animation
reconstruction is a separate verified post-capture step.

## Original ROM artwork

The capture branch also saves a **local-only immutable** image of the
emulator-decoded CPS2 `CpsGfx` data for the MVC1 ROM, in a hash-named
`mvc1_decoded_gfx_*.bin` file. `gfx_index.tsv` records its source driver,
decoded length, mask and Scroll1/2/3 tile offsets. This gives the offline
builder actual decoded 4-bit pixel sources for scroll tiles and sprite
compositions, rather than relying solely on tile IDs and screen pixels.

The export is attempted once per source run, capped at 256 MiB, and
does not alter any ROM, GFX, palette, or actor memory. Existing hash-named
files are never truncated. Set `XM87203_EXPORT_GFX=0` to disable it (which
must keep art coverage incomplete). Both artifacts remain under git-ignored
`test/`; do not upload the decoded original ROM artwork to GitHub.

## Donor execution isolation

When RUN_TEST.bat runs `mvsc*`, it disables the preexisting experimental
XMVSF DHC, tag, tournament-flight and Wily replacement-renderer switches
inside that donor process. Original MVC1 inputs and emulation run through the
normal FBNeo CPS2 driver; only source capture/replay flags remain. MVC F9
replays do the same. The earlier XMVSF DHC tests stay available by launching
`RUN_TEST.bat xmvsfu`.

## Missing donor ROMs and nonzero exit codes

If FBNeo says `Loading program (mvce.03a)... (not found)` while
launching `RUN_TEST.bat mvsc`, the **Europe `mvsc` parent set** failed
loading. This is neither a native build failure nor a stage-capture error.
Per this repository's `d_cps2.cpp`, `mvsc` requires `mvce.03a` (0x80000
bytes, CRC32 `824e4a90`) while the original Wily donor `mvscur1`
requires `mvcu.03` (0x80000, CRC32 `13f2be57`). `mvscur1` is
declared a `mvsc` clone and may also need shared data from its parent.
The ROM archives must contain the correct files in FBNeo's configured
search paths; the launcher must never invent files or silently fall back.

The donor `DrvInit` failure is now propagated as a **nonzero** native
process/TEST exit status. `RUN_TEST.bat` reports the missing-ROM cause
with an `ERROR` event in the existing MSYS watcher and skips irrelevant
XMVSF DHC observers/disassembly. A failed ROM cannot produce native
layer coverage or any `PART_DONE`/`LEVEL_DONE` status.

## 87.203 MVC donor tilemap byte-order correction (from real 24,058-frame capture)

Actual `stage_capture.zip` recorded valid little-endian CPS2 register bytes: register offsets 0x02, 0x04, 0x06 contain 0x9080, 0x90C0, 0x9100 and therefore map to `0x908000`, `0x90C000`, and `0x910000` after shifting. The prior `be16` helper incorrectly decoded them outside CPS2 graphics RAM, producing a **zero-byte maps.bin** even after 24,058 frames. We now read native registers with `le16` consistently for tilemap addresses, camera columns, row-scroll enable and row base. The new regression asserts original recorded register bytes, and the runtime stops after five consecutive unmapped graphics-bank frames or 180 frames without any map snapshots. It NEVER reports level completion from those checks alone. `row_scroll.bin` was correctly empty: MVC register 0x22 was `0x000e` (row-scroll bit 0 clear) throughout the actual capture. The earlier archived camera/ OAM / palette / decoded-gfx evidence remains useful; a new play sweep is needed for missing tiles.

## Immutable runs and MSYS root log

Each new 87.203 donor run now creates a **new `session_XXXX/` directory** under `test/87.203/stage_capture/`; never append new frame/binary streams to an earlier sweep. `latest.txt` identifies the newest session. The existing `stage_capture/status.log` remains the root MSYS watcher target, including its historical lines. The offline checker resolves `latest.txt`, validates artifacts in that session directory and posts blue `PART_DONE` and green `LEVEL_DONE` into the common root status with the session ID. If there is no certified manifest, it stays incomplete. Existing root-level capture files remain unchanged and useful for earlier camera/OAM/palette recovery.

### 87.203 build smoke-test crash fix

The 2026-11-06 build stopped before native compilation when `test_native_stage_capture_203.exe` raised `std::out_of_range: basic_string::substr` (position 46 beyond a 13-character path). The session creator accidentally used `sessionDir+"/"+name` while `sessionDir` was empty. The correct path is `root+"/"+name`. The writer now validates the root prefix before extracting `latest.txt`, and the smoke test asserts that the directory lives under the configured capture root and that two runs never reuse the same session directory. No Stable binaries, warm objects or old recordings are deleted.

## Offline MVC scene reconstruction: Stage 75/76 Wily model

The strongest old reconstruction implementation is the Stage75/76 Wily
SDL2 importer/painter in src/intf/video/sdl/xmvsf_stage76_wily.h, together
with runtime/stable/assets/xmvsf/wily75/manifest.tsv. The original
Stage44/47 exporter captures donor pixels but is not a complete scene
reconstruction model. Stage75/76 has independent layered XRGA textures,
animation states and actor timelines, so it is the reference foundation.
Its Wily-specific camera bounds, factory stops, and 28/45-frame clocks
are never copied into another stage.

The new standard-library-only Python importer is:

    dev/stage_capture/BUILD_STAGE_FROM_CAPTURE.py

Run the isolated regression locally, without any GitHub Actions:

    cd /d F:\xmvsf
    python dev\stage_capture\test_build_stage_from_capture.py

Build a selected MVC native frame into three independent Wily-format
XRGA texture atlases, retaining the complete tile-change timeline:

    python dev\stage_capture\BUILD_STAGE_FROM_CAPTURE.py --capture "F:\xmvsf\test\87.203\stage_capture" --session latest --frame 18000 --output "F:\xmvsf\test\87.203\reconstruction_18000"

A ZIP can also be supplied as --capture with --session session_0002.
Always choose a NEW --output directory: existing results are never
overwritten, and no source ROM / gfx is committed.

Generated output:

- layer1_*.xrga (512x512, native 8px tiles)
- layer2_*.xrga (1024x1024, native 16px tiles)
- layer3_*.xrga (2048x2048, native 32px tiles)
- map_snapshots.tsv: every map snapshot's frame/bank/native hash
- cell_changes.tsv.gz: all observed per-cell tile, palette, flip changes
- animation_candidates.tsv: observed changed cells, not verified loops
- stage76_import_manifest.tsv: Wily-style atlas/size records for generic
  import; not yet compatible with Stage75's hard-coded scene contract
- stage_manifest.json: provenance, asset hashes, verification gaps

The old compositor and the live native gameplay code are not modified.
This is an offline atlas and observation builder, not a complete automatic
stage installation. It does not claim any blue PART_DONE or green LEVEL_DONE
until independently verified. Stage ID, actual camera world bounds,
raster priority, true CpsPal effective colors, sealed F9 replay, full
actor classification and animation cycle completion remain unverified.
Only the source captured tilemap pages, not a guessed stage panorama,
are drawn; visual preview colors need comparison with native frames.


## Camera-aligned MVC verification pass (frame 18000)

The Stage75/76 importer now retains BOTH native page atlases and a distinct
camera-facing diagnostic. This is based on the user-supplied reconstruction
and the corresponding 36,000-frame mvscur1 donor capture. The original archive
had valid XRGA pages, but full 64-by-64 tilemap pages include unoccupied and
recycled RAM; they are NOT stage vistas.

Run the existing source-only regressions:

    python dev/stage_capture/test_build_stage_from_capture.py

Import the recorded source again into a NEW directory, leaving old runs,
Stable, and the donor archive untouched:

    python dev/stage_capture/BUILD_STAGE_FROM_CAPTURE.py --capture F:/xmvsf/test/87.203/stage_capture --session latest --frame 18000 --output F:/xmvsf/test/87.203/reconstruction_camera_18000

The resulting artifacts include:

- camera_timeline.tsv.gz: all native frames and raster slices; original
  per-layer raw hardware scroll, map-bank and control-register words, without
  invented parallax ratios or world bounds.
- preview_scroll1/2/3_frame018000.png: transparent 384-by-224 previews of
  the camera-aligned portion of each independently decoded tilemap.
- preview_approx_composite_frame018000.png: an explicitly APPROXIMATE
  three-layer composite in 3->2->1 draw order, NOT a native pixel match.
- All original full-page XRGA atlases, cell_changes.tsv.gz, candidates,
  and manifest files from the earlier importer.

The PNG writer needs only the Python standard library. The camera viewport
respects tilemap-page wrapping but does NOT yet apply verified CPS2 global
offset corrections, hardware layer-priority masks, raster-band slices,
row-scroll, or effective RGB palette DMA state. Those remain unresolved gaps,
as do exact stage identity, boundaries, and actor ownership. All 13,300
changed-cell animation candidates are UNVERIFIED: stage transitions and bulk
tilemap repaints can produce false candidates. A sample scene preview must
NEVER authorize blue PART_DONE or green LEVEL_DONE.

Neither this importer nor its tests modify the live Wily renderer or create
GitHub Actions. Output is local only and original ROM graphics are not
redistributed through source control.
