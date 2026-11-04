# XMVSF TEST 87.155 — Ken DHC two-super native probe

Native DHC remains an experimental **TEST-only** feature. Stable is untouched.
Use only the standard root `BUILD_TEST.bat` then `RUN_TEST.bat`.

## One build; two choices

While P1 Ryu's Shinkuu Hadoken is running, enter a fresh directional
sequence and a *three-button* chord for the **reserve** Ken:

| DHC request | Ken attack | Native action | Native script |
| --- | --- | --- | --- |
| QCF + LP + MP + HP | Shoryureppa | `2E00` | `0D6722` |
| QCF + LK + MK + HK | Shinryuken | `2E02` | `0D6976` |

Both destinations are **resolved and logged simultaneously**, but only the
requested one is eligible for gameplay confirmation. The native ROM must
execute the move through its own action/controller parser. No direct injection
of either `2E` animation script or hand-crafted fake super action is allowed.
The new `dhc_native_handoff.tsv` has separate columns for both script
availability and observed native super actions, plus selected command.

## 87.152 reset investigation and gate

The supplied failed trace at 1744 had Ryu Shinkuu Hadoken with timer
`+4E=0x000D` (13 frames). After DHC controller-8 staging, the ROM chose
Ryu `2C04`, not native tag `3200`. A later timeout copied old
`A0=2A28` and controller/step bytes onto that different live action.
All fighter IDs were then zero in the next sampled frame. This is a strong
correlation, not yet proof of the internal reset cause.

The successful 87.150 trace entered tag processing with `+4E=0x0008`.
87.155 queues the verified Ken choice while Ryu's native `2E08` super is
live, then waits for `+4E<=8` before staging the controller. While waiting
there are no RAM writes. If the super has changed, the outgoing owner is
wrong, or the valid late window is missed, the request is discarded with
no gameplay state rollback.

If the native controller was consumed and the ROM selected `2C04` instead
of `3200`, DHC aborts immediately without restoring stale memory onto
`2C04`. The known-working native `3200`/Ryu tag-out and Ken `2A08`
kickless entry paths remain intact.

## How to evaluate

Use F10 during the match, and capture at least one DHC attempt per chord.
In `test/87.155/dhc_native_handoff.tsv`, inspect:
- `DHC_DUAL_KEN_TARGETS_REPPA_2E00_D6722_SHINRYUKEN_2E02_D6976`
- `DHC_WAIT_RYU_SUPER_TIMER_08` and `DHC_LATE_SUPER_TIMER_WINDOW_ACCEPTED`
- `DHC_ROM_TAG_3200_AFTER_CONTROLLER_ACCEPT` and `REAL_ROM_POINT_RESERVE_SWAP`
- `DHC_KEN_NATIVE_NEUTRAL_INPUT_ARMED`, `DHC_KEN_NATIVE_QCF_INPUT_BEGIN`
- `DHC_KEN_REPPA_2E00_ROM_SEEN` or `DHC_KEN_SHINRYUKEN_2E02_ROM_SEEN`
- `DHC_KEN_NATIVE_SUPER_ROM_ACTION_CONFIRMED` (requires correct selected move)

A row showing both ROM targets ready proves only ROM-script lookup. It does
**not** prove native super execution. If the next capture fails to reach
Ken or enters the wrong action, use the exact row and native CPU trace
rather than guessing another sprite/script address.

Isolated GitHub C++11 tests exercise both options, wrong-super unlock,
87.152 wrong-action no-rollback, and the late-super timer wait. These do
not substitute for a local native Windows playtest. Preserve existing
Training options, Tournament rules and Stable binaries.
