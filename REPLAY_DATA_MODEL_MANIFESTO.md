# XMVSF — The Replay Data Model Manifesto

**Status:** authoritative repository contract for F9 capture, playback, evidence, and handoff.  
**Applies to:** TEST 87.166 and all later stages.  
**Scope:** the native XMVSF application, custom boot menu, SDL host state,
FBNeo/CPS-2 execution, Training, Versus, Arcade, Tournament rules, DHC
research, and fast-forward.  
**Implementation:** `dev/stable/buildtree/src/burner/sdl/xmvsf_stage87_164_host_replay.h`;
`run.cpp`; `main.cpp`; `xmvsf_stage14_boot.h`; permanent
`BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat`.

> **A recording is evidence, not a guess.** A replay may report completion
> only for state, inputs, and host decisions that were actually captured.
> The emulator is not the entire program: a controller-only movie cannot
> replay a custom SDL menu, in-process game restart, or out-of-band
> `BurnStateLoad`.

## 1. Immutable principles

1. **One recorder, one timeline, multiple native game sessions.** F9 is
   the user's single capture switch. A session begins/ends with real
   `DoGame`/`RunMessageLoop` boundaries; a segment begins whenever a
   verified host transition demands a new native state anchor.
2. **Two kinds of state, never conflated.** Guest state belongs to FBNeo
   savestates and frame inputs. Host state belongs to the XMVSF application:
   game mode, input ownership, menu state, Training settings, stage, rule
   selection and character-selection handoff. A savestate does **not**
   imply restoration of host state.
3. **Every simulated input is anchored.** Each segment has one real,
   complete native checkpoint recorded at a safe pre-input boundary,
   one input stream with the exact emulator slot layout, and a journal
   entry tying the two to the monotonic movie timeline.
4. **Successful native loads are discontinuities.** Training triple-Start,
   KO-landing resets, Versus rematches and reselection, and explicit
   host quickloads start a new checkpoint at the next safe boundary.
   Never infer a new emulator state from an old frame or from a visual
   screenshot.
5. **Keep raw evidence.** Do not overwrite existing recordings, import
   guessed ROM scripts, edit their input bytes, relocate old sessions,
   silently truncate movies, delete failed attempts, or reclassify legacy
   v1 data as v2.
6. **Stop on divergence, not improvisation.** Missing checkpoints,
   invalid input sizes/layouts, impossible session order, failed loads,
   and corrupt/contradictory manifests produce a named diagnostic.
   No fallback to live controller inputs, a different ROM, Stable, or
   a fabricated host transition.
7. **Capture UX is part of correctness.** F9 shows `RECORDING` in both
   the custom boot menu and gameplay. A successful seal shows `SAVED`;
   failure shows `ERROR`. A durable status file must be produced even
   when the session folder cannot be created.
8. **Fast-forward changes presentation, never history.** It executes
   every guest input and CPU frame in order. It may reduce drawing and
   mute real-time audio; it must not compress time by skipping emulator
   ticks or merging host events.
9. **Permanent tooling, no build regressions.** The root launchers stay
   `BUILD_TEST.bat`, `RUN_TEST.bat`, and `RUN_REPLAY.bat`.
   Keep warm compiler objects, all existing working source and ROM
   behaviors, and `XMVSF_Stable.exe` intact. Increment TEST version
   internally; never proliferate alternate builder/runner BAT names.
10. **Tests are necessary, not runtime proof.** Isolated C++11 checks
    establish serialization and file-contract correctness. Only native
    Windows SDL playback of a newly captured menu/reset/return session
    can establish full operational replay fidelity.

## 2. The authoritative on-disk layout

For each point version, recordings live with the other diagnostic data
that the existing point packager collects:

```text
F:\xmvsf\
  BUILD_TEST.bat                       permanent
  RUN_TEST.bat                         permanent
  RUN_REPLAY.bat                       permanent
  test\
    build_logs\BUILD_TEST.log          append-only compiler/linker transcript
    87.166\
      replays\
        F9_status.log                  append-only root-level capture status
        latest.txt                     published ONLY for a sealed capture
        session_0001\
          manifest.tsv                 ordered, versioned host/session journal
          replay_v2_status.log         session-specific status
          s0001_seg0000_start.fs       complete native CPS2 state
          s0001_seg0000_inputs.bin     exact frames beginning at that state
          s0001_seg0001_start.fs
          s0001_seg0001_inputs.bin
          ...
      replay_results\                  separate replay diagnostics
```

