# TEST 87.223 — native Storm flying screen across Tournament, voice mute in both modes

## Goal

Try genuine Storm-like flying-screen behavior as the default Tournament *connected airborne heavy finisher*, rather than the forced Cyclops Down+MP-style steep knockout. Test its feel and fairness in real gameplay **before making any OTG policy changes**. Classic retains original ROM fighting rules. TEST 87.222 menu transition/dash corrections remain intact.

## Native evidence and explicit uncertainty

The existing original-rom 87.86 impact signature identifies **Storm super-jump HP/HK family** in bank `0022`, action `1A04`, animation pointer `5DAA` entering native victim `1C18` with hit-reaction/flight index `0230` and approximately **16.664 horizontal fixed-point speed**. Earlier 87.80 Storm Air HK→Air HP velocity tracing yielded `+0x14 = 0x0010AA00` and `+0x1C = 0x0000B000` (0.6875 downward). This evidence supports a **flight trial**; it does not independently prove every original 68000 instruction behind the screen transfer, gravity, or OTG behavior. The old Tournament host writes `+0x14 ≈ ±0x00066200` and `+0x1C=0x00132A00` and forces the Cyclops-like `1C18/0230` pair. That dive is much steeper than Storm's recorded launch.

TEST 87.223 now supplies the Storm native **`+0x60` action and `+0xE0` reaction index** together with Storm X/Y flight momentum on the previously validated hit-onset/substate-2 path. If a current-session verified Storm HP supplies its precise velocity, use that bounded reading; otherwise use the above archived Storm signature. Avoid guessed sprite/animation pointers, replacing any 68000 program code, or copying other character-specific actor controllers. No new OTG gate, bypass, permission or punishment: evaluate the behavior in the emulator.

The one-boost-per-airborne-sequence latch remains. It rearms on the ROM ground-contact condition or character replacement; not by a new hit. Earlier restrictions stay: no automatic FS on first unconnected aerial hit, no medium conversion, preserve Storm MK, Drill Claw, Cyclops Down+MP and Sabretooth/Cammy HP restrictions. The game can still choose native behavior for attacks that don't satisfy the existing confirmed follow-up logic.

## 68000 tracing included

`XM87223Flight::Tracker` observes **post-native-68000, pre-host-conversion** Storm victim actor state in both rulesets and records action `+0x60`, reaction index `+0xE0`, momentum `+0x14/+0x1C`, gravity `+0x20`, actor mode `+0xAA`, and additional gate bytes `+0x82/+0x83`. It tracks actual HP button edges across the native animation delay and distinguishes native severe flight from soft reactions. The optional decrypted-68000-opcode scan produces potential `MOVE.[B/W/L] #imm,(d16,A6)` writer addresses for those fields. **Static opcode candidates do not establish executed 68000 PCs.** That requires a future bounded executed-PC and write-before/after trace once real 87.223 gameplay identifies the exact event.

Files in `test/87.223/observer`:
- `storm_hp_native_fs.tsv` — actual Storm HP source/index/velocity samples
- `storm_fs_68000_opcode_candidates.tsv` — static source candidates for disassembly, not execution proof
- `storm_like_flight_events.tsv` — converted/held/blocked Tournament flight decisions, before/after victim action/flight index, X/Y velocities
- `storm_voice_all_modes.tsv` — muted QSound sample-6508 volume on channel 12, including delayed Z80 commands
- `dash_startup.tsv`, `crash_report.log` and `crash.dmp` if an exception occurs

## Sound

The QSound filter is not Tournament-gated; it only silences confirmed Storm sample `6508` in register `0x61` / volume `0x66` when Storm's targeted normal-attack actions are active or occurred within a bounded 12-frame audio dispatch delay. It retains standing HP voice and every other sample/channel. Classic and Tournament should now behave identically for Storm voice muting.

## Build and gameplay verification

Compile the branch with local `BUILD_TEST.bat` and run `XMVSF_Test_87_223.exe` using `RUN_TEST.bat`. Previous TEST executables and Stable are preserved; **no GitHub Actions**.

Try in Tournament: connected launch → medium/light air follow-up → heavy air finisher with Storm and several different characters; confirm screen transfer and whether an OTG follow-up remains possible. Then repeat an aerial follow-up without landing: the old anti-infinite latch should block a second synthetic boost. Finally, return to main menu and switch to Classic: check that Classic native flight and Storm's targeted voice suppression remain correct. There is no claim here that the full native Windows gameplay has already passed.
