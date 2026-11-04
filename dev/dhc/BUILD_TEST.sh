#!/usr/bin/env bash
# Stage 87.138 experimental TEST builder. Called ONLY by root BUILD_TEST.bat.
# Build the CURRENT local source in place using the warm FBNeo object cache.
# Never touch Stable executable, Stable manifests, user source, or old TEST binaries.
set -euo pipefail
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_87_138.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_87_138_pending.exe"

if [[ ! -f "$TREE/makefile.sdl2" || ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_138_tag_land_test.h" ]]; then
  echo "ERROR: Current Stage 87.138 source was not pulled into $TREE" >&2
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
echo "XMVSF Stage 87.138 TEST: compiling current local buildtree with warm cache"
make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 -j"${XMVSF_BUILD_JOBS:-12}" all

BUILT=""
for exe in "$TREE/fbneo.exe" "$TREE/fbneo"; do
  if [[ -s "$exe" ]]; then BUILT="$exe"; break; fi
done
if [[ -z "$BUILT" ]]; then
  echo "ERROR: Compiler succeeded but did not produce fbneo executable." >&2
  exit 2
fi

# Never silently publish a stale 87.132.4 binary. These strings must come
# from the experimental native tag and capture source actually linked.
if ! grep -aFq 'XM87138_TAG_LAND_TEST' "$BUILT" ||
   ! grep -aFq 'XM87137_DHC_CAPTURE_TSV' "$BUILT"; then
  echo "ERROR: Built executable lacks Stage 87.138 native markers." >&2
  echo "Old TEST and Stable binaries have not been replaced." >&2
  exit 2
fi
mkdir -p "$PROJECT/runtime/stable"
# Only publish after the linked executable passed checks; replace the
# dedicated 87.138 TEST binary, never XMVSF_Stable.exe.
cp -p "$BUILT" "$PENDING"
if ! cmp -s "$BUILT" "$PENDING"; then
  echo "ERROR: Test binary copy verification failed." >&2
  exit 2
fi
mv -f "$PENDING" "$DEST"
echo "TEST BUILD READY: $DEST"
echo "This is the Stage 87.138 native tag-landing experiment, not full DHC."
