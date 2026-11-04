# XMVSF native input replay — permanent TEST launchers

**Authoritative data contract:** [XMVSF Replay Data Model Manifesto](REPLAY_DATA_MODEL_MANIFESTO.md).
This guide is operational; any changes to the file format, F9 lifecycle,
checkpoint boundaries, legacy support or replay completion claims
must satisfy that manifesto and its regression gates.

## Host-aware replay v2 — TEST 87.167 (sealed multi-session captures)

**This is the current recording workflow.** The permanent root files
`BUILD_TEST.bat`, `RUN_TEST.bat`, and `RUN_REPLAY.bat` remain stable
across point-version updates.

### Record an entire XMVSF session

1. Run `BUILD_TEST.bat`, then `RUN_TEST.bat`.
2. **Press F9 in the custom boot menu** to begin a v2 recording, or
   press F9 while already fighting. F9 records until pressed again.
3. Use the game normally. Select Training, run characters/supers, enter
   or leave the Training menu, change options, return to the main menu,
   start another mode or team, perform KO resets / triple-Start restarts,
   and use Versus if desired.
4. **Press F9 again to finish**. This closes the last input segment and
   publishes the last completed recording.

**F9 status is visible again:** the main menu and gameplay show a
red `RECORDING 000000 F9 STOP` counter while active, a green
`RECORDING SAVED` confirmation when the completed recording is
published, or an amber `F9 RECORD ERROR` indicator if saving fails.

A new F9 recording creates its `session_XXXX/manifest.tsv`
**immediately, even when F9 is pressed at the main menu before any
CPS2 frames exist**. The full `.bin` input stream and `.fs` save
state are then written into that same session folder once the
first real emulator frame runs.

The permanent, append-only failure/status log is
`test/87.167/replays/F9_status.log`, even if the new session folder
cannot be created. To share an issue, ZIP the entire
`test/87.167/replays` folder, including its
`session_XXXX` subfolder and `F9_status.log`.

87.166 fixed a serious 87.164/165 recorder bug: it previously
treated Training's **incrementing character-select frame counter**
as a change requiring a new full save-state every frame. It now
creates checkpoints only on semantic host changes and explicit
native state reloads; normal per-frame selection counters remain
inside the ongoing input segment instead of flooding the disk.



A new directory is created per recording:
`test/87.167/replays/session_0001/`, then `session_0002/`, etc.
Its `manifest.tsv` journals game sessions, menu choices, host events
and checkpoint boundaries. Each `s####_seg####_inputs.bin` has a
matching native `s####_seg####_start.fs` state. These are **not**
old single-segment `xm8761_f9_..._inputs.bin` files.

A completed F9 recording updates
`test/87.167/replays/latest.txt` with the latest session path.
The launcher never chooses a half-finished F9 recording by default.

**87.167 writes a `FINAL` record** after all committed `DONE`
segments, with exact total frames, sessions, segments and a stop reason.
Pressing F9 writes `F9_STOP`; closing the application while F9
remains armed writes `APPLICATION_EXIT` automatically. `latest.txt`
is published only after finalization succeeds. No need to reopen the
game merely to press F9 after returning to the main menu.

The actual uploaded 87.166 recording has 3,407 complete frames across
eight segments but no `FINAL` or `latest.txt`. It remains an
**unsealed recoverable prefix**. To replay this older capture, drag the
existing `test/87.166/replays/session_0001` folder onto
`RUN_REPLAY.bat` after building the current TEST. The reader warns
that it is unsealed. It does not modify, seal or fabricate missing
frames inside that older recording.

### Inspect or validate a replay without launching the game

The standard-library-only, **read-only** verifier enforces the
manifesto's binary and journal rules for a recording folder,
`manifest.tsv` or a ZIP:

```bat
python dev\dhc\tools\VERIFY_REPLAY_V2.py "test\87.167\replays\session_0001"
python dev\dhc\tools\VERIFY_REPLAY_V2.py "87.166.zip" --allow-unsealed
```

It reports `SEALED` or `UNSEALED_PREFIX`, frame/session/segment
counts, native input slot layouts, and whether every manifest
`SEG`/`DONE` has an exact-size, contiguous-index `.bin`
and a valid `FB1` state header. It **never** modifies the source
recording or generates fake `FINAL` entries.

`RUN_REPLAY.bat` calls the verifier when Python is installed:
normal `latest.txt` playback requires `SEALED`, while explicitly
dragging an older session allows a verified unsealed prefix.
The emulator's own native input checks still run if Python is
unavailable.

### Replay with high-speed fast-forward

Run **`RUN_REPLAY.bat`** from the repository root. By default it uses
the latest completed v2 recording. It searches the current
`test/87.###/replays/` directory first, then earlier numbered TEST
replay directories (newest first), preserving recordings across future
point-version bumps. You can also drag a specific session folder or
its `manifest.tsv` onto the launcher.

The launcher reads the recorded host session timeline and enters the
appropriate Arcade, Training or Versus mode before loading the next
full native checkpoint. Native per-frame input bytes then drive CPS2.
At a recorded game-session boundary the existing `DrvExit/DrvInit`
and persistent SDL application loop run again; replay chooses the next
recorded host mode **without requiring physical controller input**.
At in-session host-option changes and successful native state reloads
it applies the recorded host settings and saved CPS2 checkpoint.

**Fast-forward defaults to 32 emulated frames per display update**, up
to 128, without speculative frames or audio-clock throttling. The
actual real-time speedup depends on CPU/GPU performance; 32 frames per
batch does not guarantee 32× wall-clock speed. Change the line

`set "XM8764_FAST_FORWARD=32"`

in the root `RUN_REPLAY.bat` to select another batch size.
Use `0` for normal-speed replay. Visual rendering is reduced during
fast playback, but actual emulator CPU/input steps are never skipped.

Logs are kept in the session's `replay_v2_status.log`, plus the
current TEST version's `test/87.167/replay_results/`. A missing
checkpoint, invalid frame number, input-layout mismatch or other
recognized failure emits `REPLAY_DIVERGED` and a nonzero process exit.

The latest recording is separate from the original old 87.161
file, which predates this host-aware recorder. Its 17,426-frame
`XM8761R1` stream **cannot reconstruct custom menu resets** that
were never captured. `RUN_REPLAY.bat` retains legacy input-only
playback as fallback; it does not mislabel legacy input EOF as a
complete 14-fighter host-session replay.

### Current verification scope

87.167 includes a native movie round-trip C++11 test that covers two
distinct game sessions and checkpointing after a host Training change,
plus a replay-direct-start test for v1. It has **not yet been proven**
end-to-end inside the user's native Windows SDL application. In
particular, the recorder checkpoints final host state and replays
recorded mode choices; it does not visually reenact every individual
intermediate menu-cursor movement. It also has no full RAM-hash
divergence detector yet. These limits remain explicit until native
playback verifies the workflow.

> **Legacy v1 limitation:** The original 87.61/87.163
> format contains one initial emulator state and CPS-2 input frames only.
> The real XMVSF application has a custom boot menu, Training controls,
> mode changes, and native state reloads outside the ROM input stream.
> The **legacy v1 fallback** plays one native game segment, not a
> verified replay of the full original 87.161 multi-roster session.
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

The original 87.61 input/state recorder remains intact for legacy
v1 movies. The current v2 recorder adds a separate host/session
manifest and native checkpoints; `RUN_REPLAY.bat` can detect either
format without silently converting or overwriting original movies.

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
