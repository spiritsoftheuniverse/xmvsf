#!/usr/bin/env python3
"""XM87.127 anchored project-relative source-only handoff."""
from __future__ import annotations
from pathlib import Path
import argparse,hashlib,json,os,sys,shutil,tempfile
from datetime import datetime
BASE=Path(__file__).resolve().parent
PAYLOAD=BASE.parent.parent
class Refuse(RuntimeError): pass
def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for data in iter(lambda:f.read(1048576),b''):h.update(data)
    return h.hexdigest()
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--apply',action='store_true',help='apply after reviewing preview')
    ap.add_argument('--dry-run',action='store_true',help='preview only (default)')
    ap.add_argument('--project-root',default=os.environ.get('XMVSF_PROJECT',r'F:\xmvsf'))
    args=ap.parse_args()
    if args.apply and args.dry_run:raise Refuse('Choose preview or apply, not both')
    root=Path(args.project_root).resolve()
    if root==PAYLOAD.resolve() or root in PAYLOAD.resolve().parents:
        raise Refuse('Extract handoff outside project (e.g., F:\\XMVSF_INCOMING_87127)')
    if not (root/'dev/stable/buildtree/makefile.sdl2').is_file():
        raise Refuse('Existing buildtree makefile is missing: '+str(root))
    if not (root/'runtime/stable/XMVSF_Stable.exe').is_file() or not (root/'dev/stable/STABLE_BASELINE.json').is_file():
        raise Refuse('Protected Stable and its baseline manifest must exist')
    try:
        baseline=json.loads((root/'dev/stable/STABLE_BASELINE.json').read_text(encoding='utf-8-sig'))
        stable_expected=baseline['build_executable_sha256'].lower()
    except (ValueError,KeyError) as err:
        raise Refuse('Protected Stable baseline invalid: '+str(err))
    stable_path=root/'runtime/stable/XMVSF_Stable.exe'
    if digest(stable_path).lower()!=stable_expected:
        raise Refuse('Protected Stable differs from baseline; no installation')
    anchor=json.loads((BASE/'anchor_87127.json').read_text(encoding='utf-8'))
    if anchor.get('version')!='87.127':raise Refuse('Invalid manifest version')
    for rel,want in anchor['new_files'].items():
        f=PAYLOAD/rel
        if not f.is_file() or digest(f)!=want:raise Refuse('Payload absent or invalid SHA-256: '+rel)
    for rel,want in anchor['required_old'].items():
        f=root/rel
        if not f.is_file() or digest(f)!=want:
            raise Refuse('Prior 87.126 project source missing or unexpectedly modified: '+rel)
    # The old release's prior source files should be intact, never rebase over newer edits.
    for version in ('87_115','87_118','87_119','87_120','87_121','87_122','87_123','87_124','87_125','87_127'):
        lock=root/'dev/stable/experiments'/version/'.build.lock'
        if lock.exists():raise Refuse('Build lock exists: '+str(lock))
    changes=[]
    for rel,want in anchor['new_files'].items():
        dst=root/rel
        if dst.exists() and not dst.is_file():raise Refuse('Destination is not a file: '+rel)
        was=digest(dst) if dst.is_file() else None
        if was==want:continue
        known=anchor['expected_old'].get(rel)
        if was is not None and was!=known:
            raise Refuse('Unknown local edits, refusing overwrite: '+rel+' SHA256='+was)
        changes.append((rel,dst,was))
    print('XMVSF 87.127 '+('APPLY' if args.apply else 'PREVIEW')+' project='+str(root))
    for rel,_,was in changes:print(('REPLACE ' if was else 'CREATE  ')+rel)
    for rel in ('dev/stable/releases/87_127/build_87127.py','dev/stable/releases/87_127/run_87127.py'):
        if not (PAYLOAD/rel).is_file():raise Refuse('Required 87.127 build/run script missing from handoff: '+rel)
    if not args.apply:
        print('PREVIEW ONLY; no files changed. Run --apply to install.')
        return
    if not changes:
        print('Already installed and verified. No changes.')
        return
    backup=root/'testdata/handoff_backups'/('87_127_'+datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    backup.mkdir(parents=True,exist_ok=False)
    changed=[]
    try:
        for rel,dst,was in changes:
            if was:
                archived=backup/rel
                archived.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(dst,archived)
                if digest(archived)!=was:raise Refuse('Backup failed verification: '+rel)
        for rel,dst,was in changes:
            dst.parent.mkdir(parents=True,exist_ok=True)
            handle,tmp=tempfile.mkstemp(prefix='.xm87127_',dir=dst.parent)
            try:
                with os.fdopen(handle,'wb') as out:
                    with (PAYLOAD/rel).open('rb') as source:shutil.copyfileobj(source,out)
                    out.flush();os.fsync(out.fileno())
                if digest(Path(tmp))!=anchor['new_files'][rel]:raise Refuse('Temporary source checksum mismatch: '+rel)
                os.replace(tmp,dst)
                changed.append((rel,dst))
            finally:
                if os.path.exists(tmp):os.unlink(tmp)
        for rel,want in anchor['new_files'].items():
            f=root/rel
            if not f.is_file() or digest(f)!=want:raise Refuse('Post-apply checksum mismatch: '+rel)
        if digest(stable_path).lower()!=stable_expected:
            raise Refuse('Stable unexpectedly changed during installation')
        (backup/'INSTALL_MANIFEST.json').write_text(json.dumps([
            {'file':rel,'previous_sha256':was,'new_sha256':anchor['new_files'][rel]}
            for rel,_,was in changes],indent=2)+'\n',encoding='utf-8')
    except Exception:
        for rel,dst in reversed(changed):
            archived=backup/rel
            if archived.is_file():shutil.copy2(archived,dst)
            else:dst.unlink(missing_ok=True)
        raise
    print('APPLIED AND VERIFIED: '+str(len(changes))+' files')
    print('Backups: '+str(backup))
    print('No previous EXEs, stable, archive, or object files changed.')
if __name__=='__main__':
    try:main()
    except (Refuse,OSError,ValueError,KeyError) as exc:
        print('XM87.127 INSTALL REFUSED: '+str(exc),file=sys.stderr)
        sys.exit(2)
