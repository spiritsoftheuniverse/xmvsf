# XMVSF 87.192 — Reserve-owned native super input discovery

87.191 solved outgoing super availability. 87.192 removes the OTHER arbitrary part: choosing the incoming hyper from a fixed Ken/Ryu action/name table.

## Experimental pipeline

1. While Ryu's original 2E super is active, observe the player's **new** motion/attack edge. Capture a generic QCF, QCB, DPF, DPB, HCF, HCB, 2QCF or 2QCB proposal plus its actual attack mask. No name, action number, selector, or script is attached.
2. The existing Ryu native controller-8 tag and native point swap remain intact. Once Ken has entered, feed the captured proposal to the **incoming fighter's native controller input port**, not to Ryu's outgoing parser. Use the established two-frame 2P/2K pulse and native direction/facing conversion. Do not directly set a super action or animation.
3. After the incoming fighter's original 68000 parser runs, look at the **actual native action**. Accept a genuinely ROM-resolved 2E family action with real 2A controller, even step and native D6, whose script lies in its character's own original ROM table. The actor/ROM result, NOT a hardcoded name or expected `2E00`/`2E02`, determines which hyper was selected.
4. Append unique accepted fighter/action/ROM-root/input combinations to `reserve_owned_supers.tsv`. Reject and log 0002, 2Cxx and any non-2E or ROM-invalid result. There are no predeclared entries in the 87.192 learned list.

## Scope and important distinction

The first TEST implementation is intentionally Ryu→Ken only because the outgoing tag writer and incoming kickless entry have been verified only for that pair. The reserve **input and acceptance** are generic and no longer depend on Ken's super whitelist. All previous TEST paths remain available with `XM87192_TEST_RESERVE_OWNERSHIP=0`.

**Post-tag refusal is NOT pre-tag refusal.** The reserve's normal input parser does not execute for an inactive actor. This version can reject an unrelated action as a super, but it may have already allowed the native Ryu→Ken swap. It cannot correctly promise to leave Ryu in place for an unknown non-super until a true reserve-side preview/shadow dispatcher or ROM-proven learned input association exists. Do not call this a final roster-general DHC validator.

The proposal parser currently carries standard directional motions, not every charge, mash, 360-degree, SPD, SGS, custom-command or follow-up pattern. For safety, initial active tag proposals also require a fresh two-punch or two-kick chord: QCF+single-punch must remain a normal special, NOT an unsolicited DHC swap. This chord test does not declare anything a super; the incoming 68000 ROM still decides. Expanding command capture must be based on actual user-controlled inputs and native ROM outcomes, not a fabricated move mapping. The existing super-chat now records the inactive actor's read-only action, mode, step, D6 and script on each proposed command to evaluate future *pre-tag* reserve parsing.

## Regression/logging

TEST is 87.192, built by the existing root `BUILD_TEST.bat`, launched by `RUN_TEST.bat`, and replayed by `RUN_REPLAY.bat`. Binary `XMVSF_Test_87_192.exe`; logs under `test/87.192`, catalog `test/87.192/observer/reserve_owned_supers.tsv` (replay in `test/87.192/replay_results/reserve_owned_supers.tsv`).

On all proposed inputs, even those that fail, the TSV records `INPUT_PROPOSED`, `NOT_NATIVE_SUPER`, or `CONFIRMED_NEW`/`CONFIRMED_REPEAT`. The existing handoff log still captures native controller8, swap, Ken arrival and aborts.

Isolated C++11 tests validate native input timing, motion directions, action ownership, genuine ROM 2E acceptance, rejection of 2Cxx, invalid script/controller, repeated and previously-unseen 2E selectors. Native Windows execution and F9 before/after captures still required.

The opt-in switch `XM87192_TEST_RESERVE_OWNERSHIP=0` restores 87.191 named-super handoff without modifying Stable or older TEST binaries.
