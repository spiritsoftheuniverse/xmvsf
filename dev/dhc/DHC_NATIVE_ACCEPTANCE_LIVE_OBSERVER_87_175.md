# XMVSF 87.175 — Live native DHC acceptance observer

Stage 87.175 is the first TEST engine-source update since 87.167. Stages 87.168–87.174 were offline research. This is **diagnostic-only** and does not change DHC move execution, native input injection, tag logic, original game RAM writes or Stable.

## Purpose and hook placement

The CPS2 CPU finishes its frame before the current DHC host callback XM87144::state().frame executes. The observer samples the four fighter slots at these precise points:

1. POST_CPU_PRE_DHC — original game has run the 68000 frame, host DHC has not yet executed its frame callback.
2. POST_HOST_DHC_CHANGED — some tracked actor RAM changed during that host callback; do **not** attribute the resulting state to 68000 command recognition.
3. POST_HOST_DHC_PHASE_ONLY — host DHC phase changed, but no tracked actor RAM field changed.

Each row records emulator frame/movie frame, P1/P2 POINT/RESERVE fighter, synthetic DHC input marker and injected direction/attack bits, DHC phase, native command state D6, native mode A0, phase index 08, action/selector 60/61, native script 34, controller 256/259, state A4, and input-history words C0/C2/C4/C6/C8/CA.

This is not PC-level instrumentation. An action-2E observation is NOT automatically proof that the original input matcher accepted a fresh command. The Python summary parser explicitly distinguishes an action-2E echo from a **prior host write** versus a newly observed post-CPU action transition. No DHC move registry, ROM bytes or CPS2 actor state is modified by the observer.

## Sources and safety

- dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_175_native_accept_observer.h: environment-gated, append-only native state logger.
- dev/stable/buildtree/src/burn/drv/capcom/cps_run.cpp: two observation calls wrapped around existing DHC host frame callback.
- dev/dhc/tools/ANALYZE_DHC_NATIVE_OBSERVER_175.py: read-only compact JSON summary, repeated session-header-safe.
- dev/dhc/tests/test_dhc_native_observer_175.cpp: independent ROM-free C++ compile/simulation of four fighter slots and host mutation.
- dev/dhc/tests/test_dhc_native_observer_parser_175.py: Python verification rejecting a host-written 2E echo as native acceptance.

Observer is enabled only when XM87175_DHC_OBSERVER=1 and XM87175_DHC_OBSERVER_LOG identifies a writable file. The existing root launchers set both automatically, create a permanent log folder, and print file-open failures. When invoked outside XMVSF or during runahead, it is inert. All outputs append rather than truncate prior evidence.

**No old TEST or Stable overwrite:** the new binary is runtime/stable/XMVSF_Test_87_175.exe. Existing 87.167 executable remains intact; the builder preserves warm objects and only publishes the new TEST target after successful compilation.

## Existing one-click workflow

Pull feature/dhc-mvc2-stage87. Use BUILD_TEST.bat to compile, RUN_TEST.bat to play, or RUN_REPLAY.bat to replay the existing 87.167 full-session F9 capture. Keep exactly these familiar launcher names.

Logs and automatically generated summaries:

| Channel | Native TSV | Summary JSON |
|---|---|---|
| TEST | test/87.175/observer/dhc_acceptance_test.tsv | test/87.175/observer/dhc_acceptance_test_summary.json |
| REPLAY | test/87.175/observer/dhc_acceptance_replay.tsv | test/87.175/observer/dhc_acceptance_replay_summary.json |

These stay separate from historical F9 recordings and checkpoints. The dedicated replay runner retains backward compatibility with previous 87.167 sessions and produces 87.175 diagnostic output independently. The auto-summary never overrides the original game/replay exit status. Permanent build transcript remains test/build_logs/BUILD_TEST.log.

## Verification status

Linux C++11 standalone smoke: PASS, compiled with g++ -std=c++11 -Wall -Wextra -Werror. Checks four fighter offsets and labels, post-CPU observations, host RAM changes, phase-only changes and file output.

Python synthetic parser test: PASS, including two recording sessions and a 2E00 action first written by the host; that echo is not mislabeled as native acceptance.

GitHub source integration: hook ordering and both root launcher executable paths inspected, confirmed pointing to the new TEST binary.

**Not yet tested:** full MSYS2 Windows engine compilation, actual live DHC execution, playback completion under 87.175, and proof of the incoming fighter's original command-acceptance branch. Do not claim those have passed before logs exist.

Akuma Tenma Gou Zankuu remains excluded. No new supers or arbitrary script dispatch have been enabled.
