#!/usr/bin/env python3
"""Local-only MVC1 organizer with a single exact-path JSON save endpoint.

No packages, GitHub Actions or network listener. Serves only index.html and
/api/review on 127.0.0.1, writing only asset_organizer/aodata.json.
"""
from __future__ import annotations
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import tempfile
import webbrowser

MAX_REVIEW = 2_000_000
TAGS = {'character', 'effect', 'background', 'level_sprite'}
CATALOG_RE = re.compile(r'<script type="application/json" id="original-catalog">(.*?)</script>', re.S)


def catalog_from_html(path: Path) -> dict:
    content = path.read_text(encoding='utf-8')
    match = CATALOG_RE.search(content)
    if not match:
        raise ValueError('missing embedded original catalog in index.html')
    return json.loads(match.group(1))


def validate_review(review: object, catalog: dict) -> None:
    if not isinstance(review, dict) or review.get('format') != 'MVC1_ASSET_ORGANIZER_V2':
        raise ValueError('expected MVC1_ASSET_ORGANIZER_V2')
    entries = {item['id']: item for item in catalog['catalog']}
    if review.get('sourceRomSha256') != catalog['romSha256']:
        raise ValueError('original ROM checksum mismatch')
    items = review.get('items')
    if (not isinstance(items, list) or len(items) != len(entries) or
            review.get('sourceCatalogSize') != len(entries)):
        raise ValueError('review must include exactly the full source catalog')
    original_groups = catalog.get('colorAnalysis', {}).get('groups', [])
    valid_color_groups = {g['id'] for g in original_groups} if original_groups else {'palette_001'}
    seen = set()
    for item in items:
        if not isinstance(item, dict) or item.get('id') not in entries:
            raise ValueError('unrecognized asset ID')
        ident = item['id']
        if ident in seen: raise ValueError('duplicate asset ID')
        seen.add(ident)
        if item.get('romOffsetHex') != entries[ident]['offset']:
            raise ValueError('asset ROM offset mismatch')
        if item.get('wanted') is not None and type(item['wanted']) is not bool:
            raise ValueError('invalid wanted state')
        tags = item.get('tags')
        if (not isinstance(tags, list) or len(set(tags)) != len(tags)
                or any(not isinstance(t, str) or t not in TAGS for t in tags)):
            raise ValueError('invalid asset tags')
        # Optional V2 manual reassignment. Old aodata.json files omit it.
        # Only existing captured-palette containers may be referenced.
        if 'colorGroupOverride' in item:
            dest = item['colorGroupOverride']
            if dest is not None and (not isinstance(dest, str) or dest not in valid_color_groups):
                raise ValueError('invalid color assignment container')
        if item.get('reviewStatus') != ('WANTED' if item['wanted'] is True else 'UNWANTED' if item['wanted'] is False else 'UNREVIEWED'):
            raise ValueError('invalid review status')
    if len(seen) != len(entries): raise ValueError('missing asset IDs')


def save_review(dest: Path, review: dict, catalog: dict) -> None:
    validate_review(review, catalog)
    # Explicit fixed destination; browser input never selects a filesystem path.
    serialized = (json.dumps(review, ensure_ascii=False, indent=2)+'\n').encode('utf-8')
    if len(serialized) > MAX_REVIEW: raise ValueError('review is too large')
    tmp = None
    try:
        with tempfile.NamedTemporaryFile(mode='wb', dir=dest.parent, prefix='.aodata.', suffix='.tmp', delete=False) as fp:
            tmp = Path(fp.name)
            fp.write(serialized)
            fp.flush(); os.fsync(fp.fileno())
        os.replace(tmp, dest)
    finally:
        if tmp and tmp.exists(): tmp.unlink()


