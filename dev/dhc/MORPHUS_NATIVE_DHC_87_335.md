# XMVSF 87.335 — first direct Motorola 68000 incoming DHC handoff

**Branch**: `feature/dhc-morphus-precommit-87-334` (continued in place; no branch created).
**Sources**: Morphus56k / Daniel Plas Rivera, `XMvsSF-3v3-DHC-v1.1.1` `source/engine/dhc.s` and `dhc_commands.json`; verified original US `xmvsfu` opcode windows captured in `morphus_dhc_context_332.tsv`. Root README is the development workflow reference.

## Behavior implemented

This is an **actual new execution path**, not another address locator or catalog:

1. The opt-in Version-4 2v2 DHC still requires the original outgoing fighter's native 2E Hyper window, a prequalified incoming Hyper and sufficient meter. The native ROM must accept controller 8 and perform the actual point/reserve swap.
2. The 87.335 bridge installs an instruction-completion callback only during an eligible pending swap. It checks **the next original M68K PC is $011DBE**, A6 names the real incoming point slot for the correct side, and the original ROM has actually exchanged the two character IDs. It verifies five exact US opcode windows before enabling anything.
3. Rather than rewriting on-disk ROM or guessing a code cave, a private 4KB guest M68K fetch/RAM page is mapped at otherwise-unmapped **$500000**. The callback redirects the next original 68000 instruction fetch to the P1 or P2 guest entry at `$500000`/`$500100`. Normal tags execute the unmodified original `$011DBE`.
4. The **80-byte real 68000 routine** saves D0-D7/A0-A4, leaves A5 as the native `$FF8000` base, preserves the previous +A0 in +A1, then sets +A0=2A and the chosen native +D6 Hyper ID, removes stale tag-in fields, places the incoming fighter at the outgoing native X and actual ground, clears stale velocities, calls the **verified US original dispatcher $1038E** and original routine **$9E90**, restores registers and returns to the native tag-in caller. There is no host-created 2E action, fake cut-in, host meter debit or delayed input replay.
5. Host state is tagged as a **direct native guest commit**, not legacy completion; after CPU execution the existing ROM observer verifies the real fighter entered an original ROM 2E super with the expected +D6. It logs stock before/after. Missing native 2E within 16 frames fails bookkeeping only, never rolls back a genuine native swap or invents an action.
6. The prior 87.274 neutral-wait-and-input-replay route remains available when 87.335 direct mode is disabled. **When direct mode is explicitly requested, a missed native hook does not silently fall back to that old path.**

This is only enabled by `XM87274_UNIFIED_DHC=1`, `XM87334_MORPHUS_PRECOMMIT=1`, and `XM87335_NATIVE_DHC=1`, **Version 4, US `xmvsfu`**, non-runahead frames. Defaults and Version 3 are unchanged.

## Known missing dependency — no claim of a complete DHC pass

Daniel's exact 68000 `xm3_hook_dhc_commit` also calls the original European helper **$125D6** before script dispatch. The US equivalent is **not verified** from the supplied opcode capture. This implementation **does not guess $125E4 or any other target** and therefore omits that call. Consequently real Hyper startup, meter debit, cinematic freeze/cut-in, and action/animation may not complete. This needs native Windows/F9 evidence, not speculation. All original hooks are still untouched in the ROM.

The 87.335 guest is **experimental and opt-in**, not the finished 2v2 DHC release.

Other unproven areas: movement/ground placement for all fighters, inherited projectile ownership, tag sound, simultaneous two-side DHC, true one-stock debit, and the full roster including special command forms excluded by the current 34-entry restricted native-input table.

## Local TEST workflow

```bat
set "XM87274_UNIFIED_DHC=1"
set "XM87334_MORPHUS_PRECOMMIT=1"
set "XM87335_NATIVE_DHC=1"
BUILD_TEST.bat
RUN_TEST.bat xmvsfu
```

Choose Version 4. The existing DHC observer log under `test/87.335/observer/` should record `NATIVE_335_US_11DBE_DIRECT_M68K_COMMIT_ENTERED` when an exact native tag-in was intercepted, and either `NATIVE_335_68000_ROM_2E_CONFIRMED` with stock values or `NATIVE_335_GUEST_NO_ORIGINAL_ROM_SUPER`. If the hook was missed, `NATIVE_335_NO_ORIGINAL_INCOMING_HOOK_NO_INPUT_REPLAY` is emitted instead. These are *runtime tests to perform*, not results already obtained.

No GitHub Actions, extra build-time regression gates, Stable overwrites or warm-cache invalidation. Root build/run scripts use TEST **87.335**.

## Validation actually performed

- Pushed source to the same branch; verified source includes, bridge begin/end placement, and version-script references on GitHub.
- Checked the exact 80-byte Motorola instruction sequence in the committed header, including swapped guest code layout, P1/P2 D6+X operands, saved/restored registers, JSR `$1038E`/`$9E90`, and RTS.
- Standalone test is in `dev/dhc/tests/test_native_dhc_guest_335.cpp`, **not added to BUILD_TEST**.
- A complete local Windows build, live native execution and gameplay/F9 acceptance are **not yet available from the connected GitHub environment**.

## Next native step

Inspect real 87.335 gameplay acceptance and, if necessary, determine the exact US native startup-helper contract from executed M68K code. Do not introduce more speculative catalog/diagnostic branches or switch away from this DHC branch.
