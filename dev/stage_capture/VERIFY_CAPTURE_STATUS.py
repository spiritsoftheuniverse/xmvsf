#!/usr/bin/env python3
"""Append verified blue-part / green-level events; do not infer completeness."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from datetime import datetime, timezone
PARTS = ('scroll1', 'scroll2', 'scroll3', 'sprites', 'animation')
def valid_part(root, part):
    if not isinstance(part, dict) or part.get('state') != 'verified':
        return False
    items = part.get('artifacts')
    if not isinstance(items, list) or not items:
        return False
    for item in items:
        if not isinstance(item, dict):
            return False
        rel, expected = item.get('path'), item.get('sha256')
        if not isinstance(rel, str) or not isinstance(expected, str) or len(expected) != 64:
            return False
        p = Path(rel)
        if p.is_absolute() or '..' in p.parts or not p.parts:
            return False
        p = (root / p).resolve()
        if not p.is_relative_to(root.resolve()) or not p.is_file() or p.stat().st_size == 0:
            return False
        h = hashlib.sha256()
        with p.open('rb') as f:
            for block in iter(lambda: f.read(1024 * 1024), b''):
                h.update(block)
        if h.hexdigest() != expected.lower():
            return False
    return True
def emit(log, seen, event, description):
    key = event + '\t' + description
    if key in seen:
        return
    line = datetime.now(timezone.utc).isoformat(timespec='seconds') + '\t' + key + '\n'
    with log.open('a', encoding='utf-8') as f:
        f.write(line)
        f.flush()
        os.fsync(f.fileno())
    seen.add(key)
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    root = parser.parse_args().root.resolve()
    root.mkdir(parents=True, exist_ok=True)
    path = root / 'capture_manifest.json'
    if not path.exists():
        print('INCOMPLETE: stage builder has not produced capture_manifest.json')
        return 2
    try:
        manifest = json.loads(path.read_text(encoding='utf-8'))
        parts = manifest['parts']
        if not isinstance(parts, dict):
            raise ValueError('missing parts')
    except (ValueError, KeyError, OSError, TypeError) as exc:
        print('ERROR: malformed capture manifest:', exc)
        return 2
    log = root / 'status.log'
    seen = set()
    if log.exists():
        for line in log.read_text(encoding='utf-8', errors='replace').splitlines():
            fields = line.split('\t', 2)
            if len(fields) == 3:
                seen.add(fields[1] + '\t' + fields[2])
    good = []
    for name in PARTS:
        try:
            verified = valid_part(root, parts.get(name))
        except OSError:
            verified = False
        if verified:
            emit(log, seen, 'PART_DONE', name)
            good.append(name)
        else:
            print('INCOMPLETE:', name, 'not yet artifact-verified')
    certified = (manifest.get('capture_closed') is True and
                 manifest.get('coverage_complete') is True and
                 manifest.get('replay_sealed') is True and
                 manifest.get('unresolved_gaps') == [] and
                 len(good) == len(PARTS))
    if certified:
        emit(log, seen, 'LEVEL_DONE', str(manifest.get('level_id', 'unknown')))
        print('LEVEL_DONE: all five verified parts; sealed replay and certified coverage')
        return 0
    print('INCOMPLETE: level not certified; green withheld')
    return 1
if __name__ == '__main__':
    raise SystemExit(main())
