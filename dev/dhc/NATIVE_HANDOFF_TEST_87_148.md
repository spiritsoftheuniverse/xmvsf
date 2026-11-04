# Stage 87.148 — native Ryu tag scheduler and exit, preserving charge streaks

## User-provided 87.147.zip evidence

The user observed Ryu starting to leave, then freezing in tag-out pose; Ken did not arrive; super-charge streaks were visible and must remain.

The recorded DHC TSV confirms:
- Frame 1949: Ryu 12 point / Ken 14 reserve, Ken Shoryureppa QCF+PP queued, native tag controller 08 armed.
- Frame 1950: real ROM consumed controller 08 and entered mode 2A00; host selected genuine outgoing script 3200 at 0D1DAC, timer 001E.
- Frames 1951-1964: animation advances through 0D1DAC / 0D1DBC / 0D1DCC / 0D1DDC.
- Frame 1964 onward: animation reaches ROM script 0D1DEC and remains trapped; timer +4E stays at 001E, rather than counting down.
- Frame 2010: watchdog reports no ownership swap, and Ryu remains in the tag-out state.
- ROM dispatch confirms the script; chat confirms the reserve command was queued. No Ken tag-in is recorded.

## Root cause independently traced in decrypted 68000 ROM

Ryu's native action dispatcher at PC 0337BA reads actor +D6 and selects a state handler. The table entry at +D6=14 hexadecimal points to PC 0341F6, which jumps to the real shared tag routine at PC 011B00.

At PC 011B1E the native engine advances the tag script and checks actor flag byte +3C. The original script at 0D1DEC has first word 0100: this is the flagged exit record where the 87.147 trace freezes. On a normal tag, the native code detects the flag and sets controller +256=02, which then dispatches to the tag exit/continuation routine at PC 011B46.

The 87.147 host changed native animation +60=3200 and timer +4E=001E, but did not change actor behavior selector +D6. This is why the ROM never executed the code that advances the tag timer and recognizes the flagged exit frame. Ken's separate native tag state table also maps +D6=14 to the shared tag routine.

## Changes in 87.148

- Once the game itself accepts controller 8, and only during a ROM-confirmed P1 Ryu super with a newly queued Ken super, Stage 87.148 sets the outgoing Ryu actor to +D6=14 when it initializes the actual 3200 action/timer.
- The ROM now has the correct native per-fighter state route to advance 3200, detect the 0D1DEC exit flag, and request the native controller-2 exit. It must still perform the true point/reserve swap; the host does not fake it.
- Each DHC trace row now adds nativeD6 and scriptFlag3C so the next recording establishes whether the tag flag was reached and consumed.
- A failed Hadoken reassertion restores the captured D6 state. A 60-frame no-swap watchdog requests the ROM's own controller 2 as attempted recovery rather than leaving a frozen outgoing pose.
- The charge streaks and other super FX are intentionally untouched; no FX cleanup, suppression or rerouting was added.
- Ordinary tags remain on the original ROM code path. One successful DHC per chain, and failed DHC attempts are retryable.

## Windows TEST

Pull feature/dhc-mvc2-stage87, then run only the existing root launchers BUILD_TEST.bat and RUN_TEST.bat. The binary is F:\xmvsf\runtime\stable\XMVSF_Test_87_148.exe.

P1: Ryu point, Ken reserve. Execute Ryu Shinkuu Hadoken, then during the active super input Ken Shoryureppa QCF+PP. Observe Ryu exit and whether Ken flies in; the charge streaks should remain. If possible, also perform one ordinary tag as a control.

Look in F:\xmvsf\test\87.148 for:
- dhc_native_handoff.tsv: nativeD6, scriptFlag3C, timer, controller 2, and actual swap
- rom_dispatch.tsv: native super, tag-out/tag-in scripts, and point/reserve identities
- super_chat.tsv: queued Ken input and DHC state transitions

The decisive expected ROM sequence is Ryu +D6=14 -> flagged script 0D1DEC, +3C=01 -> controller256=02 -> genuine fighter swap -> native Ken arrival.

This is a native execution attempt, NOT yet verified playable. The incoming super's complete effects/hitboxes, extra meter consumption, portrait bypass, world freeze and first-active unfreeze remain unfinished. Only isolated C++11 tests passed; native Windows gameplay must confirm the result. Stable, main and old TEST executables are untouched.
