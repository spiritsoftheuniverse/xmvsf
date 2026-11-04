#!/usr/bin/env bash
set -euo pipefail
P="${XMVSF_PROJECT:-$(cd "$(dirname "$0")/../.." && pwd)}"
python3 - "$P" <<'PY_VERIFY'
from pathlib import Path
import hashlib,json,sys
p=Path(sys.argv[1]); manifest=p/'dev/stable/STABLE_BASELINE.json';tree=p/'dev/stable/buildtree';runtime=p/'runtime/stable/XMVSF_Stable.exe'
if not manifest.is_file():raise SystemExit('ERROR: no promoted stable manifest; run PROMOTE_STAGE87_STABLE.sh first')
data=json.loads(manifest.read_text(encoding='utf-8'))
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
if not runtime.is_file() or sha(runtime)!=data['build_executable_sha256']:
 raise SystemExit('ERROR: runtime executable missing or differs from last verified stable build')
for rel,expected in data['source_sha256'].items():
 f=tree/'src'/rel
 if not f.is_file() or sha(f)!=expected:raise SystemExit('ERROR: stable source changed since last build: '+rel+'; use BUILD_STABLE.sh before verification')
for f in [tree/'makefile.sdl2',tree/'obj',tree/'src']:
 if not f.exists():raise SystemExit('ERROR: missing stable worktree item '+str(f))
print('STABLE VERIFIED: binary SHA256 and',len(data['source_sha256']),'pinned source files match last local build')
print('BINARY:',runtime)
print('GAMEPLAY STATUS: pending user-side smoke test (not simulated)')
PY_VERIFY
