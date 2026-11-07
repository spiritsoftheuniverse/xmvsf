# MVC1 Asset Organizer (Stage 87.203)

This root-level folder contains the editable template for the offline original-art
organizer. **The generated `index.html` embeds copyrighted MVC1 sprite pixels
and must never be committed or pushed to GitHub.** The `.gitignore` in this
folder excludes the generated HTML, its manifest and exported review JSON.

## Generate and open locally on Windows

From the repo root, using existing captured test data:

    RUN_TEST.bat art

Open `asset_organizer\index.html` in your browser. The existing exporter under
`dev/stage_capture/BUILD_MVC_ART_REVIEW.py` reconstructs the original objects,
then inserts the private artwork into `ASSET_ORGANIZER_TEMPLATE.html` to make a
self-contained offline browser application. Input data remains under the ignored
`test/87.203` capture folders. The generator does not change the game, ROM,
Stable, or captured logs.

## How to organize

- Click a sprite image/box or its **Wanted** checkbox to choose Wanted/Unwanted.
  Initially it is **Unreviewed**, not Unwanted.
- Each sprite has four **independent, toggleable** category tags: **Character**,
  **Effect**, **Background**, and **Level Sprite**. One sprite may have multiple
  tags, and therefore can appear in multiple groups. The asset count remains
  unique; the gallery does not duplicate IDs in the saved JSON.
- **Check all** / **Uncheck all** in each group changes the review status of
  every asset with that tag, **not** its category tags. The Untagged group also
  supports those controls. Search, status filter and tag filter are available.
- Original assumed categories seed **Character**, **Effect**, and **Level
  Sprite** tags where sensible; lettering/HUD and partial records start untagged.
  Background starts untagged because the present decoded candidates are OAM
  sprite assemblies, not verified background scroll layers. All assignments
  are assumptions until the user reviews them.
- **Save assets JSON** downloads `mvc1_asset_organizer_YYYY-MM-DD.json`. The
  browser selects the download location; copy it into
  `asset_organizer\reviews\` if you want to store it beside the tool locally.
  **Load assets JSON** restores decisions and tag arrays. The previous
  `MVC1_ART_REVIEW_SELECTIONS_V1` review JSON is accepted and converted.
  The new format is `MVC1_ASSET_ORGANIZER_V2`, checksum-guarded and holds
  ROM offsets, source frames, original classification hints, Wanted state,
  and zero-to-four explicit category tags without artwork binaries.
- Browser local storage helps keep recent edits. Exported JSON is the durable
  record, especially when opening HTML from a different local browser path.

## Source and privacy

The source HTML template and this README are safe for source control. Original
CPS2 decoded graphics, raw palette snapshots, captured ROM data, the generated
`index.html` and review output are not. Colors derived from raw palette RAM
remain approximate, not certified native-effective CPS2 palette colors.

For existing synthetic exporter checks:

    python dev\stage_capture\test_build_mvc_art_review.py

The organizer interface has also been exercised with a 560-item original-art
catalog, including multi-tagging, group-wide selections, JSON V2 round-trip,
and V1 review import. Native Windows runner and gameplay execution are not
verified in the offline test environment. No GitHub Actions are used.