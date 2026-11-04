# XMVSF Stage 87.145 — fix native DHC gate from actual 87.144 capture

## Forensic source: user-supplied 87.144.zip

The attached user TEST archive contains five logs, including
dhc_native_handoff.tsv, rom_dispatch.tsv, super_chat.tsv and a F10 RAM
capture. The DHC log contained **only its header**, although the ROM and
chat logs establish that the first-super detector and command queue worked:

- Ryu's ROM-confirmed super action 2E08 started at frames 2327 and 2570.
- Ken reserve QCF+PP was matched/queued at frame 2422 (and 2455).
- Ken reserve QCF+KK was matched/queued at frame 2651.
- At frame 2422, P1 was Ryu ID 12; point action 2E08; native
  mode +A0 = **2A28**; script +34 = 0D30E0; reserve ID 14.
- At frame 2651 the actor was again Ryu in 2E08 / 2A28, with script
  0D30E0. The DHC branch silently required **2A02**, a state not
  present in those super frames. That condition prevented all handoffs.

The same capture contains a real **ordinary** Ryu-to-Ken tag:
- Frame 2763: Ryu outgoing action 3200, native mode **2A28**.
- Frame 2776: ROM exchanges P1 point/reserve IDs 12->14 / 14->12.
- Frame 2777: Ken incoming flying tag 2A0C, native mode **2A00**.
- Frame 2802: Ken reaches ground with native action 2A0E at Y=472.

This proves the 87.144 outgoing-tag state assignment was ALSO wrong:
it wrote 2A00 *before* the ROM swapped fighters. 2A00 belongs to
incoming Ken, not outgoing Ryu.

## Actual 87.145 corrections

- Accept ONLY Ryu point 2E08 AND actual native mode **2A28** with
  a queued Ken super during the genuine ROM-confirmed super window.
  Keep every other DHC eligibility restriction; ordinary tags cannot
  trigger because a queued super and super window are required.
- During the outgoing scripted tag action 3200, **preserve** original
  mode 2A28 rather than writing incoming Ken's mode 2A00.
- If a real ROM ownership swap and Ken native grounded tag-in happen,
  the queued Ken super initializes its own real native mode **2A28**,
  replacing tag-in mode 2A00.
- A queue-selection rejection now writes one explicit BLOCK_* reason,
  including the live fighter action, mode and script. This prevents
  another blank trace caused by a silent guard.
- The DHC trace records every frame during the ROM swap and landing
  waits. It includes +06/+08/+0A native scheduler steps, the +34/+38
  script pointer pair, native partner +224, actor and reserve +210
  statuses, positions, modes and fighter IDs.
- Ken position/velocity are no longer modified when the incoming ROM
  super-script initialization fails. Ordinary tags and Stable remain
  untouched.
- The Warcraft-style chat now labels recognized commands as HANDOFF
  PENDING instead of always saying NO EXECUTION.

## How to run 87.145

Select feature/dhc-mvc2-stage87 in GitHub Desktop, Fetch/Pull origin,
then run the existing **root** BUILD_TEST.bat and RUN_TEST.bat.
They produce/launch only runtime/stable/XMVSF_Test_87_145.exe.
Runahead stays disabled in TEST, and XM87144_DHC_NATIVE_TEST=1 is set
only in RUN_TEST.bat. Stable, main and prior TEST binaries are unchanged.

In Training or Versus:
1. P1 selects Ryu as point, Ken as reserve.
2. Start Ryu's real Shinkuu Hadoken (QCF+PP).
3. DURING his super, enter Ken's QCF+PP or QCF+KK once.
4. Observe whether Ryu enters his tag-out animation, then whether
   the game actually swaps fighter ownership and Ken flies in.
5. Send **test/87.145/dhc_native_handoff.tsv**, plus
   **test/87.145/super_chat.tsv** and **rom_dispatch.tsv**.

The DHC log's decisive events are
DHC_APPROVED_ROM_OUTGOING_ACTION -> REAL_ROM_POINT_RESERVE_SWAP ->
DHC_NATIVE_SUPER_REPLACES_KICK__UNVERIFIED_FX.
A lack of swap now includes per-frame native scheduler and partner
state records, while a wrong eligibility condition is explicitly
named as BLOCK_*.

**Scope:** This is still an opt-in first Ryu->Ken native transaction
experiment. It is NOT verified as fully playable. The actual ROM
handoff may require more than native script initialization; no
additional meter debit, super-portrait skip, first-active freeze
release or comprehensive per-fighter DHC coverage has been claimed.
Synthetic C++11 tests exercise transitions; Windows gameplay is
the decisive verification.
