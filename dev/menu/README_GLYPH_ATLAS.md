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
| TEST 87.206 | right edge x=942, y=522 |

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

## Animated menu background and color-separated logo effects (TEST 87.206)

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
**runtime/stable/XMVSF_Test_87_206.exe**; no fallback to 87.204 or Stable.
Build/run/replay launchers now all target the new effects version.

## TEST 87.206: purple vibration, music handoff and FIGHT crash evidence

The lowest Street Fighter extruded purple shadow and central purple VS core
now vibrate in discrete visual intervals: a 0.55-second undulation every
2.65 seconds, with independently phased brightness oscillations at
approximately 7.4 and 9.2 Hz. These are strictly original-source RGB
modulations of dark-purple pixels in two source-coordinate regions. Glyph
silhouettes, letter faces, texture coordinates and alpha are not moved.

The existing single Windows WASAPI device now crossfades the menu theme
from its current audible level to silence using a smoothstep envelope over
**500 milliseconds** at the first VS/early-combat host signal. The combat
fallback now sends that signal immediately rather than waiting for a
secondary poll. Repeat combat status observations cannot restart the fade.

The left DHC Super Chat panel is closed by default and on every new gameplay
session. Shift+G opens/closes it explicitly; F10 input probes, chat history
and log recording remain available even while hidden.

**Observed user issue:** the executable unexpectedly **exits/crashes at
"FIGHT!"**. It must not be confused with focus loss, a pause modal or a
mere rendering stall. A confirmed root cause has NOT been established
without a native exception report. 87.206 therefore adds a Windows
unhandled-exception handler that records the actual exception code,
instruction address, DLL/EXE offset and last gameplay/video checkpoint
in `test/87.206/crash_report.log`, and attempts to write
`test/87.206/crash.dmp` using the system DbgHelp DLL. It also guards
null SDL video resources during frame rendering. This is a diagnostic and
defensive pass, **not a claim that the crash is already fixed**. If an
abrupt exit persists, supply the crash_report.log, optional crash.dmp
and the current native RUN_TEST output for exact triage.

BUILD_TEST.bat now checks required 87.206 headers and regenerates just
the affected `main.cpp`, `run.cpp`, `vid_sdl2.cpp`, and `aud_sdl.cpp`
objects when this executable is missing or the related sources change.
All other warm objects, Stable, and older capture/replay files remain.

## Native stun bar replacement (TEST 87.207)

Source and screenshot reference now exist on the same branch:

- `_art/stunbar_base.png`: **61 x 6**, the ORIGINAL RIGHT-side image.
- `_art/examples/example_stunbar.png`: **61 x 8** composition reference.
  It is not loaded in-game or substituted for the source base.

The base's highest original pixel is anchored at native CPS-2 **y=45**,
which matches the preceding stun HUD; P1 starts at x=61 and P2 ends at
x=322 (P2 starts at x=262 for the 61-pixel source). The P1 source pixels
are horizontally mirrored. Do not rotate or invert the bar vertically.

The reference composition places the six-pixel source base at its rows
2..7, while the active original orange region is at rows 0..5. At full
stun, the active region spans exactly **44 pixels**, with each subsequent
row shifted right one pixel. The render code preserves that shape, but
replaces orange with moving six-pixel-wide diagonal rainbow bands. Stun
usage from the original ROM continues to control the active width.
One pixel of dark inset is drawn along the diagonal growth cut and across
the bottom of the active region at reference row 6 (inside the base).
The rest of the source PNG, existing shadows and 1-pixel diagonal grooves
remain untouched.

All drawing remains in the **existing native SDL HUD/UI order**, after
the game scene and before Training/Versus text panels. Wily's existing
compositor draws the same routine once; normal stages get it exactly once.
No new rendering layers, gameplay RAM changes, stun timing, or collision
logic. The original total/per-hit counter readouts remain.

