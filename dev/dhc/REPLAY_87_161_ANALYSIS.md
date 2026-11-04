# 87.161 native F9 replay — source-backed evidence and 87.162 replay sync

Supplied file: `87.161.zip`. The original user's 87.159 Ryu -> Ken
Shoryureppa handoff and outgoing projectile hitbox suppression remain
unchanged; **no newly captured super is enabled for DHC**.

## Decoded from the actual uploaded files

- `replays/xm8761_f9_0001_inputs.bin`: native `XM8761R1`, **17,426**
  consecutively numbered input frames, exactly **29** one-byte CPS2 input
  slots; companion `_start.fs` is the original full emulator state.
- `rom_super_observed.tsv`: **110** ROM action-2E events, including
  animation phases and repeated script roots that are NOT necessarily
  independent hyper moves.
- **26** events have a preceding *physical two-button* press and a
  matching direction path in the actual F9 input stream.
- These cover **14** native fighter IDs and **25** distinct
  fighter/action/script triples. A CSV with all 26 pairings, frame indices
  and raw motion paths is retained separately by the analysis tool.
- **21** replay matches have one consistent chronological offset across
  their group; **5** have multiple possible chord/path matches and are
  explicitly marked `AMBIGUOUS_ANCHOR`, never silently confirmed.
- The native game-frame timestamp jumps backward with game-state reloads.
  The F9 `movie_frame` increments monotonically; it is the superior
  synchronization key. 87.162 now publishes that exact movie index from
  the SDL input recorder to the native 2E observer.

The smallest replay-backed known mapping remains:
- Ryu `12/2E08/0D3070`: direction `5>2>3>6`, LP+MP.
- Ryu `12/2E00/0D2CFC`: `5>2>1>4`, LK+MK.
- Ken `14/2E00/0D6722`: `5>2>3>6`, LP+MP.
- Ken `14/2E02/0D6976`: `5>2>3>6`, LK+MK.

Other native script matches are captured evidence, not decoded 68000
input-recognizer proof. For example `16/2E0A/0DB1C4` followed two
different actual directional histories with LK+MK, despite the previous
ROM logger describing both as `DP`. Do NOT infer the move's true motion
from that heuristic.

The supplied `native_super_reverse_engineering.json` reports
`Capstone M68K unavailable`. No raw decrypted opcode image was included
with the ZIP. We cannot honestly claim the ROM command-handler branch
has been decompiled or that these 25 script entries are 25 independent
user-callable supers.

## A necessary replay limitation

The original 87.61 F9 recorder stores only:
- ONE initial full save-state anchor;
- successive `BurnDrvGetInputInfo` emulated CPS2 input bytes.

In this 17,426-frame movie, **P1/P2 Coin and Start were never pressed**;
yet the original ROM log traverses 14 fighter IDs. SDL/Training-menu
character-select changes and host save/load shortcuts are not encoded
as replay events in this old F9 movie. Thus a native playback of the
inputs **may diverge on later character swaps**, even if its initial
Ryu/Ken or other move inputs match. It is not yet valid to claim this
entire 14-fighter movie will replay identically.

The dedicated `RUN_REPLAY.bat` uses the original `_inputs.bin` and
`_start.fs` with the original native playback handler and writes
independent diagnostics under `test/87.162/replay_results`. It never
modifies those input/state files or the Stable executable.

The 87.162 read-only native observer adds a
`movie_input_frame` column to `rom_super_observed.tsv`. A replay
can now be compared directly by its source input index without guessing
the clock offset after in-game state resets. This is a diagnostic
feature; full 14-character deterministic playback still requires
recording the missing host Training/menu/save-state transitions.

## Next verification gates

1. Compile/test 87.162 on the user's native Windows setup (GitHub
   Actions only executes isolated C++11 tests; it does not execute
   the live CPS2 game).
2. Replay `87.161/replays/xm8761_f9_0001_inputs.bin` through the
   **dedicated** launcher, compare early native action/script entries,
   and inspect the new exact movie indices.
3. Only promote a fighter mapping when the decrypted 68000 command
   recognizer and actual native script entry corroborate the replay;
   follow-on 2E animations and team combinations stay excluded.
4. Preserve native Ryu -> Ken DHC's exact proven timing and outgoing
   independent projectile hitbox suppression.

Read-only evidence fixture:
`dev/dhc/evidence/REPLAY_87_161_INPUT_NATIVE_ACTIONS.tsv`.
