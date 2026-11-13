# XMVSF TEST 87.345 — connect original native Hyper input to indexed D6/phase dispatch

Branch: feature/dhc-morphus-precommit-87-334. TEST 87.345, with visible RUN_TEST.bat header and all local build versions advanced. The working Morphus-derived outgoing tag and native incoming guest, true PP/KK controls, P1/P2 actor ownership and opponent-relative X are unchanged.

## Why 87.343 tagged but did not Hyper

The uploaded 87.343 observer contained 13 genuine original 68000 DHC handoffs. Six Ryu/Ken incoming commands executed original action 2E; seven Cyclops/Charlie incoming commands used the experimental fallback call to $011DBE, which is the stock **fighter-indexed tag-in** selector, not a Hyper dispatcher. Those seven swapped and failed to execute 2E. TEST 87.344 correctly deleted that invalid fallback and refused unknown routes before suppressing input/tagging.

The original-US 68000 command matcher already determines the incoming fighter's true Hyper state (+D6). The fighter's own original **D6-indexed state handler** selects a phase from +08(A6), and its original code owns action 2E. The host must identify that actor-owned state handler, not call the tag-in selector again and not write a guessed action/animation.

## 87.345 implementation

The existing XM87336Dispatch::resolve still first checks the EXACT 87.336 original-US Ryu/Ken baselines, including the known 2E script/launch bytes. Those paths have proof=3 and remain unchanged.

For other original fighters, the dynamic resolver now:

1. Reads the original decrypted 68000's 19-entry character-indexed code tables, validating their actual MOVE.W $52(A6) / ADD.W D0,D0 / MOVEA.L (PC,D0.W) indexer signatures, code pointers and minimum valid entry counts.
2. Collects the exact original native function entry addresses for every character, in addition to inferred private code ranges. This handles fighter-owned routines outside an overly narrow min/max code band.
3. Resolves the selected **real D6 byte** as the original signed PC-relative *byte offset* in the native D6 jump table. Requires the actual original instruction signature 7000 102E 00D6 303B 0006 4EFB 0002, a valid target, and a native +08(A6) phase-zero indexed entry (either original D0 or D1 variant). Never dispatches through $011DBE.
4. When a phase-zero body has the previously audited original 2E launch pattern and a valid fighter-owned ROM script, retains that direct proof (proof=1). When its 2E launch code isn't Ryu/Ken-shaped, permits the original fighter **state/phase path** only if the exact original command matcher independently writes this D6 and mode 2A, OR the original character-indexer directly owns this very state-dispatcher PC, AND the fighter has at least one bounded original 2E ROM script (proof=2). This is original-code provenance, **not yet a gameplay guarantee that phase zero will launch 2E**.
5. Rejects ambiguous competing native state entrypoints or absent D6/phase/source/ROM evidence before suppressing any original PP/KK input or executing Morphus's native outgoing swap. It never invents a 2E selector or performs the old stock tag-only fallback.
6. Searches only the selected original fighter's private code region and exact indexed function entrypoints. It does not scan a 1 MB CPU image on every tag frame. Cached regions are scoped to the original CpsCode pointer/program fingerprint; no ROM modification.

The existing event log now emits NATIVE_345_D6_TABLE_ROUTE with actual incoming fighter ID, D6, source M68K dispatch PC, original phase-zero PC, proof class and optional script address. A refusal keeps NATIVE_344_REFUSED_BEFORE_TAG and adds NATIVE_345_INDEXER_EVIDENCE showing exact indexed pointer count, out-of-range D6/phase sites and whether the fighter's own original matcher supplies this D6. The live original actor observer remains the authority for whether action 2E really executed.

## Synthetic regression

The existing test_native_d6_dispatch_336.cpp now checks:
- Prior compact original 2E launch paths remain accepted.
- A real original fighter input matcher + D6 phase path with non-Ryu launch layout returns proof=2.
- A second original 19-entry fighter table can own a Charlie D6 dispatcher outside its inferred character code region, without using any hardcoded Charlie-specific gameplay address.
- Incorrect D6 byte, changed original indexed owner, or removed native 2E ROM table refuses the route. Same code/ROM signature guards remain.

No new mandatory BUILD_TEST test, GitHub Actions or separate user-facing diagnostic subsystem was added.

## Local Windows playtest

In the same PowerShell session, choose original-US xmvsfu and **Version 4**:

    $env:XM87274_UNIFIED_DHC = "1"
    $env:XM87334_MORPHUS_PRECOMMIT = "1"
    $env:XM87335_NATIVE_DHC = "1"
    $env:XM87340_MORPHUS_NATIVE = "1"
    .\BUILD_TEST.bat
    .\RUN_TEST.bat xmvsfu

Test an already-working incoming Ryu/Ken Hyper on both sides, then Ryu -> Cyclops QCF+PP (D6=04) and Ken -> Charlie QCF+PP (D6=04) / QCB+KK (D6=06) from the 87.343 capture. For each, the normal successful chain is real PP/KK input -> original D6/phase verification -> original M68K outgoing tag -> original swap -> incoming D6 commit -> actual action 2E. No invented assist/DHC controls.

Log: F:\xmvsf\test\87.345\observer\dhc_unified_274.tsv

If the new proof=2 route still tags but does not reach action 2E, this evidence establishes the actor's native state/phase dispatcher but not the 2E launch within it; further original 68000 flow verification is required. Do **not** claim full roster parity based on a synthetic test or source inspection. A fixed native D6 byte alone cannot substitute for the actual state machine.

**Verification:** Committed C++ source, structural checks and synthetic fixtures; actual full Windows build and 87.345 gameplay unverified here. Stable and Version 3 untouched.
