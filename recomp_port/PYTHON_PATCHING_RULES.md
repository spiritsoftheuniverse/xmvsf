# XMVSF recomp: Python patching and test rules

**Scope:** `recomp_port/*.py`, `recomp_port/tests/*.py`, and
`RECOMP_BUILD.bat`. All builds and tests run locally on Windows/MSYS2 UCRT64.
Do not add GitHub Actions. The source of truth is the in-repository
`recomp/` engine and the `recomp_port/` templates; no upstream submodule
fetches or clean rebuilds unless explicitly requested.

## Why this document exists

The menu film branch advanced from TEST 87.454 to TEST 87.455, but
`tests/test_pad_owners.py` still asserted the former literal. The local
build stopped before Ninja, despite a valid controller implementation.
Earlier releases suffered anchor count errors, regex syntax errors,
non-idempotent installers, and obsolete test assumptions.

The cure is not appending each version to more lists. Source assertions,
installer logic and build order must remain valid across future versions.

## Version contract (mandatory)

- The visible C++ menu version is updated in the template source
  `recomp_port/xmvsf_home.h` and `recomp_port/xmvsf_menu_textures.h`;
  their deployed copies in `recomp/port` must agree.
- Tests import `recomp_port/tests/version_guard.py` and call
  `require_matching_versions(home_source, renderer_source, minimum=(87, 454))`,
  or `require_minimum_version` when there is only one source.
- The minimum is the *oldest version implementing the feature being tested*,
  not the latest current build. A newer version must pass unchanged.
- No assertions such as `assert "TEST 87.454" in source`.
  No enumerations of every TEST version, either.
- The guard rejects a missing, ambiguous, mismatched, or too-old quoted
  TEST label. It compares integers, not lexicographic strings.
- `tests/test_version_guard.py` scans every Python test with the Python
  AST for pinned TEST-version string constants. It also compiles every
  project Python source in memory, checks actual templates, and tests
  simulated future version bumps and version mismatch failures.

## Installer contract (mandatory)

1. Read the existing files first; do not rewrite source merely to apply
   a no-op patch. Treat `recomp_port` templates as canonical where used.
2. Check every exact replacement anchor for **one** occurrence.
   Zero or multiple matches must fail with an informative exception
   **before** modifying any file.
3. Apply transforms in memory, then apply the transform again and
   assert that the second pass produces identical output (idempotence).
4. Exercise source patchers on representative installed and legacy
   fixtures, including nearby unrelated lookalike anchors. A replacement
   must only touch its intended function/call site.
5. Validate the complete transformed output and every necessary hook
   before performing disk writes. For multi-file changes, finish all
   checks before the first write; reject partial upgrades.
6. Install generated headers byte-for-byte from their canonical
   `recomp_port` copy only if bytes differ, to preserve warm Ninja caching.
7. Never invent regex from untested concatenated strings. Compile each
   pattern and execute replacement on old/current fixtures, including
   the second pass, before committing. Prefer fixed anchors when possible.
8. Do not replace controllers, menu audio, textures, or other subsystems
   outside the requested change. Never silently bypass a failed assertion.

## Local validation order (mandatory)

1. Run `python3 recomp_port/tests/test_version_guard.py` from the repo root.
   `RECOMP_BUILD.bat` runs this first, before source installers.
2. Run the feature-specific source preflight and installer-twice fixture
   tests. Keep the existing controller, Options, textured-menu, logo, and
   film regression tests. Do not skip unrelated tests to get green output.
3. Confirm canonical templates and deployed `recomp/port` copies agree
   after their installers run.
4. Run `RECOMP_BUILD.bat` in the **current branch**, using warm Ninja
   artifacts. Only use `RECOMP_BUILD.bat refresh` when translation or
   configuration inputs truly require M68k regeneration.
5. Treat a preflight PASS as source validation only. Mark the actual
   Windows build and game visuals verified only after they are executed.
6. Push to a feature branch, not `main`. Keep ROMs, build caches, and
   existing Stable/TEST executables untouched.

## Adding a new feature

Before modifying source: list dependent installers, every `test_*.py`,
and downstream copy points. When advancing the build number, run the
version policy preflight and the complete local test sequence. If a
test fails because of a new version, fix its **invariant** rather than
replacing its pinned constant with the next constant.

**Regression check:** advancing 87.455 -> 87.456 without any behavior
changes must not break controller, Options, texture, logo, or film tests.
