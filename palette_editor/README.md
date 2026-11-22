# XMVSF II Palette Studio

Standalone local-browser editor for X-Men vs. Street Fighter's ROM-native character palettes.
This editor lives on the `palette-editor` branch and follows the Audio Normalizer's
Windows launcher + Python standard-library + local HTML workflow.

## Launch

Double-click `palette_editor\LAUNCH.bat` on Windows, or run:

```powershell
py -3 palette_editor\palette_studio_server.py
```

A browser window opens on a random port bound to **127.0.0.1 only**.
No Python packages, npm, GitHub Actions, or compile step are required.

## If New is disabled or previews are blank

**New is always clickable**. If no ROM source has loaded, clicking New
opens the ROM chooser and continues to profile creation after a successful
source load. The page reports *why* auto-loading failed in the source status,
rather than silently locking every editor control.

The shipped `xvs.05a` stores 68000 words **low byte first**. This editor
now interprets raw file words in the same order as
`recomp/machine/romset.cpp` (which swaps each word when loading the ROM).
The previous byte-order bug incorrectly rejected valid palette pointers.

The combat canvas shows **NO STANDING SPRITE** until the Model viewer has
exported a real transparent combat sprite. That's separate from loading
the palette: ROM palette data alone cannot recreate a complete animated
character from the native graphics. See "Portrait + actual combat sprite,
side by side" below.

After pulling an update, stop the Palette Studio server with Ctrl+C and
restart `LAUNCH.bat`, then refresh the browser.

## Original palettes

The tool automatically looks for `xvs.05a` or `xmvsf.zip` in common
`recomp/rom/xmvsf/` and `rom/` locations. If not found, use **Load ROM source**
in the top-right and select your original `xvs.05a` or ZIP. A ROM uploaded
through the browser remains in server memory; it is not written to disk.

Only **CRC32 7DB6025D**, 512 KiB original `xvs.05a`, is accepted.
Modified ROMs are deliberately rejected as the immutable source.

Addresses come directly from the program's pointer table at `0x1145A8`.
Each of the 19 characters (including Apocalypse and Alpha Chun-Li)
has two sets, **P** and **K**, each with three rows of 16 color words.
Every swatch displays its native 68000 address. Each color uses 4-bit
RGB, represented as `#RRGGBB` multiples of 17. Some effects, including
Rogue's, may use fewer rows at runtime; all ROM-defined addresses remain visible.

**Original** is permanently read-only. Click **New** to clone the current
source or selected profile. Adjust the always-visible saturation/value picker,
hue slider, or hex field. The selected swatch updates all three controls.
Press **Save profile** to write a JSON file under
`palette_editor/profiles/<character>/<id>.json`. Source hashes protect against
loading a profile with a different original program.

## Matched portrait + genuine combat sprite

Palette Studio shows the selected character's original **select portrait**
alongside a native **standing combat sprite**. The source portrait PNG is
always read-only, but its **editable preview now responds to body palette
changes**. Portrait artwork is not natively indexed to fighter ROM palettes,
so this is an explicitly approximate RGB color-delta projection. It assigns
portrait pixels to the nearest original P/K body swatch and projects that
swatch's change while preserving the portrait's original shading and alpha.
Reverting to **Original** displays the untouched portrait. The portrait
file itself is never overwritten.

The combat preview uses the game's **real native Model viewer** and the
game's original graphics. Unlike the portrait, transparent standing sprites
are recolored by **exact** matching against the original P/K body palette;
pixels that do not match remain unchanged. The match percentage is shown.

Only P/K **body palette line 0** is used by a standing pose. The two other
lines are effects (shown when those animations use them). Pens 0 and 15
include transparent/invisible slots and might not change visible artwork.

### Automatic native sprite generation

**This upgrade requires one recompilation** because it adds a dedicated
opt-in headless export command. Run `RECOMP_BUILD.bat` once after pulling.

In Palette Studio, use **Generate selected sprite** or **Generate all 19**.
These buttons launch the local `recomp/build/xmvsf/xmvsf.exe` with
`--headless --palette-sprites N` (or `all`) against your existing
`recomp/rom/xmvsf` ROM set. This starts the existing Model viewer on
each selected character, enters a real fight and uses its own object-only
crop pipeline to save a native standing PNG at **1× CPS-2 pixel scale**.

The generated files are isolated under
`recomp/exports/palette_studio/<Character>.png`. Neither source ROMs nor
your normal Model viewer exports are modified. Separate temporary
Palette Studio EEPROM/settings files ensure the ordinary game save and
custom color settings are not used.

When the native exporter finishes, the editor refreshes the top sprite
strip and paired preview automatically. A missing or unsuccessful native
frame is reported; the studio never substitutes a fake sprite.

The generated output can also be requested from a terminal in the
`recomp` directory:

```powershell
.\build\xmvsf\xmvsf.exe --rom rom/xmvsf --headless --mute --palette-sprites all
```

### Previously exported or imported sprites

The studio still recognizes manually saved, transparent **Standing**
pictures from **Alternative modes → Model viewer → Export → Save picture**
in `recomp/exports/` or `recomp/work/xmvsf/exports/`. It matches only the
exact character name or name with a numeric re-export suffix, never
silently treating animation pictures as standing. Generated images take
priority over manual exports for the same name.

If your custom Model viewer export folder differs, set
`XMVSF_MODEL_EXPORT_DIR` before launching the studio. To attach a custom
transparent idle PNG manually, use **Add sprite PNGs**; the editor copies
these files into `palette_editor/sprites/<character>/` (never overwrites
the source file).

## Export to the recomp

Save a profile, then click **Export .txt**. This downloads a file named after
the character containing `[P]` and `[K]` sections. Each section has three
lines of sixteen `#RRGGBB` colors; a `-` line means "use the original".
This matches `recomp/fight/colours.cpp`'s existing text loader.

Copy the downloaded `Character Name.txt` into the recomp game's
**data-folder `palettes` directory**, not the ROM directory. Enable the
**Custom colours** display option. The existing "Load colour files again"
command reloads your changes; the game uses new colors when characters are
next colored. Saving a JSON editor profile alone does not alter the game.

## Tests

```powershell
py -3 palette_editor\test_palette_studio.py
```

Tests use an entirely synthetic 512 KiB program image (no copyrighted
ROM needed), and check address decoding, quantization, immutable originals,
profile validation, save/load/export, and basic UI/source contracts.

## Safety

- No writes to `xvs.05a`, ROM ZIPs, `recomp/`, or game palettes.
- User-created files are ignored by git.
- The HTTP server is local-only, with per-launch request authorization.
- No invented colors or addresses when the verified ROM is absent.
