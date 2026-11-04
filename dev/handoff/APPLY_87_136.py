#!/usr/bin/env python3
"""87.136 minimal source repair: reinstate the post-frame Training match auto-save.

No script rewrites, installers, file deletion, source-hash gates, or Stable edits.
Uses the existing BUILD_TEST.bat and RUN_TEST.bat after applying.
"""
from pathlib import Path
import argparse
import shutil
import sys
from datetime import datetime

REL=Path('dev/stable/buildtree/src/burner/sdl/run.cpp')
HOOK='XM87102Lab::capture().frame(bDrvOkay,(unsigned)nCurrentFrame);'
ANCHOR='    xm23OnRealFrame(bDrvOkay); /* XMVSF_STAGE23_TRAINING_MENU */'
CALL='    XM87102Lab::capture().frame(bDrvOkay,(unsigned)nCurrentFrame); // XM87.136 restore real-frame Training match-start capture'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('root',nargs='?',default=str(Path(__file__).resolve().parents[2]),help='Project root (default: script location)')
    args=ap.parse_args()
    path=Path(args.root)/REL
    if not path.is_file():
        print('NOT CHANGED: live source missing:',path,file=sys.stderr)
        return 2
    original=path.read_bytes()
    try: s=original.decode('utf-8')
    except UnicodeDecodeError:
        print('NOT CHANGED: run.cpp not UTF-8',file=sys.stderr)
        return 2
    if HOOK in s:
        print('Already hooked: Training match-start capture is present in',path)
        return 0
    if s.count(ANCHOR)!=1:
        print('NOT CHANGED: cannot identify unique post-frame Training UI hook',file=sys.stderr)
        return 2
    if 'xm87110RestartAfterFrame(bDrvOkay' not in s or 'xm87105TrainingResetOnLanding(bDrvOkay' not in s:
        print('NOT CHANGED: restart/KO hook not detected',file=sys.stderr)
        return 2
    anchor_at=s.index(ANCHOR)
    reload_at=s.find('xm87110RestartAfterFrame(bDrvOkay',anchor_at)
    if not (reload_at>=0 and reload_at-anchor_at<1400):
        print('NOT CHANGED: post-frame order unexpected',file=sys.stderr)
        return 2
    newline='\r\n' if b'\r\n' in original else '\n'
    s=s.replace(ANCHOR,ANCHOR+'\n'+CALL,1)
    if newline=='\r\n':s=s.replace('\r\n','\n').replace('\n','\r\n')
    updated=s.encode('utf-8')
    stamp=datetime.now().strftime('%Y%m%d_%H%M%S')
    backup=path.with_name(path.name+'.before_87_136_'+stamp+'.bak')
    if backup.exists():
        print('NOT CHANGED: backup path already exists:',backup,file=sys.stderr)
        return 2
    shutil.copy2(path,backup)
    try:path.write_bytes(updated)
    except Exception as e:
        shutil.copy2(backup,path)
        print('Write failed, original restored:',e,file=sys.stderr)
        return 2
    print('Repaired:',path)
    print('Original backup:',backup)
    print('The existing Training snapshot capture now runs each real game frame, before restart checks.')
    print('Use the ORIGINAL BUILD_TEST.bat and RUN_TEST.bat. Stable is untouched.')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
