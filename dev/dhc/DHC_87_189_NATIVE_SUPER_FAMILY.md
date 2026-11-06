# XMVSF 87.189 — one original-ROM native super-family inlet

Based on the user's complete 87.188 F9/native capture. This does NOT substitute an arbitrary air-Shinkuu path.

## Native evidence

- Ryu ground Shinkuu enters action 2E08 at ROM script 0D3070, then native 2E0A recovery. Both use D6=0A.
- Air Shinkuu was naturally executed at F9 frames 5213 and 5486, using 2E0C script 0D31A8 and native 2E0E recovery script 0D322C. The first was mode 2A1C; second mode 2A28. Both use native D6=0A.
- Ken Shoryureppa 2E00 uses its own native D6=0C; no universal D6=0A rule is valid.
- The 87.188 handoff log had one confirmed wrong-action case at 1721->1722 (native 0002). Existing abort cleanup and original 87.186/87.188 successes are not weakened.

## Shared inlet

XM87189Family::Tracker accepts ONLY a root step02 original action from native family 2E when the actor's exact first script equals the original per-fighter 68000 ROM table entry. It derives the subsequent script boundary from the next original 2E selector, rather than guessing a fixed 0x80-byte script span, duration, jump height or named move.

Same root action in native step04 (or later even attack steps within the ROM script boundary) is ATTACK. A subsequent 2E selector+2 action in step06 is read-only RECOVERY. Leaving the original native super action/script/controller is action EXIT. An unmatched original ROM script is never a verified root.

The input recognizer now accepts native 2E roots across all fighters without claiming that every table entry is a known player-initiated hyper. Only a known, match-start prepared reserve super can produce a queued DHC command. F9 frame and input history remain authoritative.

## Executable TEST scope

XM87189_TEST_NATIVE_SUPER_FAMILY=1 enables a generic *active attack* inlet to the existing Ryu->Ken DHC native controller 8, with the original ROM-verified Ryu 32/00 tag script, Ken 2E00/2E02 supers, native punch-consumption preflight, same-frame input, 2A28/2A1C native states, and exact wrong-action abort. The original 87.188 ground and 87.185 recovery pathways are preserved.

Other fighter rosters and generic RECOVERY execution remain read-only until their ROM tag-out and recovery bridges are verified. The user requested a shared outlet, not guessed air animations or an assumed universal native tag controller. This is a general super recognizer, not proof all matchups can execute DHC.

The 87.189 intake records DHC_189_NATIVE_2E_ROOT_ROM and DHC_189_ROM_SUPER_FAMILY_FRESH_ATTACK_TAG_TEST. A real success MUST still produce the original 68000 controller consumption, native 3200 tag, true point/reserve swap and the selected Ken 2E00/2E02 ROM super. On native 0002/2Cxx, it aborts without resetting fighters.

## Validation and rollback

- C++11 test_dhc_super_family_189.cpp verifies original-ROM ground and both air roots, Ken D6=0C, startup/attack/recovery/exit, exact-script validation and same-frame attack state.
- The existing root BUILD_TEST.bat produces only XMVSF_Test_87_189.exe; RUN_TEST.bat writes test/87.189, and RUN_REPLAY.bat finds 87.188 historical F9 sessions. Stable and earlier numbered TEST binaries remain protected.
- Set XM87189_TEST_NATIVE_SUPER_FAMILY=0 before RUN_TEST.bat to isolate the original 87.188 gameplay while retaining full logs.
- Real Windows MSYS2 build and replay are required. This source change and the C++11 model cannot prove the air-native controller accepts a tag. Do not merge into the 87.186 hold point until native handoff success and no extra post-super tags are observed.
