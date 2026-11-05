# Stage 87.168 — DHC native-command evidence

**Status:** replay verified; physical input bytes verified; 68000 command recognizer is **not yet decoded**. No new move is enabled.

## What changed

- dev/dhc/tools/VERIFY_DHC_ROM_ACTIONS.py accepts --evidence-out DIRECTORY. It exports the 43 physical input/action events, 22 distinct physically correlated ROM roots, checkpoint-bounded direction and button edges, and a separate Akuma five-input sequence check. A checkpoint's first row is a baseline rather than an invented press. Input-to-ROM lag zero is valid.
- dev/dhc/tools/DISASSEMBLE_SUPER_MOVES.py audits every live observed 2E root against actual private ROM pointers and reports catalog gaps separately from ROM mismatches. The result is never treated as a decoded input command.
- dev/dhc/tests/test_dhc_command_evidence_168.py is a synthetic, ROM-free regression for exact button order, same-frame inputs and ROM pointer arithmetic.
- Original recordings, native gameplay, Stable, warm objects, existing root launchers and DHC dispatcher remain unchanged.

## Evidence from the 29,827-frame recording

| Fighter | Action / script | Evidence / caution |
|---|---|---|
| Akuma 1E | 2E0C / 0EB986 | LP LP Right LK HP; exactly repeated twice; 68000 decoder pending |
| Ryu 12 | 2E08 / 0D3070 | QCF + LP/MP twice; known Shinkuu Hadoken baseline |
| Ryu 12 | 2E00 / 0D2CFC | kick-pair twice; known Shinkuu Tatsumaki baseline |
| Ryu 12 | 2E06 / 0D2E7C | input candidate at movie 22034 just before 2E00 at 22038; catalog gap, NOT a new verified super |
| Wolverine 00 | 2E00 / 0A37DE | same-frame 3P at movie 27126; valid input edge, move identity still pending |
| Cyclops 02 | 2E00 / 0A992E | 3 repeated QCF + LP/MP associations; named super still pending |

Akuma sequence 1: movie frames 2262 LP, 2271 LP, 2281 Right, 2289 LK, 2300 HP; ROM root 1E/2E0C/0EB986 at 2305.

Akuma sequence 2: movie frames 2530 LP, 2539 LP, 2550 Right, 2559 LK, 2569 HP; same ROM root at 2570.

Input-correlated physical observations: 43/43; 22 distinct associated roots. Other ROM observations without recent physical input: 107. Catalog gaps: Sabretooth 0A/2E16/0C0E86 and Ryu 12/2E06/0D2E7C. Akuma Tenma Gou Zankuu remains EXCLUDED. Magneto, M. Bison, Storm, and Zangief still need observed action evidence.

## Commands to run locally

On feature/dhc-mvc2-stage87, from F:\xmvsf:

    py -3 dev\dhc\tests\test_dhc_command_evidence_168.py
    py -3 dev\dhc\tools\VERIFY_DHC_ROM_ACTIONS.py test\87.167 --evidence-out test\87.167\decomp

When the private native data and decrypted opcodes exist under local_rom_dump:

    py -3 dev\dhc\tools\DISASSEMBLE_SUPER_MOVES.py --rom local_rom_dump\xmvsfu_program_data_host_swapped.bin --opcodes local_rom_dump\xmvsfu_decrypted_opcodes_host_swapped.bin --observations test\87.167\rom_super_observed.tsv --out test\87.167\decomp\command_68000_audit.json --pc 01038E --pc 013B8E --pc 00C4E6 --pc 011B00 --count 48

The local Capstone M68K package is optional. If unavailable, instruction decoding reports that accurately; no command handler is fabricated. The original supplied reverse-engineering JSON has no decoded 68000 instructions. The private opcode and data files must never be committed.

## Next evidence required

Actual 68000 opcode windows and control-flow/input-read xrefs are required to distinguish action-script selection from the native motion recognizer. Only then can fighter-specific commands be certified for DHC. Do not promote script roots into gameplay mappings based on temporal input proximity alone.
