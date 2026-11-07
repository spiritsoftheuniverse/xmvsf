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


## CPS2 clean-render-pass ripping (the WinKawaks/Shot Factory approach)

Rather than rebuilding every tile and palette by hand, REUSE the original
Stage44/45 native CPS2 renderer. It already renders through the original
Cps2Layers drawing path four times, with sprite drawing suppressed:

- layer 0: scroll1/2/3 combined by native hardware priority, NO fighters/HUD
- layer 1: native 8x8 Scroll1 alone
- layer 2: native 16x16 Scroll2 alone
- layer 3: native 32x32 Scroll3 alone

Each layer is rendered twice over different background colors to distinguish
true opaque black artwork from untouched transparent space. Artwork stays
true to the source ROM without chroma-key errors, guessed palettes or
manually tiled panoramas. This matches the documented classic WinKawaks
Shot Factory method in principle. It does NOT establish precisely how
The Fighters Generation made its particular GIFs.

To opt in, from the project root use ONLY the existing launchers:

    BUILD_TEST.bat
    RUN_TEST.bat mvscur1 clean

Play the ORIGINAL Marvel vs Capcom ROM until the DESIRED stage appears.
While the game remains open, in a second Windows CMD prompt:

    type nul > "F:\xmvsf\test\87.203\clean_rips\START.flag"

The default burst samples 360 frames at two emulated frames per sample.
XM44_FRAMES and XM44_STEP can be set before RUN_TEST.bat to alter that,
up to a hard limit of 1200 samples. After CLEAN_RIP_BURST_DONE, create
another START.flag for the next independent shot_XXXX burst, WITHOUT
restarting the donor game. The trigger is moved into each shot directory
rather than deleted; every output directory is unique, so no old rip or
Stable artifact is overwritten.

The existing open MSYS watcher reports neutral CLEAN_RIP_ARMED,
CLEAN_RIP_START and CLEAN_RIP_BURST_DONE records via the same
stage_capture/status.log. The watcher reports ERROR in red. These are
only CAPTURE events, NOT green whole-stage or blue verified part status.

Each shot directory contains layers.tsv and native unfiltered RGBA for
scrolls 0, 1, 2, 3, along with original sprite list, tile and palette
evidence. The source frame, source scroll coordinates and native FNV64
of every image are retained. CAPTURE_COMPLETE.txt means burst complete,
NOT entire stage complete.

Use Python standard library to produce lossless, transparent PNGs without
modifying the original bytes or a preexisting output folder:

    python dev\stage_capture\test_export_clean_rip.py
    python dev\stage_capture\EXPORT_CLEAN_RIP.py --input "F:\xmvsf\test\87.203\clean_rips\shot_0001" --output "F:\xmvsf\test\87.203\clean_rips_png\shot_0001"

The exporter checks path safety, hashes, exact sizes, chronological frames,
and four complete layer records per sample. It deduplicates repeated PNG
artwork while preserving every timestamp in clean_rip_manifest.json and
timeline.tsv. Raw capture remains intact, entirely local, outside Git.

Limitations: stage actors drawn as sprites do NOT appear in the isolated
scroll composite; identify them separately from the original native OBJ list,
avoiding fighters, HUD and projectiles. World coverage, stage transitions,
true animation periods and verified scenery-OBJ identity are not established
by a clean screenshot, so no automatic PART_DONE or LEVEL_DONE is allowed.
Use 87.203 telemetry to guide frame selection, gap analysis and camera
alignment among multiple clean native bursts.


## 87.203 continuous automatic batches (no terminal frame stop)

Capture via the existing root launchers only:

    BUILD_TEST.bat
    RUN_TEST.bat mvscur1 clean

This now starts clean, lossless Stage44/45 MVC1 background rendering
IMMEDIATELY and AUTOMATICALLY. No second terminal, repeated START.flag,
or user-triggered bursts are required. Each auto burst is held in an
exclusive clean_rips/shot_XXXX directory, with AUTOMATIC_BATCH.txt,
source layers.tsv, pixel-exact 0/1/2/3 .rgba streams, effective palettes,
sprite tiles and metadata, and CAPTURE_COMPLETE.txt upon closure.

Defaults for this opt-in "clean" mode:
- XM44_AUTO=1: automatically begin the first and every next clean-rip burst
- XM44_FRAMES=60: limit *each individual disk batch* to 60 layer samples
- XM44_STEP=30: sample one emulator render every 30 frames (~2 per second)
- XM87203_BATCH_FRAMES=12000: rotate the source-native telemetry after
  every 12,000 consecutive emulated frames

