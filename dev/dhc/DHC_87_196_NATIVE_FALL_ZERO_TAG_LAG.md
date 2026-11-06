# XMVSF 87.196 — remove the proven 67-frame native Ken tag-in lag

## Source: uploaded 87.195.zip (native Windows traces)

The zip contains `dhc_native_handoff.tsv`, `observer/native_pc_trace_test.tsv`, input/subframe records, F9 replay V2, and the `reserve_owned_supers.tsv` catalog. The five original ROM Ken tag-ins, with naturally observed first `0212` frames:

| Ken incoming swap | Host selects nonattacking 2A08 | Original ROM selects 0212/0600/D3D76 | Real native floor contact | ROM super |
|---:|---:|---:|---:|---|
| 1644 | 1645 | **1712 (+67)** | 1728 | 1735: 2E00 |
| 2192 | 2193 | **2260 (+67)** | 2275 | 2282: 2E00 |
| 2880 | 2881 | **2948 (+67)** | 2983 | 2990: 2E02 |
| 3968 | 3969 | **4036 (+67)** | 4073 | 4096: NOT a super; ROM selected 0006 |
| 4407 | 4408 | **4475 (+67)** | 4490 | 4497: 2E00 |

The incoming 2A08 script is a 67-frame NON-MOVING airborne hold at Y408/154/118. When its native animation ends, the 68000 independently selects Ken `action0212, mode0600, D6=12, ROM script 0x0D3D76` and resumes gravity. At natural transition 1712: `+14=00000000, +18=00000000, +1C=00000000, +20=00006000`; on frame 1713 the ROM advances `+1C=00006000`. The artificial wait is not necessary for the eventual genuine jump and command acceptance. There is no ambiguity from different side/height trials: **67 frames consistently**.

## 87.196 TEST change

- Do NOT truncate a 2A08 animation with an invented timer. Instead skip 2A08 altogether **only** for the Ryu→Ken DHC's original Ken 2A0C/2A0E tag-in at the moment of the genuine ROM point/reserve swap.
- Resolve Ken's true original `0x02/0x12` native fall action through the installed ROM table. Verify that the resolved script is exactly `0x0D3D76`. On any mismatch refuse the experiment, never guess an actor script.
- Enter that ROM-derived action with the same observed native `A0=0600, D6=12, +14=0, +18=0, +1C=0, +20=6000`. Keep actor X/Y *including fractional words* and preserve opponent, partner, meter, original outgoing controller8→3200, captured reserve super/chord, and original ROM 2E selection.
- Use the existing 87.194/87.195 passive-jump guide to steer horizontal X to the SAME opponent-relative target along the actual native Y descent; only genuine ground contact releases the captured original incoming fighter gesture.
- A duplicate native entry attack reassertion during the jump reenters the verified 0212 fall instead of restarting 67 frames of 2A08.
- Unverified actors are not redirected. The independent Sabretooth reference is about passive standings, not proof of a reusable global 0212 script.

## Remaining native input inconsistency—do not falsify the ROM result

`reserve_owned_supers.tsv` at 3953 shows the proposed command for Ken was **HCF+KK** (`motion=5 buttons=30`), *not* the confirmed QCF+KK command that produced Shinryuken at 2990 (`motion=1 buttons=30`). The 87.195 original Ken ROM selected action `0006` after that HCF gesture. It would be incorrect to force this into 2E02 or mark it accepted. The retained 87.192 input ownership and verified 2E-only acceptance rightfully treat it as a non-super. The 87.195 recording includes both left and right on-screen target positions, but all registered transactions in `dhc_native_handoff.tsv` have `side=1` (P1 player); this isn't a proven P2 mirror test.

## Regressions and launch

`dev/dhc/tests/test_dhc_native_immediate_fall_196.cpp` tests a synthetic direct ROM `0212` lookup, exact original mode/D6/gravity and zero Y/X teleport, no partner/opponent/meter touches, unsafe-action refusals, opt-out, and unsupported fighters. The existing 87.194 and 87.195 C++ tests are retained. No ROM bytes are committed.

Root launcher names remain only `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat`; incremental compiler warms existing object cache and publishes **only** `XMVSF_Test_87_196.exe`. All TEST data stays under `test/87.196`. Stable and older TEST binaries untouched.

Default `XM87196_TEST_IMMEDIATE_NATIVE_FALL=1` for 87.196 RUN_TEST/RUN_REPLAY. Set `XM87196_TEST_IMMEDIATE_NATIVE_FALL=0` to reproduce the 87.195 67-frame 2A08 path without rebuilding. If the direct ROM fall fails native validation, the host logs `DHC_196_NATIVE_FALL_ROM_OR_AIRBORNE_UNVERIFIED` or the original 87.194 jump watchdog and **never** invents a landing. A new F9 Windows capture is required to assert native execution and check its timing.

The previously installed MvC1 GIF background at ~30% remains unchanged.
