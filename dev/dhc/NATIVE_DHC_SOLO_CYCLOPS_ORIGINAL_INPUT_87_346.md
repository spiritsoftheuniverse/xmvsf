# XMVSF TEST 87.346 — solo native input passthrough and Cyclops real alternative Hyper command

Same branch: feature/dhc-morphus-precommit-87-334. Visible RUN_TEST.bat header and both local TEST builders say 87.346. Version 3, Stable, Morphus native tag/outgoing helpers, opponent-relative entry and real Hyper-to-D6 dispatch have not been modified. No GitHub Actions or new BUILD_TEST gate.

## Evidence from observer(2).zip (87.345)

The archive contained 9,299 original actor observations, 573 DHC event rows and a **2v2 match** (P1 Cyclops/Ryu, P2 Gambit/Storm). There is **no 1v1 or one-character-team sample** in this capture; do not claim this upload reproduces the solo symptom.

Seven DHC claims were accepted. Four Cyclops->Ryu claims executed original action 2E; three Gambit->Storm claims selected original D6 (02 or 0A) but did not reach an observed 2E. Those three entered original action 2C08 in the actor observer. Whether that is a legitimate Storm native Hyper must be established from original move/script evidence; this revision does not rewrite 2C or label it a success. Seven Ryu->Cyclops original D6=04 attempts were refused at the pre-tag gate despite an original D6 and phase table, making incoming Cyclops execution a separate outstanding native path issue.

## 1. Actual Cyclops F,DF,D + PP/PPP

The user-supplied Morphus original-command catalog dhc_commands.json, **not** its assigned DHC gestures, identifies:

- Cyclops QCF+PP/PPP -> Mega Optic Blast, original Hyper D6=04.
- Cyclops **F,DF,D+PP/PPP** -> Super Optic Blast, original Hyper D6=02.

The old 87.345 parser only understood seven gestures and could not represent the second command. 87.346 adds exact motion **8 = relative forward, down-forward, down** to the existing 14-sample original motion history for both facing directions and both controller sides. Its native original-command lookup accepts motion 8 only for Cyclops and only with a 2-or-3-punch chord, selecting D6=02. QCF still selects D6=04; no fake QCB alias, forced 2E action, or new control binding is used.

Native motion history is now tracked even outside an outgoing Hyper, like the original game's input parser. **DHC still requires an eligible, real outgoing Hyper and a living incoming reserve.** PP/PPP entered in an ordinary fight is not hijacked.

The existing DHC event log reports NATIVE_346_ORIGINAL_F_DF_D_PUNCH when a live eligible DHC input selected that eighth native command, *before* the original D6 route is independently validated. It does **not** assert that an incoming Super was successfully executed. The original D6/phase and actual 2E acceptance observer are unchanged.

## 2. One-character teams / dead or absent reserve

The existing donor-native DHC path checked point/reserve fighter IDs, but **fighter ID 00 is Wolverine**, so it cannot be used as an absent-reserve sentinel. The donor's actual team.s uses **actor +$210** as the resident reserve's native remaining life. Revision 87.346 reads the real original reserve actor's +$210 word on each side. Without an actual valid different **living reserve**, the custom DHC input handling skips that side, clears stale input hiding, and leaves the *original game* to receive real PP/PPP and native Super input. The same guard safely refuses a DHC to a defeated last reserve. On a fresh real punch chord, NATIVE_346_SOLO_NATIVE_INPUT_UNTOUCHED records side, fighter IDs, reserve life and buttons, without changing input.

A solo team cannot perform a **Double** Hyper Combo without an incoming partner, but its ordinary native Hyper commands must still register. This is a local native-input passthrough protection, not a new 1v1 gameplay implementation. Because the attached capture did not contain a one-character match, **actual 1v1 gameplay input remains unverified**.

## Test scope

Existing test_morphus_native_dhc_340.cpp has additional checks for Cyclops true QCF and F->DF->D, PP and PPP, both P1 and P2 facing, rejected non-Cyclops motion 8, valid living Wolverine reserve vs empty 00, and solo input untouched. A locally reconstructed header-equivalent C++11 fixture compiled with GCC -Wall -Wextra -Werror and passed both facing directions and solo no-claim checks. This does not replace compiling the whole native Windows test game, which has not been done here. No additional mandatory gates.

## Local playtest

Use the four existing PowerShell flags and Version 4, then:

    .\BUILD_TEST.bat
    .\RUN_TEST.bat xmvsfu

First play **one fighter per team**, use normal P1 and P2 Supers and verify there is no input suppression. The log should say NATIVE_346_SOLO_NATIVE_INPUT_UNTOUCHED for a fresh PP/PPP chord when a native reserve has 0 life. A log without this event does not prove the team was solo; capture actor +$210 if input remains broken.

For a two-fighter team, use outgoing Ryu's or Ken's genuine Hyper, then issue the **incoming Cyclops's F->DF->D+PPP**. NATIVE_346_ORIGINAL_F_DF_D_PUNCH must report D6=02; native input selection is **distinct from the later original D6/phase/Hyper execution**. A tag with no actual Super means the remaining original Cyclops startup/phase path needs analysis; do not replace it with action writes or guessed dispatch addresses.

Log: F:\xmvsf\test\87.346\observer\dhc_unified_274.tsv.

No scripted replay test was added; the user's capture begins during live play and is not a start-of-game replay.
