# XMVSF Stage 87.180 — Native input clock and wrong-action recovery

Evidence source: user-provided 87.179.zip, comprising DHC event records,
87.175 post-CPU/post-host observer, ROM action trace and original F9 V2
recording across THREE native gameplay sessions.

## Real observed failures: all 16 queued DHC commands

- 4 completed Ryu outgoing action 3200 and Ken native Shoryureppa
  action 2E00/script 0D6722.
- 11 aborted following Ryu native actions 0002, 2C02, or 2C04.
- 1 was blocked because the Ryu source action was 2E0A, not the
  validated 2E08 Shinkuu Hadoken gate.

The derived per-attempt table is
dev/dhc/evidence/ORIGINAL_87_179_MULTISESSION_DHC_OUTCOMES_87_180.csv.

There are TWO independent failure types:

1. The synthetic LP+MP edge is lost by the final post-CPU sample.
   For example, session 2 frame 1700: C2=0031 / CA=0000,
   native action 0002, mode 0200. The pulse is now a previous/held
   input instead of a fresh two-punch press.
2. A fresh edge exists but the original ROM picks a DIFFERENT command.
   Session 1 frame 2430: C2=0000 / CA=0032 yet native action 2C02.
   Session 1 frame 2196: C2=0011 / CA=0040 yet native action 2C04.
   Fresh CA is NOT sufficient to prove native controller-8 acceptance.

Additional confirmed regression: the wrong native 2C02/2C04
actions set fighter D6=00. The older stale-latch predicate required
D6=0A, so it left our own +259=08 controller latch behind after
a wrong-action DHC abort. That can interfere with follow-up commands.

## Corrections applied

### Safe stale-tag latch recovery

The controller +259 cleanup now also covers exactly native action 2C02
or 2C04 with D6=00, while retaining the original strict conditions:
enabled experimental early-cancel, WAIT_TAG_CONTROLLER failure,
Ryu point/Ken reserve and unchanged owner IDs, controller +256 already
consumed, current +259=08 owned by this DHC, and originally captured
+259=00. It never rewinds a ROM-selected action, mode, super animation,
input history, projectile, or fighter ownership. Normal 3200 tags
remain excluded. The older guarded host-stage A0=002A restoration
still applies only to a different, independently qualified case.

### Delayed DHC prime is no longer automatic in interactive gameplay

The 87.179 delayed priming hypothesis was not reliable. The interactive
RUN_TEST.bat sets XM87180_EXPERIMENTAL_DELAYED_PRIME=0 by default:
an early DHC request that cannot enter the verified tag window is
gracefully abandoned, without synthetic 2P injection or controller-8
state mutation. The ordinary Ryu super remains active. This is a
temporary **deferred-DHC limitation**, not a completed timing fix.

All already-working immediate QCF+2P / QCF+3P handoffs remain
unchanged. Ken still arrives near the opponent with no long jump-in
and uses his original short native QCF+2P command; there is no charge
animation and no forced native action-2E dispatch.

The existing RUN_REPLAY.bat intentionally defaults
XM87180_EXPERIMENTAL_DELAYED_PRIME=1 to reproduce the failing 87.179
priming path FOR DIAGNOSTICS ONLY, while RUN_TEST remains safe.
One may override the variable from the parent shell. Historical
ROM-action verification remains strict and may report expected
divergence from changed gameplay, rather than manufacturing a PASS.

### New read-only intra-frame CPU observer

The new header is
dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_180_subframe_input_observer.h

Cps2Frame now captures original fighter input words
C0/C2/C4/C6/C8/CA and native action/mode/D6/controller
+256/+259 at BEFORE_CPU, AFTER_PRE_DISPLAY_CPU, AFTER_SCAN_0,
AFTER_SCAN_1, AFTER_SCAN_2, AFTER_VBLANK_CPU, and AFTER_DHC_HOST.
It also logs CPU cycles, original input-port values and whether a
synthetic DHC signal was submitted. This changes no original 68000
opcodes, ROM data, native input-history RAM, or gameplay.

The new read-only offline analyzer is
dev/dhc/tools/ANALYZE_DHC_SUBFRAME_180.py. It reports any press
edge that appeared and then disappeared **within one frame**, alongside
samples of Ryu's wrong action state and stale controller 8 latch.
A native CA edge is NOT automatically evidence of accepted DHC.

## Build and diagnostics

Use exactly the existing root BUILD_TEST.bat, RUN_TEST.bat, and
RUN_REPLAY.bat. Incremental builder targets an independent
runtime/stable/XMVSF_Test_87_180.exe; preserve Stage 87.179,
all previous TEST executables, Stable, warm object cache and F9 saves.

RUN_TEST automatically writes:
- test/87.180/observer/native_subframe_test.tsv
- test/87.180/observer/native_subframe_test_summary.json

RUN_REPLAY automatically writes:
- test/87.180/observer/native_subframe_replay.tsv
- test/87.180/observer/native_subframe_replay_summary.json

It also preserves the existing 87.175 native pre/post host observer and
the native DHC handoff/ROM action logs. Permanent build transcript is
test/build_logs/BUILD_TEST.log. The user's previous F9 recording is
reused, never rewritten or destroyed.

## Verification boundaries

Confirmed: 16/4/11/1 capture counts, both input-failure families and
D6=00 wrong-action controller-259 latch leaks. Standalone C++11
simulations for the stricter recovery predicate and read-only logger
passed. Repository source and launcher paths have been checked.

Not yet confirmed: full Windows/MSYS2 FBNeo build, actual 87.180
live gameplay behavior, replay completion, or the input refresh
boundary within the original 68000 CPU frame. The next runtime trace,
not another guessed frame count, determines the correct generic
DHC input bridge timing.
