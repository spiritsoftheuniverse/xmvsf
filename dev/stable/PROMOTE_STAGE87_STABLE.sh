#!/usr/bin/env bash
# Stage87.00: one-time promotion from last known Stage86.71/86.72 buildtree.
# NEVER writes to dev/stage77safe or runtime/stage77safe.
set -euo pipefail
P="${XMVSF_PROJECT:-$(cd "$(dirname "$0")/../.." && pwd)}"
OLD="$P/dev/stage77safe/buildtree"
OLD_RT="$P/runtime/stage77safe"
STAGED="$P/dev/stable/files/src"
STABLE="$P/dev/stable/buildtree"
WORK="$P/dev/stable/.stage87_promotion_incomplete"
OUT="$P/runtime/stable"
JOBS="${XMVSF_BUILD_JOBS:-8}"
for tool in python3 make cp mv rm mkdir find wc grep;do command -v "$tool" >/dev/null || { echo "ERROR: missing $tool" >&2;exit 1;};done
[[ ! -e "$STABLE" && ! -e "$WORK" ]] || { echo 'ERROR: stable worktree already exists or unfinished promotion found. Nothing changed.' >&2;exit 1; }
[[ -f "$OLD/makefile.sdl2" && -f "$OLD/.xm8641_global_lsb_started" ]] || { echo 'ERROR: previous guarded build tree missing' >&2;exit 1; }
[[ -d "$OLD/src" && -d "$OLD/obj" ]] || { echo 'ERROR: previous source/object tree missing' >&2;exit 1; }
[[ $(find "$OLD/obj" -name '*.o' -type f | wc -l) -ge 100 ]] || { echo 'ERROR: missing compiled object cache; refused incomplete baseline' >&2;exit 1; }
[[ -s "$STAGED/burn/drv/capcom/cps_obj.cpp" && -s "$STAGED/burner/sdl/xmvsf_stage23_ui.h" ]] || { echo 'ERROR: missing Stage87 packaged baseline sources' >&2;exit 1; }
[[ -f "$OLD/src/burn/drv/capcom/xmvsf_stage86_69_fx_index.h" ]] || { echo 'ERROR: Stage86.69 FX index missing' >&2;exit 1; }
[[ -f "$OLD/src/burn/drv/capcom/xmvsf_stage86_54_object_filter.h" ]] || { echo 'ERROR: protected Wily object filter missing' >&2;exit 1; }
[[ -f "$OLD/src/intf/video/sdl/xmvsf_stage86_49_painter.h" ]] || { echo 'ERROR: protected stage painter missing' >&2;exit 1; }
[[ -f "$OLD/src/burn/drv/capcom/xmvsf_stage86_64_strong_noir.h" ]] || { echo 'ERROR: protected grayscale and hue renderer missing' >&2;exit 1; }
python3 - "$OLD/src" "$STAGED" <<'PY_PREFLIGHT'
import hashlib,sys
from pathlib import Path
live,staged=(Path(x) for x in sys.argv[1:])
checks={
 'burn/drv/capcom/cps_obj.cpp':{'8c551be6801659909acfcfb1fc2cfba087b6c9c41fb6b24fe162bf0467ec7f99','7dec5692a6d47c47b5c23149f7e07f6972b8f6515e9643f766cbc134162d0c0b'},
 'burner/sdl/xmvsf_stage23_ui.h':{'50a50b4bf6e96f1fb1be3642f3a2a5c4aec00c17bdf3490e1e25d173cc8da65a','ffc2fbeac7a9a99ebb9509b9b5b458fa378c530871ca12880e9dee6100961918'},
}
for rel,expected in checks.items():
 f=live/rel;g=staged/rel
 if not f.is_file() or not g.is_file():raise SystemExit('ERROR: missing '+rel)
 got=hashlib.sha256(f.read_bytes().replace(b'\r\n',b'\n')).hexdigest()
 if got not in expected:raise SystemExit('ERROR: source differs from known Stage86.71/72 baseline: '+rel+' sha256='+got+'; no modification made')
exp={'burn/drv/capcom/cps_obj.cpp':'7dec5692a6d47c47b5c23149f7e07f6972b8f6515e9643f766cbc134162d0c0b',
     'burner/sdl/xmvsf_stage23_ui.h':'ffc2fbeac7a9a99ebb9509b9b5b458fa378c530871ca12880e9dee6100961918'}
