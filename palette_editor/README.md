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

## Windows native exporter DLLs

Palette Studio starts the **same MinGW/UCRT64** `xmvsf.exe` that
`RECOMP_RUN.bat` starts. On Windows, `LAUNCH.bat` and the Python
sprite-generation subprocess now prepend the matching MSYS2 UCRT64 runtime
directory to `PATH`, including `libgcc_s_seh-1.dll`. The change affects
the Palette Studio process only; it does not copy or redistribute any DLL.

By default, the UCRT64 prefix is `C:\msys64\ucrt64`, matching
`RECOMP_RUN.bat`. If MSYS2 is installed somewhere else, set
`XMVSF_UCRT64_ROOT` to the UCRT64 prefix before launching the studio.
For example:

```powershell
$env:XMVSF_UCRT64_ROOT = 'D:\msys64\ucrt64'
& 'F:\xmvsf-recomp\palette_editor\LAUNCH.bat'
```

If `libgcc_s_seh-1.dll` is genuinely absent from the UCRT64 `bin`
directory, install or repair the GCC runtime via **MSYS2 UCRT64**:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc-libs
```

A missing DLL is reported in Palette Studio's capture status **before**
starting the executable, instead of producing a Windows modal crash.
This PATH fix does **not** require recompiling the game.

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

The **profile dropdown** always begins with two separate original colors:
**Original P · read only** and **Original K · read only**, in that order.
They contain the ROM's two actual original fighter palettes, not two copies
of the same color. Selecting either changes the swatches, native addresses,
and sprite preview without changing any ROM file.

Click **New** to create an editable custom palette from the displayed
original (P or K), or from the currently edited profile. A custom palette
has one color scheme; edits are mirrored to its native P/K export banks.
The old P/K editing buttons and the **Sprite source colors** dropdown are
removed because custom palettes have no separate punch/kick editing modes.
Adjust the always-visible color square, hue slider and hexadecimal input.

Click **Save profile** to save the named JSON profile under
`palette_editor/profiles/<character>/<id>.json`; source hashes protect
against loading it under another original ROM.

## Character palette profiles strip

The former **Combat sprite loaders** strip is now a row of **colored
palette-profile thumbnails**. Each entry shows the *same real native
standing sprite* recolored independently for that entry's palette.
The first two entries are the immutable original P/K choices; each saved
custom profile gets its own named sprite thumbnail. A new unsaved draft
appears in the strip as well. Clicking a thumbnail **selects that palette**
and updates the editor, swatches, portrait, and main combat preview.
Clicking a thumbnail does **not** change the native animation.

The thumbnail art comes from the local recomp's native Model viewer, never
an invented, stylized or approximated fighter sprite. Before a genuine
sprite is generated, each entry visibly says **Generate 3 examples**;
the app does not pretend to have captured artwork it does not have.

Portrait art remains a separate read-only source PNG. Its *preview*
uses an approximate RGB-delta projection to reflect palette changes;
unlike native sprite pixels, the menu portrait is not indexed by the
same fighter palette slots. Reverting to Original P displays the
unaltered portrait.

## Generate Body, Effect A and Effect B examples

**One native recompilation is required** for this feature: run
`RECOMP_BUILD.bat` once after pulling.

Press **Generate 3 examples** for the currently selected character.
This runs the compiled game locally and headlessly using the real Model
viewer, with a separate Palette Studio EEPROM/settings file. The three
intended outputs are:

- **Body / Character:** native standing fighter, with the first palette
  line, at 1× CPS-2 pixel scale
- **Effect A:** a native animation frame using the second palette line
- **Effect B:** a native animation frame using the third palette line

Effect A and Effect B are determined by the existing native color-editor
animation scan. They are only saved when a real frame uses that palette
line. If no qualifying animation is found, the app reports that the
example is unavailable rather than displaying a mislabeled standing
sprite. Different fighters may have different usable effect frames.

The files appear beneath `recomp/exports/palette_studio/`, such as
`Ryu.png`, `Ryu Effect A.png` and `Ryu Effect B.png`.
The source ROM, normal gameplay, and manual Model viewer exports are
left intact. The main combat preview has three **example tabs** beneath
it; these change the previewed native frame while keeping the selected
palette profile unchanged.

From inside the `recomp` directory you can also request the three
examples directly for one character index (0–18):

```powershell
.\build\xmvsf\xmvsf.exe --rom rom/xmvsf --headless --mute --palette-sprites 9
```

This example selects Ryu. The Palette Studio button uses the currently
selected fighter's index automatically.

Existing manual standing PNG exports in `recomp/exports/` and
`recomp/work/xmvsf/exports/` remain readable. Exact RGB matches against
the original palette colors are replaced with the selected profile's
corresponding colors (including effect lines); pixels without a known
original match remain unchanged. The editor does not fabricate missing
native artwork.

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

- No writes to `xvs.05a`, ROM ZIPs, or the game's palettes. Native example generation writes only under `recomp/exports/palette_studio/` and uses separate Palette Studio settings.
- User-created files are ignored by git.
- The HTTP server is local-only, with per-launch request authorization.
- No invented colors or addresses when the verified ROM is absent.
