# XMVSF Stage 87.185 — Restore DHC input allowance and test the missing action windows

**Evidence:** the uploaded original `87.184.zip` (F10 probe, super chat, native handoff, 87.175 and 87.180 actor observers, and 87.182 instruction-PC trace). This analysis is from actual recorded gameplay, not a predicted frame timeline. No private ROM, saved state, or replay inputs are committed.

## What regressed in 87.184

There are **19 actual QCF+2P/3P DHC queue events** in the uploaded session, all Ryu→Ken Shoryureppa. Exactly:

| Actual result | Count | Distinguishing original ROM evidence |
|---|---:|---|
| Native Ken `2E00` Shoryureppa | 5 | Ryu `2E08` attack, controller 8 consumed, host tag script `3200`, native Ken super |
| Timer-window wait → discarded | 7 | Original `2E08` **step04** with fresh input and remaining `+4E` = `1F–28` or `22–25` (hex), already within the native attack script but not `<=1C` |
| Recovery controller → restarted Ryu super | 5 | Original `2E0A` step06; after controller 8 the ROM invokes shared action writer PC `010392` with `D0=2E,D1=08`, choosing `2E08` script `0D3070` rather than a tag |
| Consumed native input safety decline | 2 | `C0/CA` already contain punch bits, so 87.183 correctly declines unsafe controller 8 |

The stage 87.184 recognition/port-normalization gate was also restricted to only `canTag` (attack and selected recovery). This was an independent regression: **2P/3P inputs in actual `2E08` startup were no longer even recognized and physical 3P could escape native dash shielding**. There is no evidence that all startup controller-8 attempts can tag; do not confuse user command recognition with tag execution.

The derived event catalog is at `dev/dhc/evidence/ORIGINAL_87_184_QUEUED_DHC_OUTCOMES_87_185.csv`. The exact pre-host step04 on successful handoffs differs from some same-frame F10 snapshots because the host immediately resets `step08` to 0 when arming controller 8; refer to the pre-arm actor and decision events.

## Stage 87.185 changes

### A. Restore complete-action input recognition, retain exact exit

`XM87185Gate::canRecognize` recognizes original Shinkuu in startup `2E08/step02`, attack `2E08/step04` and nonterminal recovery `2E0A/step06`. It excludes script `0D3194` (terminal) and post-action state. `super_chat` accepts original QCF+any two punches / 3P for the entire genuine action. The pre-CPU 3P dash shield now also spans the same action window; it is **not** restricted to a currently executable controller-8 phase. A startup gesture is recognized and logged but is **not automatically executed** until native startup-tag behavior is proven.

Other fighters retain the prior discovery logic, and normal gameplay 3P outside DHC is not rewritten.

### B. Test immediate native controller 8 during the ACTUAL active attack

All seven 87.184 timer-wait drops occurred inside original Ryu `2E08/step04` attack with a fresh physical punch chord. They were lost solely because the host waited for `+4E<=28` decimal, at which point the original 68000 had consumed the input edge. 87.185 tests **arming controller 8 immediately**, while the original attack step, original script, fighter identities, native `D6=0A`, and safe `C0/CA` match.

Only within active script and `+4E` from `1D..3C` hex, the TEST flag `XM87185_TEST_EARLY_ACTIVE_TAG=1` allows this. This is not the 87.179 failed approach: it does **not** wait and synthesize a second press. Original script `3200` is still installed only after the native 68000 consumes controller 8. The existing 87.183 punch-safety decline protects against previously consumed input. **Actual high-timer native acceptance is not proven until a new TEST capture.**

### C. Handle the five EXACT native recovery rewinds separately

The original ROM actually executed these instructions on all five failures:

- Controller `+256` is consumed (`08→00`).
- Ryu's `A0` becomes `2A00` via original `032A14/032A1A`.
- Shared dispatcher `01038E/010392` sets action `2E08`, step02, script `0D3070`, D6 stays `0A`, controller `+259=08`.
- The host previously aborted because captured action `2E0A` no longer matched current `2E08`.

87.185 adds a **TEST-only, strictly one-frame** recovery rebound predicate: original `2E0A/step06/nonterminal script`, Ryu/Ken unchanged, current `2E08/step02/0D3070`, `A0=2A00`, `+256=00`, `+259=08`, `D6=0A`. If every field matches, the existing **host** outgoing tag bridge installs the original ROM-resolved `3200` script and controller `D6=14`. This is transparently a host-assisted bridge after an original-ROM *Shinkuu restart*; **the ROM did not independently select a tag**. It never forces Ken's `2E00`; Ken's super must still be accepted through the original native command path.

The feature is controlled by `XM87185_TEST_RECOVERY_REBOUND=1` and requires `XM87184_EXPERIMENTAL_RECOVERY_TAG=1`. If disabled, 87.185 will NOT arm the previously proven-broken 87.184 recovery controller; the input can still be recognized for diagnostics. Only the existing aborted-chain recovery may be used if the experimental bridge fails.

### D. Isolated build and repeatable measurements

The original launchers remain `BUILD_TEST.bat`, `RUN_TEST.bat`, and `RUN_REPLAY.bat`; new binary `runtime/stable/XMVSF_Test_87_185.exe`. No Stage 87.183/184 TEST executable or Stable is overwritten. The warm-cache builder retains full compiler logging and the removal of the unreliable compiled-string marker checklist.

Interactive `RUN_TEST.bat` enables both isolated 87.185 experiments by default. To isolate one without editing source, define environment variables in the same CMD terminal **before** running:

- `set XM87185_TEST_EARLY_ACTIVE_TAG=0` — only the already verified `+4E<=28` active route.
- `set XM87185_TEST_RECOVERY_REBOUND=0` — no recovery controller arming at all (safe rollback).
- `set XM87184_EXPERIMENTAL_RECOVERY_TAG=0` — disable recovery test entirely.

`RUN_REPLAY.bat` disables both new experiments. It can reuse the uploaded 87.184 F9 movie and write the independent 87.185 68000 PC, native subframe and host DHC logs, but may report expected historical divergence because the recognition/safety code changed.

## Evidence and verification limitations

A ROM-free C++11 regression was added at `dev/dhc/tests/test_dhc_action_window_185.cpp` and the source predicates were independently compiled and exercised against all observed guard families. The full Windows/MSYS2 executable, actual high-timer DHC, recovery host bridge, native Ken super response, and any regression in other gameplay have **not** yet been run or verified.

Live acceptance:
1. Reproduce early 87.184 queue attempts (around 87–95 frames after Shinkuu root). The marker `DHC_185_EARLY_ACTIVE_DIRECT_FRESH_EDGE_TEST` must be followed by native `2A00` and outgoing `3200` after controller consumption, then native Ken `2E00`.
2. Test recovery (around 129–139 frames after root). A valid test should log `DHC_185_NATIVE_RECOVERY_REBOUND_ROM_CHOSE_2E08_HOST_TAG_BRIDGE` and `DHC_185_RECOVERY_REBOUND_TO_ORIGINAL_3200_SCRIPT`, then actually swap to Ken and start his real `2E00`. Without Ken acceptance, it is **not** a DHC success.
3. Retry after an abort to verify no persistent input locks. After terminal script `0D3194` or original action exit, DHC must never arm.
4. The five previously successful 87.184 late-attack handoffs and 87.183 native consumed-punch guard must remain intact.

If either experiment fails, disable that one setting in CMD and preserve the old recording instead of tuning more arbitrary frame delays.