The PNG is decoded into RGBA bytes once and then reused across frames.
Missing or wrongly sized PNGs produce a named log entry and a readable
fallback bar; BUILD_TEST.bat requires the original PNG to prevent quietly
shipping the fallback. TEST 87.207 recompiles native UI/HUD source
(including `cps_obj.cpp`) and publishes only
`runtime/stable/XMVSF_Test_87_207.exe`, never Stable.

## TEST 87.208 — Seam-free purple vibration and corrected stun fill

The two visible horizontal limits in the logo arose from hard rectangular
`vsCore` / `bottomShadow` thresholds, together with a low-brightness
chroma cutoff. Those rectangles are removed. Purple membership now follows
the original PNG's **actual purple RGB pixels**, including dark gradients;
two overlapping vibration frequencies crossfade smoothly over the logo's
height. The moving Bayer gas remains source-pixel-locked. There is no
rectangular crop, alpha change, blur or pixel displacement.

The provided `_art/examples/example_stunbar.png` depicts the active
**44-pixel partial** fill, not the bar's maximum capacity. The native right
stun base (`_art/stunbar_base.png`, 61x6) determines its own opaque span
per source row. At **100% stun**, every original visible source span is
filled, through both ends. The P1 copy still mirrors P2 horizontally.
At partial stun, the growing diagonal leading cut is one source pixel
per row; the exact ROM stun value determines its width. The 1-pixel
shadow sits **inside** the filled span and is also intersected with
the original PNG's base alpha, preventing the former two-pixel overrun.

The ordinary accumulating fill now uses a yellow-to-orange graded band.
Only when the displayed bar reaches **100%** (including the existing
dizzy-trigger flash) does it switch to a vivid seven-color rainbow,
in 4px diagonals moving **3 pixels every emulated frame**. The UI render
order and 45 native y-coordinate are unchanged; no gameplay mechanics,
native CPS2 stun counters or Stable build are modified.

The updated builder and launchers target only
`runtime/stable/XMVSF_Test_87_208.exe`. These are source changes, not
a claim of Windows runtime or visual verification.

## Native TEST version 87.208 and local verification

**Pulling only main does not include this branch.** The source modifications
are on feature/menu-logo-glyph-atlas-selection-dimming. In the Windows
project repository:

    git fetch origin
    git switch feature/menu-logo-glyph-atlas-selection-dimming
    git pull --ff-only origin feature/menu-logo-glyph-atlas-selection-dimming

Preserve any uncommitted local work before switching branches. Keep the
original _art PNG files; do not resample or replace them.

1. Run BUILD_TEST.bat. It now publishes ONLY
   runtime/stable/XMVSF_Test_87_208.exe after compiling current source.
   A missing executable or modified menu/video/audio/crash header refreshes
   only main.cpp, run.cpp, vid_sdl2.cpp, aud_sdl.cpp and cps_obj.cpp, without
   deleting warm build objects, older TEST binaries, or Stable.
2. Read test/build_logs/BUILD_TEST.log for the 87.208 success line and
   the SHA256 hashes of the compiled executable, four source PNGs and GIF.
   If an asset or the glyph-header include is missing, the builder gives
   a precise error instead of silently building a legacy-looking menu.
3. Run RUN_TEST.bat option 1. The menu window title and bottom-right
   main-menu label both say TEST 87.208. If they do not, you are not
   looking at the newly compiled native XMVSF build.
4. The gameplay path is still the native XMVSF app's zero-ROM-argument
   boot selector, NOT the separate MVC1 capture/ROM playback mode.
   Inspect the title artwork, font tracking, Arcade, Training, Versus,
   Controls, Options, scoreboard and return-to-menu behavior.
5. Verify the unselected text is dim, selected text is bright, and that
   I, i, l, 1, punctuation and word gaps no longer waste full cell widths.
6. New F9 replays, observer/chat diagnostics and replay verification live
   under test/87.208. Legacy test/87.203 capture data is not modified;
   original replay files remain accessible through RUN_REPLAY.bat.

This source audit includes inspection of the tracked code, unchanged font
PNG blob hashes, and explicit version paths. It is not a claim that this
GitHub-only session compiled or ran the native Windows executable.
GitHub Actions are not used.
