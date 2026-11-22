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

## Portrait + actual combat sprite, side by side

The selected character's **original menu portrait** is always shown on the
left. Beside it is a **genuine Model viewer combat sprite**, recolored by the
current profile. They are different source images: the portrait is **never**
palette-edited and must not be mistaken for a standing game sprite.

### Get the real standing picture

The recomp already exports native transparent character art. No additional
CPS-2 graphics decoder or game build is needed for this workflow.

1. Start the game using `RECOMP_RUN.bat`.
2. Open **Alternative modes → Model viewer**. Choose a character and
   **Standing (you control it)**, then start the viewer.
3. Open the Model viewer's **Export** subpage and choose **Save picture**.
   The recomp writes a transparent `<Character>.png`. With the shipped
   Windows `RECOMP_RUN.bat`, it can land in `recomp/exports/`; with a
   slash-separated player-folder configuration it can land in
   `recomp/work/xmvsf/exports/`. The studio scans **both**. Repeated
   exports may be named `<Character> 2.png`, etc.
4. Return to Palette Studio, select the same character in the left panel,
   and click **Refresh viewer exports**. The native standing sprite appears
   automatically in the upper thumbnail strip and to the right of its matching
   portrait. No manual import or file copying is needed.

Only the exact name for that character, optionally followed by a numeric
re-export suffix, is matched. Animation names (for example `Ryu A0.png`)
are **not** silently mislabeled as standing. The source file is read-only
and is never moved or modified.

If the recomp is using a custom settings directory, set
`XMVSF_MODEL_EXPORT_DIR` to that viewer's exports folder before launching
the studio. Both the `recomp/exports` and
`recomp/work/xmvsf/exports` standard locations are searched automatically.

Alternatively, **Add sprite PNGs** imports transparent idle or other
character frames into `palette_editor/sprites/<character>/`. These are
kept separately from the recomp's Model viewer exports.

### Palette-tinted previews

The editor performs exact RGB pixel matches against the chosen original
Punch/Kick **body palette**. Each matching color is replaced by the selected
profile's corresponding color. This works on both imported native-colour
PNGs and the genuine Model viewer standing export. Unmatched pixels are left
unchanged rather than guessing their source color. The displayed match
percentage makes incorrect source colors visible.

Sprites exported after modifying the game's palette may not match the
read-only original reference: export with the game's **Custom colours OFF**
for the most reliable full-color preview. The thumbnail strip is also tinted
using the active profile.

The editor itself **does not run the game or emulate animations**. It reads
the real sprite PNG that the existing recomp Model viewer already generates.
That preserves exact art instead of substituting an approximate reconstruction.

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
