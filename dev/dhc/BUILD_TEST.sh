#!/usr/bin/env bash
# Stage 87.140 experimental TEST builder. Called ONLY by root BUILD_TEST.bat.
# Build the CURRENT local source in place using the warm FBNeo object cache.
# Never touch Stable executable, Stable manifests, user source, or old TEST binaries.
set -euo pipefail
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_87_140.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_87_140_pending.exe"

if [[ ! -f "$TREE/makefile.sdl2" || ! -f "$TREE/src/burner/sdl/xmvsf_stage87_140_super_chat.h" ]]; then
  echo "ERROR: Current Stage 87.140 probe source was not pulled into $TREE" >&2
  exit 2
fi
if [[ ! -f "$TREE/src/burn/drv/capcom/cps_run.cpp" ||
      ! -f "$TREE/src/intf/video/sdl/xmvsf_stage86_49_painter.h" ]]; then
  echo "ERROR: Native TEST buildtree source is incomplete." >&2
  exit 2
fi
if ! command -v make >/dev/null || ! command -v gcc >/dev/null; then
  echo "ERROR: MSYS2 UCRT64 make/gcc not on PATH." >&2
  exit 2
fi

# Preserve warm cache and anything already built on failures. Never run make clean.
cd "$TREE"
echo "XMVSF Stage 87.140 TEST: compiling current local buildtree with warm cache"
# This FBNeo makefile is a two-pass build: MAKELEVEL=1 compiles objects,
# and the nested make (level 2) links fbneo. The old invocation could report
# 'Nothing to be done' without checking newly pulled DHC source reliably.
# Treat ONLY the four changed translation units as newer for this TEST.
# -W does not alter source timestamps, files or the existing object cache.
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 \
  -j"${XMVSF_BUILD_JOBS:-12}" \
  -W src/burn/drv/capcom/cps_run.cpp \
  -W src/burner/sdl/run.cpp \
  -W src/intf/input/sdl/inp_sdl2.cpp \
  -W src/intf/video/sdl/vid_sdl2.cpp all

BUILT=""
for exe in "$TREE/fbneo.exe" "$TREE/fbneo"; do
  if [[ -s "$exe" ]]; then BUILT="$exe"; break; fi
done
if [[ -z "$BUILT" ]]; then
  echo "ERROR: Compiler succeeded but did not produce fbneo executable." >&2
  exit 2
fi

# Never silently publish a stale 87.132.4 binary. These strings must come
# from the command-matching chat and UI status actually linked.
if ! grep -aFq 'FIRST SUPER? point=' "$BUILT" ||
   ! grep -aFq 'ROM-VERIFIED RESERVE SUPER LIST: EMPTY' "$BUILT" ||
   ! grep -aFq 'DHC COMMANDS  [SHIFT+G HISTORY]' "$BUILT"; then
  echo "ERROR: Built executable lacks Stage 87.140 native markers." >&2
  echo "Old TEST and Stable binaries have not been replaced." >&2
  exit 2
fi
mkdir -p "$PROJECT/runtime/stable"
# Only publish after the linked executable passed checks; replace the
# dedicated 87.140 TEST binary, never XMVSF_Stable.exe.
# cp exits nonzero on failure (set -e); Bash -s checks copied content
# exists and is nonempty. Do not depend on an uninstalled 'cmp' binary.
cp -p "$BUILT" "$PENDING"
if [[ ! -s "$PENDING" ]]; then
  echo "ERROR: Copied TEST binary is empty or missing." >&2
  exit 2
fi
mv -f "$PENDING" "$DEST"
echo "TEST BUILD READY: $DEST"
echo "This is Stage 87.140 READ-ONLY first-super command chat; F10 probe remains available."
