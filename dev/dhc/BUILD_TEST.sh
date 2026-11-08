#!/usr/bin/env bash
# Stage 87.220 native TEST: retained native game + real logo/glyph assets. Called ONLY by root BUILD_TEST.bat.
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
printf '\n========== XMVSF TEST 87.220 BUILD BEGIN %s ==========' "$(date '+%Y-%m-%d %H:%M:%S')"
printf '\nProject: %s\nLog: %s\n' "$PROJECT" "$LOG_FILE"
trap 'rc=$?; trap - EXIT; printf "XMVSF BUILD END rc=%s time=%s log=%s\n" "$rc" "$(date "+%Y-%m-%d %H:%M:%S")" "$LOG_FILE"; exit "$rc"' EXIT

TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_87_220.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_87_220_pending.exe"
# Machine-local cache: never stored in Git and never mutates Stable.
CACHE_DIR="$PROJECT/test/build_cache"
SIGNATURE="$CACHE_DIR/compiler.signature"
BUILD_START="$CACHE_DIR/.build_started"
OBJECT_INVENTORY="$CACHE_DIR/object_inventory.tsv"
REBUILT_LIST="$CACHE_DIR/last_rebuilt_objects.txt"
SUCCESS_STATE="$CACHE_DIR/last_success.tsv"

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
      ! -s "$PROJECT/dev/dhc/tests/test_stun_and_cyclops_219.cpp" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_217_native_float.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_menu_glyph_atlas.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage14_boot.h" ]]; then
  echo "ERROR: 87.220 required native source missing or empty:" >&2
  for source in \
    "$TREE/makefile.sdl2" \
    "$TREE/src/burner/sdl/xmvsf_stage87_217_native_float.h" \
    "$TREE/src/burn/drv/capcom/xmvsf_stage87_121_cyclops_release.h" \
    "$PROJECT/dev/dhc/tests/test_stun_and_cyclops_219.cpp" \
    "$TREE/src/burner/sdl/xmvsf_menu_glyph_atlas.h" \
    "$TREE/src/burner/sdl/xmvsf_stage14_boot.h"; do
    if [[ ! -s "$source" ]]; then echo "MISSING: $source" >&2; fi
  done
  exit 2
fi
for asset in logo.png sf2font.png sf2fontsmall.png menuanimation.gif stunbar_base.png; do
  if [[ ! -s "$PROJECT/_art/$asset" ]]; then
    echo "ERROR: 87.220 requires original nonempty _art/$asset" >&2
    exit 2
  fi
done
if ! grep -Fq 'xmvsf_menu_glyph_atlas.h' "$TREE/src/burner/sdl/xmvsf_stage14_boot.h"; then
  echo "ERROR: 87.220 glyph header is not wired into the native boot menu" >&2
  exit 2
fi
# Do not gate an otherwise valid build on a particular draw helper name.
# Stage 87.220 uses menuLogo(render), which superseded logo(render).
# The C++ compiler checks the actual call and linking; the runtime atlas
# reports whether the title PNG loaded successfully.
echo "87.220 menu preflight: native boot and glyph/FX sources present; logo, fonts and menuanimation.gif verified."
echo "87.220 draw path is verified by native compile and runtime, not a stale grep for XMGlyph::logo(render)."
if [[ ! -f "$TREE/src/burn/drv/capcom/cps_run.cpp" ||
      ! -f "$TREE/src/intf/video/sdl/xmvsf_stage86_49_painter.h" ]]; then
  echo "ERROR: Native TEST buildtree source is incomplete." >&2
  exit 2
fi
if ! command -v make >/dev/null || ! command -v gcc >/dev/null; then
  echo "ERROR: MSYS2 UCRT64 make/gcc not on PATH." >&2
  exit 2
fi

