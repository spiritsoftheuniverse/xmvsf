# XMVSF Stage 87.169 — Original 68000 actor/selector provenance

**Independent ROM analysis completed:** user-supplied 1,048,576-byte decrypted-opcode image, 3,670,016-byte program-data image and the sealed 29,827-frame eight-session capture. The earlier replay completed 68/68 checkpoints and matched 150/150 native action events. This audit neither changes nor enables DHC gameplay.

## Verified facts

- Located **14 independent native fighter-ID-indexed code tables**. Example original 68000 instructions at `00CC52`: `MOVE.W $52(A6),D0; ADD.W D0,D0; MOVEA.L (PC,D0.W),A0; JMP (A0)`. The corresponding table begins at **`00CC60`** and maps native fighter IDs to code handler addresses.
- Recovered **83** exact original `MOVEQ #$2E,D0; MOVEQ #selector,D1; JSR/JMP $01038E` patterns in the opcode image; **75** lie in character-specific sections established using the **earliest pointer across all fourteen** tables. Eight are outside those sections and remain unassigned.
- Independently checked all **58 distinct** live actor/action/selector/script roots from the user's **150** replay events against the *data* ROM. **58/58** resolve to the exact native script pointers. Two legitimate script words have high bit set: Ryu `12/2E06/0D2E7C` has `8004`; Sabretooth `0A/2E16/0C0E86` has `8008`. Do not incorrectly reject them as invalid durations.
- **58/58** distinct live action selectors are now represented by a specific source in the 68000 image: **49** direct consecutive D0/D1 setters, **4** conditional D1 choices, **1** preloaded D1, and **4** PC-indexed Cammy selector-table entries. This is *code selector provenance*, not proof of the original motion-input acceptance conditions or actual execution of the particular static PC on each replay event.

## Newly proven character boundaries

| Native fighter | First indexed handler PC |
|---|---|
| Wolverine `00` | `01F3E8` |
| Cyclops `02` | `0224F2` |
| Storm `04` | `024B62` |
| Rogue `06` | `026508` |
| Gambit `08` | `029884` |
| Sabretooth `0A` | `02B618` |
| Juggernaut `0C` | `02DB40` |
| Magneto `0E` | `02FAAE` |
| Ryu `12` | `032920` |
| Ken `14` | `034538` |
| Chun-Li `16` | `0365C2` |
| Dhalsim `18` | `038820` |
| Zangief `1A` | `03AC20` |
| M. Bison `1C` | `03E19A` |
| Akuma `1E` | `041242` |
| Charlie `20` | `043854` |
| Cammy `22` | `0453AC` |

`10` contains fallback/placeholder pointers and is not assumed to be a playable target. `24` aliases another handler. Using only the first apparent per-fighter handler incorrectly assigned Juggernaut's two source PCs to Sabretooth; the earliest of fourteen valid dispatch table pointers fixes that attribution.

## Super code-to-script mappings

| Fighter/move evidence | Action/selector | Exact setter PC | Native script |
|---|---|---|---|
| Ryu Shinkuu Hadoken (prior verified move) | `2E08` | `033E0C`, D1 conditional on `$A4(A6)` | `0D3070` |
| Ryu Shinkuu Tatsumaki (prior verified move) | `2E00` | `033F80` | `0D2CFC` |
| Ken Shoryureppa (previous Ryu→Ken DHC evidence) | `2E00` | `035A4E` | `0D6722` |
| Ken Shinryuken (previous DHC evidence) | `2E02` | `035CB0` | `0D6976` |
| Akuma's recorded Shun Goku Satsu association | `2E0C` | `042C54` → native dispatcher at `042C58` | `0EB986` |
| Juggernaut's physically associated root | `2E00` | `02E82C` | `0C473E` |

Akuma's `LP→LP→Forward→LK→HP` input sequence is recorded twice and reaches selector `0C`; the complete original recognizer branch graph remains to be proved. Akuma **Tenma Gou Zankuu is explicitly excluded** and must not be promoted to executable DHC.

### Original non-immediate selector paths

- Wolverine: `02123C` chooses `16` versus `08` according to `$259(A6)`; `02131E` chooses `0C` versus `0A` according to `$50(A6)`.
- Ryu: `033E0C` chooses `08` vs `0C`; `033EC2` chooses `0A` vs `0E`, both via the state comparison `CMPI.B #04,$A4(A6)`.
- Dhalsim: `039D32` loads `D1=04` **before** `D0=2E`; the dispatcher call is at `039D36`.
- Cammy: `047404` loads D1 from an original PC-indexed word table at **`047410`** before dispatch at `04740A`; this explains captured `06`, `08`, `10`, `12` entries without imaginary standalone setter instructions.

## Input recognizer branch investigation

The original character handlers **Ryu `032920`** and **Akuma `041242`** invoke shared tests `013B48`, `013AB6`, `013A2C`, `013B9E` and fighter-local subroutines. Akuma `0412C2` and Ryu `032980` prepare a PC-relative command descriptor and an actor-relative input-history pointer, then invoke shared routine **`013C30`**. This identifies an actual native matching pathway to study, not merely an animation script. The descriptor encoding and which history/branch condition selects each named super remain to be independently decoded. Distinguish the decrypted *opcode-fetch* image from the host-word-swapped *program-data* image when inspecting the associated tables.

## Evidence and safety

Local work generated a read-only reproducible `ANALYZE_68000_FIGHTER_SUPERS.py` and small JSON/CSV evidence files, packaged as `XMVSF_87_169_Original_68000_Super_Audit.zip`. The user owns the unmodified private original ROM images; **none are committed or included in the evidence ZIP**. Current feature branch only receives this research note, not guessed gameplay move mappings, new BAT launchers, ROM binaries, savestates or changes to Stable.

The next code gate is decoding native input-history descriptor layout and comparing exact recognizer decisions to the 43 recorded physical input associations. Source-script pointer equivalence alone is **not** proof that a DHC command can be invoked from neutral, tag-in or super-cancel state.
