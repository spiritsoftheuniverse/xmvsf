# TEST 87.408 — Stage-address screenshots for original and custom arenas

Branch `feature/native-stage-capture-catalog-87-408`; extends TEST 87.407
without changing existing stage selection, combat, music or renderer routing.

## Capture workflow

1. Build locally using `BUILD_TEST.bat`, then start `RUN_TEST.bat`.
2. Play any ORIGINAL stage (Arcade works; original stage mapping is not
   yet required), or select one of the existing CUSTOM stages in
   Training/Versus.
3. Once the original level appears and the game is in active combat,
   press **F6** once. Use **0**, then F6 for a sprite-free Boomer
   background as in TEST 87.404.
4. Repeat for each original. Each press creates a distinct native
   `384x224` PNG in `F:\xmvsf\screenshots\` plus one append-only row
   in `F:\xmvsf\screenshots\stage_catalog.tsv`.
5. Send the PNGs and TSV when it's time to identify names, choose
   thumbnails and build the full stage-selection menu.

The manifest records:
- the actual PNG path and game frame,
- CPS2 native selector **address FF5082** and its observed value on the
  last emulated frame,
- the first observed non-FF selector value in this stage selection,
- CPS2 native stage **address FF5113** and its observed stage ID,
- native game phase at **FF5000** and a verified-combat flag,
- host menu choice/resolved custom renderer, stage generation,
  stage type (NATIVE / CUSTOM), and name.

A `verified_combat=1` row only means the native status is 08 and
FF5113 is nonzero when captured. It does **not** mean an unknown
native stage has been matched to a human-readable name or that the
ROM's lookup-table address has been found.

Native stage names are intentionally `UNNAMED` until the captured
artwork has been reviewed. Host custom stages get their known labels
(Wily Factory, Final Destination, Guile Stage, Sigma Stage,
Akuma Stage, Ryu Stage, Boomer Kuwanger).

Existing F6 one-shot save, filenames, scaling, rendering order and
the Wily/Boomer layer controls are unchanged. The new feature adds
only 3 read-only CPS2 byte observations per emulated frame, then
writes the catalog only after a successful F6 PNG save. No automatic
captures, in-game stage forcing, guessed ROM offsets, or edits to
original ROM assets.

## Outputs

- `F:\xmvsf\screenshots\XMVSF_native_*.png` (unchanged, unique)
- `F:\xmvsf\screenshots\stage_catalog.tsv` (append-only)

If screenshot save succeeds but catalog append fails, the game reports
that specifically and logs `XM87.408 STAGE_CATALOG_FAILED`.
If F6 occurs in a menu, stage assignment may be uncommitted; the row
retains raw data and `verified_combat=0`.

The catalog is a **capture ledger**, not a permanent validated map.
It may contain multiple screenshots of one stage, which is useful
for choosing your eventual thumbnail.

## Build and source checks

```powershell
git fetch origin
git switch feature/native-stage-capture-catalog-87-408
git pull --ff-only
.\BUILD_TEST.bat
.\RUN_TEST.bat
```

The existing SDL GIF and Boomer motion checks are retained.
`BUILD_TEST.bat` also performs a cheap C++ syntax check on the new
catalog header in `test\build_logs\STAGE_CATALOG_408.log`.
The native linker must contain the current
`XM87.408 STAGE_CATALOG_V1` marker, plus existing HUD/F6/Boomer
proofs, before publishing `XMVSF_Test_87_408.exe`.

Stable and cached native objects are not cleared by this feature.
The implementation has source checks, but a local Windows native
build and in-game screenshot are still needed to validate its output.
