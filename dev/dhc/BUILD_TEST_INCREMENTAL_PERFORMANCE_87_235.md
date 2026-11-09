# TEST 87.235 — Incremental build performance

Scope: current branch only, native local Windows/MSYS2. The GitHub repository is source control only. Neither GitHub Actions nor Stable builds are used.

## Measured issue (the committed BUILD_TEST.log)

- 2026-11-08 17:43:54 to 17:50:55: 4 of 1,174 object files rebuilt, roughly seven minutes.
- 2026-11-08 18:10:48 to 18:17:02: **zero native objects rebuilt**, yet 6m14s elapsed.
- 2026-11-08 18:36:49 to 18:43:07: **zero native objects rebuilt**, yet 6m18s elapsed; identical TEST EXE retained.

This establishes that C++ recompilation alone does not explain the long waits. Previous builds always executed the full ROM-free suite, analysis tool, and two-pass FBNeo Make traversal, then checksums and object inventory. A Perl/driverlist warning appears even on no-op builds. Precisely which phase costs most requires the new phase timers on the local machine.

## New safe paths

1. **Verified no-op shortcut:** when the previous successful native TEST has the current Git HEAD, the compiler/settings fingerprint is unchanged, the tracked tree has no edits except the append-only build log, there are no untracked native sources, and both the native FBNeo EXE and TEST EXE still match the previously recorded SHA-256, skip Make and the already-passed regression suite. This does **not** apply to source changes, a failed or missing previous TEST, a changed compiler, new native files or a corrupted executable. No files are touched.
2. **Compact menu-only incremental build:** when only approved boot/menu, SDL color/glyph, authored art/audio, test scripts or documentation changed, reuse the exact ordered object list from the previous linker's `obj/xm8635_link.map`. Supply the cached list as Make's `allobj` override, with only the four relevant SDL `.d` dependency files, and disable irrelevant `autdrv`/`autdep`/`drvdep` scanning. GNU Make still recompiles affected SDL objects and relinks normally, but does **not** traverse the full 1,174-object FBNeo driver graph. The manifest is accepted only when the map is recent, complete, all referenced objects exist, and required SDL dependencies exist. No local native objects or Stable files are deleted or manually touched. If the manifest is unavailable, use the previous warm single-pass Make; engine/driver/generated rule changes or invalid signatures still use original two-pass Make. The normal regression suite and TEST-only publish remain in place.
3. **Per-phase wall-clock output:** the local log now shows regression-suite time, native Make time, time to publish, and total duration. The existing object inventory still states precisely how many of the 1,174 objects were rebuilt. `BUILD_TEST.bat` checks `bash -n BUILD_TEST.sh` before running anything.

## Controls

- Default: allow the no-op shortcut and warm menu single-pass.
- `XMVSF_FORCE_NATIVE=1`: bypass the no-op shortcut; run the normal tests/build decision.
- `XMVSF_WARM_MENU_SINGLE_PASS=0`: force the original two-pass native Make even on menu edits.
- `XMVSF_COMPACT_MENU_MAKE=0`: disable the new compact linker-map override while retaining the previous warm-menu single-pass fallback.
- `XMVSF_CACHE_SMOKE=1`: also run the synthetic 107-object linker-map validation, including missing-object and missing-`.d` rejection.
- The cache-smoke switch also runs the existing dependency smoke test and bypasses the no-op shortcut.

## Verification status

Changes are source-reviewed, not executed on the user's actual MSYS2 native cache. Do not assume a particular speedup until a local `BUILD_TEST.bat` reports its **BUILD PHASE** timings. The synthetic compact-map fixture was tested independently in a Linux bash environment; the full Windows FBNeo link is not verified in this session. If a warm single-pass changed-menu build fails or misses dependencies, set `XMVSF_WARM_MENU_SINGLE_PASS=0` for that build and provide the build log; the original full Make path and 1,174-object cache remain intact. No cleans or forced object rebuilds were introduced.
