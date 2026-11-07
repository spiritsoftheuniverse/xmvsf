# MVC1 Asset Organizer (Stage 87.203)

The root-level folder contains the review template and a **local-only** browser
app. The built `index.html` embeds original MVC1 pixels and must remain
uncommitted. `.gitignore` also excludes `aodata.json` and generated reports.

## Generate / launch

In `F:\xmvsf` (after pulling the 87.203 branch), run:

    RUN_TEST.bat art

This reassembles sprites from existing captured data and writes
`asset_organizer\index.html`. It then starts a **127.0.0.1-only** Python
server and opens the organizer in the default browser at
`http://127.0.0.1:18720/`. Leave the organizer server console open while
editing. Closing that window stops the server; nothing runs online or on
GitHub Actions. There are no third-party Python dependencies.

If `asset_organizer\aodata.json` exists, the exporter loads the **complete**
previous review as the initial state (checksum and exact asset IDs verified).
The localhost Save button writes atomically to that same file without a file
picker or downloaded duplicates. It preserves your Wanted/Unwanted decisions
and all category tags. An updated `wanted_sequences.json` is also generated
on Save from the original local program data if it is available.

For first-time setup, place your full `MVC1_ASSET_ORGANIZER_V2` export into:

    F:\xmvsf\asset_organizer\aodata.json

Do not use the filtered Wanted-only manifest as this master review; it must
contain all 560 reviewed asset records. JSON files with a different ROM
checksum or asset catalog are rejected rather than merged by guessing.

**Without the localhost server:** You can still open `index.html` via
`file://`. Since Firefox/browser security does not allow arbitrary file
writes, Save then **downloads a file named `aodata.json`**; copy/replace it
in `asset_organizer` yourself. The standalone HTML embeds the review snapshot
found at build time, and Load JSON can import your newer edits.

## Controls

- Click a boxed sprite (or its Wanted checkbox) to set Wanted or Unwanted;
  initially each review is Unreviewed unless loaded from `aodata.json`.
- Toggle the four **independent** tags: Character, Effect, Background,
  Level Sprite. Any sprite may carry several tags.
- **Mark all as → choose type → Apply to all:** after confirmation, every
  sprite receives ONLY that one type tag, and its other tags are cleared.
  This operates on the **entire** 560-asset catalog, even if filtered or hidden,
  and **never changes** Wanted/Unwanted/Unreviewed status.
- Per-category **Check all / Uncheck all** affect *Wanted* review decisions
  in that category, not its tags. If a sprite has multiple tags it appears in
  multiple sections but only once in `aodata.json`.
- Save to aodata.json keeps full review metadata, original ROM addresses,
  source frames and categories, but no ROM images. Load JSON also accepts
  old `MVC1_ART_REVIEW_SELECTIONS_V1` selections.

## Next reconstruction pass

`BUILD_WANTED_SEQUENCES.py` links only Wanted assets to their original 68000
**structural record-adjacency regions** and groups close neighbors. It writes:

    asset_organizer\wanted_sequences.json
    asset_organizer\wanted_sequences.tsv

The currently supplied review has **105 Wanted** among 560 candidates. All
105 occur in the original record-length scan and form **8 structural-neighbor
clusters** when allowing up to eight unselected headers between Wanted items.
These are candidates for adjacent artwork, *not* proven native animation
sequences, execution order, characters, or frame timing. Later classification
and visual verification remain necessary.

Regenerate the report without launching the UI:

    python asset_organizer\BUILD_WANTED_SEQUENCES.py ^
      --trace-root test\87.203\m68k_trace ^
      --review asset_organizer\aodata.json

Synthetic, no-private-ROM tests:

    python dev\stage_capture\test_build_mvc_art_review.py
    python asset_organizer\test_asset_organizer.py

## Privacy and controls

Only the source HTML template, server source, structural analysis source,
regression tests and documentation are tracked on GitHub. Original decoded
artwork, ROM, private source bytes, generated `index.html`, `aodata.json` and
local reports remain Git-ignored. The localhost server serves only the
organizer page and `/api/review`, verifies the source checksum, full asset ID
set, exact address mapping, save origin and tags, and replaces the fixed file
atomically. It never accepts user-selected output paths.

Existing captured palettes are still an approximation of CPS2 raw palette RAM,
not certified effective DMA output. No Stable or gameplay modifications.