The previous 36,000-frame stop in the native CPS2 recorder is gone.
After reaching the per-file threshold, it closes the batch with
BATCH_DONE and opens the next session_XXXX with BATCH_NEXT. Crucially,
the frame exactly at the threshold transition belongs to the new batch,
without loss or duplication, and its first maps, palettes and native
graphics are snapshotted afresh. Frame numbers remain ORIGINAL global
emulator frame numbers, not artificial resets. This allows an indefinite
stream of independent, valid recordings while the MVC1 session runs.

The automatic clean renderer likewise reports CLEAN_RIP_START and
CLEAN_RIP_BURST_DONE and rolls to the next shot_XXXX on the following
native render frame. Neither message proves a complete level; all blue
PART_DONE and green LEVEL_DONE remain gated on separate verification.

To customize the sampling granularity and batch size before launching:

    set XM44_FRAMES=90
    set XM44_STEP=20
    set XM87203_BATCH_FRAMES=6000
    RUN_TEST.bat mvscur1 clean

To restore the former manual start-marker workflow:

    set XM44_AUTO=0
    RUN_TEST.bat mvscur1 clean

These control options apply to the current command shell, NOT Stable,
and the clean ripping engine never modifies MVC gameplay. If no "clean"
argument is passed, the clean pixel exporter remains OFF; raw Stage87.203
telemetry still runs and now rotates automatically.

Each pixel burst is approximately 4 * 384 * 224 * 4 * 60 ~= 79 MiB
before PNG compression, not counting tile/palette metadata. Long
unattended sessions can use many GiB, and clean rendering adds multiple
original CPS2 redraws per sampled frame. Keep adequate free F: space;
exit MVC1 normally to stop. Past sessions, original ROM archives, and
Stable are never truncated or deleted.

Use the standalone source-only regression in the existing Windows build:
BUILD_TEST.bat now exercises a boundary with XM87203_BATCH_FRAMES=300,
so it checks rotation without generating a 12,000-frame test. All
captured original game art remains under ignored test/; do NOT commit it
to GitHub.


## November 6: real 39,282-frame continuous-batch audit

The submitted stage_capture(2).zip contains old independent sessions 0001
(5,509 frames) and 0002 (36,000 frames), then one continuous MVC donor run:
0003 1..12000, 0004 12001..24000, 0005 24001..36000,
0006 36001..39282. All 39,282 new-run native frames are consecutive, each
session's native OAM/raster packets are valid, the ZIP CRC check passed,
all MAP1 addresses and headers were valid, and no capture ERROR was emitted.

Live status reports 3 BATCH_DONE / BATCH_NEXT pairs and 22 completed
CLEAN_RIP_BURST_DONE events (23rd started). The uploaded archive does NOT
include clean_rips/shot_XXXX, so actual clean-pixel frames are NOT validated
by this audit. Reaching a batch limit does not establish stage completion.

To avoid silently reconstructing only the latest 3,282 frames, the
Stage75/76-based importer NOW automatically identifies the maximal
contiguous suffix of session_XXXX batches when --session latest is passed.
It selects exactly sessions 0003 through 0006 for this archive and excludes
old sessions 0001/0002, whose original frame counters overlap this run.
This selection is driven by proven consecutive native frame numbers and
matching source gfx identity. It streams maps/raster/OAM/palette records
without materializing a multi-GiB concatenated blob. Repeated TSV headers
are skipped and the original immutable files remain untouched.

Example with the already uploaded archive or the local source root:

    python dev/stage_capture/BUILD_STAGE_FROM_CAPTURE.py --capture "F:/xmvsf/test/87.203/stage_capture" --session latest --frame 18000 --output "F:/xmvsf/test/87.203/reconstructed_all_batches_18000"

The manifest's source_batches list documents exactly which sessions joined.
Pass --session session_0006 to intentionally restrict the importer to the
final isolated batch; pass --session continuous to require two or more
contiguous batches, failing closed instead of selecting one silently.
It is still a diagnostic tile/camera reconstruction, not a native visual
certification or an installed playable stage.

Additional capture corrections: per-batch decoded graphics are now exported
on the very FIRST rollover frame (not one frame later), and PROGRESS event
buffers are large enough to preserve all counters in the MSYS watcher.
BUILD_TEST.bat regression checks the graphics index exists immediately
after rotating at the test-only 300-frame batch threshold.


## Batch review of REAL native clean-rip frames (Stage87.203)

The uploaded stage_capture(2).zip verified 39,282 consecutive native CPS2
frames but did NOT contain clean_rips/shot_XXXX raw artwork. A CPS2 camera
telemetry archive alone cannot establish that Stage44's isolated render
passes have correct pixels. The clean artwork must be checked on the PC,
without uploading gigabytes of raw RGBA frames.

