# XMVSF TEST 87.340 — executable Morphus native 2v2 source port

**Continuing the SAME branch:** feature/dhc-morphus-precommit-87-334.

The source is Morphus56k / Daniel Plas Rivera, XMvsSF-3v3-DHC-v1.1.1, particularly source/engine/dhc.s, dhc_table.inc and native-hooks-2p.json from the supplied zip. Original native US opcode evidence is in morphus_dhc_context_332.tsv. This is a standalone experimental native DHC path, not another extension of the controller-8 approach.

## Donor code adapted for original 2v2

- The exact seven donor motions QCF,QCB,DP,RDP,HCF,HCB,DD with 14-sample timers, most recent completed timer, and donor priority HCF,HCB,DP,RDP,QCF,QCB,DD.
- Original dhc_table.inc row by incoming fighter, including the seven real Hyper +D6 values, air-only, conditional, and throw bits; no fabricated 2E action or character-specific input whitelist.
- Donor MP+MK chord selects reserve A, the ONLY reserve in 2v2. MP+HK selects reserve B, absent here, and is intentionally refused. MP+MK+HK is ambiguous and refused. The captured chord is hidden from the original attack input while held.
- Donor xm3_hook_tag/xm3_dhc_chain is executed at verified original US $013AB6 using 32 bytes of actual Motorola 68000 instructions: MOVEM save, D0=4, JSR $04BF66, MOVEM restore, +$256/+$259=2, D0=-1, RTS. **No forced controller 8 and no expectation of a synthetic action 3200.**
- The US $04BF66 helper is proven from the uploaded original opcode call site 303C00024EB90004BF66 in the original US code near $00D59A. Its code is checked again against the running decrypted CpsCode before redirecting any M68K instruction. The five previously verified US hook ABI signatures must match too.
- The unchanged game then owns the stock point/reserve swap, and the incoming fighter must reach original US $011DBE. Both native point slots use the same verified identity guard and preserve A5.
- **Still incomplete, stated explicitly:** The incoming commit continues using the 87.336 native M68K guest and US fighter D6 dispatcher until the donor Euro $125D6 helper's original US entry and ABI are verified. No guessed $125E4 call is made. This is NOT a complete exact donor 2v2 port or a guaranteed all-cast success.

## Run this NEW path

Select Version 4 and US xmvsfu, then:

    set "XM87274_UNIFIED_DHC=1"
    set "XM87334_MORPHUS_PRECOMMIT=1"
    set "XM87335_NATIVE_DHC=1"
    set "XM87340_MORPHUS_NATIVE=1"
    BUILD_TEST.bat
    RUN_TEST.bat xmvsfu

**Different input now:** use the donor MP+MK DHC chord, not the old QCF+KK/PP combinations.

- While Ryu is performing an active Hyper, QCF + MP+MK selects incoming Ken **Shoryureppa** (+D6 0C).
- While Ryu is performing an active Hyper, QCB + MP+MK selects incoming Ken **Shinryuken** (+D6 0E).
- QCF + MP+MK targeting Cyclops selects the donor table's Mega Optic Blast (+D6 04), subject to verifying the incoming original US handler.

The old controller-8 Version-4 experiment remains reachable without the FOURTH environment flag. Stable and original Version 3 remain untouched.

## Existing DHC TSV proves exactly which boundary ran

- MORPHUS_340_NATIVE_INPUT_CLAIM: donor motion + MP+MK claim accepted.
- MORPHUS_340_NATIVE_13AB6_TAG_TAIL_EXECUTED: original M68K outgoing tag hook actually hit.
- MORPHUS_340_NATIVE_11DBE_AFTER_ORIGINAL_SWAP_COMMIT: the original ROM swap occurred; incoming native commit intercepted.
- MORPHUS_340_ACTUAL_US_2E_HYPER: original incoming 2E and D6 observed, meter before and after printed.
- MORPHUS_340_ORIGINAL_TAG_HOOK_NOT_REACHED: 12-frame stale claim; the original hook did NOT execute.
- MORPHUS_340_US_INCOMING_D6_HANDLER_UNVERIFIED: source input selected a move that the original US native 87.336 dispatcher cannot safely call.

The last two failures are real remaining unknowns. Never call a claimed chord a successful DHC.

## Validation and limitations

The Morphus input tracker and machine-code emitter passed a local C++11 -Wall -Wextra -Werror synthetic fixture using the imported donor table, including native US M68K code word order and controller 2 values. The committed standalone regression test is dev/dhc/tests/test_morphus_native_dhc_340.cpp. No new BUILD_TEST gates, GitHub Actions or cache invalidation.

**Full Windows TEST compilation and 87.340 live gameplay are not yet verified.** The source port advances its motion clocks once per host game frame, not using Morphus's native $808008 core input clock yet; Turbo timing therefore remains unproven. The source's full projectile collision list/grace tracker, direct original US $125D6 incoming startup helper, meter debit, freeze, and all-fighter entry must be verified before calling this DHC complete.

Visible RUN_TEST.bat header and local TEST build versions are **87.340**.
