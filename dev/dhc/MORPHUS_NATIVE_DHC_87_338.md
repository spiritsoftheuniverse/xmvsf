# XMVSF 87.338 — fix DHC input refusal, retain 87.336 live native Hyper baseline

**Branch:** `feature/dhc-morphus-precommit-87-334`, updated in place. No GitHub Actions, no new build steps, no Stable/Version 3 changes.

## User-confirmed regression

87.337 compiled successfully on the user's Windows system, but input no longer cancelled supers into incoming Hypers. Unlike a compiler failure, this points to an early native DHC eligibility refusal. The 87.337 roster generalizer introduced global requirements that were **not part of the live-playtested 87.336 Ryu→Ken handoff**, including ten near-complete M68K character-code indexers, no missing fighters across their tables, all helper routines confined to each inferred fighter's code range, and one textual 2E launch per phase-zero handler. The visible result is consistent with a pre-tag `NATIVE_337_REFUSED_UNVERIFIED_FIGHTER_D6`. The precise runtime rejection has **not** been determined from a user TSV.

## 87.338 correction

This edits the EXISTING native `XM87336Dispatch` resolver; it does not add another runtime DHC state machine or revert the working 80-byte native handoff.

1. Restore the four **original-US, exactly ROM-byte-and-script-verified** 87.336 native command routes as a trusted baseline:
   - Ryu +D6 `0A`: original dispatcher `$0337BA`, conditional action `2E08/2E0C`.
   - Ryu +D6 `0C`: original dispatcher `$0337BA`, action `2E00`.
   - Ken +D6 `0C`: original dispatcher `$035356`, **Shoryureppa**.
   - Ken +D6 `0E`: original dispatcher `$035356`, **Shinryuken**.
   These entries must still match the specific original program opcodes, phase-zero target, action-2E launch and original ROM script. No blind skip of ROM guards is allowed, and a mismatch fails closed rather than guessing.
2. The wider-cast original fighter-code resolver no longer requires *every* fighter slot to appear in ten candidate indexers. A genuine, signature-confirmed M68K indexer with 15 valid entries can establish the **selected fighter's** code region, even if unrelated fighters are missing. The selected native D6 handler and original 2E script must still independently validate.
3. The incoming fighter owns the D6 **entrypoint**, but its original 68000 helper and phase-zero routine may be shared or outside its inferred private code block. Stop using this false region-boundary test to veto legitimate native code targets.
4. A native phase-zero handler can contain equivalent or conditional native 2E launches. Identical launches do not count as conflicting dispatchers. Truly conflicting M68K paths remain refused rather than choosing a random Hyper.
5. The working original controller-8 tag, correct-position/pause, opt-in Version 4 guard, P1/P2 ownership, Ken QCF+KK Shinryuken mapping, ROM-owned execution and post-entry meter observation are unchanged.

## Regression sources

- `test_native_dhc_baseline_338.cpp`: synthetic original-program/ROM fixture independently exercises all four live-established Ryu/Ken routes WITHOUT any broad fighter-indexer table, including a modified Shinryuken opcode rejection.
- `test_native_d6_dispatch_336.cpp`: revisited wider-roster fixture now uses a partial 15/18 native M68K indexer and permits a Cammy phase helper outside her private inferred region, which specifically exercises the former 87.337 vetoes.
- Standalone tests are NOT added to `BUILD_TEST.bat` and have not yet been executed on the user's Windows system.

## Local TEST / evidence

```bat
set "XM87274_UNIFIED_DHC=1"
set "XM87334_MORPHUS_PRECOMMIT=1"
set "XM87335_NATIVE_DHC=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
```

Select Version 4. Start with Ryu→Ken **QCF+KK** (Shinryuken, D6 0E) to reconfirm the known 87.336 baseline, then test incoming Storm QCF+PP, Magneto QCF+KK, Cammy QCF+KK or Cyclops QCF+PP. The existing `test/87.338/observer/dhc_unified_274.tsv` log reports pretag refusals and `NATIVE_337_FIGHTER_DISPATCH_VERIFIED` when an actual fighter-owned native D6 handler is found.

Local Windows build/gameplay of 87.338 is **not verified here**. Correct opcode signatures and source integration are the only current verification; a true full-cast pass still requires live original 68000 game evidence. The next fix should follow the actual rejection in this existing trace, not create another diagnostic branch.

Both the visible `RUN_TEST.bat` header and all build/run version variables are advanced to **87.338**.
