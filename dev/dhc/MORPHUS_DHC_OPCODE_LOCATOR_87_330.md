# 87.330 — read-only mapping of Morphus DHC hooks into XMVSFU

Source: Morphus56k `XMvsSF-3v3-DHC-v1.1.1`, `source/engine/native-hooks-2p.json`.
Starting from `feature/dhc-morphus-native-87-329`.

## Evidence from actual local TEST 87.329 opcode diagnostic

`morphus_opcode_sites_329.tsv` contains:
```
xmvsfu  1048576  DHC_CHAIN             013AA8  0C2E000400A4  6E1D56025902  DIFFERENT
xmvsfu  1048576  INCOMING_HYPER_COMMIT 011DB0  302E0052D040  08007C1D0800  DIFFERENT
```
This is **only six bytes per site**, not a ROM-wide comparison. It establishes that the European hooks **cannot be installed unchanged at those US addresses**; it does not identify US equivalent routines or prove the code is incompatible.

## Changes

- New `xmvsf_stage87_330_morphus_locator.h` is **read-only** and disabled unless explicitly requested.
- Searches actual decrypted `CpsCode` instruction view for **22 byte signatures** from Daniel's EURO `native-hooks-2p.json`, including `$11DB0`, `$13AA8`, nearby tag/VC/input/swap anchors, and independent controls.
- Records all hit counts, up to eight even-aligned candidate addresses, and offset displacement only when a signature is unique. Duplicate signatures remain ambiguous; a match is not proof of runtime equivalence.
- Original 87.329 hook diagnostic and pre-tag filter are preserved. No 68000 program mutations or hooks are applied.
- `dev/dhc/tests/test_morphus_locator_330.cpp` validates relocated and duplicate signatures, corrupt patterns and bounds. **Not added to the normal build test gate**.

## Use

On this branch, local Windows/MSYS2 workflow only:

```bat
set "XM87330_MORPHUS_LOCATOR=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
```

Result: `test/87.330/observer/morphus_opcode_locator_330.tsv`.

Disable the opt-in setting afterward. Stable, Version 3, ROM bytes and the existing 87.274/87.329 experimental DHC remain unchanged.

## What the output can and cannot prove

- A **unique** hit elsewhere is an address candidate worth M68K tracing, not a relocation guarantee.
- Several hits with the same offset across distinct nearby reference anchors may support a revision mapping, but require validation of instruction context, call sites and ROM execution semantics.
- Zero hits means the exact instruction bytes differ; use executed PC trace and disassembly rather than inventing a displacement.
- No complete native DHC commit is claimed until the US hook's original code, incoming actor context, meter behavior, freeze and tag-in are proven on genuine replays.
