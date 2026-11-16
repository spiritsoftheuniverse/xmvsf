# TEST 87.400 — F6 native-size screenshots

Branch: `feature/native-f6-screenshots`, based on the accepted
`feature/hud-shock-reveal` TEST 87.399. No changes to the 87.399
Versus music, 120-game-frame HUD delay, startup concealment, 30-frame
white/blue shock, V3/V4 behavior, ROM, stages, or combat physics.

## Usage

Press **F6** during XMVSF Arcade, Training, Versus, or F9 replay.
One keydown requests **one PNG of the next completely presented game frame**.
Key repeat is ignored. A screenshot requested while paused waits for the
next normal game paint.

`RUN_TEST.bat` creates the output folder and sets
`XMVSF_SCREENSHOT_DIR=%PROJECT%\screenshots`.
The default for a direct executable launch is a local `screenshots/`
directory relative to the process working directory.

Each file is named like:
`XMVSF_native_20261116_082500_f000004278_000001.png`

PNG dimensions: **384 × 224**. Save is lossless. The screen is read
**after the current game's custom-stage painter, native fighter rendering,
separate SDL stun meter, frame-data overlay and on-screen UI**, but
**before SDL_RenderPresent**. The screenshot crops the current game
viewport: not the desktop, title bar, external pane, display gutters or
letterboxing. Capturing the finished game viewport means custom-stage
layers and final HUD are included.

The SDL renderer's actual physical backbuffer is read once on demand,
then the game viewport is sampled back to 384×224. With nearest-neighbor
video filtering, the image represents native-sized pixels. If display
filtering is set to linear, sampled colors reflect that filtering; this
is a native-*size* PNG rather than a separate raw CPS2 sprite dump.
There is no changed game scaling or filtering preference.

The capture path uses a monotonic per-session suffix, current date/time
and the emulated frame number, so repeated screenshots do not overwrite.
Success and failure are printed to stderr as
`XM87.400 F6_NATIVE_PNG_SAVED` or `XM87.400 F6_PNG_ERROR`.
The optional Shift+G DHC chat history receives a status line.

## Build and verification

Build locally using the usual `BUILD_TEST.bat` and run with
`RUN_TEST.bat`; the output is `XMVSF_Test_87_400.exe`.
No GitHub Actions and no Stable overwrite. One additional inexpensive
`grep -aFq` check validates that the compiled TEST EXE contains
`XM87.400 F6_NATIVE_PNG_SAVED`, in addition to the retained 87.399
native HUD marker. This prevents a stale SDL video object from being
published with only a changed title number.

Check F6 on a native forest stage and on a custom stage (e.g. Sigma):
verify 384×224 PNG resolution and complete stage/HUD, confirm filename
changes on another press, and verify F9/F10/F11/F12 still have their
original bindings. No event is injected into the CPS2 game.

Native Windows compile and visual screenshot acceptance have not been
performed in the remote source-editing environment.
