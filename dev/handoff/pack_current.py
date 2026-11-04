#!/usr/bin/env python3
"""XMVSF live-project handoff exporter. Never modifies source, builds, or Stable."""
import argparse, datetime, hashlib, json, os, pathlib, sys, zipfile
ROOT = pathlib.Path(__file__).resolve().parents[2]
EXCLUDE_DIRS = {'.git','__pycache__','packed','test','testdata','logs','sessions','captures','.vs'}
EXCLUDE_EXT = {'.zip','.7z','.rar','.pyc','.log','.tmp','.bak','.dmp','.map','.fs','.nv','.chd','.rom'}

def digest(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source-only',action='store_true',help='omit warm build objects')
    ap.add_argument('--no-stable',action='store_true',help='omit Stable executable')
    ap.add_argument('--output',help='output ZIP path')
    args=ap.parse_args()
    tree=ROOT/'dev/stable/buildtree'
    if not (tree/'src').is_dir(): ap.error('Missing live buildtree/src')
    for name in ('BUILD_TEST.bat','RUN_TEST.bat'):
        if not (ROOT/name).is_file(): ap.error('Missing '+name)
    included={}
    def collect(relative):
        base=ROOT/relative
        if not base.exists(): return
        paths=[base] if base.is_file() else base.rglob('*')
        for p in paths:
            if p.is_symlink() or not p.is_file(): continue
            rel=p.relative_to(ROOT)
            parts=[x.lower() for x in rel.parts]
            if any(x in EXCLUDE_DIRS for x in parts): continue
            if 'roms' in parts: continue
            if args.source_only and ('obj' in parts or 'dep' in parts): continue
            if p.suffix.lower() in EXCLUDE_EXT: continue
            if p.suffix.lower()=='.exe' and rel.as_posix()!='runtime/stable/XMVSF_Stable.exe': continue
            if args.no_stable and rel.as_posix()=='runtime/stable/XMVSF_Stable.exe': continue
            included[rel.as_posix()]=p
    for folder in ('dev/stable/buildtree/src','dev/stable/buildtree/obj','dev/stable/buildtree/dep',
                   'dev/handoff','runtime/stable','runtime/stage79','runtime/stage77/assets',
                   'FBNeo/assets','FBNeo/config','config'):
        collect(folder)
    for p in tree.iterdir():
        if p.is_file() and (p.name.startswith('makefile') or p.suffix.lower() in ('.mk','.sdl2')): collect(p.relative_to(ROOT))
    for p in ROOT.glob('*.bat'): collect(p.relative_to(ROOT))
    for p in ROOT.glob('*.cfg'): collect(p.relative_to(ROOT))
    collect('dev/stable/STABLE_BASELINE.json')
    collect('runtime/stable/STABLE_ACTIVE.json')
    # Include current release's small source/config dependencies, not accumulated historical releases.
    releases=ROOT/'dev/stable/releases'
    if releases.is_dir():
        candidates=[p for p in releases.iterdir() if p.is_dir() and p.name.startswith('87_')]
        if candidates:
            latest=max(candidates,key=lambda p:tuple(int(x) for x in p.name.split('_')[1:] if x.isdigit()))
            collect(latest.relative_to(ROOT))
    stamp=datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    out=pathlib.Path(args.output) if args.output else ROOT/'packed'/('XMVSF_CURRENT_HANDOFF_'+stamp+'.zip')
    out=out.resolve()
    if out.suffix.lower()!='.zip': ap.error('Output must be .zip')
    if out.exists(): ap.error('Output already exists: '+str(out))
    out.parent.mkdir(parents=True,exist_ok=True)
    tmp=out.with_suffix('.zip.partial')
    manifest={'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'source':'live buildtree','source_only':args.source_only,'stable_included':not args.no_stable,
              'files':{}}
    try:
        with zipfile.ZipFile(tmp,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=3,allowZip64=True) as z:
            for i,(rel,p) in enumerate(sorted(included.items()),1):
                z.write(p,rel)
                manifest['files'][rel]={'size':p.stat().st_size,'sha256':digest(p)}
                if i%1000==0: print('Packed',i,'/',len(included),flush=True)
            z.writestr('XMVSF_HANDOFF_MANIFEST.json',json.dumps(manifest,indent=2)+'\n')
        with zipfile.ZipFile(tmp) as z:
            bad=z.testzip()
            if bad: raise RuntimeError('ZIP CRC failed: '+bad)
        os.replace(tmp,out)
    finally:
        if tmp.exists(): tmp.unlink()
    print('HANDOFF READY:',out)
    print('Files:',len(included),'| Warm objects:',not args.source_only,'| Stable:',not args.no_stable)

if __name__=='__main__':
    try: main()
    except (OSError,RuntimeError,ValueError) as exc:
        print('HANDOFF FAILED:',exc,file=sys.stderr)
        sys.exit(2)
