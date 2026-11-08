# TEST 87.222 — Classic reentry isolation and access-violation capture

## Supplied 87.221.zip evidence

- Native `observer/observer_health_test.log` ends with Windows exit code `-1073741819` / `0xC0000005` (access violation).
- Native combat started at emulated frame 1444. After continued combat, the observer saw the emulated frame counter return to 780, consistent with a fresh native game session/restart before the crash.
- `tournament_rules.tsv` contains `TOURNAMENT_RULES_ACTIVE` at 780 followed by `CLASSIC_NATIVE_RULES` at 780, confirming a Tournament-to-Classic rule transition across native sessions.
- `observer/dash_startup.tsv` contains 189 dash observer events through frame 2991, including 17 native dash entries, **but no `FS_DASH_LOOP_EXIT` mutation** in this recording. The capture does not implicate the 87.221 watchdog as the direct source of the access violation. No crash instruction address or stack trace was included with the uploaded `87.221.zip`.

## Narrow TEST 87.222 changes

- Preserve TEST 87.220's zero-wait dash startup in both Classic and Tournament.
- Prevent the TEST 87.221 late ground-dash controller restore from running in Classic. Tournament-only safety, because Classic must use untouched native ROM flow for flying-screen and follow-up rules.
- Clear per-fighter dash neutral script snapshots on each Tournament/Classic rule transition, emulated-frame rewind, and noncombat phase. Rule transitions are recognized even when the emulated frame number is identical to the previous frame.
- Put Windows `0xC0000005` crash report and a DbgHelp minidump in **`test/87.222/observer/crash_report.log`** and **`test/87.222/observer/crash.dmp`**, which will now be included when the current observer folder is zipped. Report includes native host checkpoint phase, emulated frame, crash module offset, and Classic-vs-Tournament status. Refresh the unhandled-exception filter on each game's SDL media reinitialization.
- Do not alter the existing 87.95/96/99 Classic/Tournament ROM patch signatures, Cyclops 87.219 recovery, Stable, DHC, or ordinary movement.
- ROM-free regression tests: `test_dash_startup_220`, `test_flying_screen_dash_exit_221`, `test_classic_transition_222`.

**Known limit:** A Windows native executable rebuild and actual Tournament -> Classic -> match test are still required. The immediate root cause of the recorded access violation cannot be known without a crash exception address / stack or a reproducing debugger session. 87.222 is a mode-lifecycle safety and diagnostics pass, not a proven fix for every Classic transition crash.
