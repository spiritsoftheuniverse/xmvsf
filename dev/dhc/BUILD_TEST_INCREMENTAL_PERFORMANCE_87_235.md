# TEST 87.235 — Incremental build performance

Scope: current branch only, native local Windows/MSYS2. The GitHub repository is source control only. Neither GitHub Actions nor Stable builds are used.

## Measured issue (the committed BUILD_TEST.log)

- 2026-11-08 17:43:54 to 17:50:55: 4 of 1,174 object files rebuilt, roughly seven minutes.
- 2026-11-08 18:10:48 to 18:17:02: **zero native objects rebuilt**, yet 6m14s elapsed.
- 2026-11-08 18:36:49 to 18:43:07: **zero native objects rebuilt**, yet 6m18s elapsed; identical TEST EXE retained.

This establishes that C++ recompilation alone does not explain the long waits. Previous builds always executed the full ROM-free suite, analysis tool, and two-pass FBNeo Make traversal, then checksums and object inventory. A Perl/driverlist warning appears even on no-op builds. Precisely which phase costs most requires the new phase timers on the local machine.

## New safe paths

1. **Verified no-op shortcut:** when the previous successful native TEST has the current Git HEAD, the compiler/settings fingerprint is unchanged, the tracked tree has no edits except the append-only build log, there are no untracked native sources, and both the native FBNeo EXE and TEST EXE still match the previously recorded SHA-256, skip Make and the already-passed regression suite. This does **not** apply to source changes, a failed or missing previous TEST, a changed compiler, new native files or a corrupted executable. No files are touched.
2. **Warm menu-only incremental build:** if code changed but only approved boot menu, SDL glyph/color, authored audio/artwork, or local TEST launcher/test files changed, and the previous successful dependency cache is intact, use Make's **second pass directly**, with its existing `.d` dependencies. This avoids running the complete driver dependency traversal twice. Any engine/driver/generated rule change, incomplete cache or changed compiler falls back to normal two-pass Make. The optimized mode still recompiles changed native code, links if necessary, publishes only the TEST executable and runs the normal regression suite.
3. **Per-phase wall-clock output:** the local log now shows regression-suite time, native Make time, time to publish, and total duration. The existing object inventory still states precisely how many of the 1,174 objects were rebuilt. `BUILD_TEST.bat` checks `bash -n BUILD_TEST.sh` before running anything.

## Controls

- Default: allow the no-op shortcut and warm menu single-pass.
- `XMVSF_FORCE_NATIVE=1`: bypass the no-op shortcut; run the normal tests/build decision.
- `XMVSF_WARM_MENU_SINGLE_PASS=0`: force the original two-pass native Make even on menu edits.
- `XMVSF_CACHE_SMOKE=1`: run the existing dependency smoke test and bypass the no-op shortcut.

## Verification status

Changes are source-reviewed, not executed on the user's actual MSYS2 native cache. Do not assume a particular speedup until a local `BUILD_TEST.bat` reports its **BUILD PHASE** timings. If a warm single-pass changed-menu build fails or misses dependencies, set `XMVSF_WARM_MENU_SINGLE_PASS=0` for that build and provide the build log; the original full Make path and 1,174-object cache remain intact. No cleans or forced object rebuilds were introduced.