After pulling this feature branch, the EXISTING launcher runs a quick
read-only review AUTOMATICALLY after the MVC donor exits successfully:

    BUILD_TEST.bat
    RUN_TEST.bat mvscur1 clean

It scans all numbered shots under test/87.203/clean_rips/ and writes to
test/87.203/clean_rips_review/:

- clean_rips_contact_sheet.png: actual native no-fighter MVC background
  composites, first/middle/last selected samples per burst, with checkerboard
  indicating originally transparent pixels; up to 36 representative shots.
- clean_rips_review.json: each shot's count, source frame range, 4 native
  layers, verified hash count, sampled visible-pixel coverage and the row
  index/shot name mapping for the contact sheet.
- clean_rips_review.txt: a small summary with corrupt or missing source
  files highlighted. The review summary and contact-sheet previews can be
  replaced by a later review; original clean-rip RGBA shots remain untouched.
- clean_rips_review_bundle.zip: a compact ZIP containing ONLY the review
  JSON, text and contact-sheet PNG (no source ROM `.rgba` files). Upload
  this one file to the chat for actual-artwork inspection; no multi-GiB
  shot_XXXX upload is necessary for the first visual audit.

Default SAMPLED verification checks the size/presence of EVERY indexed
native RGBA image plus native FNV64 hashes at the first, middle, last
sample of each shot, all four layers. It requires a valid completed burst
count and exact four-layer frames. Unfinished bursts are called INCOMPLETE,
and missing/truncated/hash-mismatched frames produce ERROR. This is NOT
a blanket statement that all raw pixels were verified.

For a FULL integrity review of every file, run manually after closing MVC:

    python dev\stage_capture\test_review_clean_rips.py
    python dev\stage_capture\REVIEW_CLEAN_RIPS.py --root "F:\xmvsf\test\87.203\clean_rips" --output "F:\xmvsf\test\87.203\clean_rips_review" --full --status-log "F:\xmvsf\test\87.203\stage_capture\status.log"

The automatic sampled review does not change RUN_TEST's original ROM exit
code. Its result is appended to the same existing MSYS status.log. It does
not create blue PART_DONE or green LEVEL_DONE merely because screenshots
look plausible. Full level geography, sprites-as-scenery, animation-cycle
coverage, and visual comparison against native MVC frames remain separate
work. No GitHub Actions, added launcher, or Stable modifications.


## OFFLINE review without starting MVC, loading ROMs or rebuilding

The previous automatic review had two accidental gates: ROM return code had
to equal zero AND shot_0001/layers.tsv had to exist. If either failed,
there was no review ZIP and no explanation. That is fixed.

From F:\xmvsf, AFTER pulling the latest capture feature branch, run:

    RUN_TEST.bat review

This is a new OFFLINE mode of the EXISTING root RUN_TEST.bat (no new
version-specific launcher). It branches before any EXE validation or ROM
loading; no BUILD_TEST.bat is needed. Uses only installed Python and
existing files under test/87.203, then creates or refreshes:

    F:\xmvsf\test\87.203\clean_rips_review\clean_rips_review_bundle.zip

If real clean_rips/shot_XXXX image files exist, it validates samples and
writes the actual CPS2 clean artwork contact sheet into this ZIP.
If no clean-rip images are available, it STILL creates a text/JSON
diagnostic ZIP showing the missing root and source capture status event
counts (CLEAN_RIP_START / CLEAN_RIP_BURST_DONE, etc.). It cannot turn a
native telemetry status.log into pixel-exact CPS2 background images, and
must not claim it did. Missing pixel source returns exit code 2, but the
bundle is intentionally still written, with no fabricated contact sheet.
This makes the original failure actionable without entering the game.
An independent ROM crash no longer blocks producing a review.

Equivalent direct Python command without RUN_TEST.bat:

    python dev\stage_capture\REVIEW_CLEAN_RIPS.py --root "F:\xmvsf\test\87.203\clean_rips" --output "F:\xmvsf\test\87.203\clean_rips_review" --status-log "F:\xmvsf\test\87.203\stage_capture\status.log"

The tool never overwrites shot_XXXX, original ROM assets, warm objects,
Stable, prior raw captures, or GitHub history. It only refreshes its own
review JSON/text/ZIP and contact sheet when actual pixel evidence exists.
The regression test explicitly covers an absent clean_rips directory plus
a status.log recording completed bursts, without entering MVC.


## Fix: offline review exceeded the PNG height limit with 16+ shots

