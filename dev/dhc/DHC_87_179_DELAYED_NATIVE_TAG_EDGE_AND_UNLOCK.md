# XMVSF Stage 87.179 — Delayed native DHC tag edge and narrow unlock

**Base:** the user's full 87.178.zip, containing native DHC event TSV,
87.175 observer snapshots, ROM action traces and a single F9 V2 session.
The raw ROM, savestate and input movie are **not** included in the repo.

## The seven actual handoffs

| Queued | Tag controller armed | Input wait | Native CA next frame | Outcome |
|---:|---:|---:|---|---|
| 2192 | 2192 | 0 frames | 0030 | Ken 2E00 at 2216 |
| 2674 | 2674 | 0 frames | 0030 | Ken 2E00 at 2697 |
| 3107 | 3114 | 7 frames | 0000 | controller timeout 3127 |
| 3502 | 3511 | 9 frames | 0000 | controller timeout 3524 |
| 3907 | 3907 | 0 frames | 0050 | Ken 2E00 at 3931 |
| 4403 | 4406 | 3 frames | 0000 | controller timeout 4419 |
| 5017 | 5023 | 6 frames | 0000 | controller timeout 5036 |

These counts are exact for the submitted run, not a guarantee for all
gameplay. The three successes directly stage controller 8 on the
same frame the host matcher queues DHC, and the next ROM frame sees a
fresh punch edge. All four failures wait for Ryu's super timer,
consume and release the original physical punch input before staging
controller 8, and the ROM then sees CA=0000 and remains A0=002A.

87.178 already added a strict cleanup of the host-owned +259=08 tag
latch, and all four failures log that cleanup. It did not solve the
earlier missing native input edge. Each timeout also left the
host-written A0=002A in place for about 10-20 native frames after
the abort, contributing to difficult-to-control follow-up gameplay.

## Correction: pipeline-safe late-tag input prime

The CPS2 host port is sampled **before** the frame's 68000 CPU run;
the original game's actor input history registers update with a
one-frame relationship observed in the 87.178 trace. Merely sending
new punch input on the **first frame after** controller 8 was armed
would be too late: the controller would already have consumed +256.

Instead, 87.179 introduces a single intermediate stage,
`PRIME_DELAYED_TAG_EDGE=11`, ONLY for early-cancel
Ryu(12) -> Ken(14) when the original queued command had to wait for
the safe tag timer window.

1. When the Ryu super timer first enters +4E<=28, set the new phase,
   schedule the single prime input frame `now+1`, and **do not yet**
   write the native controller 8/A0/actor tag state.
2. On that next frame, with verified fighter IDs, action 2E08,
   native A0 2A28/2A02, D6=0A, and neutral native punch history,
   submit one canonical **forward + LP+MP** (host mask 03) through
   the existing CPS2 P1 input sampling path.
3. After that same frame's CPU runs, validate the original
   Ryu actor and tag timer again. **Only then** run the existing
   `armOutgoingTag` call.
4. The next 68000 frame sees the freshly prepared native 2P edge
   **at the same time** it consumes controller 8, analogous to the
   already observed working immediate-tag handoffs. The ROM still
   selects its own actions; no 2E/32 script ID is forced by this step.
5. If priming is refused, abort without arming controller 8;
   phase 11 otherwise isolates physical P1 input so stray 3P/dash
   cannot leak through the rejected prime.

**This is an engineering inference based on exactly observed input
timestamps, not a new proven runtime success.** The existing
zero-wait 3P and all punch pairs still follow their working immediate
path without the new priming phase. Ken's original 9-frame QCF+2P,
instant positioning, no-charge presentation, and native 2E00
Shoryureppa dispatch remain unchanged.

## Recovery for abandoned host-staged mode

87.178 restored +259 when controller 8 was consumed without a real
tag. The 87.178 submitted observer now also proves that host-written
A0=002A persists after the four timeouts. 87.179 may restore
**only that host-written A0** to its captured original 2A28 or 2A02,
and **only** when all of the following are true:

- A failed phase WAIT_TAG_CONTROLLER in an enabled early-cancel DHC.
- The original Ryu point/Ken reserve pairing is intact.
- Action remains exactly original Ryu 2E08, D6 is still 0A.
- Controller 8 was consumed (256=00); DHC-owned 259=08 was set
  from an originally captured 259=00.
- A0 is still the exact host-written 002A, and the captured A0 is
  a verified native Ryu super mode.
- No native tag action 3200 or other ROM-owned action occurred.

The older, separately guarded stale +259 restoration is preserved.
This does **not** rewind a ROM-chosen action, mutate the 68000 command
history, force an animation, reset the roster, or enable another DHC.

## Diagnostic markers

- `DHC_179_DELAYED_TAG_EDGE_PRIME_SCHEDULED`
- `DHC_179_NATIVE_2P_PORT_EDGE_PREPARED_BEFORE_CONTROLLER_08`
- `DHC_179_NATIVE_2P_EDGE_PRIMED_BEFORE_CONTROLLER_08`
- `DHC_179_CONTROLLER_08_ARMED_AFTER_INPUT_PRIME`
- `DHC_179_DELAYED_TAG_EDGE_PRIME_REJECTED`
- `DHC_179_ABANDONED_HOST_002A_RESTORED_TO_CAPTURED_MODE`

The ongoing 87.175 pre/post CPU/host observer, ROM script trace,
F9 status files and build transcript remain enabled and separate.

## Source and regression

- Native port/host phase code:
  `src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h`
- Safety predicate:
  `src/burn/drv/capcom/xmvsf_stage87_179_delayed_tag_edge.h`
- Phase-11 physical input isolation:
  `src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h`
- Synthetic ROM-free timing/ownership checks:
  `dev/dhc/tests/test_dhc_delayed_tag_edge_179.cpp`
- Derived read-only evidence:
  `dev/dhc/evidence/ORIGINAL_87_178_DELAYED_TAG_NATIVE_EDGE_87_179.csv`

Standalone C++11 edge and ownership checks passed on a local Linux
compiler. The complete Windows/MSYS2 FBNeo executable has **not**
been compiled or tested in this environment; the user-supplied
87.178 F9 recording reflects the old logic, not a 87.179 result.

## Handoff and test

Run the existing `BUILD_TEST.bat` after pulling the feature branch.
It incrementally builds the new independent
`runtime/stable/XMVSF_Test_87_179.exe` without touching previous
TEST binaries, Stable, F9 savestates or warm objects. The launcher
names remain exactly BUILD_TEST.bat, RUN_TEST.bat and RUN_REPLAY.bat.

Use RUN_REPLAY.bat to play the existing full 87.178 F9 V2 movie. It
retains its checkpoint/host-event timeline and produces a fresh
87.179 observer log and DHC native handoff trace.

**Live proof required:** four formerly delayed handoffs should
log the prime stage, then produce native CA containing fresh punch
bits, A0=2A00 and Ryu tag action 3200, followed by Ken's native 2E00
without charge. Immediate 3P/2P handoffs must remain successful.
If a tag still aborts, the guarded A0 restoration should eliminate
the prolonged host-staged 002A state without changing ROM actions.
