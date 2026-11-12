# XMVSF 87.274 — universal M68k DHC integration checkpoint

**Branch:** `feature/dhc-unified-m68k` from the user-requested `main`.
**Build policy:** local `BUILD_TEST.bat` / `RUN_TEST.bat` only; no Actions, no Stable overwrite, no cache deletion.
**Scope:** experimental new Version 4 DHC path, disabled unless explicitly selected. Version 3 remains untouched. This is a source checkpoint, **not** a claimed native Windows gameplay pass.

## Why a separate executor

The legacy `XM87144` handoff and `XM87143` queue are still built around P1 Ryu (0x12) -> Ken (0x14), script 0xD1DAC, Ken action 0x2A08, and the two Ken 2E roots 0xD6722/0xD6976. Simply deleting the fighter ID check would force the wrong original ROM scripts for the rest of the cast. The original 87.192 reserve-owned parser and 87.189 native 2E super-family tracker are more general and are reused, rather than building another move-name whitelist.

## Implemented in 87.274

1. `xmvsf_stage87_274_unified_dhc.h` handles both P1 and P2 from the actual actor and bench RAM locations. It recognizes only a current-frame, native 2E ATTACK root tracked from the original ROM, a fresh reserve-owned motion and at least one additional stock; resolves the outgoing fighter's original 32/00 tag script before attempting the native controller-8 transaction.
2. The original 68000 must select 3200/2A00 and actually swap the two owned fighter records. A host-generated tag sprite or arbitrary actor ID swap is **not** a success. Wrong actions abort without forcing an invented ROM script.
3. After the genuine swap, the reserve must return naturally to neutral control. The incoming fighter receives the saved input motion via its own CPS2 input port; only the resulting original ROM 2E action can be recognized as a super. No hardcoded Ryu/Ken incoming action or named-super target.
4. Source-level save/restore covers P1 direction/punch/kick arrays and P2 direction/punch/kick arrays; physical inputs return immediately after `CpsRwGetInp`. Replayed commands are bound to side and actor identity.
5. A confirmed incoming super holds a per-side DHC lock until genuine neutral ROM exit; interrupted ownership and rejected commands release only host bookkeeping.
6. Existing Ryu/Ken DHC still runs by default, including its existing projectile, tag-entry and freeze logic. `XM87274_UNIFIED_DHC=1` selects the new runtime **instead** in Version 4. It never enables in Version 3 or during runahead frames.

## Known gaps — do not call this complete DHC yet

- **Kickless incoming entry:** there is no single verified ROM entry animation for all characters. 87.274 does **not** reuse Ken's non-attacking 2A08 or teleport any character to the ground. It waits for real native neutral. Native tag kicks may still occur; that is not the final DHC presentation.
- **Pre-tag super certainty:** the reserve is inactive and its original 68000 command parser has not run. The new runtime uses a fresh, two-button motion as a proposal; only post-tag ROM execution verifies the incoming super. A rejected native input cannot undo a tag that already happened. Complete rejection-before-tag needs a native reserve-side dispatcher preview or a verified per-fighter learned command mapping.
- **Super coverage:** the captured 87.192 gesture parser supports QCF/QCB, DP, HCF/HCB and double quarter circles. Charge, mash, 360, SGS and custom button sequences still need original command-decoder evidence; merely enumerating 2E scripts is insufficient.
- **Visuals:** no shared incoming portrait suppression or DHC-only opponent freeze is enabled by this first universal path. Those must follow confirmed native controller/first-active events, not Ken-specific timing.
- **Meter:** require at least one stock before arming and let the incoming fighter's original ROM command debit the super. Actual stock consumption must be checked from before/after native captures; no guessed direct RAM debit.
- **Controller acceptance:** original controller 8 can select a different action for non-Ryu fighters. The shared path intentionally logs and rejects those cases; it does not force a synthetic 3200 or overwrite unknown per-fighter state.

## Local validation

From a Windows/MSYS2 UCRT64 terminal, optionally run the small standalone policy test without adding steps to the normal build:

```sh
g++ -std=c++11 -O2 -Wall -Wextra dev/dhc/tests/test_dhc_unified_274.cpp -o /tmp/test_dhc_unified_274.exe
/tmp/test_dhc_unified_274.exe
```

Then run the existing local `BUILD_TEST.bat`. To opt into the new **experimental** gameplay path for original XMVSF, from the same Command Prompt:

```bat
set "XM87274_UNIFIED_DHC=1"
RUN_TEST.bat xmvsfu
```

`RUN_TEST.bat` sends the state log to `test/87.274/observer/dhc_unified_274.tsv`. Reset the test flag to `0` to restore the old DHC implementation. Neither mode changes Version 3.

First acceptance matrix: P1 and P2 independently, at least two different outgoing/incoming IDs, single valid original 2E super, wrong-action original ROM abort, no stock, double-command lock, post-super rearm, ordinary tags, super miss/hit, game-mode reentry and runahead. A true general release additionally requires natural kickless arrival, input acceptance and meter proof for all selectable supers.
