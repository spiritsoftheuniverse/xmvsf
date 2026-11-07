# MVC1 original-sprite category review (Stage 87.203)

Run the **existing** Windows launcher after obtaining the source evidence:

    cd /d F:\xmvsf
    RUN_TEST.bat art

The offline Python builder uses ONLY existing local MVC1 evidence. It does not
launch an emulator, run a native build, edit ROMs, alter original source
captures, use GitHub Actions, or touch Stable.

Generated offline file:

    test\87.203\art_review\MVC1_sprite_review.html

Open the generated HTML in a normal browser. It is fully self-contained:
original decoded MVC1 sprite pixels are embedded directly as PNG data
URLs, so the viewer does **not** require a web server, ROM access, or network
connection after generation.

## Reviewer controls

- Each boxed sprite is an assembled object candidate. Click the box to
  toggle **Wanted** / **Unwanted**. First click on an unreviewed box chooses
  Wanted. Each sprite also has a checkbox and an enlarged Zoom preview.
- **Check all** / **Uncheck all** apply to every member of that category,
  including items currently hidden by search filters.
- Categories are **assumptions**, not ROM-derived ownership or verified stage
  identities. Reassign any sprite using the category dropdown inside the
  card.
- Search by the exact original header address, sample frame, shot, category,
  or selector value. Filter Wanted / Unwanted / Unreviewed or component
  completeness. The totals distinguish these states.
- **Save review JSON** downloads a complete machine-readable
  `MVC1_ART_REVIEW_SELECTIONS_V1` file, including all original offsets,
  source shot/frame, selector, number of matching components, chosen
  category, and a `wanted` value of true, false or null (unreviewed).
  **Load review JSON** restores decisions for the matching SHA-256 program.
  The browser also attempts local persistence. The downloaded JSON is
  the durable copy and should be preserved.

## Assumed categories

- Character / figure candidates
- Stage / machinery candidates
- Lettering / HUD candidates
- Effects / abstract candidates
- Partial / unresolved fragments

These are based on image proportions and a few manual visual observations.
They are **not** game-derived object identities. A matching record with
all expected descriptor pairs is NOT proof that the grouping is a real
animation frame. Original OAM observations can reuse hardware slots and
contain multiple objects. Some large or ambiguous groups are placed under
partial/unresolved even if all descriptor pairs have matches.

## Regenerate offline

    python dev\stage_capture\BUILD_MVC_ART_REVIEW.py ^
      --trace-root test\87.203\m68k_trace ^
      --observations test\87.203\sprite_factory\sprite_observations.tsv.gz ^
      --capture test\87.203\stage_capture ^
      --output test\87.203\art_review\MVC1_sprite_review.html

The source-only synthetic tests are:

    python dev\stage_capture\test_build_mvc_art_review.py

The builder requires Python with Pillow and NumPy. Captured decoded GFX and
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
  The generated HTML contains original game artwork and MUST remain
  under ignored `test/`, alongside the original decoded GFX, program data
  and sprite capture. Do not commit the generated HTML, graphics binaries
  or private ROM snapshots.
- The original sprite positions, exact tile/attribute pairs, hardware
  dimensions, palette slots and recorded frames are retained. Pixel colors
  currently use the captured **raw CPS2 palette RAM approximation**; the
  post-DMA effective native palette is not yet certified.
- Gallery categories can be refined by human review using the saved JSON.
  This tool does not identify verified 68000 runtime frame durations, script
  targets, stage actors, or cycle timing.
