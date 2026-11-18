# XMVSF native front end — 87.440

The real XMVSF front end now renders in CPS-2 Recomp's **SDL3 native
window and pixel presenter**, not an additional SDL launcher. The preserved
FBNeo reference in `dev/stable/buildtree/src/burner/sdl/xmvsf_stage14_boot.h`
is the source of the composition: a 960x540 logical canvas, original
logo alpha-crop, proportional native SF2 glyphs (large and small),
selected white / unselected dim, centered rows and unchanged menu positions.

**Source art is NOT exported again:** `_art/logo.png`, `_art/logo_II.png`,
`_art/sf2font.png`, `_art/sf2fontsmall.png` and `_art/menuanimation.gif` are
loaded from the repository's existing `_art/` folder at runtime. The
animated GIF retains its frame timing, center-cover crop, 30% brightness,
and 36-second hue rotation. Source images stay untouched; only in-memory
pictures are animated. Fallback text exists if image assets cannot be found.

The menu is shown in the recomp's **existing window**, on startup and via
Esc / gamepad menu input; while shown, no game ticks are executed.

## Current actions

- **ARCADE:** native V2 arcade mode, with its original rules (reset).
- **TRAINING:** starts the existing recomp training session; custom training
  configuration still awaits porting.
- **VERSUS:** initial *local arcade two-player* mode with free play (both
  players join with Start); our complete dedicated versus setup is not yet
  restored.
- **OPTIONS:** currently opens the existing recomp settings as a temporary
  bridge. Native Game / Graphics / Controller UI remains a follow-up.

Navigation uses both keyboard and gamepad: arrows / D-pad, Enter or Space / A.
Esc returns to the home menu, and F1 there opens the original recomp diagnostics.
Use `XMVSF_RECOMP_HOME_OFF=1` when comparing against untouched upstream.

## 87.440 SDL2-style renderer port

The XMVSF game and native menu now use SDL3's accelerated 2D
SDL_Renderer path with the old SDL2 window/rendering policy: nearest
texture sampling, SDL-managed centered 960x540 logical menu, and one
persistent OS window. The custom recomp GPU shader and its linear
edge-blending pass are bypassed for interactive XMVSF.

This does NOT force square pixels or change existing Game/Display
settings, menu layout, artwork, M68000 translation, or window presets.
The native game is still shown with its original FitPicture rectangle.

The port is installed by recomp_port/apply_classic_renderer.py, after
the existing menu installer and before incremental Ninja, and has a
source-only regression under recomp_port/tests/test_classic_renderer.py.
See CLASSIC_RENDERER_87_440.md for details. Set
XMVSF_RECOMP_CLASSIC_OFF=1 at runtime to disable the bridge and restore
upstream behavior for diagnostics.

## 87.439 restoration of original display behavior

87.438 was retracted because its integer-scale/menu downsample changes
altered the intended appearance. **87.439 restores the complete 87.437
drawing/window/presentation source** and changes only the visible version
number. This keeps:

- Original 87.437 physical scaling presets, SDL3 presenter defaults,
  fractional menu fitting, and SDL/VSync behavior.
- The source artwork, PCM menu music, menu movement/selection sounds,
  and controller input.
- The pinned recomp engine, warm Ninja cache, local build scripts, and
  the original FBNeo Stable/TEST builds.

Since `recomp/` is a pinned submodule with locally applied generated
patches, `RECOMP_BUILD.bat` automatically detects and **reverses exactly
the six 87.438 frontend changes** already present in a local checkout
before reapplying the known 87.437 menu integration. An unknown/partly
modified source refuses the rollback without writing a partial file;
there is no `git reset --hard` or removal of ROMs or object caches.

**The rapid compilation is a Ninja/build-cache improvement and is unaffected
by this presentation rollback.** The actual cause of sluggish OS dragging
under SDL3 remains unproven and requires separate profiling rather than
guessing from the size of the source image.

## Window controls (87.437)

While the game or our menu has focus:

- **Ctrl++ / Ctrl+= / Ctrl+keypad plus** advances to the next window-size preset.
- **Ctrl+- / Ctrl+keypad minus** steps to the preceding size.
- **Ctrl+Backspace** toggles maximize/restore without switching to fullscreen.

The six original Stage29 presets remain 384x224 through 2304x1344. Only
the SDL3 window's physical size changes; the game stays in its original
native resolution and the authored 960x540 menu stays fitted/letterboxed.
Scaling keys are ignored while already fullscreen.

## Original menu audio (87.437)

Load WAVs directly from `FBNeo/assets/xmvsf/`, once when the normal native
menu initializes (or the folder named by `XMVSF_MENU_AUDIO`):

- `xmvsfMenu.wav` — loop continuously **only while our home menu is visible**.
- `menu_movement1.wav` — each changed navigation row.
- `menu_selection1.wav` — confirmed menu choice.

One independent SDL3 audio stream converts sources to stereo 48kHz S16
once and mixes with bounded buffering. When a menu selection enters
gameplay, music and queued buffers stop immediately; the selection effect
can finish naturally. No FFNeo sound code and no SDL_mixer dependency.
Missing WAVs are logged and don't prevent gameplay.

## Building and storage

Upstream source is still pinned under the `recomp/` Git submodule. This repo
owns **only** `recomp_port/xmvsf_home.h`, the pinned image decoder under
`recomp_port/vendor/stb_image.h`, and the deterministic
`recomp_port/apply_menu.py` installer. The installer copies those two
headers into the submodule working tree and updates only the known menu
hooks in `recomp/port/frontend_sdl.cpp`; it is idempotent and fails closed
when upstream anchors differ. The upgraded 87.435 menu is handled without
resetting the submodule, so local translated code, ROMs, and build caches remain.

Image decoding uses stb_image v2.32 (upstream nothings/stb, public domain/MIT)
for **PNG and animated GIF**; it is compiled into one translation unit.
No SDL3_image or new runtime DLL is required.

Pull only this branch into your separate worktree:

```powershell
git -C F:\xmvsf-recomp fetch origin feature/recomp-v2-foundation
git -C F:\xmvsf-recomp merge --ff-only FETCH_HEAD
F:\xmvsf-recomp\RECOMP_BUILD.bat
F:\xmvsf-recomp\RECOMP_RUN.bat
```

For a fast patch sanity test (no Windows compilation required):
`python3 recomp_port/tests/check_patch.py`. The actual SDL3 executable must
still be built locally; this commit has not been verified by a Windows build.

No GitHub Actions. No edits to `main`, Stable, FBNeo builds, or ROM binaries.

## Remaining presentation work

Source-authored full intro/reveal timing;
accurate Game/Graphics/Controller tabs with persistence; and our dedicated
Versus/Training lifecycle. These should be ported as **separate functional
modules**, not more version-specific hooks.
