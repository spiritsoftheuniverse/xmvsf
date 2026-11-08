# TEST 87.224 — Charlie air heavy infinite-loop diagnosis and 150% flying-screen trial

## Evidence: supplied 87.223.zip

`observer/storm_like_flight_events.tsv` contains 4,069 native hit/air-attack records. Charlie is native fighter ID `0020`, action bank `0028`.

- Charlie `1804` (air HP) underwent **23** Storm-index flying-screen conversions.
- Charlie `1A04` (super-jump HK with confirmed HK button-edge tag `0x20`) underwent **2** conversions.
- **14** additional Charlie airborne heavy hit attempts were rejected by `STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING` (8 during 1804 and 6 during 1A04).
- At the actual pre-host impact, Charlie's launches predominantly report victim `1C16/0220`, the normal soft air-hit pair. These observed lines contain no independently confirmed Charlie-specific 68000 `1C18/0230` special knockdown writer.
- The TEST 87.223 host code *itself* sets `1C18/0230` on qualifying hits; that is not evidence of an original Charlie-only 68000 special case.
- `observer/storm_fs_68000_opcode_candidates.tsv` lists STATIC immediate/A6 MOVE candidates from the decrypted code. They are **not an executed-PC trace** and cannot prove a Charlie-specific instruction branch. The DHC-specific `native_pc_trace_test.tsv` provided no Charlie hit writes.

The repeated conversions can be seen in the log across successive aerial attempts; no inference about user input precision or full combo validity is made beyond these observed transitions.

## Implemented fallback

Because the supplied 87.223 runtime evidence does not establish an original-ROM Charlie special knockdown override, TEST 87.224 leaves all original 68000 logic, 87.223 hit gates, flight index, OTG rules, Storm velocity reference and one-flight-until-landing latch in place.

For **Charlie only** (id `0020`, bank `0028`) and only confirmed action `1804` and `1A04` conversions:

- Multiply the Storm-derived horizontal knockback magnitude by **3/2**, in exact 16.16 fixed-point. Default `0x0010AA00` → `0x0018FF00` (about 16.664 → 24.996 px/frame), retaining native left/right direction and capping at `0x00300000`.
- Multiply the selected downward motion by **3/2**, default `0x0000B000` → `0x00010800` (0.6875 → 1.03125 px/frame), capped at `0x000C0000`.
- Other actors, Charlie's other attacks, the first hit of an aerial combo, native Cyclops special slam, Drill Claw, and the one-boost-until-landing latch remain unchanged.
- Existing actual-native steep-severe reactions are preserved: do not forcibly clear a rumored special knockdown bit without an executed-instruction proof.

The TEST 87.224 event label for this experiment is `CHARLIE_AIR_HEAVY_FS_150PCT_NATIVE_INDEX`.

## Bounded executed Motorola 68000 trace

Added `xmvsf_stage87_224_charlie_pc.h`, hooked between the 87.182 DHC-PC begin/end and the real 68000 CPU frame.

The Windows runner enables `XM87224_CHARLIE_M68K_TRACE=1`; `test/87.224/observer/charlie_native_68000_pc_writes.tsv` captures exact *executed* PCs and A6 values whenever Charlie's victim undergoes an original-ROM write to native reaction `+06`, step `+08`, action `+60`, flight index `+E0`, velocity `+14/+1C`, air flag `+AA` or related `+82/+83` flags. This happens **before** the 87.223 host post-CPU flight conversion.

To keep the game playable, trace collection is limited to 32 original CPU frames each for `1804` and `1A04` and at most 96 actual state-change rows per frame, with truncation noted. An existing 87.182 callback is forwarded and restored; no guest RAM writes or CPU timing adjustments. This test is diagnostic, not a claimed verified Charlie special-case implementation.

## Native Windows tests

Build with existing `BUILD_TEST.bat`, then run `RUN_TEST.bat` on branch `feature/charlie-air-fs-87-224`. The source includes a ROM-free `test_charlie_air_fs_224.cpp` test covering both Charlie moves, 1.5x fixed-point scaling, other characters, other moves, and preservation of original steep-severe native reaction. Build target: `XMVSF_Test_87_224.exe`. Stable and older TEST binaries remain untouched. No GitHub Actions.

Observe in Tournament whether increased separation actually interrupts the repeat. Compare `storm_like_flight_events.tsv` (host conversion) with `charlie_native_68000_pc_writes.tsv` (true 68k PC and native victim state changes). The new native Windows executable and real combo outcomes have **not yet been verified**.
