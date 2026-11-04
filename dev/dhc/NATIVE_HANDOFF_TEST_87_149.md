# XMVSF Stage 87.149 — Ken kickless entry and Tournament rules

## User test 87.148.zip — native frame evidence

- Frame 3142: Ryu point, Ken reserve, Ken QCF+PP Shoryureppa queued.
- Frame 3143: native Ryu outgoing controller, D6=14, action 3200.
- Frame 3157: game ROM swaps fighter IDs: Ken point / Ryu reserve.
- Frames 3158–3182: Ken incoming attack/flight action 2A0C, D6=10.
- Frame 3183: Ken's native grounded entry kick action 2A0E (Y=472).
  Previous DHC host copied Shoryureppa animation 2C04 but retained
  Ken's native incoming D6=10 controller.
- Frame 3184: game ROM rewrote action 2A0C and restarted the entry kick.
  Thus the user saw an interrupted first kick and then a second kick.

Ryu exit now works. The remaining bug is Ken's native action controller,
not user input or the reserve-super queue.

## Native 68000 Ken state selector

The ROM-disassembled Ken actor state dispatcher at PC 035356 reads the
even actor D6 selector and routes:

- D6=10 -> PC 035E4C: native incoming kick/flight, action 2A0C
- D6=12 -> PC 035EFA: real non-attacking entry exit, action 2A08;
  PC 035F0A selects native 2A08 and PC 035F18 returns to ground control
- D6=0A -> PC 035956: native 2C Shoryureppa super family
- D6=0C -> PC 0359F6: native 2E Shinryuken super family
- D6=14 -> PC 035F28: native outgoing Ken tag

Ken action 2A08 resolves in CpsRom to script 0D567E. These ROM
addresses were found from the user-owned ROM and no proprietary ROM
bytes are committed.

## 87.149 implementation

Only during an already queued P1 Ryu-to-Ken DHC, after REAL game ROM
ownership swap, allow native Ken 2A0C flight to progress until Y is no
more than 8 native units above its actual ground-floor byte. Then BEFORE
the 2A0E grounded kick:

1. Confirm the actual ROM script for Ken 2A08 is 0D567E.
2. Put Ken at ground Y, 48 native units to the left/right of the
   live opponent on the original Ryu side, facing opponent.
3. Clear native motion, entry attack latch and scheduler, and set
   Ken behavior selector D6=12, not action 2C04. The ROM must select
   the genuine non-attacking 2A08 exit.
4. Observe a native 2A08 exit and its ROM completion flag or a
   subsequent *non-kick* action. A second 2A0C/2A0E kick NEVER counts
   as completed exit; log it and abort on timeout.
5. Request Ken's actual queued super behavior: D6=0A (QCF+PP) or
   D6=0C (QCF+KK). Let the original ROM select native super animation,
   hitboxes and effects. The host no longer forces 2C04 sprite.
6. Only classify successful incoming super when the ROM actually
   selects the corresponding 2Cxx / 2Exx action family.
7. Existing charge streaks are preserved. Ordinary native tag-ins do
   not enter the new code, and original Stable remains untouched.

Additional meter, portrait bypass, freeze and first-active release are
NOT yet implemented. Linux unit tests do not prove gameplay.

## Tournament rules — mode selected vs actual hooks enabled

The saved Options GAME MODE uses FBNeo/config/games/xmvsfu_rule_mode.ini,
but 87.148 TEST lacked four *independent* enable flags for the current
Tournament-only gameplay modules:

- XM8786_STORM_BUTTON_TEST=1: latest connected Storm flight behavior.
- XM8795_NATIVE_HIT_TEST=1: native dizzy-air collision.
- XM8796_DIZZY_HANDOFF_TEST=1: native dizzy handoff.
- XM8799_GROUND_DEFENSE_TEST=1: native ground-defense and combo close.

87.149 TEST sets all four, but each rule remains gated by the actual
Options Tournament mode. Classic stays ROM-original; do not enable
the older 80/83/84/85 experimental flying-screen writers.

On persistent menu re-entry, GAME MODE now reapplies the selected
Classic/Tournament environment without resetting the selection.

The new read-only tournament_rules.tsv reports each mode/hook
transition as CLASSIC_NATIVE_RULES, TOURNAMENT_RULES_ACTIVE or
TOURNAMENT_RULES_INCOMPLETE. It validates *actual installed* 95/96/99
hooks, not just the menu label. Signature protection remains unchanged.

## Local Windows test

Pull feature/dhc-mvc2-stage87; use the original root BUILD_TEST.bat and
RUN_TEST.bat. The dedicated binary is
F:\xmvsf\runtime\stable\XMVSF_Test_87_149.exe.

With P1 Ryu point and Ken reserve, launch Ryu's true QCF+PP super and
then Ken's queued QCF+PP during it. Look for actual Ken flying-in,
grounded non-attacking 2A08 exit next to opponent, and real 2C-family
super. Make one normal tag to check for regressions.

In Options select Tournament, play and verify the native rule audit.
Switch Classic, and return to menu then start another fight, to check
mode rule continuity.

Share files under F:\xmvsf\test\87.149:
- dhc_native_handoff.tsv (Ken D6, kickless exit and true super)
- rom_dispatch.tsv (native script allocation and fighter swap)
- super_chat.tsv (selected reserve command and DHC phase)
- tournament_rules.tsv (actual installed Tournament native rules)

Stable, main and older TEST executables remain untouched.
