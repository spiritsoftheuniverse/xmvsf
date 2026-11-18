# Native XMVSF menu port — 87.435 (first slice)

This is the beginning of the **real SDL3 application migration**, not a standalone
external launcher. The program displays an Arcade / Training / Versus / Options
menu before the ROM is ticked, using the recomp's existing pixel presenter and
the same SDL3 keyboard/gamepad input loop.

The original SDL2 design is in
`dev/stable/buildtree/src/burner/sdl/xmvsf_stage14_boot.h`.
This slice reproduces the top-level choices and interaction *without* importing
FBNeo's renderer, SDL2 code or build artifacts.

## User-visible behavior

- **ARCADE:** Reset to the original arcade rules, let the native board run.
- **TRAINING:** Enter the recomp's existing working session via
  `cps2recomp::session::StartSession`. Our Training options will be ported next.
- **VERSUS:** Bring up the original local two-player game with free play
  enabled. Both players must use Start; this does **not yet** reproduce our
  dedicated Versus stage/character-selection lifecycle.
- **OPTIONS:** Open the currently functional recomp menu; on exit return
  to our main menu. This is not yet a port of our complete Game / Graphics /
  Controller tabs and does not yet expose remapping in-game.

Menu input: keyboard arrows + Enter/Space, or D-pad + A. Esc / gamepad
menu toggles the XMVSF home screen. F1 while on the home screen opens the
original recomp settings for diagnostics. Escape hatch:
`XMVSF_RECOMP_HOME_OFF=1` disables this frontend to diagnose upstream.

## Build mechanism

Upstream is pinned as the `recomp/` Git submodule. Do **not** edit upstream
in GitHub and expect it to be in this project's branch. Instead, the root
`RECOMP_BUILD.bat` runs `python3 ../recomp_port/apply_menu.py`
while inside `recomp/`. The script adds
`recomp/port/xmvsf_home.h` and applies anchored, idempotent source
changes to `recomp/port/frontend_sdl.cpp`. It refuses unknown upstream
layouts rather than applying an unsafe patch. The affected recomp working
tree will show local modifications; that's deliberate. Nothing touches
`main`, `Stable`, `BUILD_TEST.bat`, or FBNeo.

All recomp source changes are retained as portable sources in
`recomp_port/`; the submodule remains pinned and can be refreshed later.
The build log remains `test/build_logs/RECOMP_BUILD.log`. There are no
GitHub Actions.

## Next slices

1. Port our existing pixel-accurate glyph atlas, logo, animation and menu audio
   (in isolation from SDL2).
2. Port the Game / Graphics / Controller tabs with persistent device bindings.
3. Replace upstream training/versus setup flows with our state machines.
4. Preserve the shared SDL3 window through each transition.

Not verified with a Windows build yet. The batch performs its normal Ninja
compilation; source-only inspection does not establish compile or play success.