# Make's .d dependency graph remains authoritative for C++ source/header
# changes. A persistent fingerprint is an ADDITIONAL prerequisite only
# when the compiler, settings, SDK or Make rules change. Git commits alone
# never trigger compilation.
command -v sha256sum >/dev/null || {
  echo "ERROR: MSYS2 sha256sum required for build fingerprint." >&2; exit 2;
}
command -v sdl2-config >/dev/null || {
  echo "ERROR: sdl2-config required by native SDL build." >&2; exit 2;
}
mkdir -p "$CACHE_DIR"
# The historical 87.215 BUILD_TEST log showed Make compiling the signature
# instead of five actual C++ sources. Fix exactly this known bad manifest,
# without invalidating 1,174 otherwise usable cached native objects.
REPAIR_NATIVE=()
REPAIR_MARK="$CACHE_DIR/.repaired_87_216_signature_source"
if [[ ! -e "$REPAIR_MARK" && -s "$SUCCESS_STATE" ]] &&
   grep -Fq $'source_revision\ta76b4041464f34c284b17adc56fa55eabff464c8' "$SUCCESS_STATE" &&
   grep -Fq $'native_objects_recompiled\t0' "$SUCCESS_STATE" &&
   grep -Fq $'test_executable_published\t1' "$SUCCESS_STATE" &&
   grep -Fq $'compiler_signature\ta7fefd01dd1626afd8fd6802c066b869708257059794c1a54918040ead11c7f2' "$SUCCESS_STATE"; then
  echo "BUILD CACHE REPAIR: old 87.215 signature was compiled instead of five source units."
  REPAIR_NATIVE=(
    src/burn/drv/capcom/cps_draw.cpp
    src/burn/drv/capcom/cps_run.cpp
    src/burner/sdl/main.cpp
    src/burner/sdl/run.cpp
    src/intf/video/sdl/vid_sdl2.cpp
  )
fi
compiler_identity="$(
  {
    printf 'XMVSF_COMPILER_IDENTITY_V1\n'
    printf 'MAKE_ARGS: LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1\n'
    printf 'MSYSTEM=%s\n' "${MSYSTEM:-UCRT64}"
    printf 'gcc_path=%s\n' "$(command -v gcc)"
    gcc -dumpmachine
    gcc -dumpfullversion -dumpversion
    printf 'g++_path=%s\n' "$(command -v g++)"
    g++ -dumpmachine
    g++ -dumpfullversion -dumpversion
    printf 'sdl_cflags=%s\n' "$(sdl2-config --cflags)"
    printf 'sdl_libs=%s\n' "$(sdl2-config --libs)"
    sha256sum "$TREE/makefile.sdl2" "$TREE/makefile.burn_rules"
  } | sha256sum | cut -d ' ' -f 1
)"
if [[ ! -s "$SIGNATURE" ]]; then
  printf '%s\n' "$compiler_identity" > "$SIGNATURE.pending"
  mv -f "$SIGNATURE.pending" "$SIGNATURE"
  # Bootstrap pre-existing warm objects without a needless full rebuild.
  # Native .o/.d mtime checks still work, as before this improvement.
  touch -t 200001010000 "$SIGNATURE"
  echo "BUILD CACHE: initialized fingerprint; adopting warm Make objects."
