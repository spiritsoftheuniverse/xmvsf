#!/usr/bin/env python3
"""Review all native Stage44/45 MVC clean-rip bursts in one safe batch.

- Read-only raw source; completed shots validated, unfinished shots reported.
- Fast mode hashes three sample times x four layers per shot, and size-checks
  ALL indexed RGBA frames. --full hashes every original frame/layer.
- Generates paginated contact sheets from REAL CPS2-rendered composite frames.
- Keeps stage/animation completion UNVERIFIED; no fake green/blue statuses.
- Uses Python standard library. Never deletes or rewrites source captures.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
import zlib
import zipfile

from EXPORT_CLEAN_RIP import SAFE_RGBA, fnv64, png_bytes

FRAME_SIZE = 384 * 224 * 4
# PNG source helper caps width/height at 2048. Twelve sheet rows yield 1640px.
# Pagination is essential: 16+ captures exceeded the height cap and aborted
# before the review ZIP could be written.
CONTACT_ROWS_PER_PAGE = 12
REQUIRED = {'sample', 'emulated_frame', 'layer', 'file', 'fnv64',
            'width', 'height', 'scroll_x', 'scroll_y'}
SHOT = re.compile(r'shot_\d{4}\Z', re.ASCII)
COMPLETE = re.compile(r'samples=(\d+)\s*\Z', re.ASCII)


class RipError(ValueError):
    pass


def sheet_rgba(source_rows: list[tuple[str, list[bytes]]], target: Path) -> dict:
    """3 source-composite views per selected shot; 1/2-sized, no Pillow.

    The indexed legend records exact shot names and native frame IDs. The
    checkerboard is only a display aid for transparent native pixels.
    """
    if not source_rows:
        return {'file': None, 'shots': 0, 'legend': []}
    if len(source_rows) > CONTACT_ROWS_PER_PAGE:
        raise RipError('contact sheet page exceeds safe 2048-pixel dimensions')
    thumb_w, thumb_h = 192, 112
    gutter, head = 8, 16
    width = 3 * thumb_w + 4 * gutter
    height = len(source_rows) * (thumb_h + head + gutter) + gutter
    buf = bytearray(width * height * 4)
    # Flat dark base and a light separator around each shot.
    for y in range(height):
        for x in range(width):
            at = 4 * (y * width + x)
            g = 37 if (y % (thumb_h + head + gutter)) >= head else 25
            buf[at:at + 4] = bytes((g, g, g, 255))
    legend = []
    for row_idx, (name, previews) in enumerate(source_rows):
        row_y = gutter + row_idx * (thumb_h + head + gutter)
        for col, raw in enumerate(previews):
            x0 = gutter + col * (thumb_w + gutter)
            for sy in range(thumb_h):
                sample_y = sy * 2
                for sx in range(thumb_w):
                    sample_x = sx * 2
                    src = (sample_y * 384 + sample_x) * 4
                    r, g, b, a = raw[src:src + 4]
                    check = 70 if ((sx // 8 + sy // 8) % 2) else 51
                    if a == 0:
                        r = g = b = check
                    elif a != 255:
                        r = (r * a + check * (255 - a)) // 255
                        g = (g * a + check * (255 - a)) // 255
                        b = (b * a + check * (255 - a)) // 255
                    dst = ((row_y + head + sy) * width + x0 + sx) * 4
                    buf[dst:dst + 4] = bytes((r, g, b, 255))
        legend.append({'row': row_idx, 'shot': name,
                       'columns': ['first composite', 'middle composite', 'last composite']})
    image = png_bytes(width, height, bytes(buf))
    write_atomic(target, image)
    return {'file': str(target.name), 'shots': len(source_rows),
            'width': width, 'height': height, 'legend': legend}


def write_atomic(path: Path, data: bytes) -> None:
    """Replace only review outputs, never original shot_XXXX source files."""
    temp = path.with_name(path.name + '.tmp')
    try:
        temp.write_bytes(data)
        os.replace(temp, path)
    finally:
        if temp.exists():
            temp.unlink()


def scan_shot(directory: Path, full: bool) -> tuple[dict, list[bytes]]:
    name = directory.name
    if not SHOT.fullmatch(name):
        raise RipError('invalid native shot name')
    index = directory / 'layers.tsv'
    completed = directory / 'CAPTURE_COMPLETE.txt'
    if not index.is_file():
        if completed.exists():
            raise RipError('completed shot missing layers.tsv')
        return ({'shot': name, 'status': 'INCOMPLETE', 'samples': 0,
                 'reason': 'no layers.tsv yet'}, [])
    with index.open('r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        if not reader.fieldnames or not REQUIRED.issubset(reader.fieldnames):
            raise RipError('missing Stage44 native layer columns')
        rows = list(reader)
    groups = {}
    for row in rows:
        try:
            sample = int(row['sample'])
            frame = int(row['emulated_frame'])
            layer = int(row['layer'])
            w, h = int(row['width']), int(row['height'])
            x, y = int(row['scroll_x']), int(row['scroll_y'])
            hash_expected = int(row['fnv64'], 16)
        except (ValueError, TypeError, KeyError) as exc:
            raise RipError('invalid numeric field in layers.tsv') from exc
        filename = row['file']
        if sample < 0 or layer not in range(4) or w != 384 or h != 224:
            raise RipError('invalid frame/layer/viewport dimensions')
        match = SAFE_RGBA.fullmatch(filename)
        if not match or int(match.group(1)) != layer or int(match.group(2)) != sample:
            raise RipError('unsafe or inconsistent native RGBA name')
        path = directory / filename
        if not path.is_file() or path.stat().st_size != FRAME_SIZE:
            raise RipError('missing or truncated native RGBA: ' + filename)
        if sample not in groups:
            groups[sample] = {'native_frame': frame, 'layers': {}}
        group = groups[sample]
        if group['native_frame'] != frame or layer in group['layers']:
            raise RipError('duplicate layer or conflicting native frame index')
        group['layers'][layer] = (path, hash_expected, x, y)
    seq = sorted(groups)
    if seq != list(range(len(seq))):
        raise RipError('nonconsecutive native sample indices')
    if any(set(g['layers']) != set(range(4)) for g in groups.values()):
        raise RipError('partial four-layer sample; 0/1/2/3 required')
    timestamps = [groups[i]['native_frame'] for i in seq]
    if any(b <= a for a, b in zip(timestamps, timestamps[1:])):
        raise RipError('nonmonotonic emulated frame numbers')
    finished = completed.is_file()
    if finished:
        marker = COMPLETE.fullmatch(completed.read_text(encoding='ascii'))
        if not marker or int(marker.group(1)) != len(seq):
            raise RipError('CAPTURE_COMPLETE sample count does not match layers.tsv')
    positions = ([seq[0], seq[len(seq) // 2], seq[-1]] if seq else [])
    required_hashes = {(sample, layer) for sample in seq for layer in range(4)} if full else {
        (sample, layer) for sample in positions for layer in range(4)
    }
    previews = {}
    unique_composites = set()
    opacity = {}
    checked = 0
    for sample, layer in sorted(required_hashes):
        path, expected, _, _ = groups[sample]['layers'][layer]
        raw = path.read_bytes()
        if fnv64(raw) != expected:
            raise RipError('native CPS2 FNV mismatch: ' + path.name)
        checked += 1
        if layer == 0:
            unique_composites.add(hashlib.sha256(raw).hexdigest())
        if (sample in positions) and (layer == 0):
            previews[sample] = raw
            opacity[sample] = sum(a != 0 for a in raw[3::4]) / (384 * 224)
    info = {
        'shot': name, 'status': 'FULL_VALIDATED' if full and finished else
                            'SAMPLED_VALIDATED' if finished else 'INCOMPLETE',
        'burst_finished': finished,
        'samples': len(seq),
        'source_layers': 4,
        'indexed_rgba_files': 4 * len(seq),
        'hash_checked_rgba_files': checked,
        'frame_first': timestamps[0] if timestamps else None,
        'frame_last': timestamps[-1] if timestamps else None,
        'composite_distinct_in_checked_frames': len(unique_composites),
        'sampled_composite_coverage': {str(k): round(v, 5) for k, v in opacity.items()},
        'sampled_native_frames': [groups[i]['native_frame'] for i in positions],
        'raw_bytes_indexed': len(seq) * 4 * FRAME_SIZE,
    }
    if finished and positions and max(opacity.values(), default=0) == 0:
        info['warning'] = 'three sampled native composite frames fully transparent; scene may be blank or renderer needs inspection'
    return info, [previews[sample] for sample in positions]


def inspect_capture_status(root: Path, status_log: Path | None) -> dict:
    """Read existing log evidence offline; do not pretend it is pixel evidence."""
    origin = status_log or (root.parent / 'stage_capture' / 'status.log')
    evidence = {
        'status_log': str(origin),
        'status_log_exists': origin.is_file(),
        'status_bytes_scanned': 0,
        'status_truncated_to_tail': False,
        'CLEAN_RIP_ARMED': 0,
        'CLEAN_RIP_START': 0,
        'CLEAN_RIP_BURST_DONE': 0,
        'BATCH_DONE': 0,
        'ERROR': 0,
        'logged_completed_shots': [],
    }
    if origin.is_file():
        with origin.open('rb') as fp:
            fp.seek(0, 2)
            length = fp.tell()
            limit = 4 * 1024 * 1024
            clipped = length > limit
            if clipped:
                fp.seek(length - limit)
                fp.readline()  # ignore one partial first line
            else:
                fp.seek(0)
            data = fp.read().decode('utf-8', errors='replace')
        evidence['status_bytes_scanned'] = len(data.encode('utf-8'))
        evidence['status_truncated_to_tail'] = clipped
        for line in data.splitlines():
            columns = line.split('\t', 2)
            if len(columns) < 2:
                continue
            event = columns[1]
            if event in evidence and isinstance(evidence[event], int):
                evidence[event] += 1
            if event == 'CLEAN_RIP_BURST_DONE':
                found = re.search(r'shot_\d{4}\b', line)
                if found:
                    evidence['logged_completed_shots'].append(found.group())
    capture = root.parent / 'stage_capture'
    if capture.is_dir():
        evidence['native_session_directories'] = sorted(
            p.name for p in capture.iterdir()
            if p.is_dir() and re.fullmatch(r'session_\d{4}', p.name))
    else:
        evidence['native_session_directories'] = []
    return evidence


def review(root: Path, output: Path, *, full: bool = False,
           max_rows: int = 36, status_log: Path | None = None) -> dict:
    root, output = root.resolve(), output.resolve()
    if output == root or root in output.parents or output in root.parents:
        raise RipError('review output must be a sibling of source clean_rips, not nested')
    if not 1 <= max_rows <= 150:
        raise RipError('max_rows must be 1..150')
    dirs = sorted((p for p in root.iterdir() if p.is_dir() and SHOT.fullmatch(p.name)),
                  key=lambda p: int(p.name[-4:])) if root.is_dir() else []
    evidence = inspect_capture_status(root, status_log)
    result = {'format': 'XM87203_NATIVE_CLEAN_RIP_REVIEW_V1',
              'input_root': str(root), 'verification': 'FULL' if full else 'SAMPLED',
              'level_complete': False, 'stage_identity_verified': False,
              'animation_cycles_verified': False, 'shots': [],
              'warnings': [], 'errors': [],
              'source_root_exists': root.is_dir(),
              'capture_log_evidence': evidence}
    if not dirs:
        result['warnings'].append(
            'NO_NATIVE_CLEAN_RIP_ARTWORK: no shot_XXXX folders found under ' +
            str(root) + '. No source pixel image can be generated from the '
            'telemetry status log alone.')
    elif evidence['CLEAN_RIP_BURST_DONE'] > len(dirs):
        result['warnings'].append(
            'The status log records more completed bursts than the current '
            'clean_rips directory contains; check the configured capture '
            'root or include the raw shot_XXXX folders.')
    render_rows = []
    previous_last = None
    for directory in dirs:
        try:
            info, previews = scan_shot(directory, full)
            result['shots'].append(info)
            if previous_last is not None and info['frame_first'] is not None:
                if info['frame_first'] <= previous_last:
                    result['warnings'].append(f'{directory.name}: native frame rewind; possible reset or new ROM run')
            if info['frame_last'] is not None:
                previous_last = info['frame_last']
            if info.get('warning'):
                result['warnings'].append(f'{directory.name}: {info["warning"]}')
            if previews:
                render_rows.append((directory.name, previews))
        except (RipError, OSError, UnicodeDecodeError) as exc:
            result['errors'].append({'shot': directory.name, 'error': str(exc)})
            result['shots'].append({'shot': directory.name, 'status': 'ERROR',
                                    'reason': str(exc)})
    result['finished_bursts'] = sum(s.get('burst_finished', False) for s in result['shots'])
    result['incomplete_bursts'] = sum(s['status'] == 'INCOMPLETE' for s in result['shots'])
    result['indexed_samples'] = sum(s.get('samples', 0) for s in result['shots'])
    result['verified_source_bytes'] = sum(s.get('hash_checked_rgba_files', 0) * FRAME_SIZE
                                          for s in result['shots'])
    result['total_bursts'] = len(result['shots'])
    if len(render_rows) > max_rows:
        indexes = [round(i * (len(render_rows)-1) / (max_rows-1))
                   for i in range(max_rows)] if max_rows > 1 else [len(render_rows)-1]
        render_rows = [render_rows[i] for i in sorted(set(indexes))]
    output.mkdir(parents=True, exist_ok=True)
    contact_pages = []
    for start in range(0, len(render_rows), CONTACT_ROWS_PER_PAGE):
        number = start // CONTACT_ROWS_PER_PAGE + 1
        filename = ('clean_rips_contact_sheet.png' if number == 1 else
                    f'clean_rips_contact_sheet_{number:03d}.png')
        page = sheet_rgba(render_rows[start:start + CONTACT_ROWS_PER_PAGE],
                          output / filename)
        page['page'] = number
        page['row_offset'] = start
        contact_pages.append(page)
    # Keep the old single-page key for consumers of the original manifest.
    contact = contact_pages[0] if contact_pages else {'file': None, 'shots': 0, 'legend': []}
    result['contact_sheet'] = contact
    result['contact_sheets'] = contact_pages
    write_atomic(output / 'clean_rips_review.json',
                 (json.dumps(result, indent=2) + '\n').encode('utf-8'))
    lines = [f"MVC NATIVE CLEAN RIP REVIEW: {result['verification']}",
             f"Native clean-rip source: {'PRESENT' if dirs else 'NOT FOUND'}",
             f"Source path: {root}",
             f"Capture status log: {evidence['status_log']}",
             f"Source log clean bursts: {evidence['CLEAN_RIP_BURST_DONE']} completed, {evidence['CLEAN_RIP_START']} started",
             f"Bursts: {result['finished_bursts']} finished, {result['incomplete_bursts']} incomplete, "
             f"{len(result['errors'])} errors",
             f"Samples indexed: {result['indexed_samples']}",
             f"Raw frames verified by FNV: {result['verified_source_bytes']:,} bytes",
             'Full stage coverage: UNVERIFIED',
             'Contact sheets: ' + (', '.join(page['file'] for page in contact_pages)
                                    if contact_pages else 'UNAVAILABLE (no native source images)')]
    for warning in result['warnings']:
        lines.append('WARNING ' + warning)
    for problem in result['errors']:
        lines.append('ERROR ' + problem['shot'] + ': ' + problem['error'])
    write_atomic(output / 'clean_rips_review.txt', ('\n'.join(lines)+'\n').encode('utf-8'))
    # The user can attach this compact proof bundle instead of a multi-GiB
    # set of original ROM pixel captures. Never include source .rgba bytes.
    bundle = output / 'clean_rips_review_bundle.zip'
    pending = output / 'clean_rips_review_bundle.zip.tmp'
    try:
        with zipfile.ZipFile(pending, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for item in ('clean_rips_review.json', 'clean_rips_review.txt'):
                archive.write(output / item, arcname=item)
            for page in contact_pages:
                archive.write(output / page['file'], arcname=page['file'])
        os.replace(pending, bundle)
    finally:
        if pending.exists():
            pending.unlink()
    if status_log is not None:
        status_log.parent.mkdir(parents=True, exist_ok=True)
        with status_log.open('a', encoding='utf-8') as status:
            event = 'ERROR' if result['errors'] else ('INCOMPLETE' if not dirs or result['incomplete_bursts']
                                                     else 'CLEAN_RIP_REVIEW')
            status.write(f"frame=0\t{event}\tclean_rip_review={result['verification']} "
                         f"finished={result['finished_bursts']} partial={result['incomplete_bursts']} "
                         f"errors={len(result['errors'])} review={output}\n")
    for line in lines:
        print(line)
    print('Small review bundle: ' + str(bundle))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True, help='existing clean_rips')
    parser.add_argument('--output', type=Path, required=True, help='review-only, non-source directory')
    parser.add_argument('--full', action='store_true', help='FNV-check every raw layer (large/slow)')
    parser.add_argument('--max-rows', type=int, default=36, help='contact-sheet rows, 1..150')
    parser.add_argument('--status-log', type=Path, help='append the review to existing MSYS status.log')
    args = parser.parse_args()
    try:
        report = review(args.root, args.output, full=args.full,
                        max_rows=args.max_rows, status_log=args.status_log)
        return 2 if report['errors'] or not report['shots'] else 0
    except (RipError, OSError, ValueError) as exc:
        print('CLEAN_RIP_REVIEW ERROR: ' + str(exc), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
