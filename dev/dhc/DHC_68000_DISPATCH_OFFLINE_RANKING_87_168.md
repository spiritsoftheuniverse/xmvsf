# XMVSF Stage 87.168 — independently executed offline dispatcher ranking

**Source:** user-uploaded `dispatch_callers.json` SHA-256 `9c6781d0df3ca785c8cd9dac5ef1951cca42f7d218337d527476f4afceb9ae94`, cross-checked against user-uploaded `command_68000_audit.json`. Analysis was executed on the uploaded data; no game executable or private ROM binary was available in the analysis environment.

## Counts established directly

- The local opcode scanner examined **1,048,576** original decrypted 68000 bytes and reported **1,328 syntactic references** to native dispatcher 01038E / 0103C6.
- Its JSON retained **512 entries** (top-ranked only), with **816 omitted** due to the export limit.
- Of the retained entries, **103** have a nearby syntactic action-2E immediate. All 103 are preserved because the scanner sorts action-2E proximity first. Among those, **83** have an exact **4-byte** gap between the `MOVEQ #$2E,D0` opcode and the native dispatch call; another **2** have an exact **2-byte** gap, giving **85** in the strongest proximity tier. Remaining near-action entries: 8 with 6–16 byte gaps, 10 with longer (up to 96-byte) gaps.
- Among the emitted 512 candidate calls, 503 target 01038E and 9 target 0103C6, with 313 JMP_ABS_L, 193 JSR_ABS_L, 3 JMP_PC_W and 3 JSR_PC_W. The report **cannot** establish instruction boundaries/executable reachability for every syntactic match.
- Densest 4-KB regions among the strongest 85: **037000 (10)**, **030000 (9)**, **047000 (7)**, **021000 (6)**, **028000 (6)**, **023000 (5)**, **042000 (5)**. Dense does not imply a particular character or super.

## Grounded interpretation

The earlier decoded real 68000 instruction window at PC 01038E writes `move.b d0,$60(a6)` then `move.b d1,$61(a6)`, and uses the original fighter/action/selector ROM tables. Thus the source code at a correctly executed, validated action-2E caller can supply the actual subaction selector in D1. **But this uploaded cross-reference JSON deliberately exports PC values and pattern labels, not the actual opcode bytes at those PCs.** For the 83 sites with a four-byte gap, there are two unknown code bytes between the D0 assignment and call. It would be inaccurate to invent a `MOVEQ #selector,D1` instruction from the absence of those bytes.

Cross-check with the 29,827-frame original recorded movie remains complete: 43 physical input correlations, 150 ROM script events, 22 distinct input-associated native roots. No D1-origin correlation, CPU branch reachability, actor-specific command handler, or new DHC move has yet been proven.

## Artifacts produced from actual upload

Offline report:
- `XMVSF_87_168_dispatch_offline_ranked.json`: 103 annotated near-action candidates, with all 85 strongest sites in order and 4-KB grouping.
- `XMVSF_87_168_dispatch_offline_ranked.csv`: spreadsheet-friendly PC/target/proximity/tier listing.
- `XMVSF_87_168_dispatch_offline_analysis.zip`: both outputs and README.

These were created in the analysis container and are linked in the user-facing answer. This GitHub document is a permanent audit, not an instruction to run new scripts or switch launchers.

**Limit of current material:** the private 1 MB `local_rom_dump/xmvsfu_decrypted_opcodes_host_swapped.bin` was available on the user's machine for the original scan but is **not** in the uploaded JSON, original replay ZIPs or analysis container. The missing two-byte D1 instructions cannot be reconstructed from derived addresses. No further user action is requested merely to deliver this offline ranking; true instruction-level recognizer work still requires that underlying opcode image or a derived instruction-window export.

**No gameplay modifications, no Stable edits, and no new launcher scripts.** Akuma Tenma Gou Zankuu remains excluded.
