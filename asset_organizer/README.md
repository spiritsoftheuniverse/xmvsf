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

## Captured animations and scene placement (87.203)

`RUN_TEST.bat art` also produces ignored, local-only files:

    asset_organizer\animations.html
    asset_organizer\captured_animations.json

Open the **Animation & Stage Preview** link at the top of the organizer.
It joins the 105 Wanted records (or whatever is currently selected in
`aodata.json`) with exact descriptor pairs in the captured OAM observations.
It splits the eight adjacent original control-record groups by original capture
shot and type, then separates source-frame gaps exceeding 210 native frames.
These are **candidate sampled sequences**: their preview interval is user
controlled and is not the true 68000 animation timer. Single-pose groups stay
visible and never become fabricated animation frames.

Each frame shows the source sprite in the native 384×224 viewport at the
observed CPS2 OAM object's source coordinates. The dashed rectangle is the
original matching descriptor group's untrimmed bounds. When source art starts
offscreen, a separate full-art view displays the complete cropped sprite
without pretending it was on-screen. The scene's captured `scroll2x/y`,
original program-ROM address, capture session, shot and emulated source frame
are all retained in the metadata manifest, including an explicitly unverified
SCROLL2-relative tilemap coordinate candidate.

A background diagnostic is independently reconstructed from the source
SCROLL2 map page, decoded original CPS2 GFX and raw palette snapshot at the
selected native frame. This is useful for placing artwork against **real**
captured level geometry. It is **not** certified as the native three-layer
priority/raster composite. Stage IDs are the unrecognized `0xFFFFFFFF`
sentinel in these captures, so we do **not** claim named level ownership,
verified world coordinates, animation speed or runtime playback transitions.

The localhost server exposes exactly `/animations.html` and read-only
`/api/captured_animations`; after saving `aodata.json`, it refreshes the
sequence metadata and animations from existing captures. No stage changes
or gameplay placement is installed until a named-stage anchor and true
frame-advance behavior are verified. All generated original sprites, GFX,
backgrounds and private ROM-derived output stay Git-ignored.

## Palette workbench and multi-selection

The organizer is grouped **once per asset** by measured, frequency-weighted
CIELAB similarity of the opaque original sprite pixels. This is a clustering
heuristic, not duplicate detection or proof of sprite/animation ownership.
Common dark outlines are downweighted; transparent pixels are ignored.
Recorded raw CPS2 palette RAM remains an approximation of effective DMA color.

The top controls have a fixed context grid. **Expanded** is the default and
shows ROM offsets, original source frames, component counts and RGB swatches;
**Compact** shows just image, four independent tag switches, Wanted checkbox,
Zoom and multi-select controls. Neither display mode alters saved decisions.

Use mouse drag across gallery artwork to rubber-band multiple sprite cards.
Ctrl+click and the small **Select** button allow additive selection. Editing
any tag on a card while a selection exists copies that *card's complete new
tag set* to all selected assets; other tags on those assets are cleared.
Wanted checkbox changes also apply to the selected set when editing a
selected card. Selection never changes source ROM data.

**Ctrl+X** stages selected cards in a visible cut tray without deleting them.
Select **Set destination** on a color-family container and press **Ctrl+V**,
or use that container's **Paste here** button. This manually reassigns the
cards' color container; their original automatic `colorGroup` and graphics
are retained. Press Esc to cancel a pending cut without modifications,
**Ctrl+Z** or **Undo** to reverse an assignment, and **Auto color** to restore
the computed family on selected cards. Pending cuts are not saved as moves.

`aodata.json` remains `MVC1_ASSET_ORGANIZER_V2`. Each item may now optionally
include `colorGroupOverride: null | "palette_NNN"`; old reviews without the
field load without migration. The local API validates overrides against the
known color families embedded in the matching source catalog. The full
Wanted/Unwanted/tag dataset, including previously confirmed 105 Wanted assets,
is preserved. The bundle contains private original pixel artwork and stays
local; only the empty template, generator and grouping source are committed.

## Stage 87.203 — complete source-family recovery and near foreground

`RUN_TEST.bat art` now follows every explicitly Wanted original MVC1
sprite-data record into its entire **structurally verified contiguous
five-word source-data region**, preserving every original descriptor and
referenced hardware 16x16 tile ID. The output is under the ignored
`asset_organizer/source_families/` folder: `family_recovery.json`,
`family_recovery.tsv`, `family_recovery.html` and an original-GFX packed
tile archive `recovered_sprite_tiles.bin` containing u32 native tile ID
plus the original 128-byte CPS2 tile for each record.

This is a *source-closure candidate set*, **not** verified executed
animation frames, placement coordinates, or original timing. An explicit
Unwanted decision stays Unwanted even when it shares a source region with
Wanted records. Previously unseen records remain SOURCE_DISCOVERED, not
Wanted. The gallery mosaic is an index of **first tiles per descriptor**,
not invented sprite geometry. Each discovered record retains all its
descriptor pairs and expanded native tile IDs for future true frame
decoding. Captured tile-bank matches are distinguished from tile-bank
assignments inferred from other records in the same source region.
Reviewed untagged assets are considered **UI**, without forcibly changing
the user's four original tags or saved review data.

The original SCROLL1 foreground was also missing from the former
SCROLL2-only stage preview. Monster Rock Bar's tables, chairs, candle
stands and major near decorative artwork are present in captured 8x8
**SCROLL1** tilemaps. The new `BUILD_FOREGROUND_MAPS.py` recovers their
original screen/map positions and all referenced tile bytes, and scans
every recorded native SCROLL1 map snapshot within each selected shot's
AO-observed frame range for additional tile IDs and changing cells.
Original SCROLL1 tile data is saved as u32 native map tile ID plus 64
source GFX bytes per entry; the evidence includes full original map-cell
variant history as `foreground_tile_histories.json.gz`.

Review comparisons in `asset_organizer/foreground_recovered/foreground.html`.
The source-complete map evidence is in `foreground_maps.json`, alongside
`scroll1_tiles.bin`. These files are private/local-only; the
source template/algorithms belong in Git, not generated ROM pixels or
`aodata.json`. Both new review links are available in the organizer
toolbar and the localhost server refreshes them after saving the canonical
review. No ROM execution or GitHub Actions are required.

The November 7 source review contains 105 Wanted seeds; six native
structural regions contain 371 records: 105 Wanted, 46 explicitly
Unwanted, and 220 never shown by the former sampled sprite gallery.
The original gfx recovery includes 3,567 unique 16x16 sprite tiles.
The 14 selected captured scene windows contain 878 historical SCROLL1
map snapshots, yielding 9,190 distinct 8x8 foreground tile IDs.
These are source-backed counts for **the available captures**, not proof
that every ROM-managed frame or stage is represented. The native
SCROLL-versus-OBJ priority mask, named stage identifiers and true frame
advance logic remain unverified.

Offline tests: `python asset_organizer\test_source_and_foreground.py`.
All source outputs remain Git-ignored and Stable is untouched.
