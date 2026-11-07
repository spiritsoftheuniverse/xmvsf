#!/usr/bin/env bash
# Stage 87.208 native TEST: retained native game + real logo/glyph assets. Called ONLY by root BUILD_TEST.bat.
# Build the CURRENT local source in place using the warm FBNeo object cache.
# Never touch Stable executable, Stable manifests, user source, or old TEST binaries.
set -euo pipefail
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Permanent TEST build transcript. This starts BEFORE checking toolchains,
# source files or running make. Never hide fatal compiler/linker diagnostics.
# Tee preserves live console output and an on-disk copy. Logs live outside
# the buildtree so incremental make objects and Stable remain untouched.
LOG_DIR="$PROJECT/test/build_logs"
if ! mkdir -p "$LOG_DIR"; then
  echo "ERROR: Could not create build log directory: $LOG_DIR" >&2
  exit 2
fi
LOG_FILE="$LOG_DIR/BUILD_TEST.log"
if ! command -v tee >/dev/null; then
  echo "ERROR: MSYS2 tee is missing; cannot preserve the TEST build transcript." >&2
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] ERROR: tee missing" >> "$LOG_FILE"
  exit 2
fi
exec > >(tee -a "$LOG_FILE") 2>&1
printf '\n========== XMVSF TEST 87.208 BUILD BEGIN %s ==========' "$(date '+%Y-%m-%d %H:%M:%S')"
printf '\nProject: %s\nLog: %s\n' "$PROJECT" "$LOG_FILE"
trap 'rc=$?; trap - EXIT; printf "XMVSF BUILD END rc=%s time=%s log=%s\n" "$rc" "$(date "+%Y-%m-%d %H:%M:%S")" "$LOG_FILE"; exit "$rc"' EXIT

TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_87_208.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_87_208_pending.exe"

