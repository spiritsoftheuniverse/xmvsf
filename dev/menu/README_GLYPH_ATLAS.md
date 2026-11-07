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
menu background. The source is a 400x300 **export canvas**, NOT a 400x300
on-screen target. The renderer measures the alpha-visible art, crops only
the extra transparent source margins, then fits the original painted pixels
proportionally inside the exact reference area. No resaved, repainted, or
warped source PNG is necessary.

### User's screenshot: fixed native-coordinate alignment

The supplied 1811x1358 example is nearly 4:3. Its positions map uniformly
into a 720x540 safe frame centered at x=120 inside the game's 960x540 SDL
logical canvas, so the **artwork and glyphs keep the original aspect ratio**
rather than being distorted to fill the widescreen window.

All values below are **logical SDL coordinates**, not physical display
pixels (window resizing leaves them anchored):

| Element | Native target |
| --- | --- |
| Logo visible artwork | x=235, y=51, maximum w=544, h=289 (fit and center) |
| Menu text centerline | x=489 |
| ARCADE | y=347 |
| TRAINING / VERSUS / CONTROLS / OPTIONS | y=378 / 409 / 440 / 471 |
| A SELECT  B BACK | centered x=489, y=495 |
| ESC TO QUIT | centered x=500, y=522 |
| TEST 87.205 | right edge x=942, y=522 |

These are explicitly stored in XMGlyph::MainLayout, so one presentation
contract controls the initial frame and every subsequent boot/menu frame.
The title block ends near y=340; the selected entry begins at y=347.
The 7-pixel gap matches the reference rather than the previous 400x300
canvas assumption.

Primary menu choices use the **large SF2 sprite font at 1.35x**. The
control hint uses the small sprite font at 1.375x; the exit hint is at 1.20x,
and the version stamp at 1.08x. SDL's nearest-neighbor sampler snaps those
fractionally scaled pixels to the pixel grid without blurring.
The other menus retain their existing layout and small font behavior.

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
- All spacing measurements happen in source pixels. The native menu
  fits them at the screenshot-derived fractional scale with pixel-snapped
  destination rectangles and nearest-neighbor sampling; auxiliary menus
  retain their existing integer scale.
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

## Animated menu background and color-separated logo effects (TEST 87.205)

The native boot menu now loads **_art/menuanimation.gif** instead of the
previous Marvel vs. Capcom super-effect GIF. The source is an authentic GIF87a
384 x 224 image with **8 frames**, each **50 ms** long. It loops at the
original 400 ms cycle, cover-scaled proportionally to 960 x 540 and
center-cropped as needed. RGB colors undergo a continuous 36-second full HSV
hue rotation at 30% menu-background brightness. Alpha is untouched.
A streaming texture avoids allocating a new GPU texture per GIF frame.
If SDL_image cannot decode animation, the first frame remains available as
a static, hue-shifting fallback.

The foreground **_art/logo.png** remains the original source artwork. Its
alpha-visible rectangle and menu alignment are unchanged. A second streaming
texture (separate from the unmodified fallback logo) applies per-pixel
effects by original hue category:

- **Yellow/orange:** two independent slow pulse envelopes with a small
  lift toward white, so fully saturated bright colors visibly glow.
- **Green:** its own brightness pulse envelope and saturation-safe lightening.
- **Blue/purple:** intersecting moving sine-wave value fields plus a
  source-locked 4 x 4 Bayer dither. This gives a gaseous brightness
  undulation without repainting or blurring the logo.
- **Red and grayscale:** retained without modification.
- **Source alpha:** always retained; texture updates do not compound over
  previous frames. The logo effect updates at most every 80 ms.

The original large/small SF2 font atlases and their proportional tracking are
unaffected. These are TEST-only SDL frontend changes; no CPS2 ROM input,
hitbox, DHC, stage capture or other game simulation code is touched.
The code resides in xmvsf_menu_color_fx.h,
xmvsf_stage87_195_menu_gif.h and xmvsf_menu_glyph_atlas.h.

The native builder checks that **menuanimation.gif** is present and nonempty,
and includes its SHA256 in test/build_logs/BUILD_TEST.log. The binary is
**runtime/stable/XMVSF_Test_87_205.exe**; no fallback to 87.204 or Stable.
Build/run/replay launchers now all target the new effects version.

## Native TEST version 87.205 and local verification

**Pulling only main does not include this branch.** The source modifications
are on feature/menu-logo-glyph-atlas-selection-dimming. In the Windows
project repository:

    git fetch origin
    git switch feature/menu-logo-glyph-atlas-selection-dimming
    git pull --ff-only origin feature/menu-logo-glyph-atlas-selection-dimming

Preserve any uncommitted local work before switching branches. Keep the
original _art PNG files; do not resample or replace them.

1. Run BUILD_TEST.bat. It now publishes ONLY
   runtime/stable/XMVSF_Test_87_205.exe after compiling current source.
   A missing executable or modified menu/glyph header refreshes SDL
   main.cpp and run.cpp without deleting warm build objects, old TEST
   binaries, or Stable.
2. Read test/build_logs/BUILD_TEST.log for the 87.205 success line and
   the SHA256 hashes of the compiled executable and three source PNGs.
   If an asset or the glyph-header include is missing, the builder gives
   a precise error instead of silently building a legacy-looking menu.
3. Run RUN_TEST.bat option 1. The menu window title and bottom-right
   main-menu label both say TEST 87.205. If they do not, you are not
   looking at the newly compiled native XMVSF build.
4. The gameplay path is still the native XMVSF app's zero-ROM-argument
   boot selector, NOT the separate MVC1 capture/ROM playback mode.
   Inspect the title artwork, font tracking, Arcade, Training, Versus,
   Controls, Options, scoreboard and return-to-menu behavior.
5. Verify the unselected text is dim, selected text is bright, and that
   I, i, l, 1, punctuation and word gaps no longer waste full cell widths.
6. New F9 replays, observer/chat diagnostics and replay verification live
   under test/87.205. Legacy test/87.203 capture data is not modified;
   original replay files remain accessible through RUN_REPLAY.bat.

This source audit includes inspection of the tracked code, unchanged font
PNG blob hashes, and explicit version paths. It is not a claim that this
GitHub-only session compiled or ran the native Windows executable.
GitHub Actions are not used.