for rel,h in exp.items():
 if hashlib.sha256((staged/rel).read_bytes()).hexdigest()!=h:
  raise SystemExit('ERROR: provided Stage87 baseline source changed: '+rel)
input_src=(live/'intf/input/sdl/inp_sdl2.cpp').read_text(errors='replace')
if not all(x in input_src for x in ('xm14ReadControllerFor(p)','xm23FilterInput(p,','xm17BlockSlot(p)')):
 raise SystemExit('ERROR: P1/P2 training ownership route changed; refusing unverified migration')
ui=(staged/'burner/sdl/xmvsf_stage23_ui.h').read_text()
obj=(staged/'burn/drv/capcom/cps_obj.cpp').read_text()
for marker in ('XM86.72 TRAINING_CONTROL_CHANGED','XM86.72 CONTROL_SIDE_LOG'):
 if marker not in ui+obj:raise SystemExit('ERROR: expected Stage86.72 marker missing: '+marker)
print('STAGE87 PRE-FLIGHT PASS: working source is Stage86.71 or Stage86.72, staged Stage86.72 trusted')
PY_PREFLIGHT
GOOD_EXE=''
for candidate in "$OLD_RT/XMVSF_Stage86_72_TrainingPlayerSelect.exe" "$OLD_RT/XMVSF_Stage86_71_FullSessionCapture.exe";do
 if [[ -s "$candidate" ]] && grep -aFq 'XM86.69 P1_FX_INDEX' "$candidate";then GOOD_EXE="$candidate";break;fi
done
[[ -n "$GOOD_EXE" ]] || { echo 'ERROR: need a compiled Stage86.71 or 86.72 executable. No changes made.' >&2;exit 1; }
echo "STAGE87: source $OLD is read-only, baseline executable $(basename "$GOOD_EXE")"
echo 'STAGE87: copying full build state into isolated dev/stable tree. Original stays untouched.'
SUCCESS=0
cleanup(){
 local rc=$?
 if [[ "$SUCCESS" != 1 ]];then
  echo 'STAGE87: promotion failed. Old Stage77 files untouched. Removing temporary build clone.' >&2
  if [[ -d "$WORK" ]];then rm -rf "$WORK";fi
 fi
 return "$rc"
}
trap cleanup EXIT
cp -a "$OLD" "$WORK"
# Reject aliases that would make the new worktree depend on the old one.
python3 - "$WORK" "$OLD" <<'PY_LINKS'
from pathlib import Path
import os,sys
root,old=map(Path,sys.argv[1:])
for parent,dirs,files in os.walk(root,followlinks=False):
 for name in dirs+files:
  p=Path(parent)/name
  if p.is_symlink() and str(p.resolve()).lower().startswith(str(old.resolve()).lower()):
   raise SystemExit('ERROR: symlink in cloned buildtree refers back to old tree: '+str(p))
print('STAGE87: clone does not contain a symlink back to Stage77 buildtree')
PY_LINKS
for rel in 'burn/drv/capcom/cps_obj.cpp' 'burner/sdl/xmvsf_stage23_ui.h';do
 cp "$STAGED/$rel" "$WORK/src/$rel"
done
# Force only the known header consumers and renderer to be rebuilt here.
for rel in 'burn/drv/capcom/cps_obj.o' 'burner/sdl/main.o' 'burner/sdl/run.o' 'intf/input/sdl/inp_sdl2.o';do rm -f "$WORK/obj/$rel";done
rm -f "$WORK/fbneo" "$WORK/fbneo.exe"
cd "$WORK"
MAKELEVEL=1 make -n -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 -j"$JOBS" all >"$P/dev/stable/stage87_make_preview.tmp" 2>&1 || { echo 'ERROR: make dry-run failed in isolated tree' >&2;exit 1; }
python3 - "$P/dev/stable/stage87_make_preview.tmp" <<'PY_AUDIT'
from pathlib import Path
import collections,re,shlex,sys
lines=Path(sys.argv[1]).read_text(errors='replace').splitlines()
required={'burn/drv/capcom/cps_obj.cpp','burner/sdl/main.cpp','burner/sdl/run.cpp','intf/input/sdl/inp_sdl2.cpp'}
optional={'burn/drv/capcom/cps_draw.cpp','intf/audio/sdl/aud_sdl.cpp','intf/video/sdl/vid_sdl2.cpp','burner/sdl/sdl2_gui.cpp','burner/sdl/sdl2_gui_ingame.cpp'}
allowed=required|optional;counts=collections.Counter()
for line in lines:
 if not re.search(r'(?<!\S)-c(?=\s|$)',line):continue
 try:
  parts=shlex.split(line);path=parts[parts.index('-c')+1].replace('\\','/')
 except (ValueError,IndexError):raise SystemExit('ERROR: unknown compile command '+line[:160])
 matched=[x for x in allowed if path==x or path.endswith('/'+x)]
 if len(matched)!=1:raise SystemExit('ERROR: unexpected compilation in promotion '+path)
 counts[matched[0]]+=1
