#!/usr/bin/env bash
# Stage 87.176 queued-input-shield TEST builder. Called ONLY by root BUILD_TEST.bat.
# Build the CURRENT local source in place using the warm FBNeo object cache.
# Never touch Stable executable, Stable manifests, user source, or old TEST binaries.
set -euo pipefail
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Permanent 87.176 build transcript. This starts BEFORE checking toolchains,
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
printf '\n========== XMVSF TEST 87.176 BUILD BEGIN %s ==========' "$(date '+%Y-%m-%d %H:%M:%S')"
printf '\nProject: %s\nLog: %s\n' "$PROJECT" "$LOG_FILE"
trap 'rc=$?; trap - EXIT; printf "XMVSF BUILD END rc=%s time=%s log=%s\n" "$rc" "$(date "+%Y-%m-%d %H:%M:%S")" "$LOG_FILE"; exit "$rc"' EXIT

TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_87_176.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_87_176_pending.exe"

if [[ ! -f "$TREE/makefile.sdl2" || ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h" ]]; then
  echo "ERROR: Current Stage 87.176 source was not pulled into $TREE" >&2
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
echo "XMVSF Stage 87.176 TEST: incremental build; preserving cached objects"
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
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 DEPEND=1 \
  -j"${XMVSF_BUILD_JOBS:-12}" all

BUILT=""
for exe in "$TREE/fbneo.exe" "$TREE/fbneo"; do
  if [[ -s "$exe" ]]; then BUILT="$exe"; break; fi
done
if [[ -z "$BUILT" ]]; then
  echo "ERROR: Compiler succeeded but did not produce fbneo executable." >&2
  exit 2
fi

# The original 87.167 source retired the forced-animation marker
# DHC_APPROVED_ROM_OUTGOING_ACTION in favor of the *native* controller-8
# handoff. Never require strings no longer present in compiled source.
# Emit the exact missing marker to the console, rather than reporting a
# generic "build execution failed" after a successful compile.
required_markers=(
  "DHC_ROM_TAG_3200_AFTER_CONTROLLER_ACCEPT"
  "DHC_NATIVE_EXIT_02_WATCHDOG_RECOVERY"
  "ROM_REASSERTED_RYU_HADOKEN_AFTER_TAG_3200"
  "DHC_NATIVE_TAG_CONTROLLER_08_ARMED"
  "NATIVE_CONTROLLER_ACCEPTED_OUTGOING_3200"
  "NATIVE_TAG_CONTROLLER_REJECTED_12F"
  "DHC_KEN_2A0C_ATTACK_SUPPRESSED_IMMEDIATE_2A08"
  "KEN_ROM_NONATTACKING_2A08_CONFIRMED"
  "KEN_ENTRY_ATTACK_REASSERTED_AFTER_CANCEL"
  "DHC_KEN_NATIVE_NEUTRAL_INPUT_ARMED"
  "DHC_KEN_NATIVE_QCF_INPUT_BEGIN"
  "PROOF_156_KEN_NATIVE_TWO_BUTTON_EDGE"
  "DHC_158_RYU_TIMER_28_151_ACCEPTED"
  "DHC_158_RESTORED_151_KEN_ARRIVAL_HANDOFF"
  "DHC_P1_OLD_PROJECTILE_HITBOX_DISABLED"
  "XM87.163 REPLAY_DIRECT_BOOT"
  "PLAY_READY: native state loaded"
  "PLAY_PROGRESS"
  "XM8764V2"
  "HOST_MENU_STATE_CHANGED"
  "REPLAY_DIVERGED"
  "V2_MANIFEST_SEALED"
  "V2_UNSEALED_PREFIX"
  "RECORD_SAVED total_frames="
  "V2_FINAL_TOTAL_MISMATCH"
  "movie_input_frame"
  "UNVERIFIED_ROM_2E_SCRIPT"
  "ROM_2E_SCRIPT_OBSERVED_COMMAND_UNVERIFIED"
  "EXCLUDE_VARIABLE_COMBINATION"
  "OUTGOING_RYU_PROJECTILE_CAPTURED"
  "KEN_POINT_SWAP_FREEZE_RYU_LINK_SET"
  "FAST_157_KEN_2A08_CANCEL_TO_VERIFIED_ROM_IDLE"
  "FAST_157_KEN_NATIVE_TWO_BUTTON_EDGE"
  "DHC_NATIVE_INPUT_DISPATCH_ACCEPTED"
  "XM87.175 OBSERVER SESSION"
  "DHC_PHYSICAL_CHORD_SHIELDED_DURING_NATIVE_HANDOFF"
  "POST_CPU_PRE_DHC"
  "POST_HOST_DHC_CHANGED"
  "DHC_NATIVE_INPUT_NO_SUPER_ROM_ACCEPT"
  "KEN_INPUT_DISPATCH_TIMEOUT_UNLOCKED"
  "XM87.153 TRAINING_OPTIONS_LOADED"
  "XM87.153 TRAINING_OPTIONS_SAVED"
  "XM87.153 TRAINING_PLAYER_SELECT_RESTORED"
  "ATTACK DURATION"
  "DURATION UNKNOWN"
  "DHC_KEN_ALTERNATE_ROM_SUPERS_REPPA_2E00_D6722_SHINRYUKEN_2E02_D6976"
  "DHC_152_ROM_2C04_WRONG_ACTION_ABORT_NO_ROLLBACK"
  "DHC_KEN_REPPA_2E00_ROM_SEEN"
  "DHC_KEN_SHINRYUKEN_2E02_ROM_SEEN"
  "PROOF_156_RYU_SUPER_TO_NATIVE_TAG_WAIT_NO_RAM_WRITES"
  "PROOF_156_NATIVE_HP_HK_TAG_INPUT_ARMED"
  "PROOF_156_ROM_HP_HK_NATIVE_3200_ACCEPTED"
  "PROOF_156_KEN_NATIVE_INPUT_READY_NO_KICK_OVERRIDES"
  "DHC_KEN_NATIVE_SUPER_ROM_ACTION_CONFIRMED"
  "DHC_ABORT_RELEASED_CHAIN_LOCK"
  "DHC_REARM_AFTER_NATIVE_SUPER_NEUTRAL"
  "DHC INPUT ATTEMPT"
  "TOURNAMENT_RULES_ACTIVE"
  "TOURNAMENT_RULES_INCOMPLETE"
  "MATCH-START QUEUE"
  "DHC QUEUED "
)
for marker in "${required_markers[@]}"; do
  if ! grep -aFq "$marker" "$BUILT"; then
    echo "ERROR: TEST executable is missing compiled 87.167 marker: $marker" >&2
    echo "Built file: $BUILT" >&2
    echo "Existing TEST and Stable binaries are preserved." >&2
    exit 2
  fi
done
mkdir -p "$PROJECT/runtime/stable"
# Only publish after the linked executable passed checks; replace the
# dedicated 87.167 TEST binary, never XMVSF_Stable.exe.
# cp exits nonzero on failure (set -e); Bash -s checks copied content
# exists and is nonempty. Do not depend on an uninstalled 'cmp' binary.
cp -p "$BUILT" "$PENDING"
if [[ ! -s "$PENDING" ]]; then
  echo "ERROR: Copied TEST binary is empty or missing." >&2
  exit 2
fi
mv -f "$PENDING" "$DEST"
echo "TEST BUILD READY: $DEST"
echo "Stage 87.176: isolated DHC physical input; 87.175 observer and F9 multi-session recording FINAL seal + normal-exit recovery; DHC, warm cache, and Stable preserved."