On 2026-11-06, RUN_TEST.bat review printed:

    CLEAN_RIP_REVIEW ERROR: invalid unfiltered CPS2 image dimensions
    ERROR: Review bundle could not be created.

This was a REVIEW-OUTPUT bug, not proof of broken original CPS2 pixels.
The contact-sheet renderer wrote every shot into one vertically stacked
PNG. Each row is 136 pixels tall, plus 8 pixels overall; 16 shots make
2184 pixels, beyond the existing lossless PNG helper's 2048-pixel limit.
The user's log already indicated 22 completed clean-rip bursts.

The review now paginates at 12 shots (1640 pixels) per image. Page 1 keeps
the original name clean_rips_contact_sheet.png; subsequent pages are
clean_rips_contact_sheet_002.png, _003.png and so on. ALL pages are
included in clean_rips_review_bundle.zip, in addition to the review JSON
and TXT. The JSON retains its backward-compatible contact_sheet field
and adds contact_sheets with page indices, row offsets, legends and shot
mapping. Per-page bounded rendering continues even when the global
--max-rows selection is 36 (or up to 150). NO original .rgba files are
modified, and no ROM, build or gameplay is needed.

With the updated feature branch pulled, rerun simply:

    cd /d F:\xmvsf
    RUN_TEST.bat review

Expected output for 22 nonempty shots is at least two numbered PNG pages
in test/87.203/clean_rips_review/clean_rips_review_bundle.zip.
The exact number depends on invalid or empty shots and --max-rows.
Source-only regression tests now include an 18-shot paginated ZIP case;
a separate 22-shot synthetic replay of the failure condition also passed
locally. Native Windows testing against the user's actual clean-rip
directory remains outstanding until the regenerated ZIP is inspected.


## Offline CPS2 Sprite Factory: address actual decoded object graphics

The MVC donor already gives us the proper pixel addresses; sprite ripping
does NOT require re-entering the game or keying out a character screenshot.
Stage44/47 stores original native object records (sprites.tsv), source 16x16
packed-4bpp tiles (tiles.bin) and the emulator-effective 512-word 32-bit
sprite palette for each clean sample (palettes.bin). The source nibbles are
decoded with the same low-level layout as ctv_do.h; object width, height,
flips, palette and original tile-address wrap are retained. This yields
independent transparent sprites in their native object dimensions.

Pull feature/stage-87-203-native-layer-recorder and run ONLY the existing
launcher; BUILD_TEST.bat is NOT necessary for offline source processing:

    cd /d F:\xmvsf
    RUN_TEST.bat sprites

This checks test/87.203/clean_rips/shot_XXXX first. When source clean shots
exist, these are preferred because palettes.bin contains the actual
CpsPal effective 32-bit color cache. It does NOT need a working emulator.
Without clean shot sources, it falls back to the existing original native
stage_capture/session_XXXX data: OAM, decoded gfx, and raw palette RAM. Raw
palette RAM does not prove the effective output of per-page CPS2 palette DMA,
so this fallback is clearly flagged as color-PROVISIONAL. It still decodes
genuine original sprite tile bytes, never invented artwork.

The offline runner writes:

    F:\xmvsf\test\87.203\sprite_factory\sprite_factory_review_bundle.zip