if not required.issubset(counts) or any(v>2 for v in counts.values()):
 raise SystemExit('ERROR: unexpected compile set '+repr(dict(counts)))
print('STAGE87: compile-guard PASS',dict(counts))
PY_AUDIT
MAKELEVEL=1 make -f makefile.sdl2 LSB_FIRST=1 INCLUDE_7Z_SUPPORT=1 -j"$JOBS" all
BUILT=''
for exe in "$WORK/fbneo.exe" "$WORK/fbneo";do [[ -s "$exe" ]] && { BUILT="$exe";break;};done
[[ -n "$BUILT" ]] || { echo 'ERROR: isolated build produced no executable' >&2;exit 1; }
for marker in 'XM86.64 BG_30_PERCENT_70_SAT' 'XM86.64 WILY_DARK_SCENE' 'XM86.64 WILY_COMPOSITE' 'XM86.54 OBJ_FILTER' 'XM86.55 AUDIO_MENU_RETURN' 'XM86.65 SELECTION_LUMA' 'XM86.69 P1_FX_INDEX' 'XM86.72 TRAINING_CONTROL_CHANGED' 'XM86.72 CONTROL_SIDE_LOG';do
 grep -aFq "$marker" "$BUILT" || { echo "ERROR: expected compiled marker missing: $marker" >&2;exit 1; }
done
python3 - "$WORK" "$BUILT" "$P/dev/stable/STABLE_BASELINE.json" <<'PY_MANIFEST'
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,sys
root,exe,manifest=map(Path,sys.argv[1:])
rels=['burn/drv/capcom/cps_obj.cpp','burner/sdl/xmvsf_stage23_ui.h',
'burn/drv/capcom/xmvsf_stage86_54_object_filter.h',
'burn/drv/capcom/xmvsf_stage86_64_strong_noir.h',
'intf/video/sdl/xmvsf_stage86_49_painter.h',
'intf/input/sdl/inp_sdl2.cpp']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
data={'version':'87.00','baseline':'86.72 Training Player Select + 86.69 FX index + 86.71 unlimited capture',
'created_utc':datetime.now(timezone.utc).isoformat(),
'build_executable_sha256':sha(exe),'build_executable_size':exe.stat().st_size,
'source_sha256':{rel:sha(root/'src'/rel) for rel in rels},
'compile_verified':True,'gameplay_verified':False,
'worktree':'dev/stable/buildtree','runtime':'runtime/stable/XMVSF_Stable.exe'}
manifest.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
print('STAGE87: compiled executable SHA256',data['build_executable_sha256'])
PY_MANIFEST
# Commit working tree only once the full compile + binary guards have succeeded.
mv "$WORK" "$STABLE"
mkdir -p "$OUT"
cp -p "$STABLE/$(basename "$BUILT")" "$OUT/XMVSF_Stable.exe"
for dll in SDL2.dll SDL2_image.dll;do
 if [[ -f "$OLD_RT/$dll" ]];then cp -p "$OLD_RT/$dll" "$OUT/$dll"
 elif [[ -f "$P/FBNeo/$dll" ]];then cp -p "$P/FBNeo/$dll" "$OUT/$dll"
 else echo "WARNING: $dll unavailable to copy; verify DLL is available via PATH" >&2;fi
done
for cfg in XMVSF_DoubleFeature_FX_Families.cfg XMVSF_DoubleFeature_Scenery_Families.cfg;do
 [[ ! -f "$OLD_RT/$cfg" || -f "$OUT/$cfg" ]] || cp -p "$OLD_RT/$cfg" "$OUT/$cfg"
done
rm -f "$P/dev/stable/stage87_make_preview.tmp"
SUCCESS=1
bash "$P/dev/stable/VERIFY_STABLE.sh"
echo 'STAGE87 PROMOTION COMPLETE: dev/stable/buildtree + runtime/stable/XMVSF_Stable.exe'
echo 'NEXT: run runtime/stable/RUN_STABLE.bat. Stage77 remains unchanged and available for rollback.'
