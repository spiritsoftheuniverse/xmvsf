# Replay v2 — host-aware menu / reset / save-state session contract

## Status

**DESIGN / NOT IMPLEMENTED AS OF 87.163.**

87.163's `RUN_REPLAY.bat` is a permanent root launcher for the original
87.61 *CPS-2-input-only* movie. Starting `xmvsfu` directly is insufficient
to replay a multi-session XMVSF recording. It skips the custom menu and
cannot recreate missing host transitions. Do not describe an input-only
playback completing as a verified re-execution of a whole roster session.

The user-supplied 87.161 F9 movie contains 17,426 emulated input frames,
one initial full state, and no records of SDL host-menu choices or
subsequent host save-state loads. The ROM action logs independently show
multiple fighter IDs, but do not encode their menu navigation or the
full emulator state following each reset. **The original 87.161 movie
cannot be converted into a guaranteed deterministic full-session replay
from these files alone.** Preserve it as input/action evidence.

## What the real app does

The current repo contains these verified native host paths:

- `src/burner/sdl/main.cpp`: custom `xmvsfBootSelector`, mode and
  stage selection, `DoGame`, persistent-window return to the menu,
  `DrvExit`, and new `DrvInit` when the user starts again.
- `src/burner/sdl/run.cpp`: the original
  `XM8761::afterInput()` records inputs only inside `RunFrame`;
  `RunMessageLoop` can return to the menu with `xm26ReturnToBoot`
  without an emulated frame or an input-movie entry.
- `src/burner/sdl/xmvsf_stage23_ui.h`: Training menu actions are
  handled by host SDL events and direct controller polling (including
  `SDL_JoystickGetButton`), not by the ROM's six attack inputs.
  `MAIN MENU`, `PLAYER SELECT`, training flags, and other options
  are host state.
- `src/burner/sdl/xmvsf_stage87_110_reload.h`: triple-Start
  Training reload and follow-on native positional/camera mutation.
- `src/burner/sdl/xmvsf_stage87_105_training_reset.h`: KO-landing
  automatic match-snapshot reload.
- `src/burner/sdl/xmvsf_stage24_engine.h`: Versus rematch,
  reselect, mode change, and return-main operations with native
  `BurnStateLoad` calls.

This is a graph of **application** state transitions, not one linear
CPS-2 movie. An F9 movie containing just `BurnDrvGetInputInfo`
slots and one `BurnStateSave` anchor cannot reconstruct it.

## Required capture format for a future host-aware recorder

Keep the existing F9 movie bytes as a compatible per-session input
stream, but add a versioned manifest and an ordered host-event journal.

Each record needs a **monotonic session timeline** distinct from
`GetCurrentFrame()`, which can jump backward when loading states.
Include: session number, host event sequence, emulated movie-frame
index, active game mode, native ROM driver, event type, normalized
choice/result, and input ownership.

Record semantic actions through the **same existing menu handlers**,
not fabricated keyboard/controller events:
`BOOT_MODE_SELECT`, `STAGE_SELECT`,
`TRAINING_MENU_OPEN/CLOSE`, `TRAINING_OPTION_CHANGE`,
`PLAYER_SELECT`, `RETURN_MAIN`, `VERSUS_REMATCH`,
`VERSUS_RESELECT`, and other actual host actions.

A distinct `SESSION_END/BEGIN` must represent each `RunMessageLoop`
exit and `DrvExit/DrvInit` pair. Replay must recreate those boundaries
rather than assuming one endless ROM instance.

At every **successful** host `BurnStateLoad` and any new game session
with no replayable transition state, capture a numbered native full-state
checkpoint at the next safe pre-input frame. Also persist a concise
host-mode snapshot (Training/Versus, input ownership, menu state,
relevant training options and rule mode). Checkpoints must be exact,
self-contained, versioned and never replace the user's original save
files. Never save from inside an open 68000 CPU scope.

The journal and checkpoints must be tied to an exact movie index:
`before input N: replay host event E, load checkpoint C, validate
state signature, then apply controller frame N`. Host frames where
the game is paused still need their own ordered sequence number.
Capture success/failure of loads; never assume a requested load
actually happened.

## Required playback behavior

A replay that includes host events should start through the
**normal custom boot menu**. It should feed *semantic host actions* to
the existing menu/Training/Versus state machines and run their normal
code paths. It must not force `xmvsfu` at startup unless the manifest
explicitly says the movie begins within an already-running game
session.

For every `SESSION_BEGIN` initialize the correct driver/mode,
rebind current emulated input pointers, and restore the recorded
host + CPS-2 state. The native decoder replays per-frame controller
inputs only while that recorded emulation session runs. Never feed
raw recorded CPS-2 bytes into an uninitialized driver or a boot menu.

For every recorded load, use the original native load boundary,
replay checkpoint and verify a state signature (phase, fighter
identities, relevant RAM hash). Stop with an explicit divergence
report rather than silently marching through 17,426 unrelated
inputs.

The dedicated permanent `RUN_REPLAY.bat` should detect whether
the selected recording contains a host-event manifest. For v2,
launch the complete application/menu playback flow. For legacy v1,
clearly identify it as input-only playback; do not claim full
roster/menu fidelity. Stable and original movie files stay untouched.

## Regressions to require

1. F9 start in menu, select Training, play and perform a super,
   return to menu, choose another fighter/team, and perform a super.
2. Triple-Start Training restart restores positions and menu flags.
3. KO-landing auto-reload returns to the same recorded checkpoint.
4. Versus rematch and character reselect remain on the recorded path.
5. Replay does not require a physical controller (the menu must
   replay semantic events, not poll an actual joystick).
6. On any missing checkpoint or hash divergence, emit
   `REPLAY_DIVERGED` with exact session/event/movie-frame and stop.
7. Original 87.159 Ryu -> Ken DHC, outgoing projectile cutoff,
   `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat` and
   Stable remain unchanged by recorder-specific changes.

Do not promote this plan to “implemented” before the native Windows
game has completed these end-to-end captures and playbacks.
