# XMVSF Stage 87.170 — Native 68000 command matcher research

**Read-only independent analysis, performed on the original user-supplied decrypted opcodes and program-data ROM files. No game engine, build launchers, recordings, or Stable files were modified.**

## Verified findings

- The original actor ID pointer index has **14** valid native index tables; code regions for all 17 playable fighter IDs in the target roster were located.
- Identified **124** exact native instructions sequences of `LEA descriptor(PC),A2; LEA history(A6),A3; JSR shared matcher` across **five** different matcher entry points. This scan covers this syntactic call shape; other calling conventions may exist.
- **112** call `013C30` and have a consistent contiguous **3- or 5-token canonical record**. **94** have first record word `0003`; **18** have `0005`. The record consists of four header words, 3/5 raw tokens, and one final mask word, respectively 16 or 20 bytes.
- The remaining **12** call alternate matcher routines: `013F76` (1), `014096` (7), `014392` (2), `01458E` (2). These have special descriptor shapes, including first word `0028`, which is **not** a 40-step motion. We deliberately do NOT use the canonical layout for these 12.
- All 112 canonical records have zero at word three (offset +6); all are assigned to a known fighter-code section. Common final mask words are `0070` (62), `0700` (41), and `0770` (3); others occur less often. These are raw ROM masks, **not** yet certified as punch/kick labels.
- The matcher at `013C30` directly compares the low byte of descriptor word at `+4` with the actor's native state byte `$D6(A6)`, under `$03(A6)` / `$029B(A6)` gates. It uses the actor-relative history byte `(A3)` to index the descriptor payload after the eight-byte header, and reads the descriptor's first word to select a handler branch. This proves a descriptor → native-state / history matching relationship, not an executable DHC command.

## Two exact entry examples

| Fighter | Native call PC | Matcher | Original data descriptor PC | Actor history | Data words (host word-swapped program data, interpreted as 68000 words) |
|---|---|---|---|---|---|
| Ryu (`12`) | `03298A` | `013C30` | `0344C6` | `+0180` | `0003 1000 0000 0000 0004 0006 0002 0070` |
| Akuma (`1E`) | `0412CA` | `013C30` | `0437E0` | `+01B0` | `0003 1000 000C 0000 0002 0004 0006 0770` |

Akuma's captured Shun Goku Satsu reaches native `2E0C / 0EB986`, but **this alone does not identify Akuma's descriptor at `0437E0` as its Shun Goku Satsu recognizer**. The action `2E0C` source at `042C54` and its predecessor input-history branches must still be linked through actual 68000 control flow. Do not equate equal-looking values in different state fields.

## Reproduce

    python test_dhc_command_descriptors_170.py
    python ANALYZE_68000_COMMAND_DESCRIPTORS.py --opcodes xmvsfu_decrypted_opcodes_host_swapped.bin --rom xmvsfu_program_data_host_swapped.bin --out output_dir

The analyzer never exports the private ROM/opcode bytes. It emits the full structured JSON plus a 112-row CSV. It rejects unexpected canonical record types rather than silently guessing. The synthetic regression uses no ROM bytes.

## Cryptographic input checks

- Decrypted opcode dump SHA-256: `985d2a4285c8ded10bb8deaebaf31541d88fd8f690fecf402e0c6da8b0c7c0c8`
- Program-data ROM dump SHA-256: `f60a9000bc9a0693e2e1b0970d0045e15ade845fbe0f2ba4f6619f211e0fffe2`

## Further work required

- Decode the branches and data formats for the four alternate matcher routines; preserve their raw header words until then.
- Determine precise raw motion-token and input-mask meanings from the shared 68000 matcher handlers and input history fields `$C6/$C8`.
- Correlate recognizer branch acceptance and the actor's live state transition with each recorded physical command, keeping follow-on script transitions separate from independent moves.
- Only then upgrade specific move identities to `COMMAND_RECOGNIZER_VERIFIED` in the DHC registry. Akuma Tenma Gou Zankuu stays excluded; Stable and all gameplay are untouched.