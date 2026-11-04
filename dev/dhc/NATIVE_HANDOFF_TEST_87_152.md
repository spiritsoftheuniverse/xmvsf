# XMVSF Stage 87.152 — subscribe to the actual native super dispatcher

## Evidence: two user 87.151 captures

These user-supplied logs are independent:

* `87.151.zip` is the failed DHC: Ryu native tag swaps at frame 1872, Ken's incoming attack is suppressed immediately at 1873, and the old implementation **injects Ken action 2C00/script 0D5C46** at 1875, while forcing A0=2A28/D6=0A. At frame 1877, it labels action 2C00 successful after the animation pointer advances to D5C56. Actual game instead loops `2C00`, reselecting D5C46 every roughly 45–46 frames (1920, 1966, 2011, 2057, 2102, ... 2511), **without returning player control**. The 87.151 implementation incorrectly treated script progress as controller acceptance.
* `87.151(1).zip` is the **natural four-super/Variable Combination reference**. In its 1,605 continuous P1 point-fighter frames, every real listed super entered `modeA0=2A02` as selected by the ROM and later returned to neutral. Physical CPS2 button samples and script pointers were captured simultaneously.

| Capture | Fighter | At | Input buttons | Native action | Original script | Subsequent native stages |
|---|---|---:|---|---|---|---|
| Ryu Shinkuu Hadoken | 12 | 1892 | 07 (three punches) | 2E08 | 0D3070 | 2E0A / 0D30F4 at 2014, neutral at 2055 |
| Ryu Shinkuu Tatsumaki | 12 | 2079 | 38 (three kicks) | 2E00 | 0D2CFC | 2E02, 2E10, 2E04, 2E06, neutral 2250 |
| Ken Shoryureppa | 14 | 2457 | 07 (three punches) | 2E00 | 0D6722 | Neutral 2619 before independent next input |
| Ken Shinryuken | 14 | 2625 | 38 (three kicks) | 2E02 | 0D6976 | 2E04 / 0D6B0A at 2745, neutral 2788 |
| Ken Variable Combination reference | 14 | 2879 | 24 (HP+HK) | 2E06 | 0D6B3E | 2E08 / 0D6BC2 at 2976, neutral 3019 |

**Caution:** The absolute 2Cxx ROM scripts in older 87.142/87.150 captures were real scripts but **not proof they are the command-owned native supers requested here**. The earlier Ken Shoryureppa `2C00/2C04` selection is now retired; the 87.151 user DHC recording shows why it cannot be used as an isolated substitute. 2E06 is a combined-hyper reference, not a QCF+PP/KK substitute.

## Root cause and new design

The old handoff forged the **animation script**, **native D6 selector** and **A0=2A28** after canceling the first non-attacking Ken tag exit. The ROM kept restarting a genuine but wrong 2C00 attack. A successful animation record is not a successful move-controller invocation.

The 87.152 TEST **does not force Ken's 2E script, D6, or A0**. Ryu's existing native tag-out and Ken's first-frame kickless 2A08 remain unchanged. After the real **2A08 exit completes into ROM neutral**:

1. Confirm the queued Ryu-to-Ken command resolves exactly to Ken's observed native `2E00/0D6722` (QCF+PP) or `2E02/0D6976` (QCF+KK).
2. Deliver a single **17-frame native CPS2 controller stream** through the original `CpsRwGetInp` port packing: down (4f), down+forward (4f), forward (3f), forward+3P/3K (3f), release (3f). Face relative to the incoming Ken's current native direction. Override only P1's 4 direction and 6 attack bits for that frame; restore the actual controller arrays immediately after the port read.
3. Let the 68000 ROM receive those port bits and use its normal motion parser, actor state machine, native animation transitions, hitboxes and super recovery. Reject any attempt where the real ROM never selects the expected 2E action and corresponding script. Never fake a success from sprite frames or 2C action family.
4. One input sequence per queued DHC only; no retries from a held chord. If the native command is refused, report failure and unlock the DHC transaction without overwriting the actor's gameplay state. Super completion is signaled by real neutral on consecutive native frames or a native tag swap. A long watchdog releases bookkeeping only, not game state.
5. The native evidence recorder adds D6, controller256/259, scheduler step08, timer4E, and attack latch A4 columns, so the exact 68000 route can be verified in next Windows gameplay capture. This is a test of the **real input-command path**, not a claim that the target super has already been proven to start after the DHC tag.

The isolated compiler tests use synthetic pointer-table cells, not copyrighted ROM data. This attempt retains the original root launchers `BUILD_TEST.bat` and `RUN_TEST.bat`; they now publish only `runtime/stable/XMVSF_Test_87_152.exe`. Stable, prior test bins, and `main` are untouched. Tournament rules remain separately gated by the actual Options mode.

## What to test

1. P1 Ryu point / Ken reserve. Ryu natural Shinkuu Hadoken, then Ken QCF+PP during Ryu's super. Ken should enter **kickless**, finish his 2A08 non-attacking exit, then attempt a ROM-controlled `2E00` once. If refused, normal input must return; 2C00 infinite Hadoken cannot recur.
2. Repeat from a new first super with Ken QCF+KK; expected `2E02`. After Ken completes, check whether ordinary controls work.
3. Compare a normal native Ken super with F10 recording, in particular A0/D6/256/259 when native `2E00` and `2E02` are selected. DHC's `dhc_native_handoff.tsv` should report `DHC_KEN_NATIVE_NEUTRAL_INPUT_ARMED`, `DHC_KEN_NATIVE_QCF_INPUT_BEGIN`, `DHC_KEN_NATIVE_THREE_BUTTON_EDGE` and either `DHC_NATIVE_INPUT_DISPATCH_ACCEPTED` or `DHC_NATIVE_INPUT_NO_SUPER_ROM_ACCEPT` / `KEN_INPUT_DISPATCH_TIMEOUT_UNLOCKED`.
4. In Classic and Tournament, verify original tags and gameplay rules still behave normally.

Outputs are in `F:\xmvsf\test\87.152\`. **Expect a natural 2A08 exit delay** in this build: early 2A08 cancellation was intentionally removed to first prove controller ownership. Speeding it up and integrating the Variable Combination freeze/portrait path requires another confirmed native transaction.

GitHub C++11 isolation CI and user Windows runtime are separate verification layers; neither synthetic unit tests nor source commits prove live DHC execution.
