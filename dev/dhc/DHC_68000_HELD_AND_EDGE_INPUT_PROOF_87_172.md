# XMVSF Stage 87.172 — Native input state: held vs. new press

**Executed on the original user-supplied ROM buffers.** The main outcome is the exact original input-word update path, independently cross-checked against the 87.170 command-descriptor inventory and the 87.171 matcher input reads.

## Verified actor-relative words (A6 base)

| Offset | Native role | Original instruction sequence |
| --- | --- | --- |
| `$00C0` | Raw current controller word | `01042C` reads `MOVE.W (A0),D0`, saves D0 at C0 |
| `$00C2` | Raw previous controller word | `01042E` reads previous C0 then stores it at C2 |
| `$00C4` | Raw new-press edges | `010442 NOT.W D1` then `010444 AND.W D1,D0`; store result at C4 |
| `$00C6` | Current, facing-relative held-input word | Copy from C0, optionally converted at `0104C2` |
| `$00C8` | Previous, facing-relative word | Copy from C2, optionally converted at `0104C2` |
| `$00CA` | Facing-relative newly pressed input word | Copy from C4, optionally converted at `0104C2` |

This exactly establishes **`C4 = C0 & (~C2)`**, within the 16-bit word. With actor `$00A3 != 0`, the original converter masks to `077C` and swaps the low two horizontal-direction bits: output `= (input & 0x077C) | ((input & 1) << 1) | ((input & 2) >> 1)`.

The update routine at `010402` is reached by a real `BSR.W` at `00C31A`; both update paths call the same converter at `0104C2` (all six branch displacements verified). When facing is not flipped, the words are copied unchanged. Note that a held direction/button stays present in C6 across frames, but disappears from CA once it is no longer a new edge. A release by itself does **not** set CA.

## Proven native matcher consumption

| Input checker | Reads | Meaning |
| --- | --- | --- |
| `013C30` canonical matcher | `C6` | Current held direction/input word; at `013CE0`, masked `000F` for direction |
| `013F76` | `CA` | New-press edges masked by descriptor |
| `014096` | `C6` | Held input masked by descriptor |
| `014392` | `CA` | New-press edges, ANDI `077F`, then compare |
| `01458E` | `C6` | Held input, four-way comparison |

The **Ryu handler at `03298A`** and **Akuma handler at `0412CA`** each call `013C30` and immediately use a `BEQ` to skip further action on a zero matcher result. This shows that the matcher return influences actor command acceptance, not merely animation display. It does **not** yet prove which specific *named* super follows either branch or how all descriptor flags work.

## Directional bit corroboration

Ryu's two original three-token motion families from the 87.170 descriptor data are `0004 0006 0002` (quarter-circle forward) and `0004 0005 0001` (quarter-circle backward). Together with the facing swap, these provide strong evidence that, in the normalized directional nibble, `0004=down`, `0002=forward`, and `0001=back`. `0008` is the remaining directional bit, likely up, but this report does not assert a separate live input-to-RAM proof for it.

Descriptor final masks `0070` and `0700` are consistent with punch and kick groups respectively, corroborated by the earlier known Ryu QCF+PP and QCB+KK captures. **Individual button bits remain unassigned here**, and a descriptor matching those masks must not automatically be classified as a super move.

## Reproducibility

The original private buffers were read in memory only. No ROM bytes are included in this ZIP or GitHub. The read-only analyzer validates **16 native instruction site signatures**, six branch destinations, and the exact descriptor shape of Ryu's canonical forward/back motion. The independent ROM-free test checks 2,048 facing-transform cases and 16,384 held/edge input combinations.

```bat
py -3 dev\dhc\tests\test_dhc_input_words_172.py
py -3 dev\dhc\tools\ANALYZE_68000_INPUT_WORDS.py --opcodes local_rom_dump\xmvsfu_decrypted_opcodes_host_swapped.bin --rom local_rom_dump\xmvsfu_program_data_host_swapped.bin --canonical-json test\87.167\decomp\68000_command_descriptor_audit.json --out test\87.167\decomp\68000_held_vs_edge_audit.json
```

The `--canonical-json` parameter accepts the prior 87.170 `68000_command_descriptor_audit.json` (this is independent derived evidence, not the original ROM). The packaged 87.172 ZIP contains the analyzer, test, derived audit JSON, and this README; no original ROM or original capture is modified.

## Next verified engineering target

Use `C6` and `CA` as separate signals in the command recognizer tracing. Recover the actual return/branch paths, history timers and flags for the Ryu and Akuma matcher descriptors and link them to the `2E` setter code. Do **not** enable other fighters or the excluded Akuma Tenma Gou Zankuu. No change to TEST 87.167, Stable, the DHC state machine or permanent launcher names has been made.