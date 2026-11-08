# TEST 87.225 — fixed Storm velocity baseline before Charlie 1.5× FS

## Supplied runtime evidence: 87.224.zip

The 87.224 gameplay observer recorded **seven** Charlie flying-screen substitutions, all action `1804` (air HP). Every such substitution emitted horizontal `0x0018FF00`, i.e. the expected `0x0010AA00 × 1.5`. Six wrote downward `0x00010800` but one (frame **2540**) wrote `0x00021000`: the prior native downward velocity was `0x00016000`, and the 87.224 algorithm kept that *instead* of the Storm base, multiplying it by 1.5.

Four Charlie `1A04` super-jump HK connected attempts were blocked by `STORM_FLIGHT_ALREADY_USED_UNTIL_NATIVE_LANDING` after HP, so **none** of those hits exercised the special 1.5× release in this capture. They could still change victim reaction through original native 68000 collision logic. The one-boost latch is intentionally preserved.

No Storm-specific HP reference was learned in the 87.224 session (`storm_hp_native_fs.tsv` had only its header); the 87.224 horizontal value came from the archived Storm `87.80` reference. It was the *vertical carry-over*, not a learned reference, that explains the inconsistent launch vectors.

The 68000 native-PC trace found `PPC 00E54A` setting victim X velocity and `00E566` changing native reaction substate, with `A6=FF4400`. These are observed executed writer PCs, not proof of any special Charlie-specific permanent-knockdown flag. No speculative opcode edits are made.

## Corrected TEST logic

For Charlie native ID `0x0020`, action bank `0x0028`, and only the existing authorized FS release on `1804` and `1A04`:

- **Horizontal** = signed direction × `(0x0010AA00 * 3 / 2)` = ±`0x0018FF00` (~24.996 fixed-point units).
- **Vertical** = `(0x0000B000 * 3 / 2)` = `0x00010800` (1.03125).
- No use of the victim's preexisting downward velocity, and no dependence on the optional dynamic Storm motion sampler for Charlie.
- Native attack confirmation, Storm 1C18/0230 FS reaction/index, Classic/Tournament rules, OTG handling, one-shot-per-airborne sequence, and other character velocities remain as in 87.224.

The new launch decision is `CHARLIE_STORM_FIXED_BASE_XY_150PCT`, written to `test/87.225/observer/storm_like_flight_events.tsv`. `dev/dhc/tests/test_charlie_air_fs_224.cpp` is expanded to test the exact failure from frame 2540 and insensitivity to future learned Storm velocity updates.

Build locally with `BUILD_TEST.bat` and run `RUN_TEST.bat`. New target is `runtime/stable/XMVSF_Test_87_225.exe`. Stable and previous TEST executables are not overwritten. There are no GitHub Actions. Native Windows gameplay is not yet verified.

**Fairness observation:** in the 87.224 sample, after the HP-generated flight Charlie's next HK could still connect natively while the synthetic-boost latch blocked a second boost. If 87.225 still permits repeat attacks, identify whether the issue is recovery/air-followup availability rather than launch velocity; do not keep multiplying a velocity that is already applied exactly.
