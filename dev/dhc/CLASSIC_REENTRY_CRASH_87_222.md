# TEST 87.222 — Classic reentry isolation and access-violation capture

## Supplied 87.221.zip evidence

- Native `observer/observer_health_test.log` ends with Windows exit code `-1073741819` / `0xC0000005` (access violation).
- Native combat started at emulated frame 1444. After continued combat, the observer saw the emulated frame counter return to 780, consistent with a fresh native game session/restart before the crash.
- `tournament_rules.tsv` contains `TOURNAMENT_RULES_ACTIVE` at 780 followed by `CLASSIC_NATIVE_RULES` at 780, confirming a Tournament-to-Classic rule transition across native sessions.
- `observer/dash_startup.tsv` contains 189 dash observer events through frame 2991, including 17 native dash entries, **but no `FS_DASH_LOOP_EXIT` mutation** in this recording. The capture does not implicate the 87.221 watchdog as the direct source of the access violation. No crash instruction address or stack trace was included with the uploaded `87.221.zip`.

## Narrow TEST 87.222 changes

- Preserve TEST 87.220's zero-wait dash startup in both Classic and Tournament.
- Preserve the TEST 87.221 runaway ground-dash exit in both Classic and Tournament; the dash watchdog has no recorded mutation preceding this crash. Never carry a captured native neutral script across mode changes.
- Clear per-fighter dash neutral script snapshots on each Tournament/Classic rule transition, emulated-frame rewind, and noncombat phase. Rule transitions are recognized even when the emulated frame number is identical to the previous frame.
- The persistent SDL shell calls `DrvExit()/MediaExit()` at the end of a game, then `DrvInit()` unconditionally called `MediaExit()` again at the next launch. Avoid this second teardown when video/audio/input backends are already inactive; preserve active-backend cleanup. This is a concrete re-entry hazard, **not yet independently proven** to be the crash's exact faulting instruction.
- Reassert the user's currently selected Classic/Tournament rule in the persistent shell **before `DrvInit()`**, so a new game cannot inherit a stale native mode environment.
- Put Windows `0xC0000005` crash report and a DbgHelp minidump in **`test/87.222/observer/crash_report.log`** and **`test/87.222/observer/crash.dmp`**, which will now be included when the current observer folder is zipped. Report includes native host checkpoint phase, emulated frame, crash module offset, and Classic-vs-Tournament status. Refresh the unhandled-exception filter on each game's SDL media reinitialization.
- Do not alter the existing 87.95/96/99 Classic/Tournament ROM patch signatures, Cyclops 87.219 recovery, Stable, DHC, or ordinary movement.
- Update boot title/version labels to 87.222 to identify the running TEST build unambiguously.
- ROM-free regression tests: `test_dash_startup_220`, `test_flying_screen_dash_exit_221`, `test_classic_transition_222`.

**Known limit:** ROM-free C++ unit tests passed for 87.220, 87.221 and 87.222 on the equivalent local dashboard source. A Windows native executable rebuild and the exact user reproduction (main menu → Options → Tournament to Classic → exit Options → start game) are still required. The immediate root cause of the recorded access violation cannot be known without a crash exception address / stack or a reproducing debugger session. 87.222 is a mode-lifecycle safety and diagnostics pass, not a proven fix for every Classic transition crash.
