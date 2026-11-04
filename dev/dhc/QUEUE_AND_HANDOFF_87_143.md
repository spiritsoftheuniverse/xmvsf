# Stage 87.143 — Prepare native supers at match start, execute at DHC time

## Architecture

At the beginning of combat, resolve and queue all known supers for the two
selected fighters. When the point fighter performs the first super, parse
the DHC input against the RESERVE fighter's prepared queue. The match
selects one queue entry; it must not execute it prematurely.

Actual native transaction required:
1. Charge exactly one additional stock and cancel outgoing native super.
2. Invoke the ROM's tag-out path, removing outgoing point fighter.
3. Invoke ROM tag-in path for reserve, with natural jump landing next to
   opponent on same side rather than outgoing fighter's position.
4. Suppress tag-kick behavior on DHC path only. Ordinary tags unchanged.
5. After native landing, use reserved action/subaction and ROM dispatch
   for the queued incoming super. Skip portrait script and portrait timer.
6. Keep world/opponent held during incoming startup and release precisely
   at first active native attack frame. One DHC per chain.

Preloading definitions is not pre-executing scripts. All actor state,
stock usage, animation, hit resolution and renderer timing occur at DHC time.

## Implemented now (read-only)

The new xmvsf_stage87_143_super_queue.h reads point/reserve character IDs
and resolves stored native ROM-script options as soon as the combat roster
is valid. It builds per-side, per-selected-fighter super queues, persists
them through normal tag swaps without rebuilding, and refreshes for a
different roster.

The Ryu and Ken command catalog is used by both the queue and the input
chat rather than maintaining competing proxy lists. While a ROM-verified
first super is active, an input match selects the reserve fighter's
prepared entry, emitting DHC QUEUED ... ROM=x/y; NO EXECUTION.

Current catalog, deliberately limited to the user-confirmed commands:
- Ryu 12: QCF+PP (native script 0D3070); QCB+KK (script unknown).
- Ken 14: QCF+PP (native 2C00/2C04 alternatives); QCF+KK
  (native 2E00 candidate). Mapping the Ken script variants to the
  exact command strength is not yet proved.
- All other fighters: no invented entries. Expand from ROM evidence.

## ROM leads

- Native 68000 PC 01038E is fighter action selector: D0 action,
  D1 subaction, actor A6, fighter-ID-indexed table at 08A946+ID*2.
  A script pointer is data, not a callable C++ super function.
- PC 011B16 selects native outgoing tag action 32/00.
- PC 01147E reads the partner pointer at fighter +224, checks +210,
  switches A6 to partner and calls 01038E with 00/00 to initialize
  reserve state. Its callers and invocation requirements still matter.
- User trace observed 14 frames between Ryu outgoing action 3200
  and fighter ownership swap, then Ken incoming script 2A0C.
  That is a normal tag, not DHC.

These give the allocation route, but do not yet identify the safe 68000
call context needed for a host-triggered DHC. Do not write actor
coordinates/facing after native tag as in the retired 87.138 experiment;
that corrupted ordinary tags.

## TEST

Pull experimental branch feature/dhc-mvc2-stage87 and run the existing
root BUILD_TEST.bat and RUN_TEST.bat. The new TEST executable is
F:\xmvsf\runtime\stable\XMVSF_Test_87_143.exe.

The chat transcript under F:\xmvsf\test\87.143\super_chat.tsv reports
MATCH-START QUEUE and DHC QUEUED entries. ROM script/role events remain
in rom_dispatch.tsv; Shift+G/F10 continue to work.

GitHub isolated C++11 tests cover queue initialization, Ryu/Ken
descriptors and native script lookup, tag-swap persistence and roster
change rebuilding. Native Windows game compilation and actual DHC
execution are still unverified. Stable is untouched.
