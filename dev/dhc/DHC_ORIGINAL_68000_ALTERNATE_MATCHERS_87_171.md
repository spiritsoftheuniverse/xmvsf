# XMVSF Stage 87.171 — Original 68000 alternate command matchers

This audit was **run locally** against the two original native ROM buffers the user supplied. It is a read-only continuation of the Stage 87.170 report; the existing 29,827-frame, eight-session F9 replay and 150 identical native action events remain the reference capture. No new recording or game build was necessary.

## Verified result

All **12** noncanonical command-matcher call sites previously left unresolved by Stage 87.170 now have **bounded descriptor layouts**, exact native input-read opcode evidence, and original actor IDs. Together with the previous **112** canonical `013C30` descriptors this accounts for **124** observed call sites of these five call shapes.

| Matcher | Call sites | 68000 input word | Bounded descriptor layout | Observed fighters |
|---|---:|---|---|---|
| `013F76` | 1 | `$00CA(A6)` | 10 bytes; class `0004` | Chun-Li |
| `014096` | 7 | `$00C6(A6)` | 14 bytes; class `0028` | Gambit (2), Chun-Li (1), M. Bison (2), Charlie (2) |
| `014392` | 2 | `$00CA(A6)` | 16 bytes; class `0004` | Rogue, Akuma |
| `01458E` | 2 | `$00C6(A6)` | 10 bytes; class `0002` | Zangief (2) |

The four matcher routines share a verified actor guard that compares the descriptor word at `+4` against native `$D6(A6)`, conditioned by `$0003(A6)` and `$029B(A6)`. The decoder preserves class, flags, native-state, reserved, and payload words **without claiming the class word is a motion length**.

### Specific input-processing discoveries

- At `013FF2`, the `013F76` handler reads `MOVE.W $00CA(A6),D1` and applies `AND.W (A2),D1`.
- At `0140D2`, `014096` reads `MOVE.W $00C6(A6),D1` then applies `AND.W (A2),D1`. Its seven 14-byte records contain two raw history-mask words followed by a final raw input mask. Header `0028` must **not** be interpreted as forty motion steps.
- At `014426`, `014392` reads `MOVE.W $00CA(A6),D1`, applies `ANDI.W #$077F,D1`, then compares against `(A2)`. Its Rogue and Akuma records preserve three raw history words and a final mask.
- At `0145C8`, Zangief's `01458E` path reads `MOVE.W $00C6(A6),D0`, compares exact numeric values `0008`, `0002`, `0004`, `0001`, and assigns history class indices `0`, `1`, `2`, `3`. At `0145EE`–`0145F8` it advances `(A3)` and writes class/state bytes to `$0002(A3)` and `$0003(A3)`. This establishes a four-way input-state recognizer, **not which direction or named super each value represents**.

### Example original descriptor words

| Fighter | Matcher | Descriptor PC | Raw descriptor words |
|---|---|---|---|
| Chun-Li | `013F76` | `0385FE` | `0004 000A 000C 0000 0700` |
| Gambit | `014096` | `02B5DA` | `0028 1000 0008 0000 0004 0008 0700` |
| M. Bison | `014096` | `0410F0` | `0028 0000 0006 0000 0004 0008 0700` |
| Akuma | `014392` | `043830` | `0004 1000 0010 0000 0010 0010 0002 0100` |
| Zangief | `01458E` | `03E15C` | `0002 0000 0006 0000 0070` |

**Important CPU-bus qualification:** both decrypted opcode switch tables (at `0140C8` and `0145BA`) differ from the program-data image at the same offsets. The report includes *candidate* branch targets decoded from the opcode-fetch image, alongside the program-data words, and labels runtime memory-bus selection **UNVERIFIED**. This is not evidence of a broken ROM and must not be silently resolved from one buffer alone.

## Artifact files

- `ANALYZE_68000_ALTERNATE_MATCHERS.py` — reproducible read-only audit of the exact input opcodes and all four native formats; validates native guards, signature bytes, actor states and input masks. The user-owned raw ROM dumps stay local and are not embedded in this package.
- `test_dhc_alternate_matchers_171.py` — synthetic regression of record class, 10/14/16-byte lengths, zero reserved fields, truncation failures.
- `68000_alternate_matcher_audit.json` — twelve parsed records, exact input-read instructions and data/opcode switch-table source distinction.
- `68000_alternate_matchers.csv` — one row per original matcher call; all raw payload words retained.

## Proof boundary and next stage

This validates **native matcher data structures and code paths**, not full input acceptance branches. The per-bit meaning of `$00C6` versus `$00CA`, the effects of the record flags, and which particular motion reaches the known Ryu/Ken/Akuma super actions remain to be verified against the live command-history state. Do not auto-promote any of these descriptors to `COMMAND_RECOGNIZER_VERIFIED`, and do not add guessed DHC commands. Akuma's **Tenma Gou Zankuu remains excluded**.

Existing `BUILD_TEST.bat`, `RUN_TEST.bat`, `RUN_REPLAY.bat`, native gameplay, Stable, cached objects, F9 recordings and save states remain untouched. No raw program-data/decrypted-opcode binaries are included in this audit.