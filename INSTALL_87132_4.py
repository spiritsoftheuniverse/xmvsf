#!/usr/bin/env python3
"""One-time consolidated installation. Backs up live files; no SHA revision list."""
from pathlib import Path
import os,sys,shutil,re
from datetime import datetime
PKG=Path(__file__).resolve().parent
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(r'F:\xmvsf')
TREE=ROOT/'dev/stable/buildtree'
if not TREE.is_dir() or not (TREE/'makefile.sdl2').is_file():
    sys.exit('ERROR: Original XMVSF buildtree not found: '+str(TREE))
# Conservative backup of every file we replace, without enforcing old package hashes.
files=['BUILD_TEST.bat','BUILD_STABLE.bat','RUN_TEST.bat','RUN_STABLE.bat',
       'PACK_XMVSF_HANDOFF.bat','PACK_XMVSF_HANDOFF_SAFE.bat',
       'dev/stable/buildtree/src/burner/sdl/run.cpp',
       'dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_56_final.h',
       'dev/stable/buildtree/src/burner/sdl/xmvsf_stage87_61_replay.h',
       'dev/stable/buildtree/src/intf/video/sdl/vid_sdl2.cpp',
       'dev/stable/releases/87_132/build_87132.py',
       'dev/stable/releases/87_132/build_stable_87132.py',
       'dev/stable/releases/87_132/run_87132.py',
       'dev/handoff/pack_current.py']
wily_rel='dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage86_49_painter.h'
wily=TREE/'src/intf/video/sdl/xmvsf_stage86_49_painter.h'
if not wily.is_file():sys.exit('ERROR: Original Wily painter missing: '+str(wily))
# No old revision lookup. Preserve any unknown local source in the backup.
backup=ROOT/'testdata/consolidated_source_backups'/('87_132_4_'+datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
backup.mkdir(parents=True,exist_ok=False)
try:
    for rel in files+[wily_rel]:
        current=ROOT/rel
        if current.is_file():
            out=backup/rel;out.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(current,out)
    raw=wily.read_text(encoding='utf-8')
    # Only retired 87.130 geometry probe calls, which have no implementation.
    lines=raw.splitlines(keepends=True)
    remove=set()
    for i,line in enumerate(lines):
        if 'XM87130::enabled()' in line:
            if i+1>=len(lines) or 'XM87130::geometry(' not in lines[i+1]:
                raise RuntimeError('Unexpected Wily probe layout at line '+str(i+1))
            remove.update((i,i+1))
    cleaned=''.join(l for i,l in enumerate(lines) if i not in remove)
    if 'XM87130::' in cleaned:raise RuntimeError('Unrecognized 87.130 Wily references: abort')
    # Stage files in temp within project, so install only after all checks pass.
    edits={wily_rel:cleaned.encode('utf-8')}
    for rel in files:
        incoming=PKG/rel
        if not incoming.is_file():raise RuntimeError('Handoff missing file: '+rel)
        edits[rel]=incoming.read_bytes()
    for rel,data in edits.items():
        dest=ROOT/rel;dest.parent.mkdir(parents=True,exist_ok=True)
        tmp=dest.with_name(dest.name+'.xm1324_tmp')
        tmp.write_bytes(data)
        os.replace(tmp,dest)
        if dest.read_bytes()!=data:raise RuntimeError('Verification failed: '+rel)
    # Do not delete obj/ or rewrite the Stable executable.
    print('SOURCE READY: 87.132.4 installed; Wily probes removed: %d'%(len(remove)//2))
    print('Previous files backed up: '+str(backup))
    print('Stable executable and warm object cache unchanged')
except Exception as e:
    print('ERROR: '+str(e),file=sys.stderr)
    print('Original files are backed up at '+str(backup),file=sys.stderr)
    sys.exit(2)
