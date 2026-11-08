# XMVSF native incremental build cache — TEST 87.216

This system runs **entirely on the local Windows/MSYS2 UCRT64 machine**.
GitHub manages source history. There are **no GitHub Actions, hosted
builds, setup gates, compiler downloads or Stable changes**.

## One command

`BUILD_TEST.bat` remains the only build entry point. It calls
`dev/dhc/BUILD_TEST.sh` and publishes ONLY
`runtime/stable/XMVSF_Test_87_216.exe` after successful native Make
validation and nonempty executable checks. `RUN_TEST.bat` continues to
launch that numbered TEST.

## What actually triggers compilation

| Change | What is rebuilt |
| --- | --- |
| One native `.cpp` | Its `.o` (then necessary link) |
| A header used by three `.cpp` files | The three dependent `.o` files |
| An unrelated asset, Markdown file or Git commit | No `.o` rebuild |
| No input changes | No C++ compile; an identical TEST exe is not copied |
| Compiler version, SDL settings or Make rules change | Cached native `.o` and `.d` become stale through the signature prerequisite and rebuild |
| A ROM-free regression's source/included headers change | Only that test executable |

The dependency graph comes from the **existing native**
`dev/stable/buildtree/makefile.sdl2` and its generated `obj/**/*.d`
files. Do not replace it with manual lists of “interesting” headers:
compiler-generated dependencies are authoritative.

The old `FRESHEN=(-W ...seven translation units...)` block is gone.
On an ordinary edit, Make does not rebuild unchanged objects. There is
no `make clean`, source timestamp mass-touch, or deleting existing
caches. The `MAKELEVEL=1` two-pass dependency build is preserved.

## Persistent state (local only)

All machine-specific cache metadata lives under ignored
`test/build_cache/`:

- `compiler.signature`: content-derived compiler/build-settings
  identity. On the **first** build, adopt the existing warm `.o/.d`
  tree without a forced full rebuild. If the compiler/settings later
  change, update this stamp, causing Make to rebuild the affected
  translation units safely.
- `unit_tests/`: existing ROM-free regression executables, including
  their compiler-generated `.d` files.
- `last_rebuilt_objects.txt`: native `.o` paths rewritten in the
  most recent successful build (mtime-based **reporting**, not the
  build decision).
- `object_inventory.tsv`: the remaining native `.o` inventory,
  including sizes and mtimes.
- `last_success.tsv`: successful build timestamp, Git branch/revision,
  compiler signature, number of reused/rebuilt objects, executable SHA256,
  and whether the TEST executable was newly published.
- `ccache/` (optional): content-addressed GCC results reused even
  between different branches. This is on the project drive, not C:.

An unsuccessful build retains the last successful manifest. It does
not publish the pending TEST executable or touch Stable.

## Optional ccache

If `ccache` is already on PATH, it is used as a compiler accelerator.
No installation is required; without it the build is still incremental.
Set `XMVSF_CCACHE=0` to disable it. Set `XMVSF_CCACHE_DIR` to choose
another directory. GNU Make still decides when to compile; ccache
only accelerates those compiler calls.

## Optional dependency smoke test

Set `XMVSF_CACHE_SMOKE=1` for a single `BUILD_TEST.bat` invocation
to run `dev/dhc/tests/test_incremental_cache.sh`. It builds two tiny
generated files under ignored `test/build_cache/cache_smoke/` and checks:

1. The first build compiles both tiny objects.
2. Re-running unchanged compiles neither object.
3. Editing one included header recompiles only its dependent object.
4. Changing the compiler signature recompiles both.

The fixture is isolated from real CPS2 sources and artifacts. On success,
the log prints `INCREMENTAL_CACHE_PASS`. Normally leave this disabled
to avoid spending ~2 seconds on an intentionally repeated test.

## 87.216 correction: 87.215 false-success build

The 87.215 `last_success.tsv` and `BUILD_TEST.log` revealed an unsafe
Make prerequisite: `compiler.signature` was passed as `$<` to GCC
instead of the `.cpp` source. GCC warned that the file was an unused
linker input and exited successfully. Consequently the manifest reported
**0 recompiled / 1,174 reused**, even though five native dependency
files were regenerated and the executable was published. **Do not rely
on the 87.215 executable as evidence that new source was compiled.**

This was fixed by using GNU Make's target-specific `.EXTRA_PREREQS`.
That participates in freshness checks without changing `$<` or `$^`.
The smoke test now uses the same implicit `%.o: %.cpp` recipe and
explicitly rejects anything except a `.cpp` as the compiler input.

One-time automatic cache repair matches the **exact uploaded 87.215
manifest identity**. It recompiles the five source units named in the
log (`cps_draw.cpp`, `cps_run.cpp`, `main.cpp`, `run.cpp`,
`vid_sdl2.cpp`) using temporary Make `-W` overrides **only for this
known-bad build**. It neither deletes nor forces the other cached
objects. Before TEST publishing, the builder checks the five output
`.o` files were actually recreated during the build. A successful
repair writes a local one-time marker; subsequent builds return to
ordinary dependency-only compilation.

The build number is now **87.216**. Native Windows compilation and the
new repair outcome still require local verification with BUILD_TEST.bat.

## Troubleshooting

Run `BUILD_TEST.bat` and inspect
`test/build_logs/BUILD_TEST.log` for the `BUILD CACHE SUMMARY` line.
Check `test/build_cache/last_rebuilt_objects.txt` for exact rebuilt
units, and `last_success.tsv` for the committed source SHA and binary
checksum. A purely unchanged build should report zero rewritten native
objects. Editing a single source should rebuild only the dependent
translation unit(s).

The normal native toolchain is still the sole authority on whether the
binary compiles and links. These source changes are **not** a claim of
a completed local Windows build. Do not try to verify C++ builds through
GitHub Actions; use the existing local scripts and attach the build log
if the native compilation fails.
