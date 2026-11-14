# XMVSF TEST 87.337 — Roster-wide original 68000 DHC dispatcher

**Same branch:** `feature/dhc-morphus-precommit-87-334`, based directly on the already user-playtested 87.336 Ryu→Ken direct native handoff. The previous working 80-byte 68000 guest routine, original native point/reserve tag, placement, pause and original 2E launch remain unchanged.

## Removed the Ryu/Ken-specific restriction

87.336 identified four static D6→2E paths belonging to Ryu/Ken. 87.337 no longer uses that four-row list to decide whether a fighter may DHC.

Instead, the native DHC preflight and incoming CPU bridge use the **same generic resolver** (`xmvsf_stage87_336_native_d6_dispatch.h`), based on the repository's original-US byte analysis from `dev/dhc/tools/ANALYZE_68000_COMMAND_DESCRIPTORS.py` (stage 87.170) and `ANALYZE_68000_D6_TO_2E_PATHS.py` (stage 87.174):

1. Find the actual M68K fighter-code indexers `302E0052D040207B`, following the original relative dispatch-table addresses (not a guessed +14 displacement).
2. Require at least ten original 19-character indexer tables, each with at least fifteen plausible real 68000 fighter code pointers; derive character-owned code region boundaries. Apocalypse ($10) remains nonplayable; alternate Chun-Li ($24) is supported if her original ROM supplies its region.
3. Inside the **incoming fighter's** code region, resolve the proposed real D6 index through the original first-level `7000102E00D6303B00064EFB0002` dispatcher, then the native +$08 phase-zero table.
4. Verify exactly one original phase-zero action-2E launch that branches to the original $01038E action/script resolver with D0=2E and its actual selector. Also recognize the original 2E08/2E0C conditional launch pattern. Validate that fighter's action-2E selector against the original `CpsRom` script table.
5. Capture native source-matcher evidence when the fighter uses the canonical original command format; do NOT wrongly exclude Cyclops, Zangief, Akuma or other original noncanonical input matchers merely because their native command input shape is different.
6. Refuse **before issuing native controller 8 / swapping fighters** if no unique fighter-owned D6→2E dispatch or ROM action/script exists. Both sides use the same validation, with no Ken/Ryu identity shortcuts.

The original 87.335/87.336 guest M68K Hyper executor then calls this *resolved fighter's* native D6 state dispatcher. It never injects a made-up 2E selector, overwrites on-disk ROM, or changes original Version 3 / ordinary tags.

## Input routing for the wider cast

The precommit input table still preferentially uses native PP/KK input family and Hyper D6 combinations from Daniel Plas Rivera / Morphus56k's original `dhc_commands.json`:

- Ken **QCF+PP → Shoryureppa D6=0C**, **QCF+KK → Shinryuken D6=0E**. The Ryu/Ken native test was not intended to make Shoryureppa a replacement for Shinryuken.
- Wolverine QCF+PP, Storm QCF+PP or QCB+PP, Gambit QCF+PP, Magneto QCF+PP or QCF+KK, Chun-Li QCF+PP or QCF+KK, Charlie QCF+PP or QCF+KK, Cammy QCF+KK and other source-native entries are available to the fighter-owned resolver.
- If **no native PP/KK entry** uses the requested motion/family, use the original Morphus **DHC-specific assigned motion** for that fighter and Hyper D6 as a nonconflicting alias. That makes Cyclops's original F,DF,D Hyper available through a DHC QCB input, Zangief's original 360 Hyper available through a DHC QCF input, and Akuma's original SGS available through its DHC DP assignment *without lying about their ordinary native inputs*. Source air-only and conditional Hyper entries remain rejected. Throws are marked terminal.
- All alias and native-input candidates still require a **unique original fighter-owned D6 → action-2E launch** before a native tag. Unverified ROM revisions/moves refuse rather than falling into another fighter's code.

The old 87.329 motion-only preflight is superseded when this detailed D6 precommit is enabled; it must not veto a valid source-native button family.

## Playtest acceptance

This is the **first roster-wide implementation**, not a claim every character's move has been playtested. The previous **87.336 Ryu→Ken handoff** was the last user-confirmed gameplay result; the new generic resolver and DHC alias input still require a local Windows TEST build and F9 confirmation.

To test on the **same branch**, select Version 4, set the existing flags:

```bat
set "XM87274_UNIFIED_DHC=1"
set "XM87334_MORPHUS_PRECOMMIT=1"
set "XM87335_NATIVE_DHC=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
```

Suggested practical first-pass incoming targets: **Storm QCF+PP**, **Cyclops QCF+PP**, **Magneto QCF+KK**, **Cammy QCF+KK**, **Zangief QCF+PP** (DHC alias), **Ken QCF+KK** (Shinryuken). Outgoing point can be any fighter in a genuinely active ROM 2E Hyper; test each side separately. Unsupported or ambiguous paths reject before tag, and the old 87.336 Ryu/Ken handoff remains the baseline comparison.

The existing `test/87.337/observer/dhc_unified_274.tsv` records `NATIVE_337_FIGHTER_DISPATCH_VERIFIED fighter=... d6=... state=... launch=...` when original US ROM validation succeeds, or `NATIVE_337_REFUSED_UNVERIFIED_FIGHTER_D6` before swap when it fails. Actual incoming 68000 entry reports `NATIVE_336_ORIGINAL_D6_DISPATCH_ENTERED`. Original ROM action-2E and native stock before/after still provide separate execution evidence.

**Versioning:** `RUN_TEST.bat` visible header is **87.337** and startup echo resolves from `XMVSF_TEST_VERSION`. `BUILD_TEST.bat` and `dev/dhc/BUILD_TEST.sh` also select **87.337**. Stable, existing warm build cache, and original Version 3 unchanged.

## Validation limits

The C++ source regression now models ten original M68K fighter indexers, synthetic fighter-owned state dispatch tables, unique D6/phase-zero 2E selectors and scripts for Wolverine, Cyclops, Storm, Ken (two distinct D6 choices), Zangief, Akuma, Cammy and alternate Chun-Li. A tampered original launch is rejected. The precommit policy test separately checks native-family priority and DHC-specific aliases. Neither standalone test is added to the normal local build.

No complete Windows build or new 87.337 gameplay/F9 capture has been performed in this environment. As with 87.336, meter exactly once, general superfreeze/FX, no duplicate/lock, all-fighter aerial commands and outgoing projectile ownership must be validated in gameplay rather than claimed by static code.
