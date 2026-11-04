# DHC roster capture audit — 87.167(2), 4 November 2026

**Status: SEALED RECORDING / INPUT-BYTE CORRELATION PASS. RUNTIME PLAYBACK OF THIS NEW CAPTURE NOT YET VERIFIED. 68000 COMMAND RECOGNIZERS NOT YET DECODED.**

Source: user-supplied `87.167(2).zip`, SHA-256 `acfe1469894179c27d40eac7eaa460ab1fb4b0076406b35cf9c9a94b5ee8952a`. Do not overwrite it. For baseline replay progress, refer to `REPLAY_87_167_DATA_VERIFICATION.md` (the earlier, shorter 6,395-frame recording).

## Data integrity and findings

- `XM8764V2 1` sealed with `FINAL 29827 8 68 F9_STOP`; 82 host events and 68 committed `DONE` records; ZIP CRC valid.
- All **68 native `.fs` checkpoints** have FBNeo state signatures. There are 56 distinct state hashes (reusing an identical snapshot is not a corruption indicator).
- All 68 CPS2 input streams use `xmvsfu`, 29 identical native input slots, valid per-segment sequential frame counters and exactly matching payload sizes.
- **150** observed ROM action-`2E` transitions represent **58** distinct native script roots and **12** distinct fighter IDs.
- **43/43** `PHYSICAL_CANDIDATE` controller chords match the actual captured CPS2 input bytes. **One** of these, Wolverine 3P at movie frame **27126**, is a valid *same-frame* action/input event, with lag = 0; the verifier must permit it.
- The other **107** action observations have no recent physical attack chord; never call them standalone supers just because an animation or 2E script changed.
- **22** distinct roots have physically associated button input and are available for command-recognizer disassembly.
- **Two** observed roots match the native ROM resolver at runtime but are omitted from both the exported ROM catalog and reverse-engineering JSON: Sabretooth `0A/2E16/0C0E86` and Ryu `12/2E06/0D2E7C`. These are **catalog gaps**, NOT corrupt input and NOT verified command mappings. Revisit selector enumeration bounds or JSON export, never fill the gap by guessing.
- **Session 8** has 2,058 recorded frames and six checkpoints, but no observed action-2E entry. All eight sessions were recorded; only sessions 1–7 have super-action observations.
- **No replay_results for this longer recording** are embedded in the archive. Require a fresh `RUN_REPLAY.bat` playback showing `PLAY_END complete_frames=29827`, 68 `CHECKPOINT_LOADED` records, and identical 150-event ROM trace before declaring the multi-roster runtime pipeline validated.

## Target roster and actual evidence

ID assignment is cross-checked against known Ryu=12 and Ken=14 native identifiers. This table lists *input-associated script roots*, not verified named moves. `P` is physical button observations.

| Fighter | Native ID | Root candidates with physically matched input | Target names requested |
|---|---|---|---|
| Akuma | 1E | 2E00/0EB42E (2P); 2E02/0EB692 (2P); 2E06/0EB87A (2P); 2E0C/0EB986 (2P) | Messatsu Gou Hadou; Messatsu Gou Shouryuu; Shun Goku Satsu. **EXCLUDE Tenma Gou Zankuu** |
| Cammy | 22 | 2E18/0F3C92 (1P) | Spin Drive Smasher; Killer Bee Assault |
| Cyclops | 02 | 2E00/0A992E (3P) | Mega Optic Blast; Super Optic Blast (directional) |
| Charlie | 20 | 2E00/0EF9B4 (3P) | Sonic Break; Crossfire Blitz; Somersault Justice |
| Chun-Li | 16 | 2E00/0DA7E8 (2P); 2E0A/0DB1C4 (2P); 2E16/0DB8F8 (2P) | Senretsukyaku; Hazan Tenshokyaku |
| Dhalsim | 18 | 2E04/0E0A9E (2P); 2E06/0E0DBE (2P) | Yoga Inferno (flame control); Yoga Strike |
| Gambit | 08 | 2E00/0BC1D2 (2P) | Input/follow-up descriptions supplied, but move names did not survive the pasted formatting |
| Juggernaut | 0C | 2E00/0C473E (2P) | Juggernaut Headcrush |
| Magneto | 0E | **No observed 2E input candidate** | Magnetic Tempest; Magnetic Shockwave |
| M. Bison | 1C | **No observed 2E input candidate** | Psycho Crusher (variants); Knee Press Nightmare |
| Rogue | 06 | 2E00/0B6774 (1P) | Goodnight Sugar |
| Ryu | 12 | 2E00/0D2CFC (2P); 2E06/0D2E7C (1P; catalog gap); 2E08/0D3070 (2P) | Shinkuu Hadoken (2E08, prior verified); Shinkuu Tatsumaki (2E00, prior verified) |
| Sabretooth | 0A | 2E04/0C0E1E (2P); 2E12/0C0012 (4P) | Berserker Claw X; Weapon X Dash; Heavy Armed Birdie |
| Storm | 04 | **No observed 2E input candidate** | Lightning Storm; Hail Storm |
| Wolverine | 00 | 2E00/0A37DE (1P, same-frame 3P); 2E16/0A364A (1P) | Berserker Barrage X; Weapon X |
| Zangief | 1A | **No observed 2E input candidate** | Final Atomic Buster (close) |

**Important:** Target move names are user requirements, not independent proof assigning names to those roots. Some entries may be follow-on scripts or non-target moves. Inputs and commands absent from the pasted message must not be fabricated. Additional script roots with no physical input are retained separately in the ROM observation log.

## High-value native input sequences

**Akuma Shun Goku Satsu**: around movie frames 2262, 2271, 2281, 2289, 2300 (repeated at 2530, 2539, 2550, 2559, 2569), the native recording contains **LP, LP, forward, LK, HP**, and the action root `1E/2E0C/0EB986` is observed at frames 2305 and 2570. This is exceptionally strong command-sequence evidence but is not yet a decoded 68000 call graph. Keep `Tenma Gou Zankuu` excluded regardless of other Akuma script activity.

**Ryu known ground truth** from earlier native tests: `12/2E08/0D3070` for Shinkuu Hadoken; `12/2E00/0D2CFC` for Shinkuu Tatsumaki. New root `12/2E06/0D2E7C` remains **unmapped**, even though it appeared during the kick sequence.

## Cast-wide certification rules

1. Treat successful original capture integrity, playback trace equality, and exact 68000 move-command proof as **three different certifications**. Only the first has passed for 29,827-frame capture.
2. Permanent root `BUILD_TEST.bat`, `RUN_TEST.bat`, and `RUN_REPLAY.bat` names stay unchanged. Existing warm build, Stable runtime, recording files, and native checkpoints are authoritative and untouched.
3. `VERIFY_DHC_ROM_ACTIONS.py` must validate native input slots and allow zero-frame lag, and report incomplete offline enumeration without falsely labeling it data corruption. A new ROM script is never automatically promoted to an executable DHC command.
4. Confirm repeatable paired move executions and compare the entire playback `rom_super_observed.tsv` before expanding the gameplay dispatcher.
5. Carry explicit per-move tags: `TARGET`, `EXCLUDED`, `CAPTURED_INPUT_CANDIDATE`, `ROM_ROOT_RESOLVED`, `COMMAND_RECOGNIZER_VERIFIED`, `DHC_EXECUTION_VERIFIED`. These are not interchangeable.
