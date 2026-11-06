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
