#!/usr/bin/env python3
"""87.121 surgical source handoff: preview, integrity, backups, guarded replace."""
from __future__ import annotations
import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

HERE=Path(__file__).resolve().parent
PACK=HERE.parent.parent
class Refuse(RuntimeError):pass

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--apply',action='store_true')
    p.add_argument('--project-root',default=os.environ.get('XMVSF_PROJECT',r'F:\xmvsf'))
    args=p.parse_args()
    root=Path(args.project_root).resolve()
    if root==PACK.resolve():raise Refuse('Extract the incoming package OUTSIDE the working project first')
    if not (root/'dev/stable/buildtree/makefile.sdl2').is_file():
        raise Refuse('Not an existing XMVSF buildtree: '+str(root))
    anchor=json.loads((HERE/'anchor_87121.json').read_text(encoding='utf-8'))
    if anchor.get('version')!='87.121':raise Refuse('Incorrect package manifest version')
    for rel,digest in anchor['new_files'].items():
        path=PACK/rel
        if not path.is_file() or sha(path)!=digest:
            raise Refuse('Corrupt/missing incoming payload: '+rel)
    for rel,digest in anchor['required_old'].items():
        path=root/rel
        if not path.is_file() or sha(path)!=digest:
            raise Refuse('Required prior 87.120 header differs or is missing: '+rel)
    for ver in ('87_115','87_118','87_119','87_120','87_121'):
        lock=root/'dev/stable/experiments'/ver/'.build.lock'
        if lock.exists(): raise Refuse('Build lock exists: '+str(lock))
    protected=root/'runtime/stable/XMVSF_Stable.exe'
    if not protected.is_file():raise Refuse('Protected Stable executable absent')
    changes=[]
    for rel,digest in anchor['new_files'].items():
        dst=root/rel
        if dst.exists() and not dst.is_file():raise Refuse('Not a regular file: '+rel)
        oldhash=sha(dst) if dst.is_file() else None
        if oldhash==digest:continue
        allowed=anchor['expected_old'].get(rel)
        if oldhash is not None and oldhash!=allowed:
            raise Refuse('Unknown local source/launcher modifications; refusing overwrite: '+rel+' SHA256='+oldhash)
        changes.append((rel,dst,(PACK/rel).read_bytes(),oldhash))
    print('XM87.121 '+('APPLY' if args.apply else 'PREVIEW')+' project='+str(root))
    for rel,_,_,oldhash in changes:print(('REPLACE ' if oldhash else 'CREATE  ')+rel)
    if not args.apply:
        print('PREVIEW ONLY. Run with --apply to install; no files changed.')
        return
    if not changes:
        print('Already installed: verified all six patch files and prerequisites.')
        return
    backups=root/'testdata/handoff_backups'/('87_121_'+datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    backups.mkdir(parents=True,exist_ok=False)
    replaced=[]
    try:
        report=[]
        for rel,dst,new,oldhash in changes:
            if oldhash is not None:
                saved=backups/rel
                saved.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(dst,saved)
                if sha(saved)!=oldhash:raise Refuse('Backup failed checksum verification: '+rel)
            report.append(dict(file=rel,previous_sha256=oldhash,new_sha256=hashlib.sha256(new).hexdigest()))
        for rel,dst,new,_ in changes:
            dst.parent.mkdir(parents=True,exist_ok=True)
            fd,tmp=tempfile.mkstemp(prefix='.xm87121_',dir=dst.parent)
            try:
                with os.fdopen(fd,'wb') as f:
                    f.write(new);f.flush();os.fsync(f.fileno())
                if hashlib.sha256(Path(tmp).read_bytes()).hexdigest()!=anchor['new_files'][rel]:
                    raise Refuse('Staged file differs: '+rel)
                os.replace(tmp,dst)
                replaced.append((rel,dst))
            finally:
                if os.path.exists(tmp):os.unlink(tmp)
        for rel,digest in anchor['new_files'].items():
            if not (root/rel).is_file() or sha(root/rel)!=digest:
                raise Refuse('Post-install SHA256 verification failed: '+rel)
        (backups/'INSTALL_MANIFEST.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    except Exception:
        for rel,dst in reversed(replaced):
            saved=backups/rel
            if saved.is_file():shutil.copy2(saved,dst)
            else:dst.unlink(missing_ok=True)
        raise
    print('APPLIED AND VERIFIED: %d file(s)'%len(changes))
    print('Backup: '+str(backups))
    print('Protected Stable, original archive, existing test EXEs and obj cache unchanged.')

if __name__=='__main__':
    try:main()
    except (Refuse,OSError,ValueError,KeyError) as e:
        print('XM87.121 INSTALL REFUSED: '+str(e),file=sys.stderr);sys.exit(2)
