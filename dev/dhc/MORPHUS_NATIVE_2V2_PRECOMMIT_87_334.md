# XMVSF 87.334 — native D6 2v2 precommit integration

Forked from `feature/dhc-morphus-native-abi-87-333`, preserving Version 3, Stable, F9/replay and the original warm Windows build.

Source credit: Daniel Plas Rivera / Morphus56k, `XMvsSF-3v3-DHC-v1.1.1`, `source/engine/dhc.s`, `dhc_table.inc`, `DHC-COMMANDS.md`.

## Included
- True 2-member native Hyper **D6 selection** from Daniel's character table. It maps the existing captured reserve motion to a candidate native +$D6 value and pre-rejects absent, air-only, conditional, mixed-button, single-button or unsupported entries. Throws are identified as terminal. It does **not** invent 2E action selectors.
- A one-use, side/actor-bound native precommit descriptor, protecting against a duplicated handoff or the wrong fighter consuming a pending command.
- In the existing opt-in Version-4 87.274 unified executor, the 87.334 precommit path validates and stages the selected +$D6 **before** invoking the existing original-controller-8 tag. It checks the actual point/reserve identity after the original swap; a failure clears the descriptor. Successful actor ownership is logged, but does NOT claim an original Hyper was started.
- Small standalone C++11 test. Not added to the normal build steps.
- `XM87334_MORPHUS_PRECOMMIT=1` enables this stricter experimental validation only when `XM87274_UNIFIED_DHC=1` and Version 4 are selected; off by default.

## Critical remaining dependency for the actual direct DHC

This is **not** Daniel's complete 68000 execution bridge. The original US incoming hook is `$011DBE`, outgoing hook `$013AB6` (both proven by 87.332). At commit, his native routine additionally invokes original helper calls `EURO $125D6`, `$10380`, `$9E90` and outgoing tail `$4BF54`. Only original US `$1038E` is independently established by the current codebase. Other US helper entrypoints, return/stack behavior, A5 preservation, helper lifetimes and tag timing are not verified yet. In particular the EURO voice jump is `$78BA4` while US is `$78BB6`; there is *no global shift*. Therefore installing a US `JMP` into guessed M68K code would be unsafe.

The 87.274 executor STILL uses its original genuine tag/swap/neutral/replayed-input flow. D6 staging alone does not initialize a Hyper or charge meter. The new Pending data remains host-owned only. It never writes +$D6 to a fighter or creates a synthetic animation.

## Local acceptance
Build using the unchanged `BUILD_TEST.bat` and `RUN_TEST.bat xmvsfu`. For comparison set both `XM87274_UNIFIED_DHC=1` and `XM87334_MORPHUS_PRECOMMIT=1` in the same command prompt; the Version-4 selection is required. Local Windows compilation and runtime gameplay are not verified in the connector environment. The target pass criteria remain a **direct** original-ROM incoming Hyper at the hook, exactly one native meter debit, native freeze/cut-in, both sides/roster, no tag kick or duplicate, and unaffected Version 3.
