# XMVSF 87.336 — native D6 Hyper startup after the proven tag-in

**Same branch:** `feature/dhc-morphus-precommit-87-334`. No new branch, no GitHub Actions, no new BUILD_TEST gates, no Stable changes. The original Version-3 game remains unchanged.

## Why 87.335 could position Ken correctly without starting his Hyper

The 87.335 incoming guest set +$A0=2A and the selected +$D6, but then invoked original US **$01038E with D0=D1=0**, which is the ordinary action/script selector, **not** the native fighter's Hyper command dispatcher. It can select idle instead of executing a D6-selected phase-zero Hyper.

The repository already documented the real US control flow in `dev/dhc/DHC_NATIVE_D6_TO_2E_ORIGINAL_ROM_87_174.md` and its original-ROM opcode verifier `dev/dhc/tools/ANALYZE_68000_D6_TO_2E_PATHS.py`:

`+$D6 -> per-character D6 dispatch -> +$08 phase-zero handler -> original action 2E / selector -> $01038E -> original script`.

## Changes in 87.336

- `xmvsf_stage87_336_native_d6_dispatch.h`: uses original **US decrypted 68000 opcode signatures** to verify the D6 first-level jump, the second-level phase-zero jump, the actual 2E launch instructions, and the original `CpsRom` action/script pointer. This is not a guessed address shift or hand-authored 2E animation.
- Verified entries at this stage (from the older native 87.174 source audit):

  | Incoming fighter | D6 | Original US D6 dispatcher | Original phase-zero entry | Native action/script |
  | --- | --- | --- | --- | --- |
  | Ryu | 0A | $0337BA | $033DA6 | 2E08 or conditional 2E0C; base script 0D3070 |
  | Ryu | 0C | $0337BA | $033F28 | 2E00 / 0D2CFC |
  | Ken | 0C | $035356 | $035A10 | 2E00 / 0D6722 |
  | Ken | 0E | $035356 | $035C5A | 2E02 / 0D6976 |

- The existing original-game swap remains the owner of fighter exchange and the correct-position/pause behavior observed live in 87.335.
- The native guest now emits **JSR to the ROM-verified incoming fighter's D6 dispatcher**, not the zero-action generic dispatcher. That dispatcher's original 68000 phase code selects action 2E and its original animation automatically.
- Unsupported characters/moves, changed opcode signatures, invalid 68000 phase targets or mismatched ROM scripts **reject before controller 8 is armed**. The original outgoing Hyper continues. There is no accidental tag and no forced move.
- The existing real-ROM post-entry checker remains in place: D6 must match, original 2E must be observed, and stock before/after is logged. No host-side meter debit or synthetic superfreeze is introduced.

**Important:** These are four US program paths that were independently verified in the prior 87.174 analysis. The ROM-fingerprint check is performed again on the running decrypted program before activating any of them. Other characters are intentionally not allowed into this experimental direct path until their original character-specific dispatcher is proven; this is not a finished full-cast DHC.

## Local test

Existing opt-in flags, Version 4, original `xmvsfu` ROM:

```bat
set "XM87274_UNIFIED_DHC=1"
set "XM87334_MORPHUS_PRECOMMIT=1"
set "XM87335_NATIVE_DHC=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
```

Try **Ryu -> Ken with QCF+PP** first, then QCF+KK (Ken has a different +D6). Reverse Ken -> Ryu afterward. Event `NATIVE_336_ORIGINAL_D6_DISPATCH_ENTERED` means the correct native entry path was selected, **not** that the Hyper, meter or freeze passed. `NATIVE_335_68000_ROM_2E_CONFIRMED` is the subsequent independent original-ROM action confirmation.

## Validation performed / still required

- Branch code, linked includes, call sites, exact original dispatcher target encoding and local version scripts reviewed.
- Standalone C++11 smoke tests with synthetic ROM-free 68000 instruction/script fixtures passed for the guarded original D6 resolution and guest-code emitter; test source is `dev/dhc/tests/test_native_d6_dispatch_336.cpp` and the updated `test_native_dhc_guest_335.cpp`. No new build-time gates.
- **Not yet verified:** full Windows TEST compilation and the new 87.336 gameplay behavior, one-stock debit, native superfreeze/cut-in, active attack frames, two-sided/replay consistency and full-character coverage.
- Morphus' original Euro `$125D6` native startup helper remains unported because its exact US equivalent is still unverified. Do not invent that call or declare final DHC without in-game evidence.

**RUN_TEST.bat visible header and executable banner now reflect 87.336**, and build/version defaults match. No stale "Stage 87.329" startup line remains.
