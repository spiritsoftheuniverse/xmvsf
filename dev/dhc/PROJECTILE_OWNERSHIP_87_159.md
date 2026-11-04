# XMVSF 87.159 — Ryu's Shinkuu projectiles are independent native objects

**Correction:** the outgoing Ryu Shinkuu Hadoken is not reclaiming the
player actor or tag controller. Its already-created projectile objects
remain in the P1 native projectile list after Ryu leaves and can keep
hitting while Ken's Shoryureppa runs. The two attacks overlapping may
look like a dual hyper, but no Variable Combination needs to run.

The first 87.158 DHC was genuinely successful:
- Ryu 2E08 -> native tag 3200 at frame 2127.
- Ken becomes point at 2141.
- Ken's incoming 2A0C kick cancelled to native 2A08 at 2142.
- Ken reaches native Shoryureppa 2E00/0D6722 at 2150.
Preserve these events and their order.

**Ownership source:** the existing XM23 native hitbox reader already
follows P1 linked projectile objects at FF(E3D8 - 4*slot), slot 1..12.
Each linked object carries offensive collision IDs at +70 and +72 and
attack gate +82. The P2 object list is offset +150; it is not touched.

87.159 behavior:
- Capture the currently linked P1 objects when the queued Ryu->Ken DHC
  tag begins, and continue capturing late Ryu projectiles up to the
  actual ROM-confirmed point/reserve swap.
- Only when native outgoing 3200 is accepted: clear these objects'
  +70, +72 and +82 fields before the next 68000 CPU frame. Reapply
  after the CPU because the ROM may regenerate descriptors.
- On Ken's native point takeover, freeze the old Ryu identity set. Do
  NOT capture/disable any new Ken projectiles added to the same P1 list.
- Retire tracked addresses when they leave the original list so that
  recycled native object slots are not suppressed.
- Leave sprite and animation fields, projectile coordinates, P1
  ownership links, defensive boxes, Ken's own attack boxes, P2 hitboxes,
  regular Shinkuu outside a DHC, and the existing 87.158 native
  Ryu-to-Ken handoff alone.
- Abort before a confirmed Ken swap cancels the temporary cutoff.
  A failed Ken super *after* the swap does not reactivate leftover
  Ryu projectile offense.

Diagnostic output: `test/87.159/p1_old_projectile_hitboxes.tsv`.
Look for `OUTGOING_RYU_PROJECTILE_CAPTURED`,
`DHC_P1_OLD_PROJECTILE_HITBOX_DISABLED`,
`OLD_RYU_OFFENSE_CLEARED`, and
`KEN_POINT_SWAP_FREEZE_RYU_LINK_SET`.

The synthetic C++11 regression tests model an old Ryu projectile,
a late Ryu projectile, a fresh Ken projectile, and an independent P2
projectile. They confirm only Ryu's tracked offensive descriptors
are removed and preserved Ken/P2 hitboxes remain unchanged.

**Live ROM verification still required:** zeroing these original
offensive collision descriptors is supported by the existing XM23
hitbox reader, but a native emulation playthrough must confirm the
actual ROM collision handler does not re-create/reactivate an attack
between the before/after frame guards. If old Ryu hits persist, trace
the native collision read or install a specific collision-dispatch
guard. Do NOT modify Ryu's super action, force a new DHC, or disable
all P1 projectiles.

Build: standard `BUILD_TEST.bat` and `RUN_TEST.bat`, version 87.159.
Incremental `.o/.d` caches stay intact, Stable is untouched.
The builder suppresses only console warning lines mentioning
`-winwrite`; other diagnostics and nonzero make exit status are retained.
