# XMVSF Stage 87.178 — Native 3P edge and failed-tag controller cleanup

**Evidence:** user-provided 87.177(1).zip; checked `dhc_native_handoff.tsv`,
`super_chat.tsv`, `observer/dhc_acceptance_test.tsv`,
`rom_dispatch.tsv` and original F9 host-session manifest. The 3P
report was accurate. Do not infer that the prior 87.177 filter caused
all later failed two-punch handoffs; the recording includes an earlier
failure before 3P.

## What actually happened (all seven queued DHC commands)

| Physical host gesture | Queue frame | Outcome frame | Actual ROM/result |
|---|---:|---:|---|
| LP+HP | 11801 | 11825 | Ken 2E00 Shoryureppa accepted |
| LP+HP | 12231 | 12253 | waited for +4E timer; tag controller timed out |
| LP+MP | 12783 | 12807 | Ken 2E00 Shoryureppa accepted |
| MP+HP | 13242 | 13266 | Ken 2E00 Shoryureppa accepted |
| LP+HP | 13858 | 13882 | Ken 2E00 Shoryureppa accepted |
| LP+MP+HP / 3P | 14301 | 14302 | Ryu +A0=0200, +60=0002; no 3200 tag |
| MP+HP | 14766 | 14783 | waited for +4E timer; tag controller timed out |

At frame **14301**, the host recognized QCF+3P, scheduled the DHC
tag controller, and wrote native Ryu +256/+259 = 08, +A0=002A.
The new 87.177 prequeue filter had presented **zero punches** to
the CPS2 input mapper. At 14302, the original 68000 had +CA=0000,
action 0002, mode 0200 and +256=00. The engine aborted, leaving
host-owned +259=08. At 14304, Ryu was again on 2E08 but +259
still remained 08. Thus 3P took a different ROM control-flow path
than the successfully recorded physical two-punch handoffs.

For the four successful initial-tag transitions, the next CPU
snapshot had a punch edge in +CA (0050, 0030, 0060 or 0050) when
Ryu reached +A0=2A00. We can *correlate* this edge with the correct
native controller handling. These post-CPU snapshots do not identify
the exact CPU PC/branch; a live instruction trace would be necessary
to prove the edge's precise causal role.

## 87.178 targeted changes

1. **Do not zero 3P at the prequeue native input port.** When the
   existing strict Ryu(12) -> Ken(14) early-cancel window detects
   exactly LP+MP+HP, expose **LP+MP (03)** at `CpsRwGetInp`
   instead of `00`. Keep the real physical LP+MP+HP value for the
   host DHC command recognizer, which still requires QCF and a
   previously verified Ken move. The normal ROM never sees the
   triple-punch dash chord; it does see the same safe two-punch
   edge as earlier working DHCs. This is a hypothesis-backed
   correction, NOT yet an observed 87.178 execution.
2. **Clear only a stale host-owned +259=08 on an aborted tag.**
   The cleanup runs *only* from WAIT_TAG_CONTROLLER, in the enabled
   early-cancel experimental Ryu/Ken pairing, with unchanged owner,
   +256 consumed (00), original captured +259=00, current +259=08,
   Ryu +D6=0A and **not** in native tag action 3200. It restores
   the single +259 byte to the captured value. It deliberately does
   **not** rewrite a ROM-chosen 0002/0200 action or 2E08, meter,
   actor geometry, projectile state, input history, or Ken.
   Trace marker: `DHC_178_STALE_HOST_259_LATCH_CLEARED`.
3. **Maintain the pre-existing WAIT_RYU_SUPER_END input shield.**
   The 87.177 source already gates phase 9 (super timer wait); it
   can't remove the initial one-frame-delayed physical punch edge
   sampled before the DHC was queued. We are not falsely claiming
   phase-9 shielding alone fixes the other timed-out late handoffs.
4. **Keep Ken's native input/arrival** exactly as in working 87.177:
   the 9-frame fast QCF+LP+MP command, no charge and no distant
   jump-in, no hardcoded 2E super selection.

## Risk and verification

The prior 2P failure at 12231 demonstrates an independently
unresolved issue in **late tag activation after waiting for the
Ryu super timer**. The 87.178 latch cleanup may prevent stale
host controller state from complicating retries but does **not**
prove that the late-window input-recognition timing is fixed.
That remains a separate investigation.

A standalone C++11 test checked native 2P mask and six-bit
chords, phase-nine isolation, and conservative +259 restoration
conditions. Full Windows/MSYS2 compilation, deterministic F9
playback, and new QCF+3P runtime behavior **have not been verified**
in this research environment.

Root launchers remain BUILD_TEST.bat, RUN_TEST.bat, RUN_REPLAY.bat.
Build target is a separate XMVSF_Test_87_178.exe. Keep all older
87.177 F9 files, prior TEST executables and Stable unchanged.

## Live acceptance criteria

- QCF+3P during active Ryu Shinkuu Hadoken shows host 3P
  but native prequeue injectedButtons=03 (not 00).
- Next post-CPU Ryu actor follows the verified tag controller
  to 2A00/3200, not the 0200/0002 branch seen at 14302.
- Ken arrives near opponent without charge and naturally starts
  native 2E00 / script 0D6722.
- After a wrong-action or timed-out tag, the DHC-owned +259
  latch is cleared only when the recovery predicate holds;
  subsequent real 2P commands are free to queue again.
- Ordinary punch combos, normal tags and all other fighters
  remain unchanged; F9 replay and Stable preserved.

The 87.177 F9 movie can test regression and deterministic replay.
A NEW 3P capture on 87.178 is required to prove this corrected
runtime path, because old movies reflect the old native input filter.
