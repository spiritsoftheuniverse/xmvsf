# XMVSF — Freeze Stage 87.185 stable DHC input baseline

**Evidence:** user-provided `87.185.zip`: 17 real queued requests in one F9 session, native DHC handoff, super chat, 87.175/87.180 observers, 68000 executed-PC trace and F10 probes. **This report makes no gameplay changes.** Do not overwrite Stage 87.185 or Stable.

## Verified handoff outcomes

- **14/14 native controller-8 handoffs succeeded** all the way to the original Ken Shoryureppa action `2E00` / script `0D6722`. No failed armed handoff or persistent lock was found.
- The successful paths are **3 direct early-active tests**, **5 existing attack-phase tag bridges**, and **6 nonterminal `2E0A` recovery rebound bridges**. All six tested recovery handoffs complete normally.
- **3 of 17 queued DHC requests were declined before controller 8 was armed** by the intentional 87.183 consumed-punch guard. No DHC chain was lost, and retries worked.
- F9 V2 sealed a single 8,551-frame recording with five segments. Health reports native actor frames and all trace files exist.
- Original actions still comprise 66–67 frames of `2E08` startup, 56 attack frames and approximately 41–42 frames of `2E0A` recovery. Do not substitute the old arbitrary 200-frame candidate timeout for action identity.

The source-derived 17-attempt table is at
`dev/dhc/evidence/ORIGINAL_87_185_NATIVE_DHC_17_QUEUED_ATTEMPTS.csv`.

## Why the beginning still feels late

Three separate facts must not be conflated.

**First: startup tagging has never been proven.** The host recognizes a QCF+2P/3P gesture during the real `2E08/step02` startup, but execution still requires an actual `2E08/step04` attack or the verified nonterminal `2E0A` recovery bridge. Startup naturally lasts **66–67 frames**. The 87.185 session contains **no submitted DHC command during startup** to establish whether a new native startup-cancel strategy is safe.

**Second: the direct early-attack gate skips the first FOUR native attack frames.** `XM87185Gate::directActiveAttack` requires `remaining <= 0x3C`, but the attack phase begins with `+4E=0x40`; the first native attack countdown values `40/3F/3E/3D` are excluded. The earliest **actually tested successful** handoff was frame 2035, **91 frames after** its Shinkuu root 1944 and 24 frames after attack began at 2011 (`+4E=0x24`). The first-four-frame gap is a source-code observation, **not an observed failed attempt** in this recording. Test only as a separate newer TEST; do not declare acceptance without actual controller and Ken ROM evidence.

**Third: all three pre-arm declines are input-synchronization cases.**

- Frame **1816**, main attack, `C0=0011`: LP arrived at 1815, LP+MP at 1816. The host correctly recognized QCF+2P, but the original CPU had already registered LP.
- Frame **1859**, recovery, `C0=0035, CA=0030`: LP+MP was pressed at down-forward on 1858; final forward arrived on 1859. Native punch history was already populated before the host accepted the completed QCF.
- Frame **4833**, recovery, `C0=0011, CA=0010`: LP arrived at 4832 and LP+MP at 4833, so a partial chord was native before the host matched.

In contrast, the fourteen successful commands had their exact 2P or 3P chord coincide with the final forward direction, with clean native punch-history at controller arming (the actor observer reported `C0=0001, CA=0000` for the recorded samples). The guard is doing exactly what prevented the dangerous 87.182 wrong-action problems. **Do not disable the consumed-punch guard to hide these declines.**

## Recommended next isolated engineering step: 87.186

1. Preserve the complete 87.185 source/binary baseline and the proven six-of-six recovery behavior. Do **not** change recovery exit acceptance or Ken's native QCF input.
2. Investigate the physical-versus-68000 input sampling boundary for **staggered two-punch presses and final-forward arriving one frame after the chord**. The goal is recognizing the real user command while keeping a fresh native edge at the chosen tag controller frame, not simply bypassing the 87.183 punch-history safety check.
3. Independently test whether the very first original `2E08/step04` frames at `+4E=40/3F/3E/3D` can accept the same controller. Treat them as an unverified beginning-boundary experiment.
4. Original `2E08/step02` startup cancellation is a separate, riskier native command-controller experiment, not yet supported by this capture. Never change the proven late/recovery path to widen it.
5. Keep a real native executed-PC and F9 recording for each new experiment. Success means Ryu exits by the host-assisted original `3200` tag and Ken actually selects original `2E00`, not merely controller 8 consumed or a displayed pose.

No speculative startup or split-chord injection has been committed as part of this baseline.