def make_handler(root: Path, catalog: dict, trace_root: Path | None = None):
    page = root / 'index.html'
    review_path = root / 'aodata.json'
    class OrganizerHandler(BaseHTTPRequestHandler):
        server_version = 'MVCAssetOrganizer/1.0'
        def _reply(self, status: int, content: bytes, mime: str):
            self.send_response(status)
            self.send_header('Content-Type', mime)
            self.send_header('Content-Length', str(len(content)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            self.send_header('X-Frame-Options', 'DENY')
            self.end_headers(); self.wfile.write(content)
        def _allowed_host(self):
            host = self.headers.get('Host', '')
            return host in (f'127.0.0.1:{self.server.server_port}', f'localhost:{self.server.server_port}')
        def do_GET(self):
            if not self._allowed_host(): return self._reply(403, b'Forbidden', 'text/plain')
            if self.path in ('/', '/index.html'):
                return self._reply(200, page.read_bytes(), 'text/html; charset=utf-8')
            if self.path == '/animations.html':
                preview = root / 'animations.html'
                if not preview.is_file(): return self._reply(404, b'Animation preview not generated yet; run RUN_TEST.bat art', 'text/plain')
                return self._reply(200, preview.read_bytes(), 'text/html; charset=utf-8')
            if self.path == '/api/captured_animations':
                manifest = root / 'captured_animations.json'
                if not manifest.is_file(): return self._reply(404, b'No captured animation manifest yet', 'text/plain')
                return self._reply(200, manifest.read_bytes(), 'application/json; charset=utf-8')
            if self.path == '/api/review':
                if not review_path.is_file(): return self._reply(404, b'No aodata.json yet', 'text/plain')
                return self._reply(200, review_path.read_bytes(), 'application/json; charset=utf-8')
            return self._reply(404, b'Not found', 'text/plain')
        def do_POST(self):
            if not self._allowed_host(): return self._reply(403, b'Forbidden', 'text/plain')
            if self.path != '/api/review': return self._reply(404, b'Not found', 'text/plain')
            origin = self.headers.get('Origin', '')
            if origin not in (f'http://127.0.0.1:{self.server.server_port}', f'http://localhost:{self.server.server_port}'):
                return self._reply(403, b'Invalid Origin', 'text/plain')
            if self.headers.get('Content-Type', '').split(';')[0].strip().lower() != 'application/json':
                return self._reply(415, b'JSON required', 'text/plain')
            try: n = int(self.headers.get('Content-Length', '0'))
            except ValueError: n = 0
            if not 1 <= n <= MAX_REVIEW:
                return self._reply(413, b'JSON content length out of bounds', 'text/plain')
            try:
                review = json.loads(self.rfile.read(n).decode('utf-8'))
                save_review(review_path, review, catalog)
                if trace_root is not None:
                    try:
                        from BUILD_WANTED_SEQUENCES import build as build_wanted
                        report=build_wanted(review_path, trace_root, root/'wanted_sequences.json')
                        print('Updated',report['wanted'],'Wanted assets /',report['candidateGroupCount'],'source-adjacent clusters',flush=True)
                        observations=trace_root.parent/'sprite_factory'/'sprite_observations.tsv.gz'
                        capture=trace_root.parent/'stage_capture'
                        template=root/'CAPTURED_ANIMATIONS_TEMPLATE.html'
                        if observations.is_file() and capture.exists() and template.is_file():
                            from BUILD_CAPTURED_ANIMATIONS import build as build_animations
                            art=build_animations(review_path, root/'wanted_sequences.json',root/'index.html',
                                observations,capture,trace_root,template,root/'animations.html')
                            print('Updated',art['previewTakes'],'sampled animation takes with',art['backgroundFrames'],'capture backdrops',flush=True)
                    except (OSError, ValueError, KeyError) as error:
                        print('NOTICE: aodata.json saved, but sequence metadata not updated:', error, flush=True)
            except (ValueError, TypeError, UnicodeError) as exc:
                return self._reply(400, str(exc).encode('utf-8')[:350], 'text/plain')
            except OSError as exc:
                return self._reply(500, str(exc).encode('utf-8')[:350], 'text/plain')
            return self._reply(200, b'{"saved":"aodata.json"}', 'application/json')
    return OrganizerHandler


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, default=Path(__file__).parent)
    ap.add_argument('--port', type=int, default=18720)
    ap.add_argument('--trace-root', type=Path)
    ap.add_argument('--open', action='store_true')
    args=ap.parse_args()
    root=args.root.resolve(strict=True)
    catalog=catalog_from_html(root / 'index.html')
    with ThreadingHTTPServer(('127.0.0.1', args.port), make_handler(root, catalog, args.trace_root)) as httpd:
        url=f'http://127.0.0.1:{httpd.server_port}/'
        print('MVC1 Asset Organizer:', url, flush=True)
        print('Save target:', root / 'aodata.json', flush=True)
        print('Close this window or Ctrl+C to stop the local server.', flush=True)
        if args.open: webbrowser.open(url)
        try: httpd.serve_forever()
        except KeyboardInterrupt: pass

if __name__=='__main__': main()