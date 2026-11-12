# 87.329 — Morphus 3v3 DHC source integration, preliminary checkpoint

**Source:** [Morphus — XMvsSF Three-on-Three](https://www.morphus56k.com/blog/xmvsf-three-on-three), uploaded `XMvsSF-3v3-DHC-v1.1.1-2P-windows-x86_64.zip`.
**Original author:** Daniel Plas Rivera / Morphus56k. Respect the distribution, attribution and license terms in that package and its FBNeo sources.

## Scope actually implemented

On `feature/dhc-morphus-native-87-329`, from `feature/dhc-unified-m68k`:

- An exact 19-row, 10-byte-per-row **read-only** native Hyper catalog transcribed from `source/engine/dhc_table.inc`: incoming character +D6 Hyper ID per motion (7 columns) plus air-only, conditional and throw bitmasks.
- An **opt-in pre-tag gate** in the existing 87.274 universal DHC executor. When the experimental executor has identified a native outgoing Hyper and a fresh reserve gesture, it can refuse a *known empty*, *air-only* or *conditional* incoming Hyper **before controller 8 / native swap**. The outgoing Hyper remains intact. Unknown gestures fail open; possible and throw Hypers remain native-ROM verified after tag.
- A standalone C++11 policy test, **not added to BUILD_TEST**. No native 68000 instructions are patched here.

Enable only in **Version 4** with `XM87274_UNIFIED_DHC=1` and `XM87329_DHC_PREFLIGHT=1`. Default is off. Version 3 is untouched. Use root `BUILD_TEST.bat` / `RUN_TEST.bat xmvsfu`, with existing warm cache, and preserve Stable. No GitHub Actions.

## What Daniel's package proves

`source/engine/dhc.s` has genuine 68000 hooks at **$13AA8** (outgoing DHC chain) and **$11DB0** (incoming tag-in handoff), with a pending D6 Hyper selection. At the commit hook it changes native +$A0, +$D6 and pose/position fields; it then calls original code at `$125D6`, `$10380` and `$9E90`. The receiving fighter's normal Hyper startup handles meter, superfreeze and cut-in. The code also maintains the A5 register invariant and a chain-participation mask. This is a better basis for universal no-kick, immediate DHC than the present host-side **wait for tag neutral, then replay input** design.

## What remains unimplemented and unverified

- The supplied project targets **European 961004 parent `xmvsf`**, whereas our routine Windows target is **US `xmvsfu`**. The 68000 hook instruction bytes, *decrypted opcode view*, ROM addresses and native Hyper +D6 IDs must be verified against our loaded ROM. Never transplant its patch addresses or write +D6 on the US revision without verifying the full native dispatch.
- Morphus' 3v3 runtime includes custom memory, reserve-order and emulator facilities. Our target is **2v2**; only the DHC dispatch and tag-in mechanics should be adapted, not his 3v3 runtime or slot allocator.
- The 87.274 executor still uses controller 8, waits for the genuine swap, waits for neutral, and replays the incoming fighter's input. This commit **does not** replace it with the native commit hook, eliminate tag-in kicks, or prove meter consumption/animation/freeze timing.
- Existing host proposal matching handles only QCF/QCB, DP, HCF/HCB and double-quarter-circles. Daniel's source uses 14 core input samples, QCF/QCB/DP/RDP/HCF/HCB/double-down, MP+MK/HK chords and priority. The two models are **not interchangeable**.
- No native Windows build, F9 replay or in-game test is claimed. Standalone unit testing only.

## Next implementation work, in order

1. Run a **read-only M68K trace on `xmvsfu`** to find and verify its equivalents of `$13AA8` and `$11DB0` and Hyper startup calls; capture the exact original instruction bytes.
2. Adapt Daniel's native two-fighter transaction with **guarded opcode patching** and an original-code fallback, preserving A5 and existing objects.
3. Register incoming Hyper D6 by verified character + original ROM motion; no guessed 2E selectors. Reject invalid motion **before the swap** and retain meter until native startup.
4. Connect native tag-in to direct Hyper startup. Verify two sides, the full character matrix, once-per-chain policy, supers hitting/missing, projectiles, partner position, freeze, one-stock debit, no accidental tags/doubles and Version 3 unchanged.
5. Only then replace the experimental Version-4 executor as default DHC.

This checkpoint is **not finished DHC**. It is a non-destructive, reproducible first port from the published native 68000 material.
