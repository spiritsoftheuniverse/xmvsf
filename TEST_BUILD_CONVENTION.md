# XMVSF TEST build convention — effective 87.255

**GitHub is source control, not a build machine.** Do not add GitHub Actions, remote builds, CI workflows, monitoring jobs, or automated source testing on GitHub. Native builds, ROM gameplay, and final verification run locally through the existing Windows `BUILD_TEST.bat` / `RUN_TEST.bat` only.

## Hard release gate

1. Every change to native runtime, UI, frame-data capture, controllers, or build scripts includes a regression for the behavioral contract it changes. Preserve older tests that still describe valid behavior; never change an expected value merely to silence an assertion.
2. Run every available **ROM-free C++ unit test** and **required Python integration check** before the native build begins. Execute all independent tests even if one fails: collect compile failures, assertion failures and Python errors, and print one consolidated `REGRESSION SUMMARY` to the permanent `test/build_logs/BUILD_TEST.log`.
3. If any preflight check fails, return nonzero and **do not start native compilation or publish the TEST executable**. Preserve Stable, previously published TESTs, cached objects and logs. A local `BUILD_TEST.bat` result of exit 0 is the minimum criterion for describing the version as **build-verified**.
4. A change pushed through the GitHub connector without execution of the local Windows scripts is a **TEST candidate**, not `PASS` or a confirmed runnable build. Source inspection, structural checks, and a successful Git commit cannot substitute for the local compiler and full regression suite. Never describe an untested candidate as verified.
5. Before calling a version **gameplay-verified**, run `RUN_TEST.bat` and check the affected behavior on native Windows. Report the distinction between compiled, unit-tested, and actually exercised gameplay.
6. Use the cached incremental build. Do not `make clean`, discard the warm buildtree, overwrite Stable or earlier TEST binaries, or add installer/security prerequisites. Changes to test harnesses must not invalidate unrelated cached native objects.
7. On a regression, repair the underlying production code first; retain exact test expectations when they measure genuine contacts/timing. A changed product contract must include an explanation and regression coverage for both old and new behavior. Fix all failures exposed by the consolidated log in the same development revision when possible.

## Practical workflow

- Update a **feature branch** in source control, increment the version, add tests, and inspect source compatibility. When no local Windows runner is available in the conversation, mark the commit **unverified candidate**, never claim it passes.
- Run `BUILD_TEST.bat` once. All portable failures must appear together, not one per separate build/push. The native compile/publish phase is gated on an entirely green preflight.
- Only after **BUILD TEST READY / exit 0** and a successful native `RUN_TEST.bat` smoke check should a version be described as confirmed working or considered for Stable.

## 87.255 regression example

The 87.245 trace has **11 actual defender HP decreases** during an active super while the defender's native hit-stop counter independently reloads. The test's `contactCount==11` is correct. `+85` and combo-count fallback signals must never invent extra red ticks while HP remains above the protected minimum. The shared contact observer now accepts such fallback only when the defender's HP has been pinned at `1`; existing HP-confirmed hits remain authoritative.
