# XMVSF — Recomp V2 foundation (inspection branch)

This branch preserves the existing FBNeo/SDL project unchanged and links the complete upstream CPS-2 Recomp source at `recomp/`, pinned to commit `d5c720c3367408b0ecec4454ccc08e92d52673ae`.

**Important:** This is a Git submodule, not yet a source-vendored fork. It provides the full upstream working tree with one checkout and can be inspected and built independently. No changes have been made to upstream combat or training behavior. Neither 87.434 nor Stable was replaced.

## Windows / MSYS2 UCRT64

From a fresh checkout of this branch:

```bat
git submodule update --init --recursive
```

For source inspection, start with `recomp/games/xmvsf/src/game.cpp`, `recomp/games/xmvsf/src/xmvsf.h`, `recomp/games/xmvsf/src/engine.cpp`, `recomp/fight/`, `recomp/video/`, and `recomp/runtime/`.

For a local XMVSF recomp build, follow `recomp/docs/BUILDING.md`. Required: a compatible owned XMVSF Europe 961004 ROM set with key, MSYS2 UCRT64 toolchain, SDL3, CMake (>=3.28), Ninja, zlib and Python. Upstream's documented workflow is:

```sh
cd recomp
make GAME=xmvsf
make GAME=xmvsf explore
make GAME=xmvsf run
```

Generated translation, compiled executables and ROM files stay local, not in Git. The existing root `BUILD_TEST.bat` and `RUN_TEST.bat` remain the old FBNeo build entrypoints and must not be used to build the recomp.

## Import provenance

Upstream: https://github.com/tmcleroy/cps2recomp
Revision: https://github.com/tmcleroy/cps2recomp/commit/d5c720c3367408b0ecec4454ccc08e92d52673ae
License: GPL-3.0-only (see `recomp/LICENSE`).

The linked repository is currently an upstream snapshot. To make all its files independently editable in this repository and branch, vendor its source using a local Git subtree or source-copy commit, then remove the submodule reference. Do not commit copyrighted ROMs, decryption keys or generated translated program bytes.
