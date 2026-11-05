# XMVSF 87.182 — Exact 68000 action-arbitration trace

**Gameplay baseline:** preserve Stage 87.181. The 87.182 changes are
read-only diagnostics, not a revised DHC or a new input timing guess.

## Evidence from the 87.181 F9 capture

15 armed Ryu→Ken DHC controller-8 requests reached 13 successful native
Ken Shoryureppa 2E00 / 0D6722 super actions. Two aborted wrong-action
transitions occur at frames 7016 and 7264; the next attempt succeeded at
7602. The native 68000 subframe observer shows:

- 7016: original punch edge CA=0060 briefly appears, then Ryu action
  becomes 0002, A0=0200.
- 7264: prior held HP C2=0041 and new LP+MP CA=0030 coexist before
  original Ryu action 2C04, D6=00.
- 7580 (control): new edge CA=0030 advances Ryu to A0=2A00; the
  existing **host DHC bridge** then installs the original 3200 script.
  This is host intervention, not the original ROM choosing 3200 by
  itself. Ken's following native 2E00 is still genuinely ROM-selected.

The two failures are different arbitration paths. Replaying another
pulse into the input buffer would risk the severe 87.179 regression.

## New source mechanism

Musashi M68000 in this project's source already supports an
instruction-completion callback, invoked after each 68000 instruction.
SekGetPPC(-1) returns the executed ROM PC, while the original core
m68k_get_reg exposes D0/D1 and A6. No debug-mode recompilation or
68000 instruction-hook configuration changes are necessary.

Stage 87.182 adds only a **read-only accessor** for the previously
installed instruction-completion callback, so the DHC observer can
forward it and restore it immediately after the active CPU frame.

A bounded trace is active only when explicitly enabled and when the
original Ryu(12) point -> Ken(14) reserve, combat mode 8 and DHC phase
WAIT_TAG_CONTROLLER=2 are all present. No other game or non-DHC
frame is instrumented, and F9 runahead frames are excluded.

It records executed PPC, previous instruction PC, next PC,
D0/D1 action and selector, A6 actor register, 68000 action/mode/D6,
controller+256/+259, C0/C2/C6/CA and the native CPU instruction order.
Specific watch ranges include 01038E..0104E4 input/dispatch,
011B00..011B72 tag handler, 013C30..0145E0 matchers, and
032800..034300 Ryu state logic. In addition, native state-changing
instructions are captured regardless of PC range. Logs are buffered
to at most 8192 rows per armed frame and flushed after the CPU frame,
outside the instruction callback.

No original ROM bytes, CPU registers, original input words, native
scripts, DHC move dispatch, actor state or frame-cycle budget are
modified by this instrumentation.

## Offline 68000 opcode-source audit

The private decrypted image at native PC 01038E contains
1D40 0060 1D41 0061, which writes D0/D1 as the actor action and
selector. A literal MOVEQ D0/D1 + JMP/JSR to 01038E scan finds
three possible 0002 request sites, eight possible 2C02 sites,
and seven possible 2C04 sites (18 total).
The derived CSV is
dev/dhc/evidence/ORIGINAL_68000_ACTION_REQUESTS_87_182.csv,
reproduced by dev/dhc/tools/ANALYZE_68000_ACTION_CALLS_182.py.

These are STATIC candidates, not proven executed callers.
The new PC log will establish which request actually ran; it also
captures the preceding ROM PC when the common dispatcher is entered.

## Run and verification

Use the existing root BUILD_TEST.bat. New separate binary:
runtime/stable/XMVSF_Test_87_182.exe. Stable, Stage 87.181,
older TEST executables, warm objects and F9 recordings are untouched.

RUN_REPLAY.bat enables this read-only native PC trace by default,
finds previous 87.181 F9 recordings, and disables the old 87.179
speculative delayed priming to reproduce the 87.181 stable baseline.
The resulting files are:

- test/87.182/observer/native_pc_trace_replay.tsv
- test/87.182/observer/native_pc_trace_replay_summary.json

RUN_TEST.bat defaults tracing OFF for smooth interactive gameplay.
It can be enabled by setting XM87182_NATIVE_PC_TRACE=1 in the
parent CMD session. Corresponding output names have _test instead
of _replay. Earlier 87.175, 87.180 and 87.181 observers remain.

Regression source tests are included:
dev/dhc/tests/test_dhc_native_pc_trace_182.cpp and
dev/dhc/tests/test_dhc_native_pc_analyzer_182.py.
The static original-ROM scan was executed locally and found 18
candidates. The full Windows build, C++ regression executable and
F9 replay have NOT been run here.

**Next actual gameplay change:** use the 87.182 executed-PC and caller
evidence to determine exactly why the original 68000 selects 0002
or 2C04. Avoid another arbitrary input pulse or forced super.
