# XMVSF Stage 87.181 — Native observer lifecycle and missing-file diagnosis

## Reported symptom

The user launched 87.180 through RUN_TEST.bat. The executable exited
normally (code 0), but no 87.175 native-observer TSV was found under
test/87.180/observer/.

This output does NOT prove DHC failed or succeeded. It shows the reporter
did not find the expected file. In 87.180 the observer was initialized
only inside the post-CPU block gated on CpsRamFF, CpsRom, driver name
starting xmvsf, and no runahead frame. If this gate did not run, the
observer created no log and provided no reason. The user might have
exited from the application menu before loading a CPS2 game, but the
single warning cannot establish that.

## 87.181 fix: initialize even before combat

1. At CPS2 frame entry, when XMVSF is loaded, initialize both the
   original 87.175 observer and the newer 87.180 subframe logger. These
   open their append-only session headers before native combat mode 8,
   rather than waiting for the late DHC handler.
2. A new read-only health logger in
   src/burn/drv/capcom/xmvsf_stage87_181_observer_health.h records
   CPS2_FIRST_FRAME, XMVSF_FIRST_FRAME, XMVSF_FIRST_COMBAT, RAM/ROM
   pointer availability, old/new observer readiness, and occasional
   heartbeat frames.
3. RUN_TEST.bat and RUN_REPLAY.bat create persistent launch markers
   before starting the EXE and exit markers when the EXE returns, even
   if the game never reaches CPS2.
4. The Python analyzer ANALYZE_OBSERVER_HEALTH_181.py automatically
   prints a diagnosis and saves a JSON summary after each TEST/REPLAY.
   It counts only the latest appended native observer session, not
   stale rows from previous attempts.

Possible diagnosis codes:
- NO_CPS2_FRAME: app returned without reaching the CPS2 frame hook.
- NO_XMVSF_DRIVER: a different CPS2 game was run.
- OBSERVER_FILE_NOT_CREATED: XMVSF frame ran but logger did not open.
- OBSERVER_NOT_READY: native logger disabled or open failed.
- NO_COMBAT_FRAME: XMVSF opened but no native combat mode 8 occurred.
- NO_NATIVE_OBSERVER_ROWS: native combat occurred but no snapshots.
- NATIVE_OBSERVER_CAPTURED: the latest session has fighter rows.
An observer header or a process exit code of 0 is NOT proof of
a native DHC command being accepted.

## One-click workflow

Pull feature/dhc-mvc2-stage87. Use BUILD_TEST.bat and RUN_TEST.bat,
keeping the original root filenames unchanged. Stage 87.181 builds
runtime/stable/XMVSF_Test_87_181.exe, leaving all older TEST binaries
and Stable untouched. To produce DHC actor rows, enter actual XMVSF
Training or Arcade combat, not just the application menu.

Automatic output in test/87.181/observer/:

- observer_health_test.log — permanent run/app/driver/COMBAT gate record
- observer_health_summary.json — exact reason for missing actor rows
- dhc_acceptance_test.tsv — original 87.175 post-CPU/pre-host trace
- native_subframe_test.tsv — original 87.180 intra-CPU-frame trace

Dedicated F9 replay produces analogous *_replay filenames in the same
versioned observer directory. Old F9 recordings/checkpoints are retained;
the replay runner still discovers previous TEST-version session folders.
The health check never overrides the EXE/replay exit status.

## Safety and verification

This version changes NO DHC gameplay logic. It leaves the 87.180
default-safe delayed prime disabled for interactive TEST, preserves
immediate 2P/3P recognition, native Ken QCF startup and instant
positioning, and does not write ROM bytes or actor state.

ROM-free C++11 smoke and Python classification tests are included in
dev/dhc/tests. Full MSYS2 Windows build and actual CPS2 health output
must still be verified with the user's executable.
