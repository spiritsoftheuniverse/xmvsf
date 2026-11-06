# XMVSF Stage 87.186 — Discover the original beginning gate

**Goal:** Find the earliest verified Ryu→Ken DHC command acceptance within the original Shinkuu Hadoken action, while protecting the repeatable Stage 87.185 attack and recovery handoffs.

## What our existing 87.185 F9/F10 data actually proves

I reprocessed the user's original `87.185.zip` native F10 probe, `super_chat.tsv` and `dhc_native_handoff.tsv`, without using inferred animation times or touching replay inputs. The reproducible offline script is `dev/dhc/tools/ANALYZE_DHC_OPENING_186.py`.

- **16 native Ryu Shinkuu roots** identified at action `2E08`, step `0002`, script `0D3070`.
- Native attack phase begins 66 or 67 frames after the original action root, at `2E08/step04`, script `0D30B0..0D30E0`.
- **17 queued DHC requests**, of which **14** reached Ken's genuine original `2E00` Shoryureppa and three were safely declined for consumed punch history.
- Earliest *recorded successful* DHC is at age **91** (frame 2035, super root 1944, attack age 24).
- No recorded command was attempted at age **0–90**. Thus startup and attack ages 0–23 are **untested**, not proven untaggable, and the exact earliest ROM-safe DHC frame is **still unknown**.
- Stage 87.185 already supports 14/14 *armed* controller handoffs and all six observed recovery handoffs. These remain the protected baseline.

Do not claim that the first-four-attack experiment has succeeded merely because the ROM action/timer predicate passes.

## What has been implemented in Stage 87.186

### 1. Start observing before the native CPU handoff

New read-only `xmvsf_stage87_186_opening_observer.h` records the actual Ryu point/Ken reserve action at **PRE_CPU**, **POST_CPU_PRE_HOST**, and **POST_DHC_HOST**. Each row includes: original action, script, animation step, native super mode/D6, elapsed frames since root/attack, original input words C0/C2/C6/CA, controller +256/+259, and the host DHC phase.

The pre-CPU record also includes actual physical punch-button state *before* `CpsRwGetInp`. Thus a staggered punch or command can be examined on the first sampled input frame, not only after the 68000 has consumed it. The observer has **no writes to CPS2 RAM, ROM, input ports or player state**.

The state machine begins on the genuine original Shinkuu root and follows its original attack, recovery and action exit; it does not assume a fixed 66-frame startup for execution. A restored F9 checkpoint may begin halfway through an action; the log must not invent a root in that case.

### 2. Trace original 68000 PCs before controller 8 is armed

The existing bounded Musashi executed-PC observer previously activated only during `WAIT_TAG_CONTROLLER=2`. Stage 87.186 also enables it when the true Shinkuu action is first entered, when a *physical punch or native C0/CA punch* appears during startup, and during the first 25 native attack frames.

To prevent huge logs, the opening-only mode retains **all original action/input-state changes**, but caps otherwise unchanging watched PCs at 96 per frame. It marks frames as `# OPENING_FRAME`; the old `# DHC_FRAME` records and their armed-handoff semantics remain separate. An original 68000 PPC does not by itself prove tag acceptance; Ken's own `2E00` must still be observed.

Because the CPU has to execute to establish a new action, the first detected action-root row can be at the post-CPU boundary and the *first full-frame* executed-PC trace may begin the following frame. The initial action instruction itself is not falsely claimed as traced.

### 3. Test the four explicitly excluded attack frames, in an isolated TEST

A **separate opt-in** pure predicate `xmvsf_stage87_186_first_attack_gate.h` accepts only:

- Original Ryu `2E08`, `step04`, valid native attack script
- `+4E` timer in **0x40, 0x3F, 0x3E or 0x3D**
- P1 Ryu point/Ken reserve, verified DHC command and original resolved outgoing tag script
- Clean original punch history `(C0|CA)&0x70 == 0`
- Explicit `XM87186_TEST_FIRST_FOUR_ATTACK=1` in the separate TEST

It then uses the *same existing* controller-8 and Ken-native-super handoff. **This is a speculative first-four-frame experiment**, not permission to tag from startup or a proven ROM acceptance. No synthetic second punch, no forced Ken super, no recovery-phase changes.

RUN_TEST.bat enables the experiment by default in `XMVSF_Test_87_186.exe`. To restore 87.185 behavior inside the new binary, use `set XM87186_TEST_FIRST_FOUR_ATTACK=0` in CMD before RUN_TEST.bat. The pre-existing `XM87185_TEST_EARLY_ACTIVE_TAG` and `XM87185_TEST_RECOVERY_REBOUND` controls are unchanged.

### 4. Offline evidence, reproducibility, and safe replay

The offline tool produces:

- `native_lifetimes.csv` — 16 original action episodes; interrupted actions stop at actual exit.
- `observed_dhc_attempts.csv` — all 17 commands, their relative action age and confirmed/declined outcome.
- `opening_offset_matrix.csv` — all 165 candidate offsets, with **UNTESTED** clearly marked for offsets with no recorded command. No post-interruption animation phase is guessed.

The existing unnumbered root launchers BUILD_TEST.bat, RUN_TEST.bat and RUN_REPLAY.bat now target **`runtime/stable/XMVSF_Test_87_186.exe`**. Incremental compilation, full permanent build log, old TEST executables, warm object cache and Stable remain protected. No new numbered BAT launcher was created.

RUN_REPLAY.bat selects the existing F9 V2 session, enables original executed-PC and opening observers, restores the **working 87.185** direct-active/recovery-rebound settings, turns OFF the first-four experiment and old 87.179 delayed-input priming, and never edits original movie/savestate/checkpoint files.

New automatic diagnostic outputs:

- `test/87.186/observer/native_opening_test.tsv`
- `test/87.186/observer/native_opening_test_summary.json`
- `test/87.186/observer/native_pc_trace_test.tsv` (original ROM executed PPC with opening-frame headers)
- Corresponding `native_opening_replay.tsv` and `native_opening_replay_summary.json`

Earlier native subframe, DHC handoff and F9 status logs remain.

## Next decisive proof

1. Pull and build Stage 87.186, run the old 87.185 F9 using RUN_REPLAY.bat, and compare the 14 native Ken successes with the preserved baseline. This proves observation has not altered the successful handoffs if the replay completes.
2. In the separate interactive TEST, attempt QCF+any 2P/3P at the beginning of native attack, especially while the original `+4E` is 0x40–0x3D. Confirm the `DHC_186_TEST_FIRST_FOUR_ATTACK_FRESH_EDGE` marker, original controller +256 consumption, Ryu host-assisted outgoing `3200`, and Ken's independently ROM-selected `2E00`.
3. Examine any startup punch presses (step02) using the executed-PC and physical-versus-native-input trace. **Do not enable startup controller 8 from this data alone.** A deterministic earlier startup attempt is still required.
4. If any early attempt chooses a wrong move, the existing guard/abort path must release control; turn OFF the first-four experiment without touching 87.185.
5. Only after original 68000 acceptance and real Ken `2E00` are observed at an earlier offset may we assert a new earliest-safe-frame result.

## Verification boundaries

The 87.185 CSV analyzer ran locally: 16 roots, 17 commands, 14 confirmed Ken supers, 0 commands at offsets 0–90, attack phase begins at ages 66/67. A standalone C++11 pure first-four-frame predicate test passed including startup/recovery/consumed-input exclusions. The new source and launchers are committed. **The full MSYS2 Windows executable, extended beginning PPC logger, first-four in-game controller acceptance and historical F9 replay have not yet run.** These cannot be proven from the existing movie alone.
