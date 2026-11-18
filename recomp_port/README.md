# XMVSF native front end — 87.446 (idempotent Options recovery)

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
- **OPTIONS:** opens our native Game / Graphics / Controller tabs, drawn
  with the 87.441 source-texture renderer. Supported settings and controls
  update real recomp features, windows and input bindings. Unported legacy
  features are clearly inactive, not fake toggles.

Navigation uses both keyboard and gamepad: arrows / D-pad, Enter or Space / A.
Esc returns to the home menu, and F1 there opens our Options. Shift+F1 opens the original recomp diagnostics.
Use `XMVSF_RECOMP_HOME_OFF=1` when comparing against untouched upstream.

## 87.446 recover repeated Options/audio hooks

Repeated failed 87.443–87.445 builds exposed an installer lifecycle bug:
the 87.437 upgrade kept reinserting its earlier audio-step hook after
87.442 had already replaced that hook. The 87.442 installer then inserted
another complete native Options draw block before the unchanged home
rendering block. Repeated runs could accumulate six copies.

The 87.446 build now:
- Suppresses the old 87.437 upgrade when 87.442 Options already exists.
- Makes the 87.442 Options installer skip reinsertion when its installed
  include, controller and drawing hooks are already present.
- Recovers *only* complete identical 87.442-generated audio/draw blocks
  to one, with a safe alternative for interleaved generated audio hooks.
- Refuses cleanup if any duplicate Options block differs from the known
  emitted source. No Git reset, submodule rewrite, cache deletion, or
  change to the confirmed 87.441 source-texture presentation.
- Validates six-copy recovery, interleaved audio, source protection, and
  second-run idempotence via tests/test_options.py before local Ninja.

The 87.443 ownership installer should now find exactly one active
Options rendering invocation. The actual Windows compilation and hardware
capture still require verification on the user's system.

## 87.443 strict input edges and physical controller profiles

No hold-repeat in the home menu, Options, or upstream diagnostics. One
key/button press moves one row, then requires release before the next
move. The native gameplay button-hold and charge logic are unchanged.

The Controller tab now has separate DEVICE ASSIGNMENT and EDIT INPUT
rows, followed by all 14 original actions. Each P1/P2 slot can select
a particular connected controller by SDL instance ID and save its GUID,
serial when available, ordinal, and the actual Windows/SDL joystick
name. Reconnecting restores that controller's saved action profile
without silently shifting another controller into its player slot.

The name displayed is obtained from SDL_GetJoystickName first, as in
our working SDL2 setup (so SDL's reported Fighter PI appears as such).
Unmapped raw joysticks can be explicitly assigned and their raw
buttons, hats and axes captured. Standard mapped gamepads retain the
recomp's per-button and trigger handling. Unclaimed devices still
support additional P3/P4 recomp co-op roles.

The device identity and mapping profiles are persisted through
recomp::Ini in the existing work/xmvsf/settings.ini; existing keyboard
and gamepad mappings stay valid. Identical GUID devices are protected
by retaining both live player instances before reconnect matching.

See PAD_OWNERS_87_443.md for implementation, exact profile rules and
limits. The source patch and regression checks are committed but the
Windows build and physical Fighter PI detection must still be tested.

## 87.442 native Game / Graphics / Controller Options

The old generic recomp menu has been removed from the **normal Options
navigation**. The in-game screen now uses the original tab organization,
navigation conventions and SF2 bitmap fonts on the existing accelerated
960x540 renderer. Press F1 or choose OPTIONS; use Q/E, Tab/Shift+Tab
or LB/RB to cycle tabs. Esc/B returns to the home menu.

- **GAME:** GAME VERSION displays V2; Tournament, Double Feature and
  Dizzy Bar are visibly NOT PORTED and have no fake implementation.
  Free Play uses the real SetFeature hook. Difficulty and game speed
  use the native board features and persist until next board reset.
- **GRAPHICS:** six original size presets, fullscreen fit and maximize
  operate on the actual SDL3 window; Ctrl sizing shortcuts remain.
- **CONTROLLER:** P1/P2 selection, keyboard/gamepad editing, original
  LP/MP/HP/LK/MK/HK/3P/3K/COIN/START/LEFT/RIGHT/UP/DOWN order, direct
  SDL scancode/gamepad button/trigger capture, live g_bindings updates,
  conflict cleanup and persistent settings.ini through recomp::Ini.
  The connected SDL gamepad names appear where available. Physical pad
  ownership by durable GUID is not yet implemented; the recomp still
  assigns pads by connected order.

Only Shift+F1 from the home menu retains the upstream settings/diagnostic
view for engineering purposes. See OPTIONS_87_442.md for exact hooks,
limits and controls. No changes to 87.441's hardware renderer or original
Stable and main branches.

## 87.441 true SDL source-texture menu

Our older 87.435-87.440 recomp menu was CPU-composited: a 960x540
software buffer was rebuilt every presentation frame, including the
GIF, logo, and font glyphs, then uploaded as one image. The working
SDL2 menu instead draws separate textures directly through an accelerated
renderer. Version 87.441 now follows that original behavior through SDL3,
using the same PNG/GIF assets and 960x540 logical canvas.

The new compositor creates a texture for the logo, II, two font atlases,
and GIF backdrop; only the GIF's changed source frame/hue is uploaded.
No full-menu CPU compositor is used normally. On Windows, Direct3D11
is preferred for the SDL 2D backend, with a fallback to SDL default.
The native gameplay video path is unchanged.

On shutdown, menu textures and the menu WAV mixer are explicitly
released before the renderer/window and SDL_Quit, fixing the old
resource-lifetime mismatch.

Diagnostic switches: XMVSF_MENU_CPU_FALLBACK=1 for old CPU menu, or
XMVSF_RECOMP_CLASSIC_OFF=1 for upstream GPU output. Neither needs a
recompile. See MENU_TEXTURES_87_441.md.

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
durable pad-identity assignment, unported mode toggles, and our dedicated
Versus/Training lifecycle. These should be ported as **separate functional
modules**, not more version-specific hooks.
