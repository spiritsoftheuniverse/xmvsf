#!/usr/bin/env python3
"""Stage 87.135: small in-place source edits, with local backups.

Only modifies the three live SDL source files, and installs one new header.
Does not touch Stable, working objects, runtime configs, ROMs, saves or launchers.
Does not enforce historical source hashes and does not run a build.
"""
import argparse
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
SDL = Path('dev/stable/buildtree/src/burner/sdl')
HEADER = 'xmvsf_stage87_135_roster_snapshot.h'


def update(source, original, wanted):
    if wanted in source:
        return source, False
    count = source.count(original)
    if count != 1:
        raise ValueError(f'expected one edit anchor, got {count}: {original[:75]!r}')
    return source.replace(original, wanted, 1), True


def patch_run(source):
    old = '#include "xmvsf_stage87_105_training_reset.h"'
    new = '#include "xmvsf_stage87_135_roster_snapshot.h" // XM87.135 roster-snapshot interceptor\n' + old
    return update(source, old, new)


def patch_capture(source):
    old = '            saved=(rc==0);\n'
    new = old + '            if(saved)XM87135::cacheFirstFrame(SNAPSHOT); // XM87.135 archive this roster\n'
    return update(source, old, new)


def patch_reload(source):
    old = '#include "xmvsf_stage87_110_restart.h"'
    new = old + '\n#include "xmvsf_stage87_135_roster_snapshot.h"'
    s, changed = update(source, old, new)
    old = ' const int rc=BurnStateLoad((TCHAR*)XM87102Lab::SNAPSHOT,1,NULL);'
    new = ''' // XM87.135: LEFT+Start x3 keeps the CURRENT roster when loading the
 // first-frame Training checkpoint. The untouched original is never patched.
 std::string rosterLoadPath=XM87102Lab::SNAPSHOT;
 if(selection==XM87110::LEFT){
  XM87135::Team liveTeam;
  if(!XM87135::fromCPU(liveTeam) ||
     !XM87135::prepare(XM87102Lab::SNAPSHOT,liveTeam,rosterLoadPath)){
   std::fprintf(stderr,"XM87.135 RESTART_REFUSED frame=%u unable to preserve live roster\\n",frame);
   return false;
  }
 }
 const int rc=BurnStateLoad((TCHAR*)rosterLoadPath.c_str(),1,NULL);'''
    s, changed2 = update(s, old, new)
    return s, changed or changed2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', nargs='?', default=str(ROOT))
    args = parser.parse_args()
    root = Path(args.root)
    header_source = ROOT / SDL / HEADER
    live_header = root / SDL / HEADER
    if not header_source.is_file():
        print('NO EDIT: new header is missing beside the patch script.', file=sys.stderr)
        return 2
    replacements = {}
    for filename, patcher in [('run.cpp', patch_run),
                              ('xmvsf_stage87_102_training_lab.h', patch_capture),
                              ('xmvsf_stage87_110_reload.h', patch_reload)]:
        target = root / SDL / filename
        if not target.is_file():
            print(f'NO EDIT: live source not found: {target}', file=sys.stderr)
            return 2
        raw = target.read_bytes()
        if b'\r\n' in raw:
            newline = '\r\n'
        else:
            newline = '\n'
        try:
            original = raw.decode('utf-8')
        except UnicodeDecodeError:
            print(f'NO EDIT: not UTF-8: {target}', file=sys.stderr)
            return 2
        source = original.replace('\r\n', '\n')
        try:
            updated, changed = patcher(source)
        except ValueError as error:
            print(f'NO EDIT: source anchor not found in {target}: {error}', file=sys.stderr)
            return 2
        if changed:
            replacements[target] = updated.replace('\n', newline).encode('utf-8')
    if live_header.exists() and live_header.read_bytes() != header_source.read_bytes():
        print(f'NO EDIT: user-modified header already exists: {live_header}', file=sys.stderr)
        return 2
    # Preflight complete. Preserve one original alongside each changed source;
    # never overwrite a previous backup or the user's unrecognized edits.
    for target in replacements:
        backup = target.with_name(target.name + '.before_87_135.bak')
        if backup.exists():
            print(f'NO EDIT: backup already exists; refusing to replace it: {backup}', file=sys.stderr)
            return 2
    for target, contents in replacements.items():
        backup = target.with_name(target.name + '.before_87_135.bak')
        shutil.copy2(target, backup)
        target.write_bytes(contents)
        print(f'Patched: {target} (backup: {backup.name})')
    if not live_header.exists():
        live_header.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(header_source,live_header)
        print(f'Installed: {live_header}')
    if not replacements:
        print('Stage 87.135 edits already present; nothing changed.')
    print('Source-only update complete. Build using existing BUILD_TEST.bat; Stable not touched.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
