# Stage 87.168 — native 68000 disassembly and dispatcher cross-reference

Source: user-submitted command_68000_audit.json, produced locally from the user's private original XMVSF decrypted CpsCode and ROM data. The native 68000 code was successfully decoded. This establishes true ROM controller/script dispatch, not the fighter-specific move-input recognizer.

## Confirmed 68000 findings

- At PC 01038E: move.b d0,$60(a6); move.b d1,$61(a6); load fighter table at 8A946, index using the live fighter identity at $52(a6), follow the actor's action and selector offsets, then jmp 1AE6. This is a generic native action/script dispatcher; find its callers to locate where action 2E was chosen.
- At PC 0103C6: an adjacent dispatch variant performs a similar lookup with an additional lsl.w #4,d2 before jumping to 1AE6. Inspect it separately, not past 01038E's unconditional JMP.
- At PC 013B8E: set native controller +256 to 08, mirror to +259, return with d0=-1. State 08 is a controller state, not a super move.
- At PC 00C4E6: read +256(a6), branch to return if zero, double D0, fetch a native handler pointer from 00C506+D0, then jump through A0. For controller state 08 the pointer is at 00C516 and its native target is **011E50**. The bytes after the RTS at 00C4F4 are a jump table, not executable ori.b instructions.
- At PC 011B00: initialize outgoing-tag state, set D0=32 and D1=00, then jump to 01038E. This yields native action 3200. The adjacent handler at 011B1E must be decoded as its own basic block.

## High-bit ROM script entries

All **58 distinct** observed action-2E roots match their native ROM pointer. The two roots omitted by the conservative capture scanner carry first script words with bit 15 set:
- Ryu 12 / 2E06 / 0D2E7C: raw first word 8004
- Sabretooth 0A / 2E16 / 0C0E86: raw first word 8008

The catalog excluded them solely because it rejected first words >=8000. These are real native script pointers; the meaning of the high bit remains UNDECODED and is not proof of a new executable super. The updated offline analysis preserves raw first word and high-bit status instead of interpreting it as duration.

## Implemented this pass

1. DISASSEMBLE_SUPER_MOVES.py now stops a linear basic-block decode on unconditional JMP, BRA, RTS, etc.; otherwise the controller's pointer table was being misidentified as code. Default PC windows also include separate adjacent routines.
2. TRACE_68000_DISPATCH_CALLERS.py scans private locally exported decrypted opcodes for static 68000 JSR/JMP/BRA/BSR call sites targeting 01038E or 0103C6; optionally ranks recent action-2E immediate assignments. These are syntactic candidates until actual executable reachability and register/condition data are proven.
3. test_dhc_dispatch_xrefs_168.py contains a ROM-free fixture for the exact 011B1A PC-relative JMP back to 01038E, an absolute call and a synthetic near-action-2E candidate.

## Run locally (no new batch launchers)

From F:\xmvsf, on feature/dhc-mvc2-stage87:

    py -3 dev\dhc\tests\test_dhc_dispatch_xrefs_168.py
    py -3 dev\dhc\tools\TRACE_68000_DISPATCH_CALLERS.py --opcodes local_rom_dump\xmvsfu_decrypted_opcodes_host_swapped.bin --out test\87.167\decomp\dispatch_callers.json

The output includes PC references and pattern hints but NO private ROM dump. Send only dispatch_callers.json for subsequent exact control-flow analysis. No command handler is certified without native input-read and condition branches. No gameplay or Stable changes were made, and no new super was enabled. Akuma Tenma Gou Zankuu remains EXCLUDED.
