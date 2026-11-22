# XMVSF II Palette Studio

Standalone local-browser editor for X-Men vs. Street Fighter's ROM-native character palettes.
This editor lives on the \`palette-editor\` branch and follows the Audio Normalizer's
Windows launcher + Python standard-library + local HTML workflow.

## Launch

Double-click \`palette_editor\LAUNCH.bat\` on Windows, or run:

\`\`\`powershell
py -3 palette_editor\palette_studio_server.py
\`\`\`

A browser window opens on a random port bound to **127.0.0.1 only**.
No Python packages, npm, GitHub Actions, or compile step are required.

## Original palettes

The tool automatically looks for \`xvs.05a\` or \`xmvsf.zip\` in common
\`recomp/rom/xmvsf/\` and \`rom/\` locations. If not found, use **Load ROM source**
in the top-right and select your original \`xvs.05a\` or ZIP. A ROM uploaded
through the browser remains in server memory; it is not written to disk.

Only **CRC32 7DB6025D**, 512 KiB original \`xvs.05a\`, is accepted.
Modified ROMs are deliberately rejected as the immutable source.

Addresses come directly from the program's pointer table at \`0x1145A8\`.
Each of the 19 characters (including Apocalypse and Alpha Chun-Li)
has two sets, **P** and **K**, each with three rows of 16 color words.
Every swatch displays its native 68000 address. Each color uses 4-bit
RGB, represented as \`#RRGGBB\` multiples of 17. Some effects, including
Rogue's, may use fewer rows at runtime; all ROM-defined addresses remain visible.

**Original** is permanently read-only. Click **New** to clone the current
source or selected profile. Adjust the always-visible saturation/value picker,
hue slider, or hex field. The selected swatch updates all three controls.
Press **Save profile** to write a JSON file under
\`palette_editor/profiles/<character>/<id>.json\`. Source hashes protect against
loading a profile with a different original program.

## Sprite loader and preview

The top thumbnail strip lists imported PNG sprites for the selected character,
plus the repository's existing menu portrait as an explicit **reference only**.
The portrait is NOT an idle animation and isn't recolored.

Click **Add sprite PNGs** to attach transparent, cropped **idle** or other
character frames. Imports are copied to \`palette_editor/sprites/<character>/\`;
you can also place PNGs there yourself. Files are never overwritten during import.

When an imported sprite is painted with the game's original 16-color body
palette, every exact source-color match is substituted with the corresponding
profile color, in both the large preview and thumbnail strip.
The studio reports the percentage of visible pixels that matched. Colors that
do not exactly match the source palette remain untouched; this prevents
incorrect recoloring of unrelated artwork. Select **Original P/K** under the
preview to match the uploaded sprite's original palette.

**Current limit:** The palette editor does not yet extract or decode CPS-2
animation frames directly from the graphics ROM. For a real idle preview,
supply an exported transparent idle PNG whose RGB values match the original
character palette. Merely displaying a menu portrait would not satisfy
that requirement, so the editor labels it honestly.

## Export to the recomp

Save a profile, then click **Export .txt**. This downloads a file named after
the character containing \`[P]\` and \`[K]\` sections. Each section has three
lines of sixteen \`#RRGGBB\` colors; a \`-\` line means "use the original".
This matches \`recomp/fight/colours.cpp\`'s existing text loader.

Copy the downloaded \`Character Name.txt\` into the recomp game's
**data-folder \`palettes\` directory**, not the ROM directory. Enable the
**Custom colours** display option. The existing "Load colour files again"
command reloads your changes; the game uses new colors when characters are
next colored. Saving a JSON editor profile alone does not alter the game.

## Tests

\`\`\`powershell
py -3 palette_editor\test_palette_studio.py
\`\`\`

Tests use an entirely synthetic 512 KiB program image (no copyrighted
ROM needed), and check address decoding, quantization, immutable originals,
profile validation, save/load/export, and basic UI/source contracts.

## Safety

- No writes to \`xvs.05a\`, ROM ZIPs, \`recomp/\`, or game palettes.
- User-created files are ignored by git.
- The HTTP server is local-only, with per-launch request authorization.
- No invented colors or addresses when the verified ROM is absent.
