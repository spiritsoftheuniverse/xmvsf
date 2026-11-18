# 87.442 — Native Options / SDL3 input binding implementation

The main menu's OPTIONS entry now opens our own Game / Graphics /
Controller UI. It uses the source-texture renderer from the confirmed
87.441 baseline and preserves the native 960x540 layout. The recomp's
generic Options list is no longer the main navigation destination.

## Game tab

- GAME VERSION : V2 is the correct label for the recomp. No pretend
  V3/V4 selection — the recomp does not implement our original V3/V4 split.
- Tournament Mode, Double Feature and Dizzy Bar are visible but clearly
  marked NOT PORTED and do not pretend to affect the ROM.
- Free Play toggles the registered native feature directly at runtime and
  persists using the recomp's SetFeature.
- Board Difficulty and Game Speed cycle the real registered board-feature
  choices, saved using SetFeatureValue; they take effect when the board
  starts/reset, as the native board.cpp describes.

## Graphics tab

The existing six size presets and Fullscreen Fit / Maximize Window
operate on the actual SDL3 window. Ctrl+plus/minus, Ctrl+Backspace and
Alt+Enter remain unchanged. No changes to the 87.441 rendering path.

## Controller tab

Original action order: LP MP HP LK MK HK 3P 3K COIN START LEFT RIGHT
UP DOWN. Select P1/P2 with left/right; select the first DEVICE/EDIT row
to switch between keyboard and gamepad assignments. It displays the
actual currently connected SDL gamepad's name, or DISCONNECTED.

Highlight an action and press Enter/A to capture. Keyboard scancode,
gamepad button or left/right trigger is written through the recomp's
own Bindings and recomp::Ini to the real settings.ini. Existing settings,
sections, comments, feature toggles, and other players remain intact.
Bindings update live in g_bindings; no restart is needed. An assigned
key is removed from conflicting assignments on any other player, and
duplicate buttons on the same gamepad are cleared.

Escape cancels capture, Delete/Backspace clears a binding. Binding
changes report a write failure rather than falsely appearing saved.
The normal gamepad slots still use the recomp's connected order; durable
GUID-based physical-device assignment/hotplug identity is not implemented
yet. The input editor does not pretend to offer that behavior.

## Navigation and escape hatches

- F1 or main-menu OPTIONS — our native options
- Q/E, Tab/Shift+Tab or LB/RB — Game / Graphics / Controller tabs
- Up/Down — highlight row
- Left/Right on Controller — P1/P2
- Enter/Space/A — activate row
- Backspace/B/Esc — return to main menu
- Shift+F1 from the home menu — existing upstream diagnostic page,
  separated from normal user navigation
- Upstream GPU diagnostic mode still available when explicitly opted in

## Architecture and testing

New files: recomp_port/xmvsf_options.h (state and real data hooks),
recomp_port/apply_options.py (anchored, idempotent installer),
recomp_port/tests/test_options.py. Source-texture presentation is
extended solely with RenderOptions; old 87.441 renderer remains intact.

The new installer runs after the old three installers and before the
incremental Ninja build. No translation refresh, no GitHub Actions,
no Stable or original FBNeo changes. The texturing remains direct
SDL_RenderTexture calls; the old full-menu CPU compositor stays only
as a fallback.

This is a source-validated handoff; the Windows build and input-device
testing remain to be performed on the user's machine.
