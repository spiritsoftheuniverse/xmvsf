**SUPERSEDED FOR THE GENERIC FIGHTER ROUTE BY TEST 87.344.** Actual 87.343 gameplay showed 7/7 attempts through the $011DBE fallback tagged in but did NOT execute the incoming native Hyper. Original US $011DBE is a tag-in character indexer (MOVE.W $52(A6),D0), not a D6 super dispatcher. The generic path was removed, and nonverified moves must not tag until their original per-character D6->phase-zero->2E path is established. The 87.343 near-opponent X placement remains in use for verified Hypers.

# XMVSF TEST 87.343 — P1/P2 variable native DHC and opponent-adjacent entry

Same development branch: feature/dhc-morphus-precommit-87-334. TEST 87.343 with the visibly updated RUN_TEST.bat header, BUILD_TEST.bat and warm BUILD_TEST.sh. Original Version 3 and Stable are untouched; no Actions or new mandatory gates.

## Ground truth from the attached 87.342 live TSV

The 586-row DHC observer recorded **11 successful original-ROM native DHC paths**. Each recorded all three:
- NATIVE_341_REAL_SUPER_INPUT (actual PP or KK input, native target +D6);
- NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED and MORPHUS_340_NATIVE_11DBE_AFTER_ORIGINAL_SWAP_COMMIT;
- MORPHUS_340_ACTUAL_US_2E_HYPER the following original frame.

Examples include Ryu -> Ken Shoryureppa and Shinryuken, and Ken -> Ryu. Importantly, P2 also captured four original valid QCF+PP attempts from Cyclops (02) to **Rogue (06), native D6=28**, but those attempts were refused by the specialized-US-dispatcher preflight. P2 command capture has therefore already been established in actual play; only P2 native entry for an unverified character remained blocked.

All 11 immediate post-entry meter snapshots showed stock_before=3, stock_now=3. This **does not prove that the second meter was charged**; meter debit remains a separate follow-up after verifying actual timing and refill behavior.

## 1. Correct native close-range position

Earlier code copied the outgoing fighter's world X (+$0C) into the incoming character, so a close-range Hyper such as Ken's Shinryuken could activate correctly but miss a distant opponent.

87.343 leaves the original exit, swap, and M68K guest intact, but in the **actual incoming $011DBE callback**:
1. Identify the current side by its verified M68K A6 owner, then read the opposing active actor using original XM87193Exit::actorBase(side ^ 1).
2. Read the opponent's genuine world X from their original RAM +$0C. The saved outgoing world X chooses the left/right side to enter from.
3. Move the incoming Hyper origin to **at most 16 original world-X units from the opponent** while preserving an already-closer position. Clamp at 16-bit world bounds.
4. Re-emit just the existing 80-byte private native guest with the new X immediate, leaving its D6 and native dispatch target unchanged. The original game remains responsible for vertical ground and character facing.
5. Emit NATIVE_343_ENTRY_POSITION with side, incoming/opponent character IDs, old X, opponent X, entry X, D6, and the route used.

No opponent-screen-position approximation, guessed camera origin, per-fighter X offset, synthetic hitbox, or post-entry forced motion is added. Real attack hitbox and entry-facing still require gameplay verification.

## 2. Make the incoming character variable for BOTH native controllers

The original PP/KK input selector is already variable: each side reads the actual point/reserve pair from original M68K RAM and maps the incoming character's **real command** to its native D6. Neither side is hardcoded to Ryu or Ken.

The last blocker for other characters was the 87.336 host resolver refusing any D6 whose specialized original per-fighter phase-zero entrypoint had not been static-audited. That makes P2 Cyclops -> Rogue D6=28 fail, as the attached live TSV shows.

The new alternate path does NOT invent Rogue's routine address:
- First try the existing ROM-opcode-verified per-character D6 dispatcher. This is the original, already-playtested Ryu/Ken path; it remains unchanged.
- If it is unverified, require an ORIGINAL, bounded action-2E ROM script for the actual reserve fighter, via XM87142::resolve.
- Use the **already-verified original US $011DBE character dispatch router** as the guest's native call target. The original M68K table dispatches the actual incoming fighter through the original fighter ID; the donor's claimed D6 is unchanged.
- After the real swap, the existing guest calls the original fighter router with +$A0=2A and +$D6 as set from native input. A second visit to $011DBE is not hooked because the claim is already COMMITTED and the donorArmed flag is cleared.
- NATIVE_343_STOCK_FIGHTER_ROUTER marks this experimental, not-yet-gameplay-proven path. Refuse a reserve without any valid original 2E scripts; do not force arbitrary move or animation IDs.

This is a real original-engine path, but **successful execution for Rogue and the wider cast is not yet proven**. The original $011DBE entry includes more normal tag-in work than a specialized per-fighter D6 phase-zero routine; test the resulting controls/animation before claiming full roster parity. The data-driven route is only in the existing opt-in Version-4/Morphus-native mode. Non-DHC, Version 3 and prior paths remain unchanged.

## Local playtest

Use the same four PowerShell environment variables (PowerShell syntax, not CMD set syntax), then run the existing local scripts:

    $env:XM87274_UNIFIED_DHC = "1"
    $env:XM87334_MORPHUS_PRECOMMIT = "1"
    $env:XM87335_NATIVE_DHC = "1"
    $env:XM87340_MORPHUS_NATIVE = "1"
    .\BUILD_TEST.bat
    .\RUN_TEST.bat xmvsfu

In Version 4, try Ryu -> Ken (QCF+KK for Shinryuken and QCF+PP for Shoryureppa) on P1, then on P2 or the opposite side. Observe actual hit versus whiff and the new opponent-relative X event. Try P2 Cyclops -> Rogue QCF+PP, which previously refused four times. Also test other selected actual native Hyper commands, e.g. Cyclops, Storm, Magneto or Cammy as reserve. Unsupported unusual original inputs (360/SGS/air-only) remain unsupported rather than mapped to fake substitutes.

Expected path:
NATIVE_341_REAL_SUPER_INPUT -> (optional NATIVE_343_STOCK_FIGHTER_ROUTER) -> NATIVE_341_ORIGINAL_13AB6_TAG_TAIL_EXECUTED -> NATIVE_343_ENTRY_POSITION -> MORPHUS_340_NATIVE_11DBE_AFTER_ORIGINAL_SWAP_COMMIT -> MORPHUS_340_ACTUAL_US_2E_HYPER.

Only the actual original 2E post-entry event is a success. A captured input, guest dispatch, or observed swap does not by itself prove a playable Hyper.

## Validation scope

The C++11 near-opponent calculation and prior 87.341 native-input test pass locally in a small independent fixture. The tracked original-guest emitter remains the same machine code with different X/dispatcher immediates, and standalone tests are included in test_morphus_native_dhc_340.cpp without adding a BUILD_TEST gate.

**87.343 full native Windows build and live match have not been run here.** The close range is a first practical default, not a measured per-Hyper hitbox guarantee. The original-US stock-router fallback and its compatibility with all incoming character states are experimentally unverified; do not claim all characters work on both sides until observed in the real game.
