#!/usr/bin/env bash
# Stage 87.235 native TEST: retained native game + real logo/glyph assets. Called ONLY by root BUILD_TEST.bat.
# Build the CURRENT local source in place using the warm FBNeo object cache.
# Never touch Stable executable, Stable manifests, user source, or old TEST binaries.
set -euo pipefail
BUILD_WALL_START=$SECONDS
# Batch sets the version once at launch, before native compilation begins.
XMVSF_BUILD_VERSION="${XMVSF_TEST_VERSION:-87.344}"
XMVSF_BUILD_TAG="${XMVSF_BUILD_VERSION//./_}"
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
printf '\n========== XMVSF TEST %s BUILD BEGIN %s ==========' "$XMVSF_BUILD_VERSION" "$(date '+%Y-%m-%d %H:%M:%S')"
printf '\nProject: %s\nLog: %s\n' "$PROJECT" "$LOG_FILE"
trap 'rc=$?; trap - EXIT; printf "XMVSF BUILD END rc=%s elapsed_seconds=%s time=%s log=%s\n" "$rc" "$((SECONDS-BUILD_WALL_START))" "$(date "+%Y-%m-%d %H:%M:%S")" "$LOG_FILE"; exit "$rc"' EXIT

# Guile PSD source and all exported layers are validated by the
# extractor's own SHA256 manifest. Checking on EVERY invocation avoids a
# stale output when users replace/collapse layers or delete just PNGs
# while leaving manifest.tsv behind. Unchanged PSDs are an immediate cache
# hit: Python hashes the source and exits without decoding any pixels.
GUILE_PSD="$PROJECT/_art/guilestage/guilestage.psd"
GUILE_EXTRACT_ARGS=()
case "${XMVSF_GUILE_ACTOR_ONLY:-}" in
  "") ;;
  Joe|joe|JOE)
    GUILE_EXTRACT_ARGS=(--actor Joe)
    ;;
  *)
    echo "ERROR: Unsupported Guile actor-only build: ${XMVSF_GUILE_ACTOR_ONLY}. Only Joe is enabled." >&2
    exit 2
    ;;
