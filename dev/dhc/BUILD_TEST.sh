#!/usr/bin/env bash
# Stage 87.155 experimental TEST builder. Called ONLY by root BUILD_TEST.bat.
# Build the CURRENT local source in place using the warm FBNeo object cache.
# Never touch Stable executable, Stable manifests, user source, or old TEST binaries.
set -euo pipefail
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TREE="$PROJECT/dev/stable/buildtree"
DEST="$PROJECT/runtime/stable/XMVSF_Test_87_155.exe"
PENDING="$PROJECT/runtime/stable/.XMVSF_Test_87_155_pending.exe"

if [[ ! -f "$TREE/makefile.sdl2" || ! -f "$TREE/src/burn/drv/capcom/xmvsf_stage87_144_native_handoff.h" ]]; then
  echo "ERROR: Current Stage 87.155 source was not pulled into $TREE" >&2
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
echo "XMVSF Stage 87.155 TEST: compiling current local buildtree with warm cache"
# This FBNeo makefile is a two-pass build: MAKELEVEL=1 compiles objects,
# and the nested make (level 2) links fbneo. The old invocation could report
# 'Nothing to be done' without checking newly pulled DHC source reliably.
# Treat ONLY the four changed translation units as newer for this TEST.
# -W does not alter source timestamps, files or the existing object cache.
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 \
  -j"${XMVSF_BUILD_JOBS:-12}" \
  -W src/burn/drv/capcom/cps_run.cpp \
  -W src/burner/sdl/main.cpp \
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

# The original 87.155 source retired the forced-animation marker
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
  "DHC_KEN_NATIVE_THREE_BUTTON_EDGE"
  "DHC_NATIVE_INPUT_DISPATCH_ACCEPTED"
  "DHC_NATIVE_INPUT_NO_SUPER_ROM_ACCEPT"
  "KEN_INPUT_DISPATCH_TIMEOUT_UNLOCKED"
  "XM87.153 TRAINING_OPTIONS_LOADED"
  "XM87.153 TRAINING_OPTIONS_SAVED"
  "XM87.153 TRAINING_PLAYER_SELECT_RESTORED"
  "ATTACK DURATION"
  "DURATION UNKNOWN"
  "DHC_DUAL_KEN_TARGETS_REPPA_2E00_D6722_SHINRYUKEN_2E02_D6976"
  "DHC_152_ROM_2C04_WRONG_ACTION_ABORT_NO_ROLLBACK"
  "DHC_KEN_REPPA_2E00_ROM_SEEN"
  "DHC_KEN_SHINRYUKEN_2E02_ROM_SEEN"
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
    echo "ERROR: TEST executable is missing compiled 87.155 marker: $marker" >&2
    echo "Built file: $BUILT" >&2
    echo "Existing TEST and Stable binaries are preserved." >&2
    exit 2
  fi
done
mkdir -p "$PROJECT/runtime/stable"
# Only publish after the linked executable passed checks; replace the
# dedicated 87.155 TEST binary, never XMVSF_Stable.exe.
# cp exits nonzero on failure (set -e); Bash -s checks copied content
# exists and is nonempty. Do not depend on an uninstalled 'cmp' binary.
cp -p "$BUILT" "$PENDING"
if [[ ! -s "$PENDING" ]]; then
  echo "ERROR: Copied TEST binary is empty or missing." >&2
  exit 2
fi
mv -f "$PENDING" "$DEST"
echo "TEST BUILD READY: $DEST"
echo "Stage 87.155: both Ken supers selectable (QCF+3P/3K), dual native ROM addresses logged. Ryu 2C04 regression guarded. Stable untouched."
