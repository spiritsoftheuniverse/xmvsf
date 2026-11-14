# XMVSF 87.339 — outgoing native DHC tag edge restored

Same branch: `feature/dhc-morphus-precommit-87-334`. Version 3 and Stable untouched. GitHub remains source-control only. No new build gates.

## Proven by uploaded live 87.338 DHC log

`dhc_unified_274.tsv` contains 121 event rows and three original-controller-8 tag requests. **None entered original ROM action 3200**, so no incoming native handoff could be reached:

| Frame and side | Outgoing → incoming | Incoming selected native D6 | Original-ROM decision |
| --- | --- | --- | --- |
| P2 3163 | Cyclops 02 → Ryu 12 | 0C verified | 3164 `ABORT_WRONG_NATIVE_ACTION_NO_SCRIPT_WRITE` |
| P1 3894 | Ryu 12 → Ken 14 | **0E Shinryuken** verified | 3907 `ABORT_NATIVE_TAG_NOT_ACCEPTED_12F` |
| P1 4281 | Ken 14 → Ryu 12 | 0C verified | 4282 `ABORT_WRONG_NATIVE_ACTION_NO_SCRIPT_WRITE` |

Every one of these proposals was a two-kick physical chord (`read=38`), captured in an active original-ROM Super window and resolved to the correct incoming +D6. This is **not** a compile failure, absent pad polling, or an unrecognized incoming Hyper.

Separately, P2 frame 2861 proposed incoming Cyclops 02 D6 04, but the fighter-wide 87.338 dispatcher still refused it despite finding original fighter region `0224F2–024B62`. Those are independent issues; do not pretend the tag-edge fix proves Cyclops' D6 launch.

## Root error and minimal runtime correction

The 87.274 host state machine correctly armed original 68000 tag controller 8 at the end of the CPU frame. However, its `inputForFrame()` branch for `TAG_REQUESTED` **returned `out.active=true` with attacks=0 every subsequent input poll**. This **explicitly suppressed the original native outgoing LP+MP edge** needed to enter action 3200. The earlier deterministic 87.193 source and live audit had already demonstrated that outgoing and incoming commands have different input owners and that KK needs to become a native *outgoing* 2P tag edge, without changing the *incoming* original KK Hyper command.

87.339 changes **that one existing host input path**, not the 80-byte native guest code, to supply the required original LP+MP tag edge to controller 8:

- If reserve requested 2K/3K (as in the uploaded three failures), give native 2P on the first **two input polls after** the tag request, then release. It is a fresh punch edge because the prior physical chord was kicks.
- If reserve requested 2P/3P, release on the first input poll to avoid stale held-punch history, give native 2P on input polls 2–3, then release.
- When native tag 3200 has been accepted and the host awaits the actual swap, suppress input as before.
- This does not alter the held reserve's original motion/button mask, selected Hyper +D6, original actor script or native 68000 animations. The original controller-8 dispatch still decides whether action 3200 is accepted. No synthetic tag animation is introduced.

The **existing** two abort events now include original action, mode, controller 256/259 and input mask on failure, so an actual original-ROM rejection is visible without adding new diagnostics.

The existing `test_dhc_unified_274.cpp` standalone regression now checks both sides and exact outgoing edge scheduling, including P1 Ken Shinryuken QCF+KK preserving original reserve KK and P2 3P conversion to original native LP+MP. This is not added as a new BUILD_TEST gate.

## Local playtest

With the existing opt-in Version 4 flags:

```bat
set "XM87274_UNIFIED_DHC=1"
set "XM87334_MORPHUS_PRECOMMIT=1"
set "XM87335_NATIVE_DHC=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
```

Start with the session's exact proven incoming target: **Ryu → Ken using QCF+KK (Ken Shinryuken, D6 0E)**. Then reverse Ken → Ryu and try Cyclops → Ryu on P2. The first critical pass is **`ORIGINAL_ROM_32_00_TAG_ACCEPTED`**, followed by native swap and `NATIVE_336_ORIGINAL_D6_DISPATCH_ENTERED`; only then judge the Hyper/cut-in. This is the actionable original-ROM condition, not a mere claim that the button was polled.

**Status:** Changes committed to the same branch. Full Windows compilation and live game result for 87.339 have not yet been observed; no claimed gameplay pass. Roster-wide Cyclops 02 D6 04 resolution remains a separate original-68000 verification issue.

The visible `RUN_TEST.bat` header, batch build/run variables and shell warm-build default are all advanced to **87.339**.