`latest.txt` contains the literal full session-directory path, one
line. The runner may also accept a specific `session_XXXX` folder or
its `manifest.tsv` by drag-and-drop. An unsealed but internally
consistent legacy capture is **recoverable evidence**, not the default
latest successful recording. The runner should make it easy to select
such a capture explicitly, without altering the original.

Retain the older `test/replay_sessions/` v2 layout and all original
`xm8761_f9_XXXX_inputs.bin` / `_start.fs` v1 pairs as readable legacy
inputs. A version bump must not make any prior recording invisible.

## 3. Versioned manifest schema (`XM8764V2\t1`)

The manifest is append-only, UTF-8/ASCII TSV, with a single signature
header. The record verbs and argument order are part of the contract:

```text
XM8764V2  1
EVENT  session  absolute_movie_frame  semantic_event  arg1  arg2
SEG    session  segment  mode  reason  input_file  state_file  HOST_FIELDS...
DONE   session  segment  frame_count
FINAL  total_frames  session_count  segment_count  stop_reason
```

The real file uses **TAB separators**, not spaces. `EVENT` records
a semantic host decision at a monotonic frame boundary; it need not
have a CPS2 frame of its own. `SEG` records the pre-input anchor.
`DONE` commits the verified written length of an individual input
segment only after the stream has been flushed/closed. `FINAL`
seals the complete movie after the last `DONE`, checks all totals,
and explains why recording stopped (`F9_STOP` or
`APPLICATION_EXIT`). Unknown mandatory schema revisions must
be refused rather than guessed.

**Recorded mode values:** 0 = Arcade, 1 = Training, 2 = Versus.  
**Session ID:** starts at 1, increments for each real game launch.  
**Segment ID:** starts at 0 inside each session and increments for a
host/state discontinuity.  
**Absolute movie frame:** starts at 0 and never decreases, even
when the ROM's own `GetCurrentFrame()` jumps due to a savestate load.  
**Segment frame index:** starts at 0 and increases by 1 for every
submitted CPS2 input frame inside that segment.

`HOST_FIELDS` currently carry, in this exact order:
`training`, `versus`, `inputOwner`, `damageMode`,
`trainingMenuOpen`, `selectedMenuRow`, `hitboxes`,
`dataDisplay`, `infiniteMeter`, `dizzyAllowed`,
`frameDataEnabled`, `stageChoice`, `stageResolved`,
`tournament`, `selectPhase`, `selectFrames`,
`selectPicksP1`, `combatCandidateFrames`, `p2CarryMask`.

These are *host* snapshots, not inferred CPS2 RAM. Numeric counters
such as `selectFrames` remain in a checkpoint's host snapshot for
resumption, but **must never trigger a new checkpoint simply because
they increment every frame**. Only semantic decisions and actual
state loads may split the movie. Each such split is a bounded,
documented reason.

## 4. Binary input and state pair

Each `*_inputs.bin` is an existing `XM8761R1`-style native stream:
8-byte signature, little-endian input-slot count, 32-byte ROM driver,
two layout bytes (input type and width) for each slot, then exact
frames. Every frame is a little-endian 32-bit index followed by the
native bytes for every declared input slot. The related `*_start.fs`
is FBNeo's full `BurnStateSave` output.

Do not assume there are always 29 slots, one byte per slot, or the
same ROM in future programs: read and validate the actual header.
On loading a segment: verify signature, native ROM name, slot count,
every slot's type and width, expected byte count `DONE` advertised,
and whether `BurnStateLoad` succeeds **before applying frame 0**.

The journal cannot replace the native state or vice versa. A package
without the complete manifest and every referenced input/state pair
is incomplete.

## 5. Lifecycle and commit semantics

```text
IDLE
  F9 (boot menu OR gameplay)
  -> RECORDING / create session dir / flush manifest header immediately
  -> GAME_BEGIN / session 1 / checkpoint 0 / input frames
  -> host choice OR successful native reload
     / close input segment / write DONE / create next checkpoint + SEG
  -> GAME_END / close last input segment / write DONE / stay RECORDING
  -> next boot choice and GAME_BEGIN / session 2 / ...
  -> F9 STOP or normal APPLICATION_EXIT
     / close stream, journal and finalize totals
     / write FINAL and flush / publish latest.txt / show SAVED
```

A normal program exit is a valid stop if F9 remains on; the user
should **not** be forced to return to the app merely to press F9
again. Do not seal on ordinary return-to-main because recording may
continue through several menu sessions. Unexpected process death
may leave a prefix with complete `SEG` / `DONE` pairs but no
`FINAL` and no `latest.txt`. Preserve this as **UNSEALED /
RECOVERABLE**, never label it `SAVED`, and never invent inputs
past the last complete segment.

