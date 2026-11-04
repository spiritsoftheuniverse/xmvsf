#!/usr/bin/env python3
"""87.127 minimal hotfix: QSound one-character syntax error + Winline chatter."""
from __future__ import annotations
import argparse,hashlib,json,os,shutil,sys,tempfile
from datetime import datetime
from pathlib import Path

PROJECT_DEFAULT=r'F:\xmvsf'
QS_REL='dev/stable/buildtree/src/burn/drv/capcom/qs_z.cpp'
MK_REL='dev/stable/buildtree/makefile.sdl2'
ORIGINAL_QS_SHA='607cebcdfbd9695c14856deddbb06f35c120d15c3c8b3b3115f87b037beb7a6a'
CORRECTED_QS_SHA='16f02cbf2ef1b2924d12425559e7129bfea0a7ca34f0fac687f7696c8099d7df'
BAD=b'const unsigned xm87127Original=((unsigned)QscCmd[0]<<8)|(unsigned)QscCmd[1]);'
GOOD=b'const unsigned xm87127Original=((unsigned)QscCmd[0]<<8)|(unsigned)QscCmd[1];'

class Refuse(RuntimeError): pass

def sha(path:Path)->str:
    h=hashlib.sha256()
    with path.open('rb') as inp:
        for chunk in iter(lambda:inp.read(1048576),b''):
            h.update(chunk)
    return h.hexdigest()

def stable_check(root:Path):
    stable=root/'runtime/stable/XMVSF_Stable.exe'
    baseline=root/'dev/stable/STABLE_BASELINE.json'
    if not stable.is_file() or not baseline.is_file():
        raise Refuse('Protected Stable or STABLE_BASELINE.json is missing')
    try: want=json.loads(baseline.read_text(encoding='utf-8-sig'))['build_executable_sha256'].lower()
    except (KeyError,ValueError) as e: raise Refuse('Invalid Stable baseline: '+str(e))
    if sha(stable)!=want: raise Refuse('Protected Stable checksum mismatch')
    return want

def plan_qs(data:bytes):
    current=hashlib.sha256(data).hexdigest()
    if current==CORRECTED_QS_SHA: return data,False
    if current!=ORIGINAL_QS_SHA:
        raise Refuse('qs_z.cpp does not match the installed 87.127 source; refusing to overwrite local edits. SHA256='+current)
    if data.count(BAD)!=1:raise Refuse('Original QSound syntax target not unique')
    changed=data.replace(BAD,GOOD)
    if hashlib.sha256(changed).hexdigest()!=CORRECTED_QS_SHA:
        raise Refuse('Internal QSound patch checksum error')
    return changed,True

def plan_make(data:bytes):
    # Only a single, exact compiler warning flag is modified; every other flag remains.
    if b'CXXFLAGS' not in data:raise Refuse('Unrecognized makefile.sdl2')
    import re
    raw=list(re.finditer(rb'(?<![\w-])-Winline(?![\w-])',data))
    safe=list(re.finditer(rb'(?<![\w-])-Wno-inline(?![\w-])',data))
    if len(raw)==1 and len(safe)==0:
        new=data[:raw[0].start()]+b'-Wno-inline'+data[raw[0].end():]
        return new,True
    if len(raw)==0 and len(safe)==1:return data,False
    raise Refuse('Ambiguous -Winline in makefile: original='+str(len(raw))+' suppressed='+str(len(safe)))

def atomic_write(dst:Path,contents:bytes):
    fd,tmp=tempfile.mkstemp(prefix='.xm87127_hotfix_',dir=str(dst.parent))
    try:
        with os.fdopen(fd,'wb') as f:
            f.write(contents);f.flush();os.fsync(f.fileno())
        shutil.copymode(dst,tmp)
        os.replace(tmp,dst)
    finally:
        if os.path.exists(tmp):os.unlink(tmp)

def main():
    ap=argparse.ArgumentParser(description='87.127 QSound syntax and Winline hotfix')
    ap.add_argument('--apply',action='store_true',help='back up and apply exactly two edits')
    ap.add_argument('--project-root',default=os.environ.get('XMVSF_PROJECT',PROJECT_DEFAULT))
    args=ap.parse_args()
    root=Path(args.project_root).resolve()
    if not (root/'dev/stable/buildtree').is_dir():raise Refuse('Missing existing buildtree: '+str(root))
    payload=Path(__file__).resolve().parents[2]
    if payload==root or payload in root.parents or root in payload.parents:
        raise Refuse('Extract handoff outside the project directory')
    for ver in ('87_115','87_118','87_119','87_120','87_121','87_122','87_123','87_124','87_125','87_126','87_127'):
        lock=root/'dev/stable/experiments'/ver/'.build.lock'
        if lock.exists():raise Refuse('Build lock exists; no edits made: '+str(lock))
    want_stable=stable_check(root)
    targets=[(QS_REL,plan_qs),(MK_REL,plan_make)]
    changes=[]
    for rel,handler in targets:
        dest=root/rel
        if not dest.is_file():raise Refuse('Required file missing: '+str(dest))
        before=dest.read_bytes()
        after,need=handler(before)
        if need:changes.append((rel,dest,before,after))
    print('XMVSF 87.127 QSound + Winline '+('APPLY' if args.apply else 'PREVIEW'))
    for rel,_,_,_ in changes:print('PATCH  '+rel)
    if not changes:print('Already repaired; no changes.');return
    if not args.apply:
        print('Preview only. Re-run with --apply to back up and patch.');return
    backup=root/'testdata/handoff_backups'/('87_127_QS_WINLINE_'+datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    backup.mkdir(parents=True,exist_ok=False)
    for rel,_,before,_ in changes:
        dst=backup/rel;dst.parent.mkdir(parents=True,exist_ok=True)
        dst.write_bytes(before)
        if hashlib.sha256(before).hexdigest()!=sha(dst):raise Refuse('Backup checksum failure: '+rel)
    written=[]
    try:
        for rel,dest,before,after in changes:
            if dest.read_bytes()!=before:raise Refuse('Concurrent edit detected: '+rel)
            atomic_write(dest,after)
            written.append((rel,dest))
            if hashlib.sha256(after).hexdigest()!=sha(dest):raise Refuse('Post-write mismatch: '+rel)
        if stable_check(root)!=want_stable:raise Refuse('Protected Stable changed unexpectedly')
        (backup/'PATCH_REPORT.txt').write_text('XMVSF 87.127 syntax/Winline patch\n'+\
            ''.join(rel+'\n' for rel,_,_,_ in changes),encoding='utf-8')
    except Exception:
        for rel,dest in reversed(written):
            atomic_write(dest,(backup/rel).read_bytes())
        raise
    print('SUCCESS: '+str(len(changes))+' file(s) patched, verified, and backed up.')
    print('Backups: '+str(backup))
    print('No executable, build cache, Stable, or gameplay behavior was altered.')
if __name__=='__main__':
    try:main()
    except (Refuse,OSError,ValueError) as e:
        print('HOTFIX REFUSED: '+str(e),file=sys.stderr);sys.exit(2)
