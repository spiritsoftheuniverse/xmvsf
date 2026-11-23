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

The **profile dropdown** starts with the two native ROM references
**Original P · read only** and **Original K · read only**. Their CPS-2
palette colors and names remain locked, but **their character-select
display swatches are editable independently**. Both initially receive a
visible marker derived from a body-palette color. Select either original,
change only the dedicated **Character-select color swatch** field under
the name, and press **Save All**. This saves that display-color override
into the shared nine-slot catalog; changing it never touches the ROM or
recolors the original sprite. P and K may have entirely different markers.

Select **New** to create a custom scheme. Under **Custom palette name**
in the right inspector is a separate **Character-select color swatch**:
a large color picker and exact `#RRGGBB` field. This marker is used to
identify the palette in the planned vertical nine-swatch selector.
It is independent of the CPS-2 color pens and supports full 24-bit RGB;
changing it never changes the character sprite. You may choose a
deliberately prominent marker without altering combat artwork.

### Quick color match by dragging

With a **custom palette** selected, drag any color swatch from the
three native palette rows onto another swatch. The destination adopts
the **source swatch's exact RGB color**, while keeping its own palette
slot, index, and ROM address. The source color stays unchanged.
This works across rows, not only within one row. As with ordinary
editing, the custom profile's P/K export banks are mirrored and the
change is staged for **Save All**. Hovering a destination highlights
it; the destination becomes the selected swatch after dropping.

The **Original P** and **Original K** native palette colors remain
read-only: they cannot be dragged or overwritten. Their *separate*
character-select marker colors are still independently editable.
Dragging a swatch doesn't modify the original ROM.

There are exactly **nine selection slots per character**:
Original P, Original K, and up to seven custom palettes. Palette Studio
numbers the profile strip 1–9 and displays each custom palette's marker
above its colored sprite thumbnail. Empty slots are visible.

**Save All** replaces both the old Save profile and Export .txt
buttons. You can edit or rename one character's palette, switch to other
profiles and other characters, create more drafts, and keep working:
all changed profiles stay staged in memory. The header shows the
number of pending profiles. One **Save All** writes every pending
profile's JSON file and publishes the complete nine-slot catalog for
**all 19 characters** to:

`recomp/work/xmvsf/palettes/selection_slots.json`

This includes each slot's display swatch, profile ID, name and full
three-row native palette color data. Slot IDs remain fixed when you
rename profiles, so X/Y selection order does not depend on sorting names.
An empty slot is represented by `null`.

**Important:** The palette catalog is now generated, but the game's
native character-select X/Y input handler and per-fighter palette
application have **not** been connected yet. Save All prepares the
data for that integration; it does not yet change gameplay.
Original ROM files are never modified.

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

## Combat sprite zoom and pixel-to-swatch picker

The main **combat sprite preview** now has **−**, **+**, and **Fit**
controls. Fit uses the largest whole-number enlargement that will fit
inside the viewport (up to 5×); +/− select explicit integer scales from
1× to 12×. Pixel edges remain nearest-neighbor and square at every
size. If the enlarged sprite exceeds the preview box, **scroll within
the preview** to inspect the remaining pixels. Choosing any of the
native Body, Palette row 2, or Palette row 3 examples uses the same
zoom controls; portrait and palette-profile thumbnails remain compact.

**Click any nontransparent pixel on the main combat sprite** to select
its matching native body/secondary palette swatch in the color editor.
The picker samples the **unmodified source sprite image**, not its
recolored preview, so changing a custom palette does not break
subsequent picking. The original ROM's P/K palette RGB values are
matched **exactly**; no nearest-color guesses or automatic edits are
made. Clicking selects an existing palette row and pen, updates its
highlight and the inspector, and leaves the chosen profile unchanged.

Some ROM palette entries have **identical RGB values**. A flattened
PNG cannot tell which of those indices a given pixel came from; in
that case, the picker retains the currently selected matching entry
or picks the first matching entry (preferring the previewed native
palette row), and explicitly reports the ambiguity. Transparent
pixels and colors absent from the source palette are ignored.

## Native multi-frame palette-row discovery

**Palette row 2 and row 3 each have an independent ◀ / ▶ cycler** in
their sample windows. The counter shows the current image and the number
of discovered examples; selecting one opens that frame in the main
zoomable, pixel-clickable combat preview.