esac
if (( ${#GUILE_EXTRACT_ARGS[@]} )) && [[ ! -s "$GUILE_PSD" ]]; then
  echo "ERROR: Joe-only extraction requested but Guile PSD is missing or empty: $GUILE_PSD" >&2
  exit 2
fi
if [[ -s "$GUILE_PSD" ]]; then
  if (( ${#GUILE_EXTRACT_ARGS[@]} )); then
    echo "GUILE PSD: Joe-only extraction; preserving all other exported PNGs."
  else
    echo "GUILE PSD: verifying cached layer extraction (source SHA + PNG inventory)."
  fi
  if command -v python3 >/dev/null 2>&1; then
    python3 "$PROJECT/tools/extract_guile_stage.py" "${GUILE_EXTRACT_ARGS[@]}"
  elif command -v python >/dev/null 2>&1; then
    python "$PROJECT/tools/extract_guile_stage.py" "${GUILE_EXTRACT_ARGS[@]}"
  elif command -v py >/dev/null 2>&1; then
    py -3 "$PROJECT/tools/extract_guile_stage.py" "${GUILE_EXTRACT_ARGS[@]}"
  else
    echo "ERROR: Python 3 required for Guile PSD layer extraction." >&2
    exit 2
  fi
fi


# Ryu PSD extraction is source-hash cached (no rebuild or clean when
# the PSD did not change). Layer rectangles outside 768x471 survive verbatim.
RYU_PSD="$PROJECT/_art/ryustage/ryustage.psd"
if [[ -s "$RYU_PSD" ]]; then
  if command -v python3 >/dev/null 2>&1; then
    python3 "$PROJECT/tools/extract_ryu_stage.py"
  elif command -v python >/dev/null 2>&1; then
    python "$PROJECT/tools/extract_ryu_stage.py"
  elif command -v py >/dev/null 2>&1; then
    py -3 "$PROJECT/tools/extract_ryu_stage.py"
  else
    echo "ERROR: Python 3 required for Ryu PSD extraction." >&2
    exit 2
  fi
fi

TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_${XMVSF_BUILD_TAG}.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_${XMVSF_BUILD_TAG}_pending.exe"
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
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_235_menu_intro_timeline.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_235_ii_strikes.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_206_crash_report.h" ||
      ! -s "$TREE/src/intf/audio/sdl/xm20_wasapi_impl.h" ||
      ! -s "$TREE/src/burn/drv/capcom/xmvsf_stage87_33_dizzy_hud.h" ||
      ! -s "$TREE/src/burn/drv/capcom/xmvsf_stage87_121_cyclops_release.h" ||
      ! -s "$PROJECT/dev/dhc/tests/test_stun_and_cyclops_219.cpp" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage87_217_native_float.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_menu_glyph_atlas.h" ||
      ! -s "$TREE/src/burner/sdl/xmvsf_stage14_boot.h" ]]; then
  echo "ERROR: 87.235 required native source missing or empty:" >&2
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
for asset in logo.png logo_II.png logo_II_A.png logo_II_B.png logo_II_C.png logo_II_D.png sf2font.png sf2fontsmall.png menuanimation.gif stunbar_base.png; do
  if [[ ! -s "$PROJECT/_art/$asset" ]]; then
    echo "ERROR: 87.235 requires original nonempty _art/$asset" >&2
    exit 2
  fi
done
if ! grep -Fq 'xmvsf_menu_glyph_atlas.h' "$TREE/src/burner/sdl/xmvsf_stage14_boot.h"; then
  echo "ERROR: 87.235 glyph header is not wired into the native boot menu" >&2
  exit 2
fi
# Do not gate an otherwise valid build on a particular draw helper name.
# Stage 87.235 uses menuLogo(render), which superseded logo(render).
# The C++ compiler checks the actual call and linking; the runtime atlas
# reports whether the title PNG loaded successfully.
echo "87.235 menu preflight: native boot and glyph/FX sources present; logo, fonts and menuanimation.gif verified."
echo "87.235 draw path is verified by native compile and runtime, not a stale grep for XMGlyph::logo(render)."
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
    # Only compiler settings and the actual build rules affect the hash.
    # Do not spoof a Makefile fingerprint: its incorrect indexed replacement
    # previously caused a 1,070-dependency, 688-driver partial rebuild.
    sha256sum "$TREE/makefile.sdl2"
    sha256sum "$TREE/makefile.burn_rules"
  } | sha256sum | cut -d ' ' -f 1
)"
CACHE_SIGNATURE_UNCHANGED=1
if [[ ! -s "$SIGNATURE" ]]; then
  CACHE_SIGNATURE_UNCHANGED=0
  printf '%s\n' "$compiler_identity" > "$SIGNATURE.pending"
  mv -f "$SIGNATURE.pending" "$SIGNATURE"
  # Bootstrap pre-existing warm objects without a needless full rebuild.
  # Native .o/.d mtime checks still work, as before this improvement.
  touch -t 200001010000 "$SIGNATURE"
  echo "BUILD CACHE: initialized fingerprint; adopting warm Make objects."
elif [[ "$(cat "$SIGNATURE")" != "$compiler_identity" ]]; then
  CACHE_SIGNATURE_UNCHANGED=0
  # Recover ONLY the known failed 87.295 source-index fingerprint when
  # reverting to the exact FBNeo rules from the last SUCCESSFUL build.
  # Those makefile edits changed discovery, not compilation flags. Objects
  # from 87.292 (plus completed failed-run objects) may safely be reused.
  # Any other signature change retains normal conservative invalidation.
  restore_failed_index=0
  if [[ -s "$SUCCESS_STATE" && -s "$TREE/fbneo.exe" &&
        "$(cat "$SIGNATURE")" == "2b40e5c1311345c24d7ea66ef6cb71ffc44a72688ab76f2fe8304244b2741b40" &&
        "$(sed -n 's/^compiler_signature[[:space:]]*//p' "$SUCCESS_STATE" | tail -n 1)" == "$compiler_identity" &&
        "$(git -C "$PROJECT" rev-parse HEAD:dev/stable/buildtree/makefile.sdl2 2>/dev/null || true)" == "cbf6f841d1b4abb940f48b81ca43fa595d07a9b5" ]] &&
     git -c core.safecrlf=false -C "$PROJECT" diff --quiet HEAD -- dev/stable/buildtree/makefile.sdl2; then
    restore_failed_index=1
  fi
  printf '%s\n' "$compiler_identity" > "$SIGNATURE.pending"
  mv -f "$SIGNATURE.pending" "$SIGNATURE"
  if (( restore_failed_index )); then
    touch -t 200001010000 "$SIGNATURE"
    CACHE_SIGNATURE_UNCHANGED=1
    echo "BUILD CACHE RECOVERY: restored last-successful compiler signature; preserving warm objects."
  elif (( ${#REPAIR_NATIVE[@]} )); then
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
  bash "$PROJECT/dev/dhc/tests/test_fast_menu_link_inputs_235.sh"
fi

# No-op fast path: Git's *unchanged tracked source* plus our last
# successful native TEST record are more authoritative than running the
# enormous FBNeo Make graph again when there is literally nothing to do.
# The tracked BUILD_TEST.log is intentionally excluded: this script appends
# to it even on success. All real source, asset, build-script and config
# edits invalidate this shortcut. Do not use this for changed revisions,
# failed builds, missing artifacts or compiler/signature changes.
if [[ "${XMVSF_FORCE_NATIVE:-0}" != 1 &&
      "${XMVSF_CACHE_SMOKE:-0}" != 1 &&
      "$CACHE_SIGNATURE_UNCHANGED" == 1 &&
      ${#REPAIR_NATIVE[@]} -eq 0 &&
      -s "$SUCCESS_STATE" && -s "$DEST" && -s "$TREE/fbneo.exe" ]] &&
   command -v git >/dev/null 2>&1; then
  cached_rev="$(sed -n 's/^source_revision[[:space:]]*//p' "$SUCCESS_STATE" | tail -n 1)"
  cached_sig="$(sed -n 's/^compiler_signature[[:space:]]*//p' "$SUCCESS_STATE" | tail -n 1)"
  cached_sha="$(sed -n 's/^test_executable_sha256[[:space:]]*//p' "$SUCCESS_STATE" | tail -n 1)"
  current_rev="$(git -C "$PROJECT" rev-parse HEAD 2>/dev/null || true)"
  # Exclude only this tracked append-only transcript. This still detects
  # edited headers, source, makefiles, tests, scripts and original PNG/WAVs.
  if [[ -n "$cached_rev" && "$cached_rev" == "$current_rev" &&
        "$cached_sig" == "$compiler_identity" &&
        "${#cached_sha}" == 64 ]] &&
     git -c core.safecrlf=false -C "$PROJECT" diff --quiet HEAD -- . ':(exclude)test/build_logs/BUILD_TEST.log' &&
     [[ -z "$(git -C "$PROJECT" ls-files --others --exclude-standard                   '*.cpp' '*.c' '*.h' '*.hpp' '*.cc' '*.mk' '*.asm' | head -n 1)" ]]; then
    native_sha="$(sha256sum "$TREE/fbneo.exe" | cut -d ' ' -f 1)"
    test_sha="$(sha256sum "$DEST" | cut -d ' ' -f 1)"
    if [[ "$native_sha" == "$cached_sha" && "$test_sha" == "$cached_sha" ]]; then
      echo "BUILD CACHE FAST HIT: source=$current_rev compiler=$compiler_identity"
      echo "BUILD CACHE FAST HIT: 0 native objects changed; existing TEST SHA256=$test_sha"
      echo "BUILD CACHE FAST HIT: skipped 1,174-object Make traversal, linking and cached tests."
      echo "TEST PUBLISH: identical executable, previous TEST file untouched."
      echo "TEST BUILD READY: $DEST"
      echo "BUILD PHASE no_op_fast_hit elapsed_seconds=$((SECONDS-BUILD_WALL_START))"
      exit 0
    fi
  fi
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

if [[ "${XMVSF_RUN_REGRESSIONS:-0}" == "1" ]]; then
BUILD_TESTS_START=$SECONDS
echo "BUILD PHASE full_regression_start elapsed_seconds=$((SECONDS-BUILD_WALL_START))"
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
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_flying_screen_dash_exit_221.cpp" \
  "$CACHE_DIR/unit_tests/test_flying_screen_dash_exit_221.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_classic_transition_222.cpp" \
  "$CACHE_DIR/unit_tests/test_classic_transition_222.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_storm_flying_screen_223.cpp" \
  "$CACHE_DIR/unit_tests/test_storm_flying_screen_223.exe"
# TEST 87.235 replaces the old Charlie 1.5x experiment.
# Keep the archived 87.224 test file intact for historical comparison.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_charlie_severe_hk_226.cpp" \
  "$CACHE_DIR/unit_tests/test_charlie_severe_hk_226.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_flying_screen_dash_reentry_226.cpp" \
  "$CACHE_DIR/unit_tests/test_flying_screen_dash_reentry_226.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_flying_screen_dash_controller_227.cpp" \
  "$CACHE_DIR/unit_tests/test_flying_screen_dash_controller_227.exe"
# 87.235: the original decrypted Motorola 68000 source already
# exists under local_rom_dump. Decode only small windows around the REAL
# executed X/mode writer PCs. Diagnostic failure must never gate the build.
if command -v python3 >/dev/null 2>&1; then
  if ! python3 "$PROJECT/dev/dhc/tools/ANALYZE_FS_ENTRY_68K_232.py" \
     --output "$PROJECT/test/87.235/observer/fs_entry_68000_opcode_windows.tsv"; then
    echo "WARNING: 87.235 68000 source window analysis unavailable; native build continues." >&2
  fi
elif command -v python >/dev/null 2>&1; then
  if ! python "$PROJECT/dev/dhc/tools/ANALYZE_FS_ENTRY_68K_232.py" \
     --output "$PROJECT/test/87.235/observer/fs_entry_68000_opcode_windows.tsv"; then
    echo "WARNING: 87.235 68000 source window analysis unavailable; native build continues." >&2
  fi
fi
# 87.235 native 68k offscreen-entry and no-delay voluntary-dash isolation.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_fs_native_offscreen_entry_232.cpp" \
  "$CACHE_DIR/unit_tests/test_fs_native_offscreen_entry_232.exe"
# TEST 87.235: airborne offscreen is grounded BEFORE native ground dash
# dispatch, and the 87.232 original 1400/1600 controller handles entry.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_fs_airborne_offscreen_entry_233.cpp" \
  "$CACHE_DIR/unit_tests/test_fs_airborne_offscreen_entry_233.exe"
# TEST 87.293: native attack duration stays live across hit-confirm
# ticks; both player meters resume after hitstop; native stun gradient
# is horizontal in ten equal-value bands at 50/60 capacity.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_hit_clock_240.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_hit_clock_240.exe"
# TEST 87.293: a new native attack selector may be accepted while
# box70/72 remains high through a combo; it must create a separate card.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_combo_244.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_combo_244.exe"
# TEST 87.293: real 87.243 native trace reproducer - 43 callbacks of
# 2E stage02 phase81 cinematic, then 52 native 2E stage04 steps while
# defender +85 reloads; attacker must not be globally stopped.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_asymmetric_stop_245.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_asymmetric_stop_245.exe"
# TEST 87.293: the exact 87.293 trace defect - an accepted ongoing
# 2E super must never produce a second card because a button was pressed
# and the beam's linked projectile changed at host 1397 / 1428.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_super_identity_246.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_super_identity_246.exe"
# TEST 87.293: special 2C36 / 2C12 native 44F animation holds,
# independent fighter movement and native-only combo acquisition.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_native_stall_247.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_native_stall_247.exe"
# TEST 87.293: missing-box confirmed combo hit, later 2C stage04
# 44F stasis and stale melee boxes that outlive native action exit.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_combo_stasis_250.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_combo_stasis_250.exe"
# TEST 87.293: delayed Cyclops 2E beam, overwritten input protection,
# linked-projectile damage fallback and old-projectile attribution guard.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_projectile_super_251.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_projectile_super_251.exe"
# TEST 87.293: normalize linked-2E supers, aimed beams, post-exit
# projectile hits, Ken-style melee contacts and early frozen red ticks.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_super_universal_252.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_super_universal_252.exe"
# TEST 87.293: QCD+PP command, min-HP red hit proofs and a display
# axis that darkens the hitstop ticks on BOTH frame-data lanes.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_hitstop_visual_253.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_hitstop_visual_253.exe"
# TEST 87.293: immediate ROM-accepted super cards (including QCD 2E08),
# fixed 1px/tick overflow rows, no portrait delay or old card rescaling.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_native_entry_wrap_254.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_native_entry_wrap_254.exe"
# TEST 87.293: Xbox/DS4Windows boot controller is an explicit P1 source.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_controller_session_249.cpp" \
  "$CACHE_DIR/unit_tests/test_controller_session_249.exe"
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_stun_horizontal_240.cpp" \
  "$CACHE_DIR/unit_tests/test_stun_horizontal_240.exe"
# 87.293: real native-clock super freeze and long 2E super attack-phase
# continuity, with normal and projectile non-regression.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_frame_data_super_clock_239.cpp" \
  "$CACHE_DIR/unit_tests/test_frame_data_super_clock_239.exe"
# 87.235: this unit now guards against delayed severe knockback spilling
# from HK into later LP/LK/MP (87.228 frame 8193 / 8862 / 10055 / 10327).
# 87.235 replaces the broader 87.228/229 native relaunch rule.
# Keep 87.230's old permissive (no Storm) unit in history; 87.235
# now asserts Storm on every confirmed FS normal, severe only for two cases.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_fs_storm_baseline_231.cpp" \
  "$CACHE_DIR/unit_tests/test_fs_storm_baseline_231.exe"
# 87.235 replaces the 87.224/225 Charlie multiplier regression with the
# new native-HP / severe-HK and repeated-dash-reentry expectations.

# 87.235: native C++ timeline covers 1000/3500/500ms and input-gate edges.
run_cached_unit \
  "$PROJECT/dev/dhc/tests/test_menu_intro_timeline_235.cpp" \
  "$CACHE_DIR/unit_tests/test_menu_intro_timeline_235.exe"
# 87.235: confirm actual integration with authored repo WAVs and input gate.
if command -v python3 >/dev/null 2>&1; then
  python3 "$PROJECT/dev/dhc/tests/test_menu_intro_integration_235.py"
elif command -v python >/dev/null 2>&1; then
  python "$PROJECT/dev/dhc/tests/test_menu_intro_integration_235.py"
else
  echo "WARNING: 87.235 cold boot source test skipped (Python unavailable)." >&2
fi
# 87.235: source-level regression of the exact native SDL call order.
# No GPU/ROM or test executable required; run before native compile.
if command -v python3 >/dev/null 2>&1; then
  python3 "$PROJECT/dev/dhc/tests/test_boot_renderer_viewport_234.py"
elif command -v python >/dev/null 2>&1; then
  python "$PROJECT/dev/dhc/tests/test_boot_renderer_viewport_234.py"
else
  echo "WARNING: 87.235 centered boot viewport source test skipped (Python unavailable)." >&2
fi
# TEST 87.293: source integration prevents a repeat of the one-frame
# overlay lag or counting frozen +85 host ticks as gameplay steps.
if command -v python3 >/dev/null 2>&1; then
  python3 "$PROJECT/dev/dhc/tests/test_frame_data_paint_order_243.py"
elif command -v python >/dev/null 2>&1; then
  python "$PROJECT/dev/dhc/tests/test_frame_data_paint_order_243.py"
else
  echo "WARNING: 87.293 frame-data source timing regression skipped (Python unavailable)." >&2
fi
# TEST 87.293: prevent one-time controller enumeration, stale SDL
# instance handles and infinite menu-confirm lockout from regressing.
if command -v python3 >/dev/null 2>&1; then
  python3 "$PROJECT/dev/dhc/tests/test_controller_reconnect_248.py"
elif command -v python >/dev/null 2>&1; then
  python "$PROJECT/dev/dhc/tests/test_controller_reconnect_248.py"
else
  echo "WARNING: 87.293 controller reconnection source test skipped (Python unavailable)." >&2
fi
# TEST 87.293: ensure menu-confirmed controller virtual routing remains
# active even when the user's saved P1 profile is unassigned.
if command -v python3 >/dev/null 2>&1; then
  python3 "$PROJECT/dev/dhc/tests/test_controller_session_249.py"
elif command -v python >/dev/null 2>&1; then
  python "$PROJECT/dev/dhc/tests/test_controller_session_249.py"
else
  echo "WARNING: 87.293 controller session integration check skipped (no Python)." >&2
fi
echo "BUILD PHASE full_regression_end duration_seconds=$((SECONDS-BUILD_TESTS_START))"
else
  echo "BUILD: optional regression suite skipped; proceeding to native incremental Make."
fi
# Preserve warm cache and anything already built on failures. Never run make clean.
cd "$TREE"
echo "XMVSF Stage $XMVSF_BUILD_VERSION TEST: incremental build; preserving cached objects"
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
# XMVSF 87.235: NO broad -W, no clean, and no seven-unit FRESHEN.
# Native .d files specify exact transitive source/header dependencies.
# A compiler/signature change makes only signature-dependent objects stale.
# With an unchanged signature and unchanged files Make compiles NOTHING.
BUILD_NATIVE_START=$SECONDS
echo "BUILD PHASE native_make_start elapsed_seconds=$((SECONDS-BUILD_WALL_START))"
touch "$BUILD_START"
# One-time in-memory invalidation for five known uncompiled units only.
# No source or object file timestamps are changed manually.
REPAIR_ARGS=()
for src in "${REPAIR_NATIVE[@]}"; do REPAIR_ARGS+=(-W "$src"); done
# On a warm, already successful FBNeo cache, a menu-only change does
# not require Make's costly pass-1 traversal of all arcade drivers.
# The pass-2 graph already includes cached .d files, and GNU Make remakes
# affected .d files on changed source/headers BEFORE compiling dependents.
# This is deliberately NOT used for engine/driver/generated-rule changes.
# Set XMVSF_WARM_MENU_SINGLE_PASS=0 for the historical two-pass path.
WARM_MENU_PASS=0
if [[ "${XMVSF_WARM_MENU_SINGLE_PASS:-1}" == 1 &&
      "$CACHE_SIGNATURE_UNCHANGED" == 1 &&
      ${#REPAIR_NATIVE[@]} -eq 0 && -s "$SUCCESS_STATE" &&
      -s "$TREE/obj/burner/sdl/main.d" &&
      -s "$TREE/obj/burner/sdl/run.d" &&
      -s "$TREE/obj/intf/audio/sdl/aud_sdl.d" &&
      -s "$TREE/obj/intf/video/sdl/vid_sdl2.d" &&
      -s "$TREE/src/dep/generated/driverlist.h" ]] &&
   command -v git >/dev/null 2>&1; then
  last_native_rev="$(sed -n 's/^source_revision[[:space:]]*//p' "$SUCCESS_STATE" | tail -n 1)"
  last_native_sig="$(sed -n 's/^compiler_signature[[:space:]]*//p' "$SUCCESS_STATE" | tail -n 1)"
  if [[ -n "$last_native_rev" && "$last_native_sig" == "$compiler_identity" ]] &&
     git -C "$PROJECT" cat-file -e "${last_native_rev}^{commit}" 2>/dev/null; then
    # Include both committed changes since the last native build and local
    # tracked edits. Exclude the tracked append-only build transcript.
    changed_paths="$( { git -c core.safecrlf=false -C "$PROJECT" diff --name-only "$last_native_rev" HEAD --; git -c core.safecrlf=false -C "$PROJECT" diff --name-only HEAD --; } | sed '/^test\/build_logs\/BUILD_TEST[.]log$/d' | sort -u )"
    if [[ -n "$changed_paths" ]]; then
      WARM_MENU_PASS=1
      while IFS= read -r changed; do
        case "$changed" in
          _art/*|audio/*|tools/extract_guile_stage.py|tools/extract_ryu_stage.py|FBNeo/config/games/*|FBNeo/*.log|runtime/stable/XMVSF_Test_*.exe|runtime/stable/xmvsfu_hit*|dev/stable/buildtree/fbneo.exe|dev/dhc/tests/*|dev/dhc/MENU_INTRO_AUDIO_REVEAL_87_235.md|dev/stable/buildtree/src/burner/sdl/xmvsf_menu_*|dev/stable/buildtree/src/burner/sdl/xmvsf_stage14_boot.h|dev/stable/buildtree/src/burner/sdl/xmvsf_stage87_195_menu_gif.h|dev/stable/buildtree/src/burner/sdl/xmvsf_stage87_235_*|dev/stable/buildtree/src/burner/sdl/xmvsf_stage20_api.h|dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage87_290_guile.h|dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage87_323_ryu.h|dev/stable/buildtree/src/intf/audio/sdl/xm20_wasapi_impl.h|dev/dhc/BUILD_TEST.sh|dev/dhc/FAST_MENU_LINK_INPUTS_235.sh|dev/dhc/*.md|BUILD_TEST.bat|RUN_TEST.bat|RUN_REPLAY.bat)
            ;;
          *) WARM_MENU_PASS=0; break ;;
        esac
      done <<< "$changed_paths"
      # Never take the shortened dependency pass if there are new native sources.
      untracked_sources="$(git -C "$PROJECT" ls-files --others --exclude-standard '*.cpp' '*.c' '*.h' '*.hpp' '*.cc' '*.mk' '*.asm')"
      if [[ -n "$untracked_sources" ]]; then WARM_MENU_PASS=0; fi
    fi
  fi
fi
if (( WARM_MENU_PASS )); then
  # The ordinary pass-2 still constructs FBNeo's full Make wildcard/vpath
  # graph. Reuse the linker-map object's EXACT ordered list instead, and
  # include only the four SDL .d files affected by menu/boot/audio edits.
  # This is still REAL dependency-aware Make: changed source and headers
  # regenerate their .d and .o, and all link inputs remain present.
  # No driver or CPS2 source can use this route (strict Git whitelist above).
  if [[ "${XMVSF_COMPACT_MENU_MAKE:-1}" == 1 &&
        -s "$PROJECT/dev/dhc/FAST_MENU_LINK_INPUTS_235.sh" ]] &&
     source "$PROJECT/dev/dhc/FAST_MENU_LINK_INPUTS_235.sh" &&
     xm235_fast_link_inputs "$TREE"; then
    echo "BUILD MODE: compact menu graph, $XM235_LINK_OBJECT_COUNT cached link objects."
    echo "BUILD MODE: skipping FBNeo driver wildcard scan; refreshing 4 SDL .d files."
    echo "BUILD PHASE compact_make_start elapsed_seconds=$((SECONDS-BUILD_WALL_START))"
    compact_args=(LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1
      "XMVSF_BUILD_SIGNATURE=../../../test/build_cache/compiler.signature"
      "allobj=$XM235_LINK_OBJECTS" "alldep=$XM235_LINK_DEPFILES"
      "autdrv=" "autdep=" "drvdep=" "drvsrc=" "autobj="
      "${MAKE_COMPILER[@]}" "-j${XMVSF_BUILD_JOBS:-12}")
    # GNU Make's pass-2 checks existing .d header lists but does not always
    # regenerate them when a header adds another include. Explicitly rebuild
    # the FOUR SDL .d files in a tiny pass-1 invocation, not every driver.
    # -W affects only these dependency-scans, never native object targets.
    compact_deps_ready=0
    if MAKELEVEL=1 make -rR -f makefile.sdl2 "${compact_args[@]}" \
         -W src/burner/sdl/main.cpp -W src/burner/sdl/run.cpp \
         -W src/intf/audio/sdl/aud_sdl.cpp \
         -W src/intf/video/sdl/vid_sdl2.cpp \
         main.d run.d aud_sdl.d vid_sdl2.d; then
      compact_deps_ready=1
      for dep in ${XM235_LINK_DEPFILES}; do
        if [[ ! -s "$TREE/$dep" || "$TREE/$dep" -ot "$BUILD_START" ]]; then
          compact_deps_ready=0
          echo "BUILD CACHE: SDL dependency refresh missing/stale: $dep" >&2
        fi
      done
    fi
    if (( compact_deps_ready )); then
      echo "BUILD MODE: compact SDL deps refreshed; compiling/linking from cached object graph."
      MAKELEVEL=2 make -rR -f makefile.sdl2 "${compact_args[@]}" all
    else
      echo "BUILD MODE: compact dependency scan unavailable; using full two-pass Make."
      MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1 \
        "XMVSF_BUILD_SIGNATURE=../../../test/build_cache/compiler.signature" \
        "${MAKE_COMPILER[@]}" -j"${XMVSF_BUILD_JOBS:-12}" all
    fi
    echo "BUILD PHASE compact_make_end elapsed_seconds=$((SECONDS-BUILD_WALL_START))"
  else
    echo "BUILD MODE: cached linker object map unavailable; using normal warm Make."
    MAKELEVEL=2 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1 \
      "XMVSF_BUILD_SIGNATURE=../../../test/build_cache/compiler.signature" \
      "${MAKE_COMPILER[@]}" -j"${XMVSF_BUILD_JOBS:-12}" all
  fi
else
  echo "BUILD MODE: standard dependency-safe two-pass native Make."
  MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1 \
    "XMVSF_BUILD_SIGNATURE=../../../test/build_cache/compiler.signature" \
    "${MAKE_COMPILER[@]}" "${REPAIR_ARGS[@]}" -j"${XMVSF_BUILD_JOBS:-12}" all
fi
echo "BUILD PHASE native_make_end duration_seconds=$((SECONDS-BUILD_NATIVE_START))"
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

echo "BUILD PHASE publish_complete duration_seconds=$((SECONDS-BUILD_NATIVE_START))"
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
echo "$XMVSF_BUILD_VERSION compiled TEST sha256: $built_sha"
echo "87.235 menu source asset checksums:"
sha256sum "$PROJECT/_art/logo.png" "$PROJECT/_art/sf2font.png" \
  "$PROJECT/_art/sf2fontsmall.png" "$PROJECT/_art/menuanimation.gif" \
  "$PROJECT/_art/stunbar_base.png"
echo "Stage $XMVSF_BUILD_VERSION: native sources dependency-cached; Stable and previous TESTs untouched."
