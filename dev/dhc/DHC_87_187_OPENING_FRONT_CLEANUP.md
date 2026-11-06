# XMVSF 87.187 — original 87.186 opening-front cleanup

Hold point: the provided 87.186 native capture contained 19 completed Ryu-to-Ken Shoryureppa DHC handoffs. This work leaves the original binary, recording, Stable, ROM scripts and gameplay handoff untouched.

## Actual native failure evidence
- A startup input at 2524 was held until the ATTACK arm at 2544 (20 frames). There was no new physical punch at arm. The controller timed out at 2557; Shinkuu restarted at 2558 without input.
- A startup input at 2565 was held until the ATTACK arm at 2625 (60 frames). No fresh physical punch at arm. The controller timed out at 2638; Shinkuu restarted at 2639 without input.
- ATTACK mode 2A02 at 10629 armed controller 8 but original 68000 selected 2C02 with D6=00 at 10630. Successful active ATTACK arms in the capture used 2A28; RECOVERY 2A02 remains allowed because it completed native handoffs.
- The old error label for every wrong action read 2C04, although original ROM selected 0002 at 5181/6856 and 2C02 at 10630.

## Code changes
1. Discard startup-selected requests immediately: recognize the motion but do not carry it to the next ATTACK frame.
2. For ATTACK requests only, require selectedAt equal to now. First-four-attack acceptance stays experimental and is now tested only on a new same-frame motion/chord.
3. For ATTACK only, require 2A28 instead of the unverified 2A02; RECOVERY 2A02 and the original 87.185 rebound are untouched.
4. Abort messages now distinguish original-ROM 0002, 2C02, 2C04 and other actions. Existing abort and latch cleanup remain the same.

## Verify without regressions
- Run the new pure C++11 test dev/dhc/tests/test_dhc_opening_front_187.cpp (picked up by the existing CI test_dhc_*.cpp glob).
- Run the original root BUILD_TEST.bat on Windows MSYS2. TEST output is runtime/stable/XMVSF_Test_87_187.exe, never Stable or the previous 87.186 EXE.
- Use original RUN_TEST.bat for live DHC and RUN_REPLAY.bat for original F9 capture. New logs go to test/87.187, old test/87.186 stays intact.
- Compare all previously completed DHCs. Confirm no controller arm at 2544 or 2625 without new punch, preserve the successful active 2A28 and recovery 2A02 paths, and distinguish 0002/2C02 failures.
- Real native first-four-frame acceptance has not yet been proved; the only early failures in 87.186 were stale STARTUP requests, not actual new ATTACK presses.