if [[ ! -f "$TREE/makefile.sdl2" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_186_opening_observer.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_187_opening_front.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_188_full_attack_gate.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_189_native_super_family.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_191_super_lifetime_gate.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_192_reserve_super_ownership.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_193_deterministic_exit_protocol.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_194_native_passive_jump_entry.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_195_tag_latch_guard.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_196_native_immediate_fall.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_197_cinematic_opponent_hold.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_198_native_super_freeze_cue.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_202_air_followup_rebound.h" ||
      ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_203_stage_capture.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_195_menu_gif.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_menu_color_fx.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_206_crash_report.h" ||
      ! -s "$TREE/src/intf/audio/sdl/xm20_wasapi_impl.h" ||
      ! -s "$TREE/src/burn/drv/capcom/xmvsf_stage87_33_dizzy_hud.h" ||
      ! -s "$TREE/src/burn/drv/capcom/xmvsf_stage87_121_cyclops_release.h" ||
      ! -s "$PROJECT/dev/dhc/tests/test_stun_and_cyclops_209.cpp" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_menu_glyph_atlas.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage14_boot.h" ]]; then
  echo "ERROR: 87.208 native menu/glyph sources missing; inspect checked-out branch" >&2
  exit 2
fi
for asset in logo.png sf2font.png sf2fontsmall.png menuanimation.gif stunbar_base.png; do
  if [[ ! -s "$PROJECT/_art/$asset" ]]; then
    echo "ERROR: 87.208 requires original nonempty _art/$asset" >&2
    exit 2
  fi
done
if ! grep -Fq 'xmvsf_menu_glyph_atlas.h' "$TREE/src/burner/sdl/xmvsf_stage14_boot.h"; then
  echo "ERROR: 87.208 glyph header is not wired into the native boot menu" >&2
  exit 2
fi
# Do not gate an otherwise valid build on a particular draw helper name.
# Stage 87.208 uses menuLogo(render), which superseded logo(render).
# The C++ compiler checks the actual call and linking; the runtime atlas
# reports whether the title PNG loaded successfully.
echo "87.208 menu preflight: native boot and glyph/FX sources present; logo, fonts and menuanimation.gif verified."
echo "87.208 draw path is verified by native compile and runtime, not a stale grep for XMGlyph::logo(render)."
if [[ ! -f "$TREE/src/burn/drv/capcom/cps_run.cpp" ||
      ! -f "$TREE/src/intf/video/sdl/xmvsf_stage86_49_painter.h" ]]; then
  echo "ERROR: Native TEST buildtree source is incomplete." >&2
  exit 2
fi
if ! command -v make >/dev/null || ! command -v gcc >/dev/null; then
  echo "ERROR: MSYS2 UCRT64 make/gcc not on PATH." >&2
  exit 2
fi

# Before full make, compile/run the tiny 87.202 exact-ROM-rebound
# regression without touching Stable or the warm FBNeo object cache.
if ! command -v g++ >/dev/null; then
  echo "ERROR: Missing g++ for 87.202 local DHC predicate regression." >&2
  exit 2
fi
mkdir -p "$PROJECT/test/87.202/unit_tests"
g++ -std=c++11 -Wall -Wextra -Werror \
  "$PROJECT/dev/dhc/tests/test_dhc_air_followup_rebound_202.cpp" \
  -o "$PROJECT/test/87.202/unit_tests/test_dhc_air_followup_rebound_202.exe"
"$PROJECT/test/87.202/unit_tests/test_dhc_air_followup_rebound_202.exe"
# Compile/execute the read-only native stage telemetry regression separately.
mkdir -p "$PROJECT/test/87.203/unit_tests/capture_smoke"
g++ -std=c++11 -Wall -Wextra -Werror -I"$TREE/src/burn/drv/capcom" \
  "$PROJECT/dev/stage_capture/test_native_stage_capture.cpp" \
  -o "$PROJECT/test/87.203/unit_tests/test_native_stage_capture_203.exe"
XM87203_STAGE_CAPTURE_DIR="$PROJECT/test/87.203/unit_tests/capture_smoke" \
XM87203_BATCH_FRAMES=300 \
  "$PROJECT/test/87.203/unit_tests/test_native_stage_capture_203.exe"

# Verify the previous Cyclops 87.121 recovery and single dynamic bar geometry.
# This test is C++11, ROM-free and uses synthetic native fighter RAM;
# it runs before the full Windows build and never touches Stable/replays.
mkdir -p "$PROJECT/test/87.209/unit_tests"
g++ -std=c++11 -Wall -Wextra -Werror   -I"$TREE/src/burn/drv/capcom"   "$PROJECT/dev/dhc/tests/test_stun_and_cyclops_209.cpp"   -o "$PROJECT/test/87.209/unit_tests/test_stun_and_cyclops_209.exe"
"$PROJECT/test/87.209/unit_tests/test_stun_and_cyclops_209.exe"

# Preserve warm cache and anything already built on failures. Never run make clean.
cd "$TREE"
echo "XMVSF Stage 87.208 TEST: incremental build; preserving cached objects"
if command -v git >/dev/null 2>&1; then
  echo "Source branch: $(git -C "$PROJECT" branch --show-current 2>/dev/null || true)"
  echo "Source revision: $(git -C "$PROJECT" rev-parse --short HEAD 2>/dev/null || true)"
fi
echo "Dependencies: GNU Make + persistent obj/**/*.d header dependency files"
# The FBNeo makefile already runs a two-pass dependency-aware build:
#   MAKELEVEL=1 generates missing/stale .d files and builds changed objects;
#   MAKELEVEL=2 includes those .d files, checks transitive headers and links.
# DEPEND=1 stays enabled so changing a stage header recompiles ONLY the
# translation units that actually include it. The .d and .o caches survive
# between TEST sessions, including across launcher invocations.
# Earlier 87.153-87.167 launchers passed five unconditional -W source
# overrides. -W pretended the same five .cpp files were always newer,
# forcing redundant compilation even when nothing had changed. Do not use
# -W or clean here. Missing objects and changed C/C++ headers are handled
# by Make's normal dependency graph.
# The complete, unfiltered compiler AND linker stderr/stdout goes to
# BUILD_TEST.log as well as the console. set -e preserves make's exit code.
# Do not use awk warning filters or truncate diagnostics when compilation fails.
# No make clean, forced rebuild, source timestamp changes, or Stable writes.
# TEST 87.208 changes multiple native frontend translation units.
# First 87.208 build or newer UI/audio/crash header: force just the four
# affected .cpp files through make (without clean, Stable writes, or Actions).
# Warm cached ROM/68000/CPS-2 objects remain untouched.
FRESHEN=()
if [[ ! -s "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_menu_glyph_atlas.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_menu_color_fx.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_stage87_195_menu_gif.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_stage87_206_crash_report.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_stage87_140_super_chat.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_stage14_boot.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xm20_vs_transition.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_stage23_ui.h" -nt "$DEST" ||
      "$TREE/src/burner/sdl/xmvsf_stage25_5_ui.h" -nt "$DEST" ||
      "$TREE/src/intf/video/sdl/xmvsf_stage87_140_chat_overlay.h" -nt "$DEST" ||
      "$TREE/src/intf/video/sdl/vid_sdl2.cpp" -nt "$DEST" ||
      "$TREE/src/burn/drv/capcom/xmvsf_stage87_33_dizzy_hud.h" -nt "$DEST" ||
      "$TREE/src/burn/drv/capcom/xmvsf_stage87_121_cyclops_release.h" -nt "$DEST" ||
      "$TREE/src/burn/drv/capcom/cps_draw.cpp" -nt "$DEST" ||
      "$TREE/src/burn/drv/capcom/cps_run.cpp" -nt "$DEST" ||
      "$TREE/src/intf/audio/sdl/xm20_wasapi_impl.h" -nt "$DEST" ]]; then
  echo "87.208 STUN BAR+CRASH+FADE: rebuilding native SDL frontend and CPS2 HUD source (other build objects preserved)."
  FRESHEN=(-W src/burner/sdl/main.cpp
           -W src/burner/sdl/run.cpp
           -W src/intf/video/sdl/vid_sdl2.cpp
           -W src/intf/audio/sdl/aud_sdl.cpp
           -W src/burn/drv/capcom/cps_obj.cpp
           -W src/burn/drv/capcom/cps_draw.cpp
           -W src/burn/drv/capcom/cps_run.cpp)
fi
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1 \
  "${FRESHEN[@]}" -j"${XMVSF_BUILD_JOBS:-12}" all

BUILT=""
for exe in "$TREE/fbneo.exe" "$TREE/fbneo"; do
  if [[ -s "$exe" ]]; then BUILT="$exe"; break; fi
done
if [[ -z "$BUILT" ]]; then
  echo "ERROR: Compiler succeeded but did not produce fbneo executable." >&2
  exit 2
fi

# The compiler and linker are the authoritative success gates.
# Stage 87.203 demonstrated why scanning a linked FBNeo executable for
# every historical debug string is not a valid freshness or correctness test:
# GCC may fold/omit string literals, or only some modules may reference them.
# The 87.184 log shows make/link SUCCESS followed by an erroneous marker
# rejection. Do not block publishing a valid TEST build on binary strings.
# Keep source presence checks, make's nonzero error handling, nonempty
# executable check, and atomic TEST-only staging below. Never touch Stable.
echo "Build/link succeeded; publishing TEST binary without brittle debug-string marker gates."
mkdir -p "$PROJECT/runtime/stable"
# Publish only after successful link and nonempty output verification.
# Write a pending file first, then atomically rename TEST only.
# Stable and all previous numbered TEST executables remain untouched.
# cp exits nonzero on failure (set -e); Bash -s checks copied content
# exists and is nonempty. Do not depend on an uninstalled 'cmp' binary.
cp -p "$BUILT" "$PENDING"
if [[ ! -s "$PENDING" ]]; then
  echo "ERROR: Copied TEST binary is empty or missing." >&2
  exit 2
fi
mv -f "$PENDING" "$DEST"
echo "TEST BUILD READY: $DEST"
if command -v sha256sum >/dev/null 2>&1; then
  echo "87.208 compiled TEST sha256: $(sha256sum "$DEST")"
  echo "87.208 menu source assets sha256:"
  sha256sum "$PROJECT/_art/logo.png" "$PROJECT/_art/sf2font.png" "$PROJECT/_art/sf2fontsmall.png" "$PROJECT/_art/menuanimation.gif" "$PROJECT/_art/stunbar_base.png"
fi
echo "Stage 87.208: native title + proportional font, original ROM/combat paths retained; Stable untouched."