Previously the scan returned one animation with the highest object count
per palette row. That often selected unhelpful stationary claws. The
new generator preserves *every qualifying animation* found by the native
model viewer's ROM-frame scan, distributes sampling across animation
groups (not just early normal attacks), and captures up to **24 unique
native PNG frames per secondary row**. Frames are de-duplicated by
source-native image pixels. Each animation contributes at most one
example per sampling pass, so a single move cannot fill the row gallery.
A repeated claw pose is skipped; distinct objects and smear silhouettes
can appear even when their animations use the same palette row.

The Row 2 PNGs are `Wolverine Row 2.png` (first),
`Wolverine Row 2 02.png`, `Wolverine Row 2 03.png` and so forth.
Row 3 uses the same numbering. Regenerating explicitly replaces
previous numbered samples for that fighter alone, and the browser
discovers them in numeric order. This adds discovery data only: **it
does not label an animation as Berserker Barrage or Cyclops' kick
without verified native move/frame identification**. If Wolverine's
barrage smear uses a different sprite palette slot from Row 2, it
won't appear in Row 2 even after this scan; finding it requires
following the relevant native object/animation data across rows.

The new native multi-frame exporter requires running
`RECOMP_BUILD.bat` once after pulling. Palette Studio itself does
not need separate compilation.

## Generate genuine Body and secondary palette-row samples

**Requires one recompilation** because the headless Model viewer now filters
CPS-2 sprite objects by their **actual native OAM palette-slot attributes**.
Run `RECOMP_BUILD.bat` once after pulling this update.

The old **Effect A / Effect B** naming was not justified. Those were just
the second and third native *palette rows*. A row may be used by body tiles,
weapons, highlights, projectile visuals or other objects. It does **not**
identify a semantic effect, attack, or projectile by itself.

**Generate 3 examples** asks the native viewer for:

- **Body / Character** — a real standing character model at 1× pixel scale
- **Palette row 2** — only native CPS-2 sprite objects whose low five
  OAM attribute bits select this character's second original color row
- **Palette row 3** — only native sprite objects selecting the third row

The color editor's built-in native animation scan now retains
**multiple animations across groups** for each secondary row. Crucially,
the exporter re-renders **only the matching OAM objects**, instead of
cropping the entire standing or attacking fighter as it did before. The target row's color ink is kept at source resolution
and detached objects are included even when they would appear behind the
body in the fully composited pose.

A successful palette-row export proves that some *native objects* use that
color row. It does **not** prove that those objects are VFX or separate
projectiles. If the selected row colors a piece of the character, that
piece is what the preview should show. If no actual matching pixels are
visible, no row sample is substituted or generated.

The corrected sample files appear in
`recomp/exports/palette_studio/` as `Ryu.png`, `Ryu Row 2.png`
and `Ryu Row 3.png`. Old `Ryu Effect A.png` / `Ryu Effect B.png`
full-fighter exports are ignored by discovery and deleted for that
character when the user explicitly regenerates their samples. Other
characters' files and normal Model viewer exports are preserved.

The program uses an isolated Palette Studio settings and EEPROM file;
neither original ROMs nor gameplay palettes are modified. The preview
has three sample selectors below the main character display. The selected
custom palette persists while switching these samples.

You can also invoke this capture from the `recomp` directory:

```powershell
.\build\xmvsf\xmvsf.exe --rom rom/xmvsf --headless --mute --palette-sprites 9
```

Character index 9 is Ryu; the Palette Studio button passes the selected
fighter's index automatically.

## Saving and game integration

The previously existing per-character `.txt` exporter is retained as
a low-level compatibility endpoint, but it is no longer an editor control.
The recomp's existing text-based `custom_colours` feature applies only
one active character color configuration; it cannot implement the planned
nine selectable variants or independent P1/P2 combinations.

The new single manifest is intended for a dedicated runtime loader that
will read nine indexed slots for each fighter, then let each player's
character-select **X/Y** input choose a slot. The active slot must be
applied to the appropriate fighter's live native palette buffer, including
the partner on tag, independently of another player using the same
character. That native feature is still a separate implementation.

Do not copy a profile into the old single-palette `Character.txt`
file expecting nine selectable choices: they have different purposes.

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
