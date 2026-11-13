# XMVSF TEST 87.348 — no pre-tag certificate denial; TRY the native Super

Branch: feature/dhc-morphus-precommit-87-334. Version 4 XMVSF original-US xmvsfu ONLY, explicitly enabled by the same four existing environment flags. Visible RUN_TEST.bat header and local warm BUILD_TEST scripts advanced to 87.348. Version 3, Stable and all normal controller functions are untouched; no GitHub Actions or new build gates.

## 87.347 observed live facts from observer(4).zip

The user's observer(4).zip contains dhc_unified_274.tsv, dhc_acceptance_test.tsv and its summary. DHC boot reports version4=1, unified=1, donor=1, US_hooks=1, exit_site=1.

Eleven genuine incoming Hyper command selections were stopped by NATIVE_344_REFUSED_BEFORE_TAG:
- Wolverine incoming D6=08 five times, native source matcher found his D6 but Ryu-style D6 opcode header count=0.
- Cyclops incoming D6=04 four times: exact original 68000 D6 header, target and phase were found (D6 target $02390E), but the extra 2E launch proof rejected it.
- Cyclops incoming D6=02 twice from the now-correct original F->DF->D + PP/PPP: exact D6 table and target at $0236FC were found, but a rigid phase-zero signature rejected both.

Consequently ZERO native outgoing tag hooks and ZERO incoming Hyper outcomes occurred in the 87.347 match. This is not an input-recognition problem and not a rationale for another input mapping. The policy validator was suppressing the native handoff.

## What changed in 87.348

The actual Hyper input (original incoming fighter motion + PP or KK; Cyclops F/DF/D+PPP included), the outgoing original-ROM Hyper cancel window, a valid living native reserve, a genuine native US ROM/exit-hook signature, and matching fighter identity remain required. **Once a real input has selected D6, the previous 6KB NATIVE_344/345 pre-tag refusal code is deleted.** No confidence/proof score cancels the command before the native 68000 can perform the real tag.

The original incoming M68K guest now has three routes, selected in this priority:

1. **VERIFIED (3)**: the existing full Ryu/Ken and other proven native D6/phase/2E dispatchers. Their exact original code paths and near-opponent entry remain unchanged.
2. **NATIVE_D6_STATE (1)**: for fighters like Cyclops with an original 68000 D6-indexed state dispatch opcode and a bounded native relative target, call that original state dispatcher even if the subsequent actor-specific phase/2E instructions do NOT match Ryu/Ken's signature. Example: Cyclops D6=02 original target $0236FC, D6=04 original target $02390E. This leaves the actor's own CPU routine to choose its original animation/action.
3. **ORIGINAL_ROM_2E (2)**: when the original D6-indexed bytecode layout cannot be identified (notably Wolverine D6=08), find a **bounded original 2E ROM script for that exact incoming fighter**, preferably its D6-numbered native selector when that selector actually exists in the native ROM table, otherwise the first valid native 2E selector. Call the game's original US $01038E action/script dispatcher with actual D0=0x2E, D1=selected 2E subaction through the same native 80-byte M68K guest. Preserve the exact physically selected D6, original actor mode/ground/X, saved M68K registers, and original tag-out/in sequence.

Route 3 is **intentionally aggressive and NOT a guarantee of the requested Hyper**: the original D6 index and original ROM 2E subaction index do not necessarily match. The CPU may execute a different legitimate Hyper, a partial animation, or a broken state. This is the user's requested make-it-act experiment, and the log labels it rather than presenting a guessed mapping as verified behavior. The previous wrong 87.343 route of calling $011DBE as the Hyper dispatcher remains completely removed; $011DBE is visited **only** as the genuine tag-in/interception point.

The bridge evaluates the route for BOTH active side slots and emits its original per-side M68K page. No host-generated action2E RAM writes, synthetic controller8, arbitrary new controller chords, host hitbox or fake animation are added. 1v1/no-living-reserve ordinary Super inputs still pass through unchanged; extra Storm/Charlie observations remain suppressed in native DHC, not gameplay.

The only reason to leave a claimed Hyper unarmed is when there is neither an executable original D6 state entry nor ANY bounded native 2E script in the actual incoming fighter's ROM, or when the original game/US opcode ABI doesn't match. It is not a diagnostic confidence denial; a nonexistent code/script address cannot be safely executed.

## Logs

The existing F:\xmvsf\test\87.348\observer\dhc_unified_274.tsv now reports:

- NATIVE_348_INPUT_ALLOWED: genuine input, fighter and exact D6, selected route kind.
- NATIVE_348_ARMED_TRIAL: actual CPU routine address, handler or original 2E selector and ROM script.
- NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED: actual native outgoing M68K tag.
- NATIVE_343_ENTRY_POSITION route=TRIAL_NATIVE_D6 or TRIAL_ORIGINAL_ROM_2E: actual incoming position and which route was used.
- MORPHUS_340_NATIVE_11DBE_AFTER_ORIGINAL_SWAP_COMMIT: original native fighter identities swapped and the new 68000 guest executed.
- MORPHUS_340_ACTUAL_US_2E_HYPER: actual original native 2E observed. This last event—not input selection or a tag—is the measure of execution success.
- NATIVE_348_NO_ROM_ACTION_TO_EXECUTE: only if no actual original executable D6 address or 2E script exists. There are NO NATIVE_344_REFUSED_BEFORE_TAG or NATIVE_345_INDEXER_EVIDENCE events in this branch's new DHC path.

## Local Windows run

Select Version 4, original US xmvsfu, same PowerShell session:

    $env:XM87274_UNIFIED_DHC = "1"
    $env:XM87334_MORPHUS_PRECOMMIT = "1"
    $env:XM87335_NATIVE_DHC = "1"
    $env:XM87340_MORPHUS_NATIVE = "1"
    .\BUILD_TEST.bat
    .\RUN_TEST.bat xmvsfu

First retry Cyclops as incoming QCF+PP (D6=04) and F,DF,D+PPP (D6=02) while the outgoing Hyper is cancellable; then try Wolverine incoming QCF+PPP (D6=08). Keep the previously verified Ryu/Ken pair as a control.

Verification: the new guest's exact 80-byte original source-equivalent M68K instruction emitter, both side offsets, D6 and 2E selector bytes compiled and passed a standalone C++11 test locally. A committed independent C++11 regression fixture covers the original state-table and ROM-script trial branches. GitHub source structural checks were performed; **the native Windows full build, gameplay, and precise resulting Hyper identity for 87.348 have NOT been verified**.