The bundle contains sprite_factory_manifest.json, sprite_catalog.tsv,
paginated sprite_contact_001.png and following contact sheets, plus the
actual decoded sprites/*.png at native dimensions. The sibling output
folder also retains sprite_observations.tsv.gz with original object index,
emulation frame, position, priority, palette, tile number, dimensions and
flips. The review images are only examples selected from all catalogued
variants. A direct tile address is represented in hexadecimal in the PNG
filename: Txxxxx_Pyy_Wn_Hn_Fn.

To query one logical CPS2 object tile ID exactly without entering MVC:

    RUN_TEST.bat sprites 0x12345

More choices are available through the source-only script:

    python dev\stage_capture\test_sprite_factory.py

    python dev\stage_capture\SPRITE_FACTORY.py --clean-root "F:\xmvsf\test\87.203\clean_rips" --output "F:\xmvsf\test\87.203\sprite_factory" --shot shot_0010 --tile 0x12345 --palette 3

    python dev\stage_capture\SPRITE_FACTORY.py --capture "F:\xmvsf\test\87.203\stage_capture" --source telemetry --step 120 --output "F:\xmvsf\test\87.203\sprite_factory"

The tile addressing reproduces the existing CPS2 object's bank wrap, sprite
width/height and native horizontal/vertical flip rules. It never strips
black pixels using chroma keys. A missing referenced tile is reported, not
silently substituted by a fabricated colored rectangle. Gallery paging
has at most 16 independent sprites per contact sheet, so it avoids the
older 2048px single-sheet review overflow. It does not touch the prior
clean-rip images, ROM, source game or Stable. The source remains local
inside ignored test/ (only the converter code is committed to Git).

CRITICAL: Individual object sprites are not always whole fighter animations.
MVC draws character bodies, HUD, FX and some scenery as separate original
CPS2 objects. A tile ID reliably identifies graphical pixels and their
address; it does NOT identify semantic ownership. Gallery entries remain
UNCLASSIFIED and false stage completion is prohibited. Animation-track
grouping, scenery-vs-fighter identification, blending and accurate in-engine
priority are future verification steps. The real 39,282-frame uploaded OAM
capture generated 64 separate direct-address sprite previews offline with
the raw-palette fallback; it passed source-only tests. Native effective-color
exports from the local clean_rips are still awaiting their first user-run
visual check and must not be asserted fully pixel matched.


## HUD correction to native Sprite Factory (user-confirmed 2026-11-06)

The user's actual effective-palette Sprite Factory output had correctly
decoded graphics but the 96 highest-frequency images were mostly priority-7,
screen-stationary HUD assets such as FREE PLAY and blue interface pieces.
Their uploaded indexed evidence contained 198,050 object observations and
28,937 unique object variants across 17 shots. Native tile decoding, flips,
transparency and palettes were functioning; the GALLERY SELECTION was wrong.

The corrected offline selection now scores objects by original native
priority, sprite area, frame persistence, screen-position variability and
reuse of the *same tile/palette/size/flip signature across shots*. Repeated,
fixed-screen priority-7 objects are marked INTERFACE_LIKELY and deferred.
Review candidates are diversified across shots AND tile signatures; recurring
HUD pieces no longer consume the first 96 slots. This is a conservative
HEURISTIC, not automatic verification of a stage prop or fighter identity.
A priority alone must never permanently discard sprite source artwork.

Offline after pulling feature/stage-87-203-native-layer-recorder:

    RUN_TEST.bat sprites

This defaults to --focus scenery: a gallery of potential non-HUD objects
with a complete unsuppressed sprite_catalog.tsv. The catalog adds
cross_shot_count, candidate_role and scene_score columns so every
ranking decision can be audited. The output is still the existing
test/87.203/sprite_factory/sprite_factory_review_bundle.zip.

For the old interface-inclusive view:

    RUN_TEST.bat sprites all

For EXACT hardware tile addressing, even when the tile is flagged HUD:

    RUN_TEST.bat sprites 0x16ca0

The --tile selection bypasses scene-guess filtering. The direct-address
pixel assembly algorithm is unchanged. Original clean_rips/shot_XXXX,
palette samples, sprite observations and generated previews remain intact.
Both output modes update ONLY sprite_factory review artifacts.
No ROM execution, native recompilation or scene installation is required.


## Offline eight-priority CPS2 sprite review

The November 6 capture contains 198,050 object occurrences across ALL
CPS2 object priorities (0=928, 1=816, 2=2368, 3=2856, 4=2076,
5=5963, 6=52946, 7=130097). The old Sprite Factory collapsed each tile
signature to its most frequent priority and therefore hid priority 0/1.
The new exporter never guesses HUD, fighter, effect, or scenery identity.

Offline (NO ROM, native build, or gameplay), after pulling this branch:

    RUN_TEST.bat sprites
    REM aliases: RUN_TEST.bat channels / RUN_TEST.bat sprites channels

This runs dev/stage_capture/SPRITE_CHANNELS.py against existing original
test/87.203/clean_rips/shot_XXXX files and creates:

    test/87.203/sprite_channels/sprite_all_channels_review_bundle.zip

The ZIP contains priority_0 through priority_7 directories with separate
contact sheets, individual transparent sprite previews reconstructed using
original decoded tiles and effective palettes, and COMPLETE priority-specific
tile/palette/source-frame catalogs. The full all_priority_observations.tsv.gz
includes EVERY occurrence at its actual recorded priority; not even a HUD
sprite is dropped from the index. PNG previews are sampled (32 by default
per channel) to bound disk usage, NOT a cap on the source addresses. For
more preview images (96 per channel):

    python dev/stage_capture/SPRITE_CHANNELS.py --clean-root "F:\xmvsf\test\87.203\clean_rips" --output "F:\xmvsf\test\87.203\sprite_channels" --max-per-priority 96

Also exported: original native scroll channel 1/2/3 and scroll0
(original no-sprite, scroll-only composite), for three representative
source shots. Those are checksum-verified source-rendered 384x224 pixels.

The sprite previews are original assembled tile pixels with source colors,
but native occlusion, sprite blending, verified scenery ownership, stage
boundaries and animation-loop completion remain unknown. Scroll0 is a
composite, not a fourth scroll layer. Existing optional selectors remain:

    RUN_TEST.bat sprites scenery
    RUN_TEST.bat sprites all
    RUN_TEST.bat sprites 0x12345

Run all-channel source-only tests with:

    python dev/stage_capture/test_sprite_channels.py

No source shots are overwritten, no ROM runs, no new launcher and no
GitHub Actions. Stable is unchanged.


## Stage87.203 Motorola 68000 sprite-writer provenance

The MVC1 hardware write origin can be captured WITHOUT replacing the
CPU or changing the game's 68000 instructions. Existing Stage87.16
CPS2 0x708000..0x70FFFF WRITE handlers collect the actual SekGetPC(-1)
when an object attribute word is committed. Hardware bank, raw slot,
A0-A5/D0-D1, original sprite tile/attribute/palette/priority and
emulation frame are recorded. Reads and opcode fetches remain native.

The trace is OPT IN, not enabled for ordinary RUN_TEST or Stable.
From the existing feature branch, using root Windows launchers:

    BUILD_TEST.bat
    RUN_TEST.bat mvscur1 trace

The second command runs MVC1 and writes a NEW unique local directory:

    F:\xmvsf\test\87.203\m68k_trace\run_XXXX\
        sprite_writes.tsv
        mvc_program_data_host_words.bin
        mvc_opcodes_decrypted_host_words.bin
        program_source.tsv

The two private 68000 files are host-word-swapped emulator ROM data
and decrypted instruction-fetch bytes. They remain LOCAL and are NEVER
committed to GitHub or included in the shareable analysis ZIP.
The snapshot is once per run; Stage87.16 writer capture is bounded
to 1,500,000 attribute-write records. Later writes are not certified.

To collect clean scroll background layers AND 68000 writer provenance
in the same run, request the additional trace explicitly:

    set XM87203_M68K_TRACE=1
    RUN_TEST.bat mvscur1 clean

Offline analysis, no emulator/game required:

    RUN_TEST.bat m68k

Output:

    F:\xmvsf\test\87.203\m68k_report\m68k_sprite_report.zip

The ZIP includes m68k_writer_pcs.tsv, m68k_object_families.tsv,
m68k_tile_transitions.tsv, a JSON manifest and summary. It excludes
all private ROM data, decrypted program files and raw opcode hex.
A separate LOCAL m68k_writer_code.tsv contains PC-addressed 68000
opcode windows; install Python Capstone optionally for M68K mnemonics.
Without Capstone, native writer-PC correlations still work.

CONFIRMED evidence: executing 68000 PC at object-attribute write,
object bank/slot, tile, palette/priority, frame and CPU registers.
UNVERIFIED candidates: grouped 16-tile graphics families, possible
ROM table pointers (A0-A5) and nearby same-slot tile changes. A
reused slot is NOT evidence of a single game-animation script.

The old stage_capture(2).zip contains OAM history but no original
writer-PC trace. An offline scan cannot reconstruct the missing PCs
or prove ROM animation-table ownership. RUN_TEST.bat m68k produces an
explicit diagnostic bundle when no trace exists.

Standalone local tests:

    python dev\stage_capture\test_analyze_m68k_sprites.py

No GitHub Actions, no extra numbered launchers, no native gameplay
writes, no Stable overwrite, no automatic writer hook for XMVSF.


### Trace-only vs clean + trace

The MVC program snapshot now operates independently of the heavy original
stage telemetry, so the simple RUN_TEST.bat mvscur1 trace command disables
87.203 raster/OAM/tilemap capture and only collects the 68000 writer PC TSV
and 68000 program bytes. It does not require START.flag or an additional
clean background session, and therefore avoids several GiB of redundant
scene capture. The two command launchers remain exactly BUILD_TEST.bat and
RUN_TEST.bat.

If both sources are wanted simultaneously, explicitly set
XM87203_M68K_TRACE=1 in the caller's CMD and run
RUN_TEST.bat mvscur1 clean. The clean-rip recorder retains its existing
bounded bursts and independently preserves its session logs.

When the MVC process exits, RUN_TEST.bat automatically runs the small
68000 report generator if this trace was enabled. The existing original
game exit code remains authoritative; reporting never overwrites Stable.
The alternate RUN_TEST.bat m68k is a fully offline repeat of that report.


## 68000 run_1.zip source-backed findings (2026-11-06)

Original uploader provided run_1.zip from RUN_TEST.bat mvscur1 trace:
4,194,304 bytes original program READ data, 1,048,576 bytes decrypted
instruction FETCH data, and 41,679 68000 object write records over frames
87..1640 (1,014 frames with writes). No gameplay controller rerun is needed
to inspect or disassemble these source images offline.

There were 26 distinct *reported handler PC values*. Of the 41,679 writes,
24,800 represent nonblank objects, and every one of those 24,800 has
hardware sprite priority **7**. This trace therefore does NOT establish
stage-actor animation writers in priorities 0..6; other scene stages and
routines remain unobserved by the new PC audit.

At recorded handler PC 0x02484C, the decrypted opcode word is 0xFFF0,
the displacement extension of DBRA/DBF at 0x02484A. A preceding
MOVEM/MOVE.L loop at 0x02483C-0x02484A copies preassembled OBJ
descriptors from graphics RAM around 0x925000 to CPS2 object RAM.
The memory-handler's SekGetPC(-1) is stable provenance *near the writer*,
but is NOT necessarily an instruction-start PC. Disassembling at 0x02484C
as though it were a new instruction is incorrect.

Original host-word-swapped ROM program data contains the observed
consecutive tile IDs 0x6CD0..0x6CD7 at byte address 0x08A51E.
This neighborhood includes original THANK YOU FOR PLAYING / PRESENTED BY
text, suggesting a text/interface context (not proof of a stage prop).
Additional packed tile/attribute-like records occur near 0x2E4180.

Updated ANALYZE_M68K_SPRITES.py writes two additional metadata TSVs
from original 68000 program READ data without packaging ROM bytes:

    m68k_rom_tile_references.tsv
    m68k_rom_tile_sequences.tsv

The reference list contains 16-bit aligned candidate tile word matches;
a reference is not automatically a validated animation descriptor.
The manifest reports nonblank priorities and explicitly marks handler
PC == opcode instruction start as unverified.

Use existing local trace, with no game launch or build:

    RUN_TEST.bat m68k

To find the ORIGINAL upstream object/animation engine writers (not the
generic final OBJ copy), further analysis must follow the instructions
that populate graphics-RAM source descriptors around
0x925000..0x926FFF. Private ROM data and code dumps stay under
test/87.203/m68k_trace, never in GitHub or the sharing ZIP.


## Offline MVC Motorola 68000 upstream sprite staging map (2026-11-06)

The uploaded \`run_1.zip\` established that native object RAM receives final
8-byte sprite descriptors from a generic 68000 copier at \`$02483C–$02484C\`.
Those writes were priority-7 interface graphics only. Their writer PCs do
**not** identify the code choosing every source animation frame.

A second static program investigation, \`TRACE_MVC_STAGING.py\`, uses the
existing PRIVATE local \`mvc_opcodes_decrypted_host_words.bin\` and
\`mvc_program_data_host_words.bin\`. It does not launch MVC, modify the
emulator, build anything, or commit either image.

Source-backed results for the user's 1-MiB MVC decrypted FETCH image:

- \`$004B0E\`: original sprite-descriptor dispatcher. Its exact bank-select
  opcode signature is verified in the decrypted instruction stream:
  nonnegative type to \`$924000\`, type high nibble \`$80\` to \`$925000\`,
  other negative type values to \`$926000\`. These are staging buffer
  categories, **not** eight hardware sprite priority layers.
- \`$006B7A\`: \`MOVEA.L #$925800,A0\`, followed by setup of the four
  fields of an original 8-byte descriptor.
- \`$006B8C\`: indexed \`MOVE.W d8(PC,D5.W),D2\` candidate tile selection,
  referring to a potential base near \`$006B9C\`.
- \`$006B94\`: \`MOVEM.W D0-D3,(A0)\`, depositing X, Y, tile, attributes
  as original four-word sprite descriptors into staging RAM.
- \`$02483C–$02484C\`: final copy from \`$925000\` into CPS-2 OBJ RAM.
  As before, the traced \`$02484C\` is an instruction displacement
  extension, not an instruction-start PC.
- The encrypted original program READ view and decrypted instruction FETCH
  view have *different bytes* at several candidate PC-relative tables.
  Do not silently read apparent animation entries from the wrong view.

Exact opcode-pattern scan of the supplied program: 112 candidate staging
address instructions, 42 matching four-word descriptor emitters,
21 emitters with nearby A0 staging constants, 13 nearby indexed tile
lookup motifs, and 32 direct branch/call motifs to \`$004B0A/$004B0E\`.
These are **static code patterns**, NOT 42 proven executed routines,
13 independently verified animation tables, or stage-prop ownership.
The user's \`run_1\` CPU write trace itself only captured priority 7.

Run with existing trace, no further gameplay, build or ROM:

    cd /d F:\xmvsf
    RUN_TEST.bat m68k

The existing \`m68k_sprite_report.zip\` is retained. The new report is:

    test/87.203/m68k_report/m68k_staging_report.zip

Its TSV files index staging address references, descriptor emitters,
potential indexed tile table bases, and direct dispatcher call sites.
The ZIP includes metadata/offsets only; original and decrypted program
images remain local under ignored \`test/87.203/m68k_trace\`.
Synthetic regression command:

    python dev\stage_capture\test_trace_mvc_staging.py

Next unproven link: which 68000 paths execute while specific MVC stages
animate and which stage actor/script references those candidate table bases.
That requires correlating source tables and actual hardware object records;
this tool does not mark unexecuted opcode motifs as scene ownership.


## 68000 source descriptor vs original graphics ownership (2026-11-07)

New read-only tool \`TRACE_MVC_OBJECT_OWNERS.py\` correlates the existing
\`run_1\` MVC1 writer-PC capture, \`staging_descriptor_emitters.tsv\`,
\`sprite_factory/sprite_observations.tsv.gz\` (17 captured shots), and the
*original decoded* \`mvc1_decoded_gfx_*.bin\` from \`stage_capture\`.
It never executes MVC or modifies the original capture and never puts
ROM instructions, sprites, or GFX bytes into its review ZIP.

The original 198,050 object observations included **49,812 objects with
NO VISIBLE SOURCE PIXELS** (verified using the complete native
multi-tile 4-bpp artwork). 48,198 of those come from the completely
transparent \`$16820\` tile. This tile appears at *all eight* sprite
priorities, so a priority-channel-only gallery can overstate the number
of visible objects. These entries remain in the evidence catalog; no
source observation is discarded.

Cross-reference the 68000 program's exact \`MOVEM.W D0-D3,(A0)\`
descriptor emitters against the original object *tile + full attributes*,
not just visually similar pixels. Of 42 static patterns, **13 literal
tile-and-attribute matches** were found in this source evidence:
five land on transparent objects, six on repeated fixed-screen
priority-7 interface candidates, and two are unresolved rare objects.
A fourteenth tile-only match lands on a transparent object but has
unresolved attributes, so it is not counted as a full-field match.
These are *static code-pattern correlations*, NOT independently observed
upstream 68000 execution.

Grounded source examples:

- \`$00689E\`: 68000 immediate tile \`$6CD8\` plus attributes \`$151F\`
  matches a 6×2, palette-31 native OBJ \`$16CD8\` observed 844 times
  across all 17 shots at native screen location (144,5).
- \`$0063C0/$006402\`: tile \`$6978\`, attributes \`$001F\`;
  source OBJ \`$16978\` was seen 1,554 times across all 17 shots
  at fixed left/right interface positions.
- \`$0069F4/$006A1C\`: tile \`$6CA6\` with attribute variants
  \`$151F/$153F\`; source OBJ \`$16CA6\` is a fixed-screen,
  6×2, priority-7 candidate.
- \`$00652E/$006556\`: tile \`$6930\`, attributes \`$001F\`
  matches original tile \`$16930\` in only two shots. Its
  stage/fighter/interface ownership remains **unverified**.

The high graphics bank is derived from the Y word:
\`full_tile = tileword | ((yword & 0x6000) << 3)\`. Matching just the
16-bit tile word across different graphics banks must **not** be
presented as a proven identical image.

After pulling the active feature branch, run offline:

    RUN_TEST.bat m68k

This runs the writer-PC, staging-code, and new owner-correlation reports.
If the prior Sprite Factory observation file is absent, the owner pass
states that and leaves the two earlier reports intact; generating
observations is also offline via \`RUN_TEST.bat sprites all\`.

The new output is:

    test/87.203/m68k_owner/m68k_owner_evidence_review.zip

Files include \`object_family_evidence.tsv\` (all source object variants
with native visibility), \`m68k_descriptor_matches.tsv\` (candidate
writer function and full attributes), \`m68k_runtime_overlap.tsv\`, and
a manifest with explicit confidence limitations. If local
\`stage_capture/session_*/mvc1_decoded_gfx_*.bin\` is absent, pixel
visibility is left UNKNOWN rather than fabricated.

Offline synthetic tests:

    python dev\stage_capture\test_trace_mvc_object_owners.py

Remaining task: distinguish genuine stage machinery from fighters and FX
using animated object-world continuity and/or executed upstream 68000
source references. Repeated fixed-screen priority-7 is a *probable
interface family*, not a hard-coded gameplay filter. Never silently
drop sprites from any priority channel, and do not mark stages complete.
