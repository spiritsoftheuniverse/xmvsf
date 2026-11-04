# XMVSF Stage 87.146 — native tag controller and retryable DHC attempts

## Primary finding from user recording 87.145.zip

The recorded gameplay proves the old DHC script injection was never accepted
by the ROM's higher-level controller.

| Real native frame | Point fighter | Action | Mode | Live script | Observation |
|---|---|---|---|---|---|
| 1948 | Ryu 12 | 2E08 | 2A28 | 0D30E0 | Ryu super playing |
| 1949 | Ryu 12 | 3200 | 2A28 | 0D1DAC | **Host forced an outgoing sprite** after matching queued Ken QCF+PP |
| 1950 | Ryu 12 | 2E08 | 2A28 | 0D3070 | **ROM restored Ryu's own super**; no Ken handoff |
| 1950-2010 | Ryu 12 | 2E08 | 2A28 | 0D30xx | No swap, 60-frame timeout |
| 2759 | Ryu 12 | 3200 | **2A00** | 0D1DAC | Real natural Ryu tag begins |
| 2773 | Ken 14 | 0000 | 2A00 | 0D1DEC | ROM actually swaps point/reserve |
| 2774 | Ken 14 | 2A0C | 2A00 | 0D5702 | Native incoming tag jump |

**Crucial correction:** The true ordinary outgoing tag uses mode 2A00,
NOT mode 2A28. Earlier provisional 87.145 notes incorrectly claimed
outgoing Ryu's normal tag remained 2A28. Its super does remain in 2A28;
the native tag controller switches to 2A00 as a separate action.

The 87.145 DHC handler merely overwrote the outgoing animation without
changing that controller. The game rewrote the sprite on its next frame.

## Actual ROM control path recovered

Independent disassembly of the user-provided CPS2 ROM, with no ROM bytes
committed to source, identifies the native command dispatcher:

1. **PC 0x013B48-0x013B94** checks tag input, live partner status
   at +0x224/+0x210, fighter mode/eligibility, and writes actor
   controller **+0x256 = 8** and **+0x259 = 8** when tag is accepted.
2. **PC 0x00C4E4-0x00C4F4** reads actor +0x256, doubles the index,
   and follows the native ROM dispatcher table. Controller **8** points
   to **PC 0x011E50**.
3. **PC 0x011E50-0x011E70** clears actor scheduler +0x06/+0x08,
   clears +0xA4, sets controller +0x259=8, and branches through the
   fighter-specific native handler selected by fighter ID.
4. **PC 0x011B00-0x011B1A** is a downstream native tag action step:
   if scheduler step +0x08 is zero, initialize its tag timer +0x4E
   to 0x1E and select native outgoing action **32/00** through the
   true ROM action dispatcher at 0x01038E.

This is a real native control path. The previous forced 3200 script
skipped it entirely.

## Changes in Stage 87.146

The DHC activation now requires the same valid conditions as before:
- Native ROM-confirmed Ryu 2E08 first super with mode 2A28.
- Selected Ken reserve (ID 14) and user-entered queued QCF+PP or QCF+KK.
- P1 only, experimental flag set only by the standard TEST runner.
- Native ROM table still confirms Ryu's outgoing action 3200.

**Instead of forcibly writing action 3200**, the host prepares the
native tag-controller state: +256=8, +259=8, scheduler steps +06/+08/+0A
reset, and **+A0=002A (the observed pre-tag state)**.
PC 011E50 dispatches to Ryu's fighter-specific handler at **033762**,
which copies the prior high byte into A1 and sets A0 high to 2A.
Because the pre-tag high byte is zero, the ROM then produces the actual
tag mode **2A00**, rather than incorrectly producing 2A2A.
It then waits for the ROM itself to select 3200 and logs
NATIVE_CONTROLLER_ACCEPTED_OUTGOING_3200 if that happens.
If it never happens within 12 frames, it restores only the captured
controller fields when the fighter identities remain unchanged,
logs NATIVE_TAG_CONTROLLER_REJECTED_12F, and aborts without
manipulating the partner or opponent.

If the controller selects 3200, a 60-frame watchdog waits for the ROM
point/reserve identity swap. Only then can the incoming kickless Ken
landing and queued super code run.

## Lockout corrected

The old used[0] flag was assigned at attempt start, and never cleared
after a timeout until the game exited phase 8. It effectively allowed
only **one attempt per match**.

87.146 now distinguishes:
- A **failed attempt**: discard its consumed queue entry, clear its
  pending controller, enter six frames of recovery, and permit the
  next intentional Ken super-command input to retry.
- A **successful incoming super**: lock any further DHC for that chain.
  Rearm once the incoming super exits into a genuine non-super action.

Never retry automatically from a held button chord, and never turn a
normal tag into a DHC.

## Better native diagnostics

The output F:\xmvsf\test\87.146\dhc_native_handoff.tsv logs every
frame during controller acceptance, native fighter swap and landing,
including controller bytes +256/+259, native scheduler steps +06/+08/+0A,
timers +4E, A4, partner readiness +214, +224 partner pointer, native
script+34/+38, action, mode, fighter identities and X/Y.

A read-only **REFERENCE_REAL_ROM_NORMAL_TAG** event also samples the
same fields during ordinary ROM-triggered Ryu tag-outs. That allows
one recording to compare the real and DHC controller pathways directly.

## Test

In GitHub Desktop, Fetch/Pull branch feature/dhc-mvc2-stage87.
Run the **same root names**, BUILD_TEST.bat and RUN_TEST.bat.
Only runtime/stable/XMVSF_Test_87_146.exe is built/run.
Main, Stable, and all old TEST binaries remain untouched.

P1 Ryu (point), Ken (reserve): start Ryu QCF+PP native super,
then input Ken QCF+PP or QCF+KK. See whether Ryu actually changes to
native 3200 and Ken tags in. If the attempt fails, release the buttons
and execute a fresh Ken command **within another Ryu super** to verify
you are no longer locked out. Also perform a normal tag as reference.

Send dhc_native_handoff.tsv with super_chat.tsv and rom_dispatch.tsv.

**Still experimental.** The ROM may impose additional eligibility
and actor scheduler conditions even after native controller 8 is set.
The code does not yet fully implement extra meter spending, world freeze,
portrait bypass, previous-super effect cleanup, or native first-hit
release. C++11 isolated tests validate state transitions, retry safety
and do-not-touch-normal-tags; actual Windows-native gameplay must be
observed before claiming a playable DHC.
