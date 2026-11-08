# TEST 87.234 — centered SDL boot canvas from the first presented frame

## Regression

The persistent SDL window was correctly centered on the desktop in TEST
87.226, but its *renderer viewport* sometimes stayed pinned to the upper-left
corner until launching gameplay or maximizing the window. These are different
coordinate systems.

The native boot selector and the video game's renderer-to-menu handoff both
called `SDL_RenderSetLogicalSize(renderer,960,540)` and **then**
`SDL_RenderSetViewport(renderer,nullptr)`. SDL's logical-size call computes
the aspect-fit/letterbox viewport from the actual renderer output size.
Resetting the viewport afterwards discarded that placement.

This was most visible with saved window geometries that do not match the
menu's 16:9 `960x540` aspect ratio. The gameplay initializer separately
called `AdjustImageSize()` and reestablished its own centered source
rectangle, explaining why entering gameplay appeared to repair alignment.

## Fix

`xm8652CenteredBootCanvas()` in `xmvsf_stage86_52_window_state.h` is the
single shared presentation reset for the boot and the game-to-menu handoff:

1. Drop any previous game render target (keep the live SDL renderer/window).
2. Clear previous logical size, integer-scale, viewport and clip state.
3. **As the final viewport-affecting call**, apply
   `SDL_RenderSetLogicalSize(renderer,960,540)`; let SDL calculate and center
   the viewport for the window's physical output dimensions.
4. Print one `XM87.234 BOOT_VIEWPORT_CENTER` line with the physical output,
   requested logical canvas, resulting viewport and whether SDL accepted it.

The physical window remains centered only ONCE on first creation through the
existing `xm8652CenterInitialWindow(window)`, before computing the logical
viewport. The user-positioned window is never forcibly moved on game-to-menu
return. SDL software-renderer fallbacks still apply logical centering even
if a NULL texture target reset is not supported.

The original native gameplay `AdjustImageSize()` and CPS2 framebuffer
placement remain unchanged, as do stun bars, flying-screen/dash behavior,
F9 replay, DF final rendering and Wily compositing.

## Verification and merge boundary

- New version-specific scripts: `BUILD_TEST.bat`, `RUN_TEST.bat`,
  `RUN_REPLAY.bat` -> `runtime/stable/XMVSF_Test_87_234.exe`.
- `dev/dhc/tests/test_boot_renderer_viewport_234.py` checks the actual
  source call order, boot-before-first-present, persistent game-to-menu reset,
  untouched game viewport and aspect-fit geometry for restored, 4:3, ultrawide
  and windowed output sizes. Wired before native build.
- All original 87.231, 87.232 and 87.233 FS/dash C++ tests stay wired.
- **Source-level checks passed; native SDL/Windows first-frame rendering is
  not verified here.** Observe `XM87.234 BOOT_VIEWPORT_CENTER` at
  `menu-first-frame` and `menu-return`, and visually confirm that a
  non-16:9 restored window is centered *before* entering a match.
- No GitHub Actions, main merge, Stable overwrite or old TEST replacement.
