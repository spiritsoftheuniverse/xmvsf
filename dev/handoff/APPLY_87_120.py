#!/usr/bin/env python3
"""XM87.120 guarded handoff installer with verified launch-target validation."""
import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
from datetime import datetime
from pathlib import Path

HERE = Path(__file__).resolve().parent
PACK = HERE.parent.parent
class Refuse(RuntimeError): pass

def load(path): return Path(path).read_bytes()
def sha(data): return hashlib.sha256(data).hexdigest()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--project-root', default=os.environ.get('XMVSF_PROJECT', r'F:\xmvsf'))
    args = ap.parse_args()
    root = Path(args.project_root).resolve()
    anchor = json.loads(load(HERE / 'anchor_87120.json'))
    source = 'dev/stable/buildtree/src/burn/drv/capcom/cps_run.cpp'
    header = 'dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_120_cyclops_cancel.h'
    buildpy = 'dev/stable/releases/87_120/build_87120.py'
    runpy = 'dev/stable/releases/87_120/run_87120.py'
    rels = [source, header, buildpy, runpy, 'BUILD_TEST.bat', 'RUN_TEST.bat']
    expected = {
        source: 'patched_87120_cpp_sha256',
        header: 'header_87120_sha256',
        buildpy: 'builder_sha256',
        runpy: 'runner_sha256',
        'BUILD_TEST.bat': 'corrected_87120_build_launcher_sha256',
        'RUN_TEST.bat': 'corrected_87120_run_launcher_sha256',
    }
    for file, key in expected.items():
        if not (PACK/file).is_file() or sha(load(PACK/file)) != anchor[key]:
            raise Refuse('Corrupt or missing package payload: ' + file)

    previous = load(HERE/'baseline_87119/cps_run.cpp')
    if sha(previous) != anchor['baseline_87119_cpp_sha256']:
        raise Refuse('Corrupt original CPS source baseline')
    ph = root/'dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_119_cyclops_recovery_probe.h'
    if not ph.is_file() or sha(load(ph)) != anchor['verified_87119_probe_header_sha256']:
        raise Refuse('Original 87.119 probe header missing or changed: ' + str(ph))
    src = root/source
    if not src.is_file(): raise Refuse('Existing CPS source absent: ' + str(src))
    if sha(load(src)) not in (anchor['baseline_87119_cpp_sha256'],anchor['patched_87120_cpp_sha256']):
        raise Refuse('Unknown CPS source edit: ' + str(src))

    for v in ('87_115', '87_118', '87_119', '87_120'):
        lock = root/'dev/stable/experiments'/v/'.build.lock'
        if lock.exists(): raise Refuse('Active or stale build lock; no changes: ' + str(lock))

    old_87119 = {
        source: sha(previous),
        'BUILD_TEST.bat': sha(load(HERE/'baseline_87119/BUILD_TEST.bat')),
        'RUN_TEST.bat': sha(load(HERE/'baseline_87119/RUN_TEST.bat')),
    }
    acceptable = {
        source: {old_87119[source]},
        'BUILD_TEST.bat': {old_87119['BUILD_TEST.bat'],anchor['legacy_broken_87120_build_launcher_sha256']},
        'RUN_TEST.bat': {old_87119['RUN_TEST.bat'],anchor['legacy_broken_87120_run_launcher_sha256']},
    }
    changes = []
    for file in rels:
        dst = root/file
        data = load(PACK/file)
        if dst.exists():
            if not dst.is_file(): raise Refuse('Target is not a regular file: ' + str(dst))
            actual = sha(load(dst))
            if actual == sha(data): continue
            if actual not in acceptable.get(file,set()):
                raise Refuse('Unknown local changes; will not overwrite: ' + file + ' SHA256=' + actual)
        changes.append((file,data))

    print('XM87.120 '+('APPLY' if args.apply else 'PREVIEW')+' project='+str(root))
    for file,_ in changes:
        print(('REPLACE ' if (root/file).is_file() else 'CREATE  ')+file)
    if not args.apply:
        print('PREVIEW ONLY. Use --apply to install.'); return
    if not changes:
        print('87.120 source, builder, runner, and both root launchers already installed and verified.')
        return

    backup = root/'testdata/handoff_backups'/('87_120_launcher_repair_'+datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    backup.mkdir(parents=True,exist_ok=False)
    journal = []
    committed = []
    try:
        for file,data in changes:
            dst = root/file
            original = load(dst) if dst.is_file() else None
            if original is not None:
                dest = backup/file
                dest.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(dst,dest)
                if sha(load(dest)) != sha(original):raise Refuse('Backup checksum mismatch: '+file)
            journal.append({'file':file,'old_sha256':sha(original) if original is not None else None,'new_sha256':sha(data)})
        for file,data in changes:
            dst = root/file
            dst.parent.mkdir(parents=True,exist_ok=True)
            fd,tmp = tempfile.mkstemp(prefix='.xm87120_fix_',dir=dst.parent)
            try:
                with os.fdopen(fd,'wb') as fp:
                    fp.write(data); fp.flush();os.fsync(fp.fileno())
                if sha(load(tmp)) != sha(data):raise Refuse('Temporary file checksum mismatch: '+file)
                os.replace(tmp,dst)
            finally:
                if os.path.exists(tmp):os.unlink(tmp)
            committed.append(file)
        (backup/'INSTALL_MANIFEST.json').write_text(json.dumps(journal,indent=2)+'\n',encoding='utf-8')
        for file,key in expected.items():
            if not (root/file).is_file() or sha(load(root/file)) != anchor[key]:
                raise Refuse('Post-install verification failed: '+file)
    except Exception:
        for file in reversed(committed):
            dst = root/file
            saved = backup/file
            if saved.is_file():shutil.copy2(saved,dst)
            else:dst.unlink(missing_ok=True)
        raise
    print('APPLIED AND VERIFIED: '+str(len(changes))+' file(s)')
    print('Backups: '+str(backup))
    print('Stable, archived project, old test executables and build-object cache untouched.')

if __name__ == '__main__':
    try:main()
    except (Refuse,OSError,ValueError,KeyError) as exc:
        print('XM87.120 REFUSED: '+str(exc),file=sys.stderr)
        sys.exit(2)
