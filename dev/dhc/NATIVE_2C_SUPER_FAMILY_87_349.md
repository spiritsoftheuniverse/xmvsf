# XMVSF TEST 87.349 — native 2C is a Hyper, not a missing 2E

Branch: feature/dhc-morphus-precommit-87-334. Incremented local TEST version 87.349 in RUN_TEST.bat (visible header), BUILD_TEST.bat and dev/dhc/BUILD_TEST.sh. No GitHub Actions, additional build gates, original Version 3 or Stable changes.

## Grounded in user's observer(5).zip from TEST 87.348

The archive includes 2,197 DHC event rows (plus header), 27,688 original 68000 actor snapshots, the opcode locator and accepted-ROM site TSVs.

**61** genuine incoming DHC commands reached the original native tag and post-swap M68K guest on BOTH sides. **44** reached the original action 2E. The other **17** were previously called MORPHUS_340_INCOMING_GUEST_NO_ORIGINAL_2E after an arbitrary 16-frame deadline. However the original actor snapshots directly show those **17 actually entered original action 2C** with mode 2A, selected D6 and live advancing original sprite-script addresses, not an idle/tag state:

| Incoming fighter | Original D6 | 2C outcomes called failures | Observed original 2C |
| --- | --- | --- | --- |
| Storm (04) | 02,0A | 3 | 2C08,2C2E |
| Sabretooth (0A) | 16 | 8 | 2C36 and subsequent native 2C actions |
| M. Bison (1C) | 0A,0C | 3 | 2C20,2C2A |
| Charlie (20) | 04,06 | 3 | 2C0E and other native 2C scripts |

One unrelated but useful finding: **18** additional fresh PP/KK chords occurred when both original Super-window and eligibility were true, yet the eight-motion DHC parser did not complete a recognizable incoming motion. A large number of other ignored chords occurred outside actual DHC windows and are expected. No substitute DHC shortcuts were invented.

## Minimal 87.349 code changes

- The native pre-CPU and post-incoming Hyper recognizer in cps_run.cpp accepts **original action 2C or 2E**, but still requires original 2A mode, nonzero D6, actual native swap and matching incoming fighter ID. The 2E path remains unmodified.
- The original-ROM Super lifecycle tracker XM87189Family now supports the native 2C root and stage tables (action 2C, original script/selector) as well as 2E. It tracks original 2C phase 0 and 2, which is where Storm/Bison/Sabretooth/Charlie actually operate; requiring the Ryu/Ken 2E >= phase 4 would have barred their outgoing DHC input windows.
- **Explicit experimental opt-in:** the 2C tracking extension is enabled only by the Version-4 Morphus-native post-CPU tracker update. The original 2E-only tracker behavior for all legacy callers is unchanged. The tracker checks the actual actor, selected D6, original 2C action and script and ROM script bounds; it does not force a host 2E animation, tag, D6 rewrite, or controller-8 input.
- The formerly false timeout event is now MORPHUS_349_INCOMING_GUEST_NO_ORIGINAL_2C_OR_2E. Successful native 2C observations get their **own** MORPHUS_349_ACTUAL_US_2C_HYPER event (selected D6, action, actual original script and original meter). The old MORPHUS_340_ACTUAL_US_2E_HYPER event is retained for all previously proven 2E moves.
- Rejected PP/KK chord diagnostics retain NATIVE_342_COMMAND_NOT_CLAIMED with an `activeHyper` field instead of `active2E`, plus `complete` and `partial` eight-motion bitmasks. This helps separate a missed directional completion from an M68K execution failure in the next real test. It neither consumes inputs nor manufactures motions.

This patch does **not** assert that 17 moves hit the opponent: the native CPU ran their actual 2C actions, but real collisions, projectile construction, and full move identities still require visual testing. The original 2C branch may need more precise handling of actor-specific phase transitions, depending on next original-ROM live evidence.

## Local next test

From the existing PowerShell session with all four opt-in DHC environment variables still set, select Version 4 and original US xmvsfu:

    .\BUILD_TEST.bat
    .\RUN_TEST.bat xmvsfu

Test native 2C incoming Storm (D6=02,0A), Sabretooth (D6=16), Bison (D6=0A/0C), and Charlie (D6=04/06), then explicitly try DHC *out of their own* original 2C Hypers into a second character. Confirm new 2C event, original 2C state and actual gameplay, while preserving a proven 2E Ryu/Ken or Cyclops control.

Next log: F:\xmvsf\test\87.349\observer\dhc_unified_274.tsv.

The C++11 regression source now covers original native 2C phase-0/phase-2 root, 2C stage continuation, D6 matching, ROM-index bounds, and unchanged default 2E-only caller behavior. Source structural and consistency checks are used without adding a mandatory build gate. **Full Windows TEST compilation and 87.349 gameplay have not been executed here**, so true runtime execution remains to be observed.
