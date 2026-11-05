# XMVSF Stage 87.183 — Prevent consumed-punch DHC handoff faults

## Actual evidence: user's 87.182.zip

The 87.182 archive contains one complete F9 V2 session (15,562 frames,
five checkpoint segments), 42 host DHC queue messages, the 87.175
post-CPU actor observer, the 87.180 subframe CPU observer, and original
native handoff event logs. The 87.181 observer health check succeeded.
**No native executed-PC trace was included:** RUN_TEST left
XM87182_NATIVE_PC_TRACE=0 by default. This is corrected in 87.183.

The 42 observed queue results break down as follows:

- **23 actual controller-8 handoffs armed**: 20 reached native Ken
  2E00 / script 0D6722, two instead selected Ryu 2C04 or 2C02, and
  one stuck in Ryu A0=2A2A until controller timeout.
- **9 queued early requests** were deliberately released by the
  experimental delayed-prime safety policy. No controller was armed.
- **10 queue requests** were blocked because Ryu was not in the
  currently verified action 2E08 (often 2E0A).
- The exact original 87.182 20/23 armed success rate is 87.0%;
  87.181's 13/15 was 86.7%. These are **armed-tag** success rates,
  not total user input acceptance rates. The new build must not
  claim that the earlier nine delayed drops have been solved.

### The exact native input-history predictor

At each native controller-8 arm, BEFORE any DHC host writes,
the original 68000 fighter RAM input words C0 (raw currently held)
and CA (facing-adjusted new press) perfectly separate all 23
observed outcomes in this one capture:

- **20 successes**: C0=0001 and CA=0000 (punch group 0070 clear).
- **Failure 3245 (LP+HP)**: C0=0011, CA=0010; next native
  CPU selected Ryu 2C04 / D6=00 at 3246.
- **Failure 5111 (LP+MP)**: C0=0011, CA=0010; next native
  CPU selected Ryu 2C02 / D6=00 at 5112.
- **Failure 10756 (LP+MP)**: C0=0031, CA=0030 and original
  Ryu A0=2A02; next native CPU ended in A0=2A2A, retaining
  controller +259=08 until a timeout at frame 10769. Clearing
  +259 on abort did not immediately restore the original native A0.

All original per-arm observations are preserved under
dev/dhc/evidence/ORIGINAL_87_182_ARMED_NATIVE_EDGE_23_ATTEMPTS.csv.
These observations do NOT identify the exact 68000 branch PC;
that is the purpose of the native PPC trace.

### Narrow safety correction

The new pure helper
src/burn/drv/capcom/xmvsf_stage87_183_native_edge_preflight.h
accepts an already catalogued 2P/3P DHC input for native controller
handoff **only if (C0 | CA) & 0070 == 0** in the actual ROM-owned
actor input words. No arbitrary super registry or physical punch
gesture is altered.

This preflight is used in
src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h **before**
the DHC host captures and writes controller 256/259 or mode A0.
When native punch bits were already registered, the engine records
DHC_183_NATIVE_PUNCH_ALREADY_CONSUMED_SKIP_TAG, discards the
host-only queued request and sets a six-frame retry cooldown.
It does **not** stage controller 8, rewrite native input history,
prime synthetic punches, force 3200, or change Ryu's current ROM
action/script. A fresh, deliberate attempt may be made after the
cooldown. The source is still the ROM; this guard is a pre-arm
safety policy rather than a decoded replacement command.

The 23-case ROM-free regression predicts **20 preserved handoffs
and three newly skipped unsafe arms**. It does not prove that every
future game attempt succeeds or that ordinary native moves cannot
occur following a rejected host DHC gesture.

### Make the exact PC diagnostic actually run

RUN_TEST.bat now **forces** XM87182_NATIVE_PC_TRACE=1 and
XM87183_REPLAY_UNSAFE_CONSUMED_EDGE=0 for interactive safety.
The original Musashi completion callback is active only during
an armed Ryu->Ken controller frame; the prior callback is preserved.

RUN_REPLAY.bat enables PC trace and defaults
XM87183_REPLAY_UNSAFE_CONSUMED_EDGE=1 to **reproduce the three
old 87.182 bad handoffs in an offline F9 replay only**. This bypass
is not used by RUN_TEST. Historical verification may report
divergence due other changed runtime states, so inspect the raw
native executed-PC TSV rather than claiming deterministic replay.

The existing original root launchers remain BUILD_TEST.bat,
RUN_TEST.bat and RUN_REPLAY.bat. Build target:
runtime/stable/XMVSF_Test_87_183.exe. All previous TEST binaries,
Stable, cached source objects and original F9 recordings remain.

Expected new diagnostics after successful Windows build and play:

- test/87.183/observer/native_pc_trace_test.tsv
- test/87.183/observer/native_pc_trace_test_summary.json
- test/87.183/observer/dhc_acceptance_test.tsv
- test/87.183/observer/native_subframe_test.tsv
- test/87.183/observer/observer_health_summary.json

Replay produces corresponding _replay files. The PPC trace is
bounded to 8192 watched/changed instructions per armed frame and
records any dropped samples explicitly.

## Verification boundary

The supplied 87.182 capture and all 23 original controller-8
outcomes were inspected. A standalone C++11 regression of the
native punch bit guard passed with all 23 cases: 20 preserved,
three declined. GitHub source was updated and source references
can be checked; **full MSYS2 Windows FBNeo build, actual 87.183
gameplay, and 87.183 F9 playback have not yet run here**.

The remaining engineering target is making early/delayed 2P/3P
requests reliably reach an original native controller acceptance
without a second synthetic pulse. First inspect the actual ROM
PPC/caller trace of the three unsafe 87.182 cases from replay;
do not guess another frame delay or promote a skipped request
to a successful DHC.
