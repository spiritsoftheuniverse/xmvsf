# XMVSF TEST 87.344 — original native Hyper or no DHC

**Same branch:** feature/dhc-morphus-precommit-87-334. Existing native handoff and original real PP/KK command decoder. Version 3, Stable and warm Windows build scripts preserved.

## User's 87.343 real 68000 observation, not synthetic inference

The uploaded observer(1).zip includes 645 DHC event rows, 6,575 original actor observer rows and one full 87.343 match. All experimental flags and original US hook signatures were active.

| Original point → incoming | Side | Selected input and native D6 | Native handoffs | Actual original 2E Hyper |
| --- | --- | --- | --- | --- |
| Cyclops 02 → Ryu 12 | P1 | QCF+PP, D6=0A | 3 | 3 |
| Ryu 12 → Cyclops 02 | P1 | QCF+PP, D6=04 | 4 | **0** |
| Ken 14 → Charlie 20 | P2 | QCF+PP D6=04 or QCB+KK D6=06 | 3 | **0** |
| Charlie 20 → Ken 14 | P2 | QCF+KK, D6=0E | 3 | 3 |
| **Total** | Both | Original controls recognized | **13** | **6** |

The failed seven attempts ALL logged NATIVE_343_STOCK_FIGHTER_ROUTER entry=011DBE and MORPHUS_340_INCOMING_GUEST_NO_ORIGINAL_2E exactly 17 frames later. All 13 attempts had a real outgoing native $013AB6 tag and an incoming $011DBE callback, but only the six with independently verified original D6 phase-zero routines executed 2E.

The underlying original US opcode at $011DBE is **302E 0052 D040** followed by **207B 000A 4E90**: it is a **character-indexed TAG-IN entry** (MOVE.W $52(A6),D0 / ADD.W D0,D0 / MOVEA.L indexed table; JSR). It is NOT the fighter-owned D6 state dispatcher, whose actual original opcode signature starts **7000 102E 00D6**. Using tag-in as the guest's Hyper call recursively calls the wrong native routine.

Independent original 68000 observer shows for failing Cyclops at frame 2445: incoming Cyclops action **2A0E**, mode **2A2A**, D6 **0A** (outgoing Ryu's D6, not selected Cyclops 04). For failing Charlie at frame 8307: action **2A06**, mode **2A2A**, D6 **0A** instead of selected Charlie 04. In contrast, proven Ken at frame 9610 executes native action **2E02**, mode **2A00**, D6 **0E**. This is direct source evidence that the 87.343 tag-indexer fallback cannot stand in for native Hyper dispatch.

## Minimal corrective patch 87.344

- **Delete the original US $011DBE fallback as a callable Hyper router**. Preserve the working, real opcode- and ROM-script-verified Ryu/Ken D6 phase-zero dispatcher and both P1/P2 handoffs.
- Before the original attack input is suppressed or the native outgoing swap is armed, call the existing original-ROM D6 resolver for the actual reserve and selected original D6. If unverified, leave the physical PP/KK input and outgoing Hyper alone, refuse DHC, and record NATIVE_344_REFUSED_BEFORE_TAG.
- The refusal event now includes incoming fighter, side, D6, original fighter-region indexers and code range, native D6 header sites, valid D6 relative targets, phase-zero entries and the first matched target. This is read-only ORIGINAL US CODE diagnostic data to establish what alternate native dispatch pattern Cyclops and Charlie use, rather than guessing per-character function addresses. No new file or mandatory test is introduced.
- Double-check inside native guest preparation that no unsupported claim proceeds, and emit NATIVE_344_UNVERIFIED_D6_NO_TAG if it somehow reaches this second guard.
- Preserve 87.343's opponent-relative positioning for **verified** native Hypers. Preserve all real PP/KK input mapping; no special DHC chords, no new controller buttons, no fake 2E action.

**This deliberately prevents tag-only failures rather than pretending unverified fighters already execute their Supers.** Cyclops and Charlie incoming *still need* their original US native D6 routing decoded. The output from the new pre-tag diagnostic is the exact next evidence needed to add them without hardcoded shortcuts.

## Local TEST commands (PowerShell)

    $env:XM87274_UNIFIED_DHC = "1"
    $env:XM87334_MORPHUS_PRECOMMIT = "1"
    $env:XM87335_NATIVE_DHC = "1"
    $env:XM87340_MORPHUS_NATIVE = "1"
    .\BUILD_TEST.bat
    .\RUN_TEST.bat xmvsfu

Select Version 4. Reproduce a known valid native pair (P1 Cyclops → Ryu QCF+PP, or P2 Charlie → Ken QCF+KK), then try incoming Cyclops and Charlie. Before 87.344 the latter swapped and failed; now they should leave the outgoing Hyper running and log an original D6 dispatch refusal. No fake tag-only result should occur.

The next log location is F:\xmvsf\test\87.344\observer\dhc_unified_274.tsv. The version displayed in RUN_TEST.bat is 87.344, and BUILD_TEST.bat and BUILD_TEST.sh are aligned.

**Verification scope:** The 87.343 original-ROM log proves the regression source and the 87.344 code removes that fallback. The C++ integration/brace checks and regression assertion were reviewed. Full Windows native build and real gameplay for 87.344 are not yet verified. Do not claim this enables the whole roster or that all previously failing combinations can already Hyper.
