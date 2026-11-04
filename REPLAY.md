# XMVSF native input replay — permanent TEST launchers

> **Important — legacy F9 limitation:** The existing 87.61/87.163
> movie contains one initial emulator state and CPS-2 input frames only.
> The real XMVSF application has a custom boot menu, Training controls,
> mode changes, and native state reloads outside the ROM input stream.
> **RUN_REPLAY.bat currently exercises one continuous native game segment,
> not a verified replay of the full 87.161 multi-roster session.**
> For the complete solution and exact unimplemented requirements, see
> [host-aware replay v2](dev/dhc/REPLAY_V2_HOST_TRANSITIONS.md).
> Do not rely on `PLAY_END` as proof that all original host transitions
> or all character-specific super moves were recreated.

**Keep `RUN_REPLAY.bat` in the repository root permanently.** It is
part of the native Windows development workflow, alongside the unchanged
root `BUILD_TEST.bat` and `RUN_TEST.bat` names. Do not remove it when
incrementing TEST point versions or replace it with a one-off launcher.
It must always launch the current TEST executable and never fall back
to or overwrite `XMVSF_Stable.exe`.

## The 87.163 fix: why the previous launcher did nothing

The earlier `RUN_REPLAY.bat` invoked `XMVSF_Test_87_162.exe` without
a ROM argument. `src/burner/sdl/main.cpp` consequently opened its
interactive XMVSF boot selector instead of executing the CPS2 game.
The original `XM8761::afterInput()` movie reader can only run once
`RunFrame()` executes for a loaded driver, so no replay frame was
loaded at the menu.

**87.163 currently launches `xmvsfu -joy` directly** (which
only works for a continuous in-game segment, not for the original
session's menu resets), and sets
`XM8761_REPLAY_MODE=play`, and restores Training host mode for the
original 87.161 movie before `DrvInit`. Ordinary normal/Training/
Versus runs through `RUN_TEST.bat` still use the normal boot selector.
DHC transaction timing, native super inputs, projectile isolation,
Tournament rules, and the Stable executable are untouched.

The original native 87.61 replay code remains the one and only
input/state recorder. The launcher does NOT recreate its movie format.

## Record

1. Pull the `feature/dhc-mvc2-stage87` branch and run
   `BUILD_TEST.bat` (incremental warm cache).
2. Open `RUN_TEST.bat`, select Training, and enter a match.
3. Press **F9** to start the existing state-anchored input recorder.
4. Perform the motions you want to capture.
5. Press **F9** again to stop.

A single F9 movie always consists of **both**:
- `xm8761_f9_0001_inputs.bin`: frame-accurate CPS2 input-port bytes
  (including the ROM ID and per-frame sequence IDs).
- `xm8761_f9_0001_start.fs`: native FBNeo full save state captured
  just before the first recorded input frame.

The standard TEST record directory is `test/87.###/replays/`.
Each F9 start allocates a new numeric filename; existing recordings
are not overwritten. **Shift+F9** retains native quick-save.

## Replay — dedicated and available any time

Run **`RUN_REPLAY.bat`** from the repository root.

- With **no argument**, it searches `test/87.*/replays/` for an F9
  `*_inputs.bin` file. It prints the selected file before launching.
- To select a specific movie, drag that `*_inputs.bin` onto
  `RUN_REPLAY.bat`, or run:
  `RUN_REPLAY.bat "F:\\xmvsf\\test\\87.161\\replays\\xm8761_f9_0001_inputs.bin"`.
- The matching `*_start.fs` must be in the same directory.
- The current legacy launcher starts `xmvsfu` directly and
  sets the original F9 decoder to **play**. This cannot reconstruct the
  custom menu, Training return/restart, or multiple host sessions without
  additional events and save-state checkpoints. Treat it as input-only
  playback, NOT a full-session re-creation.
- The native decoder validates the movie ROM ID, input layout, complete
  frame payload, and starting save state before applying any inputs.
  On failure it writes a concrete reason to `replay_status.log`.
- The window title displays **XMVSF REPLAY | current / total INPUT
  FRAMES**. At EOF it displays `XMVSF REPLAY COMPLETE` and releases
  emulated inputs. Close the game window when finished.

Results are isolated from the source movie:
`test/87.163/replay_results/` for this release, including
`replay_status.log`, `rom_super_observed.tsv`, and the existing
native ROM/DHC diagnostics. A replay never deletes, modifies, or
overwrites the saved F9 input/state pair or Stable.

Use the exact paths printed by the launcher. If it says no input pair
was found, record one first, extract both files from an archive, or
drag a movie onto the BAT. Dragging a ZIP itself is not supported.

## What the original F9 format does *not* record

It saves one full CPS2 state and emulated controller ports. It does
**not** save host-side Training menu character changes, SDL controls,
or subsequent host quick-load operations. Thus the original uploaded
87.161 file has **17,426 valid frames** and is valuable for native
motion/action analysis, but a continuous 14-character session may
diverge after host-only state or selection changes. This is a known
format limitation and must not be mistaken for wrong ROM super inputs.

The read-only `movie_input_frame` column in 87.162+ native ROM logs
lets a replay be compared to a source capture by exact movie index
instead of fragile native frame offsets.

## Maintenance rule for subsequent point versions

**Permanent root launcher set:**
`BUILD_TEST.bat`, `RUN_TEST.bat`, **`RUN_REPLAY.bat`**.
No version-specific BAT proliferation.

Every TEST version bump must point `RUN_REPLAY.bat` to that same
current TEST executable and keep the native replay environment/
directory and source state+input pair working. CI validates that:

- the permanent launcher exists and starts the current TEST program
  **with the actual `xmvsfu` ROM argument**;
- playback bypasses the normal boot menu;
- the original F9 reader, starting save state, and per-frame input
  injection are still hooked;
- recording/decoding are covered by a standalone C++11 round-trip test;
- the replay results path stays separate from the input/state source
  and Stable is never touched.

The GitHub C++11 CI test suite does not run the Windows SDL native game.
A live replay remains the final integration check.
