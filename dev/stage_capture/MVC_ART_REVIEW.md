# MVC1 original-sprite Asset Organizer (Stage 87.203)

Run the **existing** Windows launcher after obtaining the source evidence:

    cd /d F:\xmvsf
    RUN_TEST.bat art

The offline Python builder uses ONLY existing local MVC1 evidence. It does not
launch an emulator, run a native build, edit ROMs, alter original source
captures, use GitHub Actions, or touch Stable.

Generated offline file:

    asset_organizer\index.html

Open the generated HTML in a normal browser. It is fully self-contained:
original decoded MVC1 sprite pixels are embedded directly as PNG data
URLs, so the viewer does **not** require a web server, ROM access, or network
connection after generation.

## Organizer controls

- Click a boxed sprite to toggle **Wanted** / **Unwanted**; the initial state
  remains **Unreviewed**. Each sprite also has a checkbox and Zoom view.
- Four **independent category tags** can be toggled on each sprite:
  **Character**, **Effect**, **Background**, and **Level Sprite**.
  A sprite may carry several tags and appear in several tag groups.
- **Check all** and **Uncheck all** within a group change Wanted status for
  all assets assigned to that tag, including those hidden by review filters.
  They do not modify the tag assignments.
- Previous tentative figure, effects and stage classifications provide only
  starting guesses for Character, Effect and Level Sprite. Lettering and
  unresolved fragments start untagged; Background starts untagged until
  supported by manual review. Tags are editable; no ROM identity is claimed.
- Search and filter by review status, completeness, and chosen tag.
- **Save assets JSON** produces `MVC1_ASSET_ORGANIZER_V2` with all 560
  asset IDs/ROM offsets, source frames, independent tag arrays and decisions.
  **Load assets JSON** restores tags and decisions, and can import
  `MVC1_ART_REVIEW_SELECTIONS_V1` files from the previous viewer.
  Exported JSON is the durable record; local browser persistence is helpful
  but not guaranteed across changed HTML paths.

## Regenerate offline

    python dev\stage_capture\BUILD_MVC_ART_REVIEW.py ^
      --trace-root test\87.203\m68k_trace ^
      --observations test\87.203\sprite_factory\sprite_observations.tsv.gz ^
      --capture test\87.203\stage_capture ^
      --template asset_organizer\ASSET_ORGANIZER_TEMPLATE.html ^
      --output asset_organizer\index.html

The source-only synthetic tests are:

    python dev\stage_capture\test_build_mvc_art_review.py

The builder requires Python 3.10+ and uses only its standard library. NumPy and Pillow are not required. Captured decoded GFX and
raw palette snapshots must already be present in the original local
stage_capture folder (or a ZIP explicitly given to --capture). If
`sprite_observations.tsv.gz` is absent, first run:

    RUN_TEST.bat sprites all

The current 87.203 user-evidence run yields **560 distinct candidate
assemblies**, **518** of which have one sampled frame containing every
expected exact tile/attribute descriptor. These are *source-observation
counts*, not engine-certified full poses or the total set of original MVC1
objects. Do not infer missing art is absent from the ROM.

## Source privacy and visual accuracy

- Only exporter **source** and the empty HTML **template** belong on GitHub.
  The generated `asset_organizer/index.html` contains original game artwork
  and MUST remain Git-ignored, alongside original source captures under `test/`. Do not commit the generated HTML, graphics binaries
  or private ROM snapshots.
- The original sprite positions, exact tile/attribute pairs, hardware
  dimensions, palette slots and recorded frames are retained. Pixel colors
  currently use the captured **raw CPS2 palette RAM approximation**; the
  post-DMA effective native palette is not yet certified.
- Asset tags can be refined by human review using the saved JSON.
  This tool does not identify verified 68000 runtime frame durations, script
  targets, stage actors, or cycle timing.
