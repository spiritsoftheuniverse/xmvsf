# Stage 87.150 — suppress Ken's FIRST incoming kick, not the second one

## Evidence: the user's 87.149.zip

The supplied native handoff log, ROM dispatch, super chat, and F10 combat probe establish two independent reasons why Ken's attack was not suppressed, although the repeated kick was removed.

**First DHC** (Ryu native super at 1742, DHC at 1850):
- 1865 ROM swaps point Ryu 12 to Ken 14.
- 1866 Ken's ROM creates incoming 2A0C / D6=10 at X304, Y408, with its native attack pose/hitbox.
- 1880 Ken is at Y461, 11 pixels above floor 472.
- 1881 Ken reaches Y467. Old code *only then* changes D6=12 (exit), but leaves action 2A0C and its hitbox active.
- 1881–1890: 2A0C **continues**; 1891 finally enters non-attacking 2A08. The old code has suppressed no initial kick.
- 1904 old controller aborts as KEN_ROM_EXIT_2A08_NEVER_CONFIRMED, despite F10 probe showing Ken's real 2A08 continues through frame 1957, transitioning to non-tag neutral around 1958. The old 22-frame timeout was premature and never dispatched queued Shoryureppa.

**Second DHC** (native Ryu super 2576, command 2656):
- 2671 ROM swaps point to Ken; 2672 incoming 2A0C starts.
- 2690–2695 incoming Ken **stays at Y461**, 11 pixels above floor.
- 2696 the ROM abruptly switches to grounded **2A0E** at Y472. There is no intervening post-frame sample within the old 8-pixel proximity window, so kickless D6=12 never activates at all.
- Ken completes the native kick. Later normal tag returns Ryu.

Tournament audit reports TOURNAMENT_RULES_ACTIVE at frame 780, with both live setting and native 95/96/99 patches installed. No Tournament regression was shown in this test.

## Stage 87.150 correction

After actual Ryu→Ken ownership swap, on the **first native Ken incoming 2A0C / D6=10 frame**, regardless of vertical distance from the floor:

1. Confirm original Ken 2A08 exit script resolves from loaded game ROM to 0D567E.
2. Position Ken at ground Y and original Ryu-side opponent X ± 48, face opponent, and zero motion.
3. Immediately initialize original **2A08 script AND D6=12 native exit handler**, plus native animation scheduler +08=2, mode 2A00, and zero entry controller/attack latch.
4. Its hitbox is then taken from the **non-attacking ROM 2A08 script**, not left on 2A0C. The old D6-only delayed suppression is removed.
5. If the ROM reasserts 2A0C/2A0E on the following frame, allow ONE bounded re-cancellation; log a rejected suppression rather than repeating kicks indefinitely.
6. After the game actually retains action 2A08 / D6=12 at floor Y with no active hitbox for at least two frames, DHC intentionally **cancels the non-attacking exit into the queued native Ken super handler** (D6=0A Shoryureppa or 0C Shinryuken, corresponding to selected QCF+PP/KK). This avoids waiting the full ~67-frame natural 2A08 exit that caused 87.149 to time out.
7. Only claim native super action success if the ROM truly selects the corresponding 2Cxx/2Exx action family. Extra meter, freeze and portrait logic are still outstanding.

Strictly DHC-only, currently P1 Ryu point/Ken reserve. The super charging streaks and ordinary native tags remain untouched. The Tournament native rule flags, menu sync, and tournament_rules.tsv audit are preserved without alteration.

## Test

Pull the experimental feature/dhc-mvc2-stage87 branch and run the unchanged root BUILD_TEST.bat followed by RUN_TEST.bat.

Dedicated new executable:
F:\xmvsf\runtime\stable\XMVSF_Test_87_150.exe

With P1 Ryu point/Ken reserve, perform Ryu Shinkuu Hadoken then the queued Ken QCF+PP Shoryureppa command DURING Ryu's super. Confirm Ken **does not visibly attack during entry** and lands beside the opponent, then check whether native Shoryureppa begins. Do an ordinary tag for comparison; it must remain unaffected.

Files under F:\xmvsf\test\87.150:
- dhc_native_handoff.tsv: DHC_KEN_2A0C_ATTACK_SUPPRESSED_IMMEDIATE_2A08, KEN_ROM_NONATTACKING_2A08_CONFIRMED, DHC_CANCEL_KEN_2A08_EXIT_TO_NATIVE_SUPER, and ideally DHC_KEN_NATIVE_SUPER_ROM_ACTION_CONFIRMED
- rom_dispatch.tsv and super_chat.tsv: native queue/ownership/super
- tournament_rules.tsv: independent Tournament status.

The isolated tests validate synthetic native ROM data and prevent the old late-ground trigger. Full Windows native gameplay is not yet verified. Stable, main, root launcher names and prior TEST binaries remain unchanged.
