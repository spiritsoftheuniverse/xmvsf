# XMVSF native front end — 87.458 (F6 native-resolution screenshot capture)

## 87.458 — F6 capture from every screen

Press **F6** once to save the currently visible menu or game state as a
PNG in the same **screenshots/** folder used by the existing F12 gameplay
capture. The exact path and pixel dimensions are printed in the console.

- **Home / film strip menu:** captures the actual SDL3 compositor to a
  960×540 offscreen target, including the scrolling character panels,
  logo, background, and selections, without window scaling.
- **Game / Graphics / Controller Options:** captures the real options
  renderer at 960×540, including the selected row and active remapping UI.
- **In-game game-settings menu:** saves its original `menuPicture`
  pixel buffer at its generated dimensions (commonly 896×672), with no
  postprocessing or desktop pixels.
- **Gameplay:** uses `Port::Screenshot()` for the true native game
  framebuffer (`ShownWidth() × 224`, ordinarily 384×224).
- **F6** is no longer previous save slot. **F7** is next slot and
  **Shift+F7** is previous; **F12** keeps its gameplay-only capture.

Screenshots do not affect ROMs, game saves, or existing build caches. Home
and Options captures render once into a native offscreen SDL texture on
the requested frame, then display the usual frame normally. The local
`test_native_capture.py` verifies all state hooks before Ninja.

Windows SDL3 rendering still needs a local runtime check; these are source
preflight assertions, not evidence of an executable test pass.

Python installers, version checks, and local build requirements are
specified in [PYTHON_PATCHING_RULES.md](PYTHON_PATCHING_RULES.md).

## 87.457 — 16-pixel gaps and head-safe portrait fitting

Panel spacing now matches the 16-pixel menu side margin. The 260×453
trapezoids advance every 469 pixels. Every character is fitted from its
visible alpha bounds into the film's full-alpha safe interior with eight
pixels of inner padding. Contain-fit avoids the previous cover-crop
that hid character heads against the sloped film edges. The original
film alpha still clips the resulting portrait. Both columns share the
same path; scroll directions are unchanged.

## 87.456 — separate film and portrait layers; 96-pixel gaps

The 260×453 film pieces now repeat every 549 vertical pixels, leaving a
full 96-pixel clear margin between consecutive shapes. The red and blue
trapezoid pixels render first; portraits then render as separately masked,
nearest-neighbor SDL textures. The mask is sourced directly from each film
PNG's alpha, and portraits begin 78 pixels below the top of the complete
source panel. Original character RGBA channel order is preserved. X-Men
scroll downward on the left and Street Fighter upward on the right.
Only the local renderer and film preflight changed; no ROM or build-cache
changes are required.

## 87.455 — initial looping menu film

The left X-Men and right Street Fighter trapezoid films are prepared
from `_art/menu` once per SDL renderer, then scrolled independently.
Feature preflights run before Ninja; a passing source test is not a
substitute for an actual Windows build and in-game inspection.

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

## 87.454 — Roman II uses the main logo's source-pixel pulse
The final `_art/logo_II.png` is now recolored using the same
`XMMenuColor::logoPalettePulse` routine and SDL tick clock as the main
title. It keeps immutable original RGBA pixels, updates the streaming
texture at most once per 80 ms, and preserves the already-established
nearest-neighbor sampling. Stroke A/B/C/D reveal geometry, masks and
spark particles are unchanged. The source preflight guards the new
palette call, renderer clock, texture upload and cleanup.

## 87.453 — nearest-neighbor filtering for all II textures
The final II numeral and all four stroke textures now explicitly use
`SDL_SCALEMODE_NEAREST` in their shared SDL3 upload helper, matching the
main logo pixel filter. The logo preflight checks this exact helper and both
callers, so a future regression fails before compilation.

## 87.452 — original Stage 87.235 logo reveal and ongoing palette pulsation

The native recomp startup now uses the **original SDL2/FBNeo source timing**,
adapted to existing SDL3 textures and the current one-stream menu mixer. All
artwork remains in `_art/` and both sound cues remain in `audio/`; no
ROMs, generated M68k files, or upstream submodule commits are changed.

| Cold-boot time | Source behavior |
| --- | --- |
| 0 ms | White/spectral background strobe; play `crash.wav` and `xmvsf_welcome.wav` together |
| 1,000 ms | Base title appears and resolves from white/hue dither over 1,000 ms |
| 2,000 ms | Four source-aligned `logo_II_A/B/C/D.png` strokes, 10 frames each at 60 Hz, with tip sparks |
| 2,667 ms | Finished II logo, original menu music starts, SF2 font menu reveals over 500 ms |
| 3,167 ms | All menu inputs unlocked; steady animated backdrop and logo effects remain |

After the reveal, the **original color-selective pulses persist**: orange,
yellow, and green breathing; purple source-pixel vapor/dither and short
vibration bursts affecting the VS center and extended shadow. This replaces
the recomp's former uniform brightness sine wave. All color transformations
start from original decoded PNGs; they never accumulate over prior effects.

The reveal runs **once on cold boot**, not every time the player returns from
combat. The original authored II stroke exports share the 400×300 pixel
canvas and retain their direction and alignment. Recomp GPU textures and
the original Settings/Controller page, gameplay logic, and input
configuration remain independent. The CPU fallback remains available as
before, but the complete effects run through the normal accelerated renderer.

Source ports: `xmvsf_intro_timeline.h`, `xmvsf_palette_fx.h`,
`xmvsf_ii_strikes.h`, plus `xmvsf_menu_audio.h` and
`xmvsf_menu_textures.h`. `apply_menu.py` copies the new headers to
the local `recomp/port` during normal builds. The build script runs
`tests/test_logo_intro.py` **before Ninja**, checking assets, SDL3 source
hooks, compiled C++ timing assertions, and helper installation. This is
a local preflight, **not evidence that the Windows SDL3 binary or audio
has already been tested**. Continue using `RECOMP_BUILD.bat` (warm
incremental Ninja; no `refresh` needed for these source-only edits)
then `RECOMP_RUN.bat`.

## 87.451 Correct the capture activator/deactivator state

Selecting a Controller binding with A or Enter used to set capture=true
and ignore EVERY input for a fixed 180 ms. A legitimate button pressed
quickly was discarded forever, with no timeout, no controller cancellation
and no interruption recovery. The old SDL2 Stage14 version instead used
a release gate between selection and capture.

87.451 adds a small, separately C++-tested capture lifecycle:
- Capture records the actual held Enter, Space, mapped gamepad A, or raw
  joystick confirm button that opened assignment, and ignores only that
  physical initiating press. A DIFFERENT button/key can be assigned
  immediately; no 180 ms discard window.
- Release of the activating key/button makes A itself bindable on the
  NEXT press. Residual analog/hat movement is ignored until A releases.
- Escape, or controller Guide when SDL delivers it, cancels capture.
  Gamepad B remains assignable as an attack button; it is NOT stolen
  as a cancel control.
- Capture times out after 12 seconds. Loss of window focus or physical
  device removal cancels capture instead of leaving it stranded.
- Save success, save failure and cancellation transition to a menu
  rearm phase. Confirm/Back must be released to navigate again; even
  an SDL driver reporting Confirm held forever cannot lock the page
  longer than 1.5 seconds. The original menu one-press edge gate stays.
- Additional SDL joystick and semantic gamepad events for a single
  physical press are suppressed while rearming. Unsupported semantic
  buttons report a message rather than silently appearing stuck.

`tests/test_capture_lifecycle.py` executes a real tiny C++ test using
the MSYS2 UCRT64 g++ compiler before the controller patch or Ninja build,
with checks for quick presses, release/repress, timeout, cancel,
stuck-confirm recovery and resetting on tab/menu transitions.
The new lifecycle header is installed by the same idempotent Options
installer; no extra source-translator run is needed.

The 87.441 GPU menu renderer and 87.447 audio path are untouched.

## 87.450 Repair Python regex and execute the installer before writing

The 87.449 idempotence repair introduced an unbalanced Python regex:
`r"gamepads(\\s*\\)\\s*;"`. It opened a capture group and never
closed it, so `re.compile` raised `unterminated subpattern at position
177` before Ninja could start. The exact fixed capture is
`r"gamepads(\\s*\\)\\s*;)"`. It preserves group 1 (the call's
argument prefix) and group 2 (the closing parenthesis/semicolon).

The real regex has been executed with Python against old/new
`homeTextures.RenderOptions` calls, including repeat matching and
unrelated call tails.

**Build change:** `tests/test_pad_owners_preflight.py` now runs
before `apply_pad_owners.py` and reads the already generated
`recomp/port/frontend_sdl.cpp`. It executes the real controller
patch in memory, then executes it again against its own output.
Both calls must succeed and the second must be identical.
No controller-file writes occur until this preflight passes.

The 87.441 source-texture renderer, 87.447 audio fix and Stage13
controller input behavior are unchanged. The first Windows compilation
still needs to occur locally; Python regex/preflight verification
does not constitute a successful C++ build.

## 87.449 Make the controller installer safe on repeat builds

The 87.448 build successfully installed the final Stage13 input capture
frontend, but tests/test_pad_owners.py then called the installer a
second time to verify idempotence. The original 87.443 gameplay-loop
replacement no longer appeared because the later 87.448 physical
routing upgrade had already transformed it. The installer incorrectly
rejected this fully installed state with "route native game actions by
owned controller ... found 0", stopping before Ninja.

The installer now validates the **final** 87.448 hooks first and
returns the source unchanged when already installed. Only the uniquely
identified Options RenderOptions call can be reconciled in that state.
The historical patch path remains available for first installations and
older frontend upgrades. Incomplete or ambiguous source fails closed.

The regression test now includes a complete-final-source fixture,
a second invocation, a missing-marker fixture, and the existing
duplicate-render-tail fixture. This is a source-only change, retaining
the 87.441 renderer, 87.447 audio fix, saved controller profiles, and
warm incremental compilation. No submodule reset or cache clearing.

## 87.448 Restore the original SDL2 Stage13 controller assignment UX

The temporary 87.442/87.443 device-cycling and EDIT INPUT switch are
removed. Source of truth:
`dev/stable/buildtree/src/burner/sdl/xmvsf_stage13_controls.h`
and `xmvsf_stage14_boot.h` (Controller tab, capture, graphics/layout).

**Device row (row zero):** press A/Enter to arm capture; press a key
to assign the original P1/P2 keyboard profile, or press a physical
button on any connected stick/gamepad to assign that exact SDL instance.
If the device was assigned to the other player, the whole player
configuration and its customized physical binding profile are swapped.
No cycling between KEYBOARD and GAMEPAD modes.

**Action rows:** after assignment press A/Enter on LP, MP, HP, LK,
MK, HK, 3P, 3K, COIN or START to capture the next key, physical
button or positive joystick axis. Keyboard mode exposes 14 actions,
including LEFT, RIGHT, UP and DOWN; controller mode exposes the first
10, with directional input taken from its stick/hat. The raw physical
index is displayed as BUTTON N, AXIS N or KEY name, not a semantic
Xbox button label. The original control column is dynamically centered,
with 12 native pixels between the device caption and first input.

Keyboard defaults are copied from Stage13:
P1 = J K L / U I O / H Y / 5 Enter / A D W S.
P2 = KP1 KP2 KP3 / KP4 KP5 KP6 / KP7 KP8 / KP0 KP Enter /
Left Right Up Down.

Both mapped SDL3 gamepads and unmapped raw sticks use physical button
bindings (SDL_GetGamepadBindings resolves a mapped pad to joystick
indices); existing 87.443 semantic profiles migrate when available.
Persisted GUID, name, ordinal, serial (if exposed) and saved per-device
action profiles remain in the existing recomp settings.ini. Reconnection
uses live SDL instance first and GUID/name fallback. The SDL3 game input
loop now avoids mixing the generic gamepad/keyboard mappings with an
explicitly assigned controller's physical profile.

The stable 87.441 SDL source-texture renderer, 87.447 single menu audio
stream, and 87.443 one-press menu gating are unchanged.

## 87.447 Fix buzzing on entry into Options

The historical installers could leave **both** audio visibility updates in
the same frontend frame:

- 87.437: `homeAudio.Step(home.Visible())`, false when Options is open
- 87.442: `homeAudio.Step(home.Visible() || options.Active())`, true in Options

Since `MenuAudio::Step` clears its SDL audio queue and rewinds playback on
a visibility transition, the simultaneous false/true calls repeatedly
restart the music and produce an audible buzz when opening Options.

The 87.447 Options installer now removes only the old **stamped** 87.437
statement, collapses duplicate 87.442 stream updates, and preserves exactly
one 87.442 update in the active menu loop. It also handles an existing
87.443 controller patch without rebuilding the pinned submodule from scratch.
The pre-build test verifies both synthetic conflicting hook scenarios and
the actual generated frontend contains one 87.442 update and zero 87.437
updates. The 87.441 source-texture renderer, menu WAV files and 87.443
controller assignment logic are unchanged.

Expected repair diagnostic (when historical hooks are present):
`XMVSF 87.447: removed N conflicting legacy audio steps ...`

No translation refresh or GitHub Actions; incremental Ninja compilation
follows the existing source-only checks.

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

The Controller tab uses press-to-capture on the single DEVICE row, then
10 physical actions (or 14 Keyboard actions). Each P1/P2 slot can select
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