elif [[ "$(cat "$SIGNATURE")" != "$compiler_identity" ]]; then
  printf '%s\n' "$compiler_identity" > "$SIGNATURE.pending"
  mv -f "$SIGNATURE.pending" "$SIGNATURE"
  if (( ${#REPAIR_NATIVE[@]} )); then
    # This signature change repairs the erroneous Make dependency rule.
    # All other objects retain their known-good existing timestamps.
    touch -t 200001010000 "$SIGNATURE"
    echo "BUILD CACHE REPAIR: preserving 1,174 objects; targeting five source files."
  else
    echo "BUILD CACHE: compiler/settings changed; dependent objects rebuild."
  fi
else
  echo "BUILD CACHE: compiler/settings match; retaining existing objects."
fi
echo "BUILD CACHE: fingerprint=$compiler_identity"
echo "BUILD CACHE: native objects=$TREE/obj; state=$CACHE_DIR"
# Opt-in, ROM-free dependency smoke check. Does not touch native objects.
if [[ "${XMVSF_CACHE_SMOKE:-0}" == 1 ]]; then
  bash "$PROJECT/dev/dhc/tests/test_incremental_cache.sh"
fi

# Optional second-layer content cache for branch switching. Keep its
# storage on the project disk, not the Windows OS drive. No install gate.
MAKE_COMPILER=()
if [[ "${XMVSF_CCACHE:-1}" != 0 ]] && command -v ccache >/dev/null 2>&1; then
  export CCACHE_DIR="${XMVSF_CCACHE_DIR:-$CACHE_DIR/ccache}"
  mkdir -p "$CCACHE_DIR"
  MAKE_COMPILER=('CC=ccache gcc' 'CXX=ccache gcc' 'LD=gcc'
                 'HOST_CC=gcc' 'HOST_CXX=gcc')
  echo "BUILD CACHE: optional ccache enabled ($CCACHE_DIR)."
else
  echo "BUILD CACHE: optional ccache unavailable/disabled; normal gcc."
fi

# Each ROM-free test also retains an executable AND compiler-produced .d.
# It is recompiled only when its own source, transitive project headers,
# or the toolchain signature actually changed.
run_cached_unit(){
  local source="$1" output="$2"
  local args=(-s -f "$PROJECT/dev/dhc/unit_test_cache.mk"
    "UNIT_SOURCE=$source" "UNIT_OUTPUT=$output"
    "UNIT_SIGNATURE=$SIGNATURE"
    "UNIT_INCLUDE_CAPCOM=$TREE/src/burn/drv/capcom"
    "UNIT_INCLUDE_SDL=$TREE/src/burner/sdl")
  mkdir -p "$(dirname "$output")"
  if [[ ! -s "$output.d" ]]; then
    echo "UNIT CACHE: first dependency scan for $(basename "$output")."
    # -W applies to this ONE tiny test source for .d bootstrap,
    # NEVER to the seven native translation units.
    make "${args[@]}" -W "$source" unit
  else
    make "${args[@]}" unit
  fi
  "$output"
}
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_dhc_air_followup_rebound_202.cpp" \
  "$CACHE_DIR/unit_tests/test_dhc_air_followup_rebound_202.exe"
mkdir -p "$PROJECT/test/87.203/unit_tests/capture_smoke"
export XM87203_STAGE_CAPTURE_DIR="$PROJECT/test/87.203/unit_tests/capture_smoke"
export XM87203_BATCH_FRAMES=300
run_cached_unit \
  "$PROJECT/dev/stage_capture/test_native_stage_capture.cpp" \
  "$CACHE_DIR/unit_tests/test_native_stage_capture_203.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_stun_and_cyclops_219.cpp" \
  "$CACHE_DIR/unit_tests/test_stun_and_cyclops_219.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_dash_startup_220.cpp" \
  "$CACHE_DIR/unit_tests/test_dash_startup_220.exe"

# Preserve warm cache and anything already built on failures. Never run make clean.
cd "$TREE"
echo "XMVSF Stage 87.220 TEST: incremental build; preserving cached objects"
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
# XMVSF 87.220: NO broad -W, no clean, and no seven-unit FRESHEN.
# Native .d files specify exact transitive source/header dependencies.
# A compiler/signature change makes only signature-dependent objects stale.
# With an unchanged signature and unchanged files Make compiles NOTHING.
touch "$BUILD_START"
# One-time in-memory invalidation for five known uncompiled units only.
# No source or object file timestamps are changed manually.
REPAIR_ARGS=()
for src in "${REPAIR_NATIVE[@]}"; do REPAIR_ARGS+=(-W "$src"); done
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1 \
  "XMVSF_BUILD_SIGNATURE=../../../test/build_cache/compiler.signature" \
  "${MAKE_COMPILER[@]}" "${REPAIR_ARGS[@]}" -j"${XMVSF_BUILD_JOBS:-12}" all

BUILT=""
for exe in "$TREE/fbneo.exe" "$TREE/fbneo"; do
  if [[ -s "$exe" ]]; then BUILT="$exe"; break; fi
done
if [[ -z "$BUILT" ]]; then
  echo "ERROR: Compiler succeeded but did not produce fbneo executable." >&2
  exit 2
fi

# The defective 87.215 run returned success after GCC silently ignored
# compiler.signature as a non-source input. Never accept another false
# success: the exact five repaired .o files MUST have been recompiled
# since this build began, before any TEST publication is permitted.
if (( ${#REPAIR_NATIVE[@]} )); then
  for source in "${REPAIR_NATIVE[@]}"; do
    obj="$TREE/obj/${source#src/}"
    obj="${obj%.cpp}.o"
    if [[ ! -s "$obj" || ! "$obj" -nt "$BUILD_START" ]]; then
      echo "ERROR: CACHE REPAIR failed; expected freshly compiled object $obj" >&2
      exit 2
    fi
  done
  echo "BUILD CACHE REPAIR: verified all five native object files were freshly compiled."
fi

# Make's successful return and nonempty executable are authoritative.
# Do not scan binary strings for arbitrary historical markers.
# Hash-compare before publishing: a no-op build must NOT overwrite TEST.
built_sha="$(sha256sum "$BUILT" | cut -d ' ' -f 1)"
mkdir -p "$PROJECT/runtime/stable"
published=0
if [[ -s "$DEST" && "$(sha256sum "$DEST" | cut -d ' ' -f 1)" == "$built_sha" ]]; then
  echo "TEST PUBLISH: identical executable, keeping previous TEST file untouched."
else
  cp -p "$BUILT" "$PENDING"
  if [[ ! -s "$PENDING" ]]; then
    echo "ERROR: Pending TEST executable missing or empty." >&2
    exit 2
  fi
  if [[ "$(sha256sum "$PENDING" | cut -d ' ' -f 1)" != "$built_sha" ]]; then
    echo "ERROR: Pending TEST executable checksum mismatch." >&2
    exit 2
  fi
  mv -f "$PENDING" "$DEST"
  published=1
  echo "TEST PUBLISH: atomic TEST-only update completed."
fi
echo "TEST BUILD READY: $DEST"

# Persist a machine-local inventory and last-success manifest. Timestamp
# comparison is used only to REPORT changed objects; Make's .d graph
# controls real recompilation. No object or dependency cache is deleted.
find "$TREE/obj" -type f -name '*.o' -newer "$BUILD_START" \
  -printf '%P\n' | sort > "$REBUILT_LIST.pending"
find "$TREE/obj" -type f -name '*.o' \
  -printf '%P\t%s\t%T@\n' | sort > "$OBJECT_INVENTORY.pending"
rebuilt="$(wc -l < "$REBUILT_LIST.pending" | tr -d ' ')"
object_count="$(wc -l < "$OBJECT_INVENTORY.pending" | tr -d ' ')"
reused=$((object_count - rebuilt))
mv -f "$REBUILT_LIST.pending" "$REBUILT_LIST"
mv -f "$OBJECT_INVENTORY.pending" "$OBJECT_INVENTORY"
{
  printf 'field\tvalue\n'
  printf 'source_branch\t%s\n' "$(git -C "$PROJECT" branch --show-current 2>/dev/null || true)"
  printf 'source_revision\t%s\n' "$(git -C "$PROJECT" rev-parse HEAD 2>/dev/null || true)"
  printf 'successful_build\t%s\n' "$(date '+%Y-%m-%d %H:%M:%S')"
  printf 'compiler_signature\t%s\n' "$compiler_identity"
  printf 'native_objects_present\t%s\n' "$object_count"
  printf 'native_objects_recompiled\t%s\n' "$rebuilt"
  printf 'native_objects_reused\t%s\n' "$reused"
  printf 'test_executable\t%s\n' "$DEST"
  printf 'test_executable_sha256\t%s\n' "$built_sha"
  printf 'test_executable_published\t%s\n' "$published"
} > "$SUCCESS_STATE.pending"
mv -f "$SUCCESS_STATE.pending" "$SUCCESS_STATE"
echo "BUILD CACHE SUMMARY: rebuilt $rebuilt of $object_count local objects; reused $reused."
echo "BUILD CACHE INVENTORY: $OBJECT_INVENTORY"
echo "BUILD CACHE REBUILT FILES: $REBUILT_LIST"
echo "BUILD CACHE LAST SUCCESS: $SUCCESS_STATE"
if (( ${#REPAIR_NATIVE[@]} )); then
  # Only seal the repair after native build, link and TEST publication.
  printf 'repaired_source_revision=%s\n' "$(git -C "$PROJECT" rev-parse HEAD 2>/dev/null || true)" > "$REPAIR_MARK"
  echo "BUILD CACHE REPAIR: completed five-source recovery."
fi
echo "87.220 compiled TEST sha256: $built_sha"
echo "87.220 menu source asset checksums:"
sha256sum "$PROJECT/_art/logo.png" "$PROJECT/_art/sf2font.png" \
  "$PROJECT/_art/sf2fontsmall.png" "$PROJECT/_art/menuanimation.gif" \
  "$PROJECT/_art/stunbar_base.png"
echo "Stage 87.220: native sources dependency-cached; Stable and previous TESTs untouched."