If F9 stops before any actual game frame, report `EMPTY` and do
not publish a bogus latest pointer. Disk errors during open, write,
flush, close, or pointer publication are `ERROR`, not `SAVED`.
A subsequent retry must use a new numbered directory, not reuse an
unfinished manifest.

## 6. Replaying without a physical controller

1. Validate the manifest, every `SEG`/`DONE` pair, native input
   layout and file sizes. If `FINAL` exists, verify its totals.
2. Enter the **real XMVSF boot-selector initialization** to create
   the normal SDL window/renderer and reapply the recorded semantic
   game/stage/rule selections; do not skip the host setup just to
   reach a ROM frame sooner.
3. Initialize the correct game driver and session-scoped host
   ownership. At the safe pre-CPU boundary load the exact recorded
   native checkpoint and its host snapshot.
4. Submit each recorded input frame in order. Ignore live physical
   attack/menu input while replaying, but permit real SDL window
   management and Quit.
5. On a recorded checkpoint boundary, verify the native
   `BurnStateLoad` and reapply relevant host-only state. On a
   recorded session boundary, use the real in-process
   `RunMessageLoop` → `DrvExit` → boot selector → `DrvInit`
   lifecycle.
6. Report session, segment, absolute frame, checkpoint state and
   any divergence to a **replay-only log**; never append playback
   diagnostics to the original recording or silently change
   existing save files.

An explicitly selected older unsealed capture may play its complete,
verified segment prefix with a prominent **UNSEALED PREFIX** warning.
That is not the same as recreating an intentionally stopped session.
The old 87.161 input-only capture cannot retroactively acquire host
events.

## 7. Fast-forward contract

`RUN_REPLAY.bat` owns the default fast-forward control
`XM8764_FAST_FORWARD=32`. Values 1–128 select the requested
number of ordered CPS2 frames per presentation batch; `0`
disables fast-forward. Actual wall-clock speed depends on CPU,
GPU, and checkpoint I/O. Never promise a fixed 32× speedup.

Time, hitstop, DHC windows, controller ownership, input edges, game
events and Training re-arm rules must count **every** emulator frame,
not every presented frame. A transition/EOF may end a batch early.
Never step one unsolicited live-input frame after recorded EOF.

## 8. Evidence from the supplied TEST 87.166 capture

The user's `87.166.zip` contains **one game session and 8 native
segments** with **3,407 complete input frames**. The segment counts
are exactly `136, 97, 6, 256, 2847, 36, 11, 18`, each with its own
358,352-byte full native state and a self-consistent
`XM8761R1` input payload with 29 declared slots.
The final manifest records `GAME_END` and a subsequent
`BOOT_MODE_SELECTED` at absolute frame 3407, but there is
**no** `RECORD_SAVED`, `FINAL`, or published `latest.txt`
in the supplied package. It is an excellent **complete-segment,
UNSEALED** regression fixture. Do not claim the recording was sealed
or the rest of the application session was captured.

The archive also includes `dhc_native_handoff.tsv`,
`rom_dispatch.tsv`, `rom_super_observed.tsv`,
`super_chat.tsv`, and `tournament_rules.tsv`.
These are independent observations, not replacement commands.
No proprietary ROM, embedded native savestate or input recording
should be committed to the public repository as a test fixture.

## 9. Regression gates for every TEST version

- F9 begins in the boot menu, immediately writes a manifest and
  displays the red recording counter; the counter survives gameplay,
  menu changes and returns to boot.
- One Training character-selection frame counter increment does
  **not** cause an extra checkpoint.
- Every explicit native reload generates a state anchor and a
  bounded `SEG` / `DONE` pair with no input lost or duplicated.
- Returning to main does not stop recording, while quitting the
  application with F9 still on **does** finalize and publish.
- Successful stop produces `FINAL`, matching totals,
  `RECORD_SAVED`, and `latest.txt`; unfinished or corrupt
  recording never does.
- The replay runner discovers the latest sealed recording even
  across TEST version bumps; explicit unsealed-prefix replay
  requires a deliberate file/folder choice.
- Multi-session replay works with **no connected controller**, uses
  the normal persistent window, and respects all checkpoint
  boundaries at fast-forward speed.
- DHC, ROM super dispatch, Tournament rules, Training menus and
  native scene composition have no replay-specific gameplay
  mutations or surprise source overrides.
- Full build logs are always written; unsuccessful TEST builds
  never replace Stable, previous TEST binaries or warm objects.

---

**The north star:** one F9, one trustworthy append-only movie,
one permanent replay runner. It must capture the *real program*,
not just the guest CPU, and it must tell the truth about what
was recorded, saved, replayed, or left unfinished.
