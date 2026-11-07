# XMVSF native menu sprite atlas

Work branch: feature/menu-logo-glyph-atlas-selection-dimming.

## Verified asset dimensions and mapping

- _art/logo.png: 400 x 300, X-Men vs. Street Fighter II foreground title.
- _art/logo.webp: retained alternative, unchanged.
- _art/sf2font.png: 190 x 70, nominal 16-column x 6-row ASCII grid, cell stride 12 x 12.
- _art/sf2fontsmall.png: 128 x 48, nominal 16-column x 6-row ASCII grid, cell stride 8 x 8.

Both sheets index printable ASCII directly (columns go left to right):
- Row 0: ASCII 32 through 47 (space through slash).
- Row 1: ASCII 48 through 63 (digits and punctuation).
- Row 2: ASCII 64 through 79 (at sign through O).
- Row 3: ASCII 80 through 95 (P through underscore).
- Row 4: ASCII 96 through 111 (backtick through o).
- Row 5: ASCII 112 through 127 (p through DEL; DEL unused).

The large image is 2 pixels short on its right and bottom edges. The renderer
clips edge cells rather than trying to sample pixels beyond the image.

Implementation: dev/stable/buildtree/src/burner/sdl/xmvsf_menu_glyph_atlas.h

## Rendering policy

The PNG is the foreground title, above the original 30%-brightness animated
menu background. Its full 400 x 300 source canvas is rendered at 1:1 with
its authored transparent padding intact. The visible artwork starts about
25 native pixels below the PNG's top edge; **do not trim that margin**,
scale up the logo, or vertically recenter only the nontransparent art.
Large sprites render primary choices. The small sheet replaces the old
hand-drawn text on secondary menus, scoreboard, and Training.

## Proportional spacing normalization

The underlying sheets retain their original 16 x 6 coordinate grids and
colored pixels. A one-time glyph measurement pass scans alpha in each cell
and extracts its leftmost/rightmost painted columns. The renderer uses those
exact source bounds and computes a per-character horizontal advance:
- Large font: painted width + 2 native pixels of tracking.
- Small font: painted width + 1 native pixel of tracking.
- Spaces use a separate half-cell word gap rather than a full sprite cell.
- Centered text is centered by the **sum of measured advances**, not the
  number of characters.
- All spacing measurements happen in source pixels, then multiply by the
  same integer scale as the glyphs (no blurry interpolation).
- A blank large-font slash uses the original small-font slash as fallback.

For example, the large capital I is only 5 painted pixels wide (versus
10-11 for most capitals), so its advance is now **7** instead of **12**.
The small lowercase i is 3 pixels wide and advances **4** instead of **8**.
The original PNG sheets are left unmodified; these layout metrics are
derived at load time, which keeps changes compatible with source artwork.

Selected choices show at full brightness. All other choices are darkened.
Do not draw selection arrows, surrounding boxes, or highlight stripes.
Only selection presentation changes; the input and menu navigation routes
remain intact. Colored mode identity bars and ordinary panel boundaries are
not selection indicators.

The source glyph pixels are rendered with nearest-neighbor sampling,
preserving original multicolor sprite pixels. SDL2_image loads the original
file without a regenerated duplicate. Missing source images log errors and
retain the old bitmap text fallback so the native menu remains operable.

For exported title PNGs with opaque white canvas, a flood fill makes only
border-connected white transparent; enclosed white highlights are preserved.
Texture atlases are released before renderer teardown or menu/game handoff.

## Native local verification

1. Check out this branch. Keep _art at the repository root.
2. Run the existing BUILD_TEST.bat. Do not run the Stable builder.
3. Run RUN_TEST.bat (option 1).
4. Check the logo foreground and background GIF, five main choices,
   Stage, Versus, scoreboard, Options, Controls, and Training.
5. Confirm every selected choice is bright with no focus boxes, and test
   uppercase, lowercase, digits, and punctuation. Return to boot from a
   match to check shared window and renderer lifecycle.

No native Windows build or runtime result has been claimed by this
GitHub-side editing session. No GitHub Actions are required.
