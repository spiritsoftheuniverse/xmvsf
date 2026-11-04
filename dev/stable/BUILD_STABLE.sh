#!/usr/bin/env bash
# Standalone stable worktree builder. Does not read or write stage77safe.
set -euo pipefail
P="${XMVSF_PROJECT:-$(cd "$(dirname "$0")/../.." && pwd)}"
TREE="$P/dev/stable/buildtree"
OUT="$P/runtime/stable"
TARGET="$OUT/XMVSF_Stable.exe"
MANIFEST="$P/dev/stable/STABLE_BASELINE.json"
[[ -f "$TREE/makefile.sdl2" && -f "$MANIFEST" ]] || { echo 'ERROR: stable worktree not promoted' >&2;exit 1; }
[[ -d "$TREE/src" && -d "$TREE/obj" ]] || { echo 'ERROR: stable build tree incomplete' >&2;exit 1; }
# Preserve known-good executable and manifest until the new link passes all guards.
# Do not assign TMP/TEMP/TMPDIR: on Windows these are exported compiler scratch directory variables.
mkdir -p "$OUT"
PENDING_EXE="$OUT/.xm87_relink_pending.exe"
rm -f "${PENDING_EXE}"
trap 'rm -f "${PENDING_EXE}"' EXIT
cd "$TREE"
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 -j"${XMVSF_BUILD_JOBS:-8}" all
BUILT=''
for exe in "$TREE/fbneo.exe" "$TREE/fbneo";do [[ -s "$exe" ]] && { BUILT="$exe";break;};done
[[ -n "$BUILT" ]] || { echo 'ERROR: no compiled executable' >&2;exit 1; }
for marker in 'XM86.64 BG_30_PERCENT_70_SAT' 'XM86.64 WILY_DARK_SCENE' 'XM86.64 WILY_COMPOSITE' 'XM86.54 OBJ_FILTER' 'XM86.55 AUDIO_MENU_RETURN' 'XM86.65 SELECTION_LUMA' 'XM86.69 P1_FX_INDEX' 'XM86.72 TRAINING_CONTROL_CHANGED' 'XM86.72 CONTROL_SIDE_LOG';do
 grep -aFq "$marker" "$BUILT" || { echo "ERROR: missing accepted marker: $marker" >&2;exit 1; }
done
cp -p "$BUILT" "${PENDING_EXE}"
python3 - "$TREE" "${PENDING_EXE}" "$MANIFEST" <<'PY_UPDATE'
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,sys
root,exe,manifest=map(Path,sys.argv[1:])
old=json.loads(manifest.read_text());sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest()
old['build_executable_sha256']=sha(exe);old['build_executable_size']=exe.stat().st_size
old['created_utc']=datetime.now(timezone.utc).isoformat()
old['source_sha256']={rel:sha(root/'src'/rel) for rel in old['source_sha256']}
old['compile_verified']=True;old['gameplay_verified']=False
# Write manifest only after the binary has been checked; atomic publication below.
tmp=manifest.with_suffix('.new');tmp.write_text(json.dumps(old,indent=2)+'\n');tmp.replace(manifest)
PY_UPDATE
mv -f "${PENDING_EXE}" "$TARGET"
echo "STABLE BUILD COMPLETE: $TARGET"
echo 'Stage77safe was not accessed. Run dev/stable/VERIFY_STABLE.sh and smoke-test gameplay.'
