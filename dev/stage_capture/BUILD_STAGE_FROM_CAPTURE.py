#!/usr/bin/env python3
"""Native MVC CPS2 capture -> Wily-75-compatible XRGA atlases and evidence.

No ROM content or generated assets are checked into source control. Output is
an independent, explicitly UNVERIFIED scene manifest, not a Wily-75 overwrite.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from contextlib import contextmanager
import csv
import gzip
import hashlib
import json
from pathlib import Path
import struct
import zipfile
import zlib

MAP_LEN = 0x4000
PAL_LEN = 0x1800
OAM_LEN = 0x2000
TILE_PX = {1: 8, 2: 16, 3: 32}
TILE_SHIFT = {1: 6, 2: 7, 3: 9}
PALETTE_BASE = {1: 0x20, 2: 0x40, 3: 0x60}


def index_for_xy(layer: int, x: int, y: int) -> int:
    if not (0 <= x < 64 and 0 <= y < 64 and layer in TILE_PX):
        raise ValueError('out-of-range CPS2 cell')
    if layer == 1:
        addr = ((y & 0x20) << 8) | (x << 7) | ((y & 0x1f) << 2)
    elif layer == 2:
        addr = ((y & 0x30) << 8) | (x << 6) | ((y & 0x0f) << 2)
    else:
        addr = ((y & 0x38) << 8) | (x << 5) | ((y & 0x07) << 2)
    return addr // 4


INDEX_COORDS = {}
for _layer in TILE_PX:
    inv = [None] * 4096
    for _y in range(64):
        for _x in range(64):
            index = index_for_xy(_layer, _x, _y)
            assert inv[index] is None
            inv[index] = (_x, _y)
    INDEX_COORDS[_layer] = inv


class Capture:
    def __init__(self, source: Path, session: str):
        self.source = source
        self.archive = zipfile.ZipFile(source) if source.is_file() else None
        if self.archive is not None:
            root = next((p for p in self.archive.namelist()
                         if p.endswith('/latest.txt')), None)
            if session == 'latest':
                if root is None:
                    raise ValueError('missing latest.txt in archive')
                session = self.archive.read(root).decode('utf-8').strip()
            if not session.startswith('session_') or '/' in session or '..' in session:
                raise ValueError('unsafe session name')
            self.base = root.rsplit('/', 1)[0] + '/' + session if root else 'stage_capture/' + session
        else:
            if session == 'latest':
                latest = source / 'latest.txt'
                if latest.is_file():
                    session = latest.read_text(encoding='utf-8').strip()
                elif (source / 'frames.tsv').is_file():
                    session = '.'
                else:
                    raise ValueError('missing latest.txt')
            if session != '.' and (not session.startswith('session_') or '/' in session or '..' in session):
                raise ValueError('unsafe session name')
            self.base = source / session
        self.session = session

    @contextmanager
    def open(self, name: str):
        if self.archive:
            try:
                with self.archive.open(self.base + '/' + name) as f:
                    yield f
            except KeyError as exc:
                raise ValueError(f'missing capture artifact {name}') from exc
        else:
            with (self.base / name).open('rb') as f:
                yield f

    def size(self, name: str) -> int:
        if self.archive:
            try:
                return self.archive.getinfo(self.base + '/' + name).file_size
            except KeyError as exc:
                raise ValueError(f'missing capture artifact {name}') from exc
        return (self.base / name).stat().st_size

    def close(self):
        if self.archive:
            self.archive.close()


def read_exact(f, size: int, what: str) -> bytes:
    data = f.read(size)
    if len(data) != size:
        raise ValueError(f'truncated {what}: expected {size}, got {len(data)}')
    return data


def native16(buf: bytes, at: int) -> int:
    return struct.unpack_from('<H', buf, at)[0]


def bank_from_reg(reg: bytes, offset: int) -> int:
    return (native16(reg, offset) << 8) & 0xffc000


def palette_rgb(raw: bytes, idx: int) -> tuple[int, int, int]:
    # CPS2 CpsPalUpdate: palette RAM 16-bit words, offset^15 pen ordering.
    value = native16(raw, 2 * (idx ^ 15))
    brightness = 15 + 2 * ((value >> 12) & 15)
    return tuple(min(255, ((value >> shift) & 15) * 17 * brightness // 45)
                 for shift in (8, 4, 0))


def tile_rgba(gfx: bytes, mask: int, offset: int, tile: int, attrs: int,
              raw_palette: bytes, layer: int) -> bytes:
    dim = TILE_PX[layer]
    addr = (offset + (tile << TILE_SHIFT[layer])) & mask
    stride = 16 if dim == 32 else 8
    if addr + (dim - 1) * stride + (dim // 2) > len(gfx):
        return bytes(dim * dim * 4)  # invalid pixel source, flagged in manifest
    pal = (PALETTE_BASE[layer] | (attrs & 0x1f)) << 4
    lookup = [(0, 0, 0, 0)] + [(*palette_rgb(raw_palette, pal + k), 255)
                                  for k in range(1, 16)]
    flip_x, flip_y = bool(attrs & 0x20), bool(attrs & 0x40)
    pixels = bytearray(dim * dim * 4)
    for y in range(dim):
        row = addr + (dim - 1 - y if flip_y else y) * stride
        for x in range(dim):
            srcx = dim - 1 - x if flip_x else x
            # ctv_do.h reads each 32-bit word in host-little-endian order,
            # then plots the high nibble first: byte3-H, byte3-L ... byte0-L.
            pos = row + (srcx // 8) * 4 + 3 - (srcx % 8) // 2
            v = gfx[pos]
            pen = (v >> 4) & 15 if (srcx % 2) == 0 else v & 15
            pixels[(y * dim + x) * 4:(y * dim + x + 1) * 4] = bytes(lookup[pen])
    return bytes(pixels)


def write_xrga(path: Path, layer: int, page: bytes, gfx: bytes, mask: int,
               offset: int, palette: bytes) -> dict:
    size = TILE_PX[layer]
    width = height = 64 * size
    canvas = bytearray(width * height * 4)
    cache = {}
    used = 0
    invalid = 0
    for y in range(64):
        for x in range(64):
            index = index_for_xy(layer, x, y)
            tile, attrs = struct.unpack_from('<HH', page, index * 4)
            key = (tile, attrs)
            if key not in cache:
                base = (offset + (tile << TILE_SHIFT[layer])) & mask
                if base + (size - 1) * (16 if size == 32 else 8) + size // 2 > len(gfx):
                    invalid += 1
                cache[key] = tile_rgba(gfx, mask, offset, tile, attrs, palette, layer)
            sprite = cache[key]
            if any(sprite[3::4]):
                used += 1
            for row in range(size):
                where = ((y * size + row) * width + x * size) * 4
                canvas[where:where + size * 4] = sprite[row * size * 4:(row + 1) * size * 4]
    with path.open('wb') as f:
        f.write(b'XRGA' + struct.pack('<II', width, height))
        f.write(canvas)
    return {'file': path.name, 'width': width, 'height': height,
            'nontransparent_cells': used, 'unique_tile_palette_pairs': len(cache),
            'invalid_tile_palette_pairs': invalid,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def crop_native_viewport(atlas: bytes, x: int, y: int,
                         screen_w: int = 384, screen_h: int = 224) -> bytes:
    """Crop a circular CPS2 page by its *raw* scroll; no guessed parallax.

    The preview deliberately omits unknown global/driver offsets, raster
    bands, row-scroll and priority. The native camera timeline preserves those
    facts for later renderer verification.
    """
    if len(atlas) < 12 or atlas[:4] != b'XRGA':
        raise ValueError('bad Stage75/76 XRGA source')
    page_w, page_h = struct.unpack_from('<II', atlas, 4)
    if page_w < screen_w or page_h < screen_h or len(atlas) != 12 + page_w*page_h*4:
        raise ValueError('XRGA dimensions or length inconsistent')
    pixels = memoryview(atlas)[12:]
    out = bytearray(screen_w*screen_h*4)
    x %= page_w
    for row in range(screen_h):
        src_y = (y+row) % page_h
        dst = row*screen_w*4
        src = src_y*page_w*4
        left = min(screen_w,page_w-x)
        out[dst:dst+left*4] = pixels[src+x*4:src+(x+left)*4]
        if left < screen_w:
            out[dst+left*4:dst+screen_w*4] = pixels[src:src+(screen_w-left)*4]
    return bytes(out)


def write_png_rgba(path: Path, width: int, height: int, pixels: bytes) -> None:
    """Small dependency-free PNG for diagnostic previews, not stage artwork."""
    if len(pixels) != width*height*4:
        raise ValueError('invalid RGBA preview size')
    def chunk(tag: bytes, value: bytes) -> bytes:
        body = tag+value
        return struct.pack('>I',len(value))+body+struct.pack('>I',zlib.crc32(body)&0xffffffff)
    raw = b''.join(b'\0'+pixels[row*width*4:(row+1)*width*4] for row in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+
                     chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+
                     chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))


def compose_diagnostic_layers(layers: dict[int, bytes]) -> bytes:
    """Illustrative back-to-front 3->2->1; NOT hardware priority certification."""
    result = bytearray(384*224*4)
    for layer in (3,2,1):
        source = layers.get(layer)
        if source is None:
            continue
        for at in range(0,len(source),4):
            if source[at+3]:
                result[at:at+4]=source[at:at+4]
    return bytes(result)


def import_capture(capture: Capture, output: Path, preview_frame: int | None = None):
    with capture.open('frames.tsv') as fp:
        rows = list(csv.DictReader((line.decode('utf-8') for line in fp), delimiter='\t'))
    if not rows:
        raise ValueError('empty frame timeline')
    frames = [int(r['frame']) for r in rows]
    if frames != list(range(frames[0], frames[0] + len(frames))):
        raise ValueError('nonconsecutive or duplicated native frame IDs')
    last = frames[-1]
    target = preview_frame if preview_frame is not None else last
    if target not in (int(x) for x in frames):
        raise ValueError('preview frame outside capture timeline')
    frame_set = set(frames)
    output.mkdir(parents=True, exist_ok=False)
    with capture.open('gfx_index.tsv') as fp:
        gfx_entry = read_exact(fp, capture.size('gfx_index.tsv'), 'graphics index').decode('utf-8').strip().split('\t')
    if len(gfx_entry) != 9 or not gfx_entry[1].startswith('mvsc'):
        raise ValueError('invalid MVC graphics provenance')
    gfx_name = gfx_entry[8]
    if '/' in gfx_name or '\\' in gfx_name or '..' in gfx_name:
        raise ValueError('unsafe graphics filename')
    with capture.open(gfx_name) as fp:
        gfx = read_exact(fp, int(gfx_entry[2]), 'MVC decoded graphics')
    mask = int(gfx_entry[3], 16)
    gfx_offsets = {layer: int(gfx_entry[3 + layer]) for layer in TILE_PX}
    if len(gfx) > 256 * 1024 * 1024:
        raise ValueError('graphics export exceeds capture limit')
    overview = {'format': 'WILY_STAGE76_NATIVE_IMPORT_V1', 'source_driver': gfx_entry[1],
                'source_session': capture.session, 'native_frame_count': len(frames),
                'first_frame': frames[0], 'last_frame': last, 'selected_preview_frame': target,
                'original_gfx_sha256': hashlib.sha256(gfx).hexdigest(),
                'original_gfx_bytes': len(gfx), 'gfx_mask': mask,
                'legacy_basis': 'Stage75/76 independent SDL2 XRGA/atlas/manifest data model',
                'level_complete': False, 'stage_id_verified': False,
                'camera_bounds_verified': False, 'priority_matched_to_native_frame': False,
                'animation_loops_verified': False, 'f9_replay_sealed': False,
                'unresolved_gaps': ['Marvel vs Capcom stage identity unverified',
                    'stage start/end and full geographic bounds unproven',
                    'no native reference framebuffer to pixel-diff colors and priority',
                    'OAM actors lack semantic classification',
                    'palette DMA-effective color cache not captured',
                    'F9 replay sealing not demonstrated by capture archive'],
                'atlas_layers': {}, 'integrity': {}}
    # Live CPU regs and raster slices are the hardware authority for page banks.
    active_banks = {}
    row_enabled = 0
    raster_count = 0
    camera_raw = {}
    camera_extents = {layer: {'x_min': 65535, 'x_max': 0,
                             'y_min': 65535, 'y_max': 0} for layer in TILE_PX}
    with gzip.open(output/'camera_timeline.tsv.gz','wt',encoding='utf-8',newline='') as cam_fp, \
         capture.open('raster.bin') as fp:
        cw = csv.writer(cam_fp,delimiter='\t')
        cw.writerow(['frame','slice','raster_start','scroll1x','scroll1y',
                     'scroll2x','scroll2y','scroll3x','scroll3y',
                     'bank1','bank2','bank3','ctrl20','rows22','ctrl26'])
        while True:
            header = fp.read(16)
            if not header:
                break
            if len(header) != 16 or header[:4] != b'FRM1':
                raise ValueError(f'bad raster record at {raster_count}')
            frame, stage, nslices = struct.unpack('<III', header[4:])
            if frame not in frame_set or nslices not in range(1, 11):
                raise ValueError(f'bad raster frame or slice count {frame}/{nslices}')
            for sl in range(nslices):
                data = read_exact(fp, 4 + 256 + 16, 'raster slice')
                reg = data[4:260]
                scroll = [native16(reg,offs) for offs in (0x0c,0x0e,0x10,0x12,0x14,0x16)]
                banks = [bank_from_reg(reg,offs) for offs in (2,4,6)]
                raster_start = struct.unpack_from('<I',data,0)[0]
                cw.writerow([frame,sl,raster_start,*scroll,
                             *(f'{b:06x}' for b in banks),
                             native16(reg,0x20),native16(reg,0x22),native16(reg,0x26)])
                if sl == 0:
                    for layer in TILE_PX:
                        ext = camera_extents[layer]
                        x,y = scroll[(layer-1)*2:(layer-1)*2+2]
                        ext['x_min']=min(ext['x_min'],x)
                        ext['x_max']=max(ext['x_max'],x)
                        ext['y_min']=min(ext['y_min'],y)
                        ext['y_max']=max(ext['y_max'],y)
                if native16(reg, 0x22) & 1:
                    row_enabled += 1
                if frame == target and sl == 0:
                    camera_raw = {'scroll': scroll, 'banks': banks,
                                  'ctrl20': native16(reg,0x20),
                                  'rows22': native16(reg,0x22),
                                  'ctrl26': native16(reg,0x26),
                                  'raster_slices': nslices}
                    for layer, reg_off in [(1, 2), (2, 4), (3, 6)]:
                        active_banks[layer] = bank_from_reg(reg, reg_off)
            raster_count += 1
    if raster_count != len(frames):
        raise ValueError(f'raster frames {raster_count} != timeline {len(frames)}')
    overview['integrity']['raster_frames'] = raster_count
    overview['camera_timeline'] = 'camera_timeline.tsv.gz'
    overview['camera_preview_raw'] = camera_raw
    overview['camera_extents_raw_all_scenes'] = camera_extents
    overview['unresolved_gaps'].append(
        'raw preview scroll omits unverified CPS global and driver-specific alignment')
    overview['integrity']['row_scroll_enabled_slices'] = row_enabled
    overview['preview_banks'] = {str(k): f'0x{v:06x}' for k, v in active_banks.items()}
    if row_enabled and capture.size('row_scroll.bin') == 0:
        overview['unresolved_gaps'].append('row scrolling enabled but row-scroll capture empty')
    # Reconstruct raw palette snapshot at chosen frame; never invent RGB cache.
    selected_palette = None
    pal_count = 0
    with capture.open('palettes.bin') as fp:
        while True:
            header = fp.read(20)
            if not header:
                break
            if len(header) != 20 or header[:4] != b'PAL1':
                raise ValueError('bad palette packet')
            frame, _stage, _fnv = struct.unpack('<IIQ', header[4:])
            if frame not in frame_set:
                raise ValueError('palette frame outside timeline')
            data = read_exact(fp, PAL_LEN, 'native palette')
            if frame <= target:
                selected_palette = data
            pal_count += 1
    if selected_palette is None:
        raise ValueError('no palette preceding preview frame')
    overview['integrity']['palette_snapshots'] = pal_count
    # Retain ALL native tile state changes; each bank is a distinct 64x64 page.
    recent = {}
    selected_pages = {}
    change_counts = Counter()
    variant_states = defaultdict(set)
    state_returns = Counter()
    all_snapshots = 0
    with gzip.open(output / 'cell_changes.tsv.gz', 'wt', encoding='utf-8', newline='') as delta_fp, \
         (output / 'map_snapshots.tsv').open('w', encoding='utf-8', newline='') as snap_fp:
        delta_writer = csv.writer(delta_fp, delimiter='\t')
        delta_writer.writerow(['frame','layer','bank','x','y','tile','attributes','palette','flip'])
        snapshot_writer = csv.writer(snap_fp, delimiter='\t')
        snapshot_writer.writerow(['frame','layer','slice','bank','changed_cells','native_hash_fnv64'])
        with capture.open('maps.bin') as fp:
            while True:
                header = fp.read(32)
                if not header:
                    break
                if len(header) != 32 or header[:4] != b'MAP1':
                    raise ValueError(f'bad map packet #{all_snapshots}')
                frame, _stage, sl, layer, bank, hashed = struct.unpack('<IIIIIQ', header[4:])
                if frame not in frame_set or layer not in TILE_PX or sl >= 10:
                    raise ValueError(f'bad map timestamp/layer at #{all_snapshots}')
                if not (0x900000 <= bank and bank + MAP_LEN <= 0x930000):
                    raise ValueError(f'bad CPS graphics RAM address 0x{bank:x}')
                data = read_exact(fp, MAP_LEN, f'map snapshot #{all_snapshots}')
                key = layer, bank
                prior = recent.get(key)
                old = memoryview(prior).cast('I') if prior is not None else None
                now = memoryview(data).cast('I')
                changed = 0
                for index, word in enumerate(now):
                    if old is not None and word == old[index]:
                        continue
                    changed += 1
                    x, y = INDEX_COORDS[layer][index]
                    tile, attrs = word & 0xffff, word >> 16
                    delta_writer.writerow([frame,layer,f'{bank:06x}',x,y,tile,attrs,
                                           attrs & 31,(attrs >> 5) & 3])
                    change_counts[layer] += 1
                    cell = (layer, bank, index)
                    if word in variant_states[cell]:
                        state_returns[cell] += 1
                    if len(variant_states[cell]) < 64:
                        variant_states[cell].add(word)
                snapshot_writer.writerow([frame,layer,sl,f'{bank:06x}',changed,f'{hashed:016x}'])
                recent[key] = data
                if frame <= target and sl == 0 and active_banks.get(layer) == bank:
                    selected_pages[layer] = (data, frame, bank)
                all_snapshots += 1
    overview['integrity']['map_snapshots'] = all_snapshots
    overview['integrity']['cell_change_events'] = sum(change_counts.values())
    overview['integrity']['map_changes_by_layer'] = {str(k): change_counts[k] for k in TILE_PX}
    overview['integrity']['candidate_animated_cells'] = sum(len(s) > 1 for s in variant_states.values())
    # A returning state is only a candidate, NOT proof of an animation cycle.
    with (output / 'animation_candidates.tsv').open('w', encoding='utf-8') as fp:
        fp.write('layer\tbank\tx\ty\tstates_seen\treturns_seen\tverified\n')
        for key, states in sorted(variant_states.items()):
            if len(states) > 1:
                layer, bank, idx = key
                x, y = INDEX_COORDS[layer][idx]
                fp.write(f'{layer}\t{bank:06x}\t{x}\t{y}\t{len(states)}\t{state_returns[key]}\t0\n')
    # OAM stays raw, because background actors must not absorb fighters/HUD.
    oam_count = 0
    with capture.open('oam.bin') as fp:
        while True:
            header = fp.read(16)
            if not header:
                break
            if len(header) != 16 or header[:4] != b'OAM1':
                raise ValueError('invalid OAM packet')
            frame, _stage, objbank = struct.unpack('<III', header[4:])
            if frame not in frame_set or objbank not in (0,1):
                raise ValueError('invalid OAM timestamp/bank')
            read_exact(fp,OAM_LEN,'raw OAM')
            oam_count += 1
    if oam_count != len(frames):
        raise ValueError('OAM data has missing/duplicate native frames')
    overview['integrity']['oam_frames'] = oam_count
    # Use the actual XRGA texture format the old Wily Stage75/76 renderer knows.
    diagnostic_viewports = {}
    for layer in TILE_PX:
        if layer not in selected_pages:
            overview['unresolved_gaps'].append(f'no active Scroll{layer} page at frame {target}')
            continue
        page, sample_frame, bank = selected_pages[layer]
        name = f'layer{layer}_{bank:06x}_frame{target:06d}.xrga'
        detail = write_xrga(output / name, layer, page, gfx, mask,
                            gfx_offsets[layer], selected_palette)
        detail.update({'layer': layer,'bank': f'{bank:06x}',
                       'page_last_written_frame': sample_frame,
                       'tile_size_px': TILE_PX[layer],
                       'gfx_offset':gfx_offsets[layer], 'palette_source': 'raw_cps2_ram_unverified_RGB'})
        overview['atlas_layers'][str(layer)] = detail
        # Unlike full VRAM pages, this is the portion aligned to the actual
        # register scroll coordinates on the selected frame. Export separately
        # from stage textures to avoid conflating image data with verification.
        raw_x,raw_y = camera_raw['scroll'][(layer-1)*2:(layer-1)*2+2]
        viewport = crop_native_viewport((output/name).read_bytes(),raw_x,raw_y)
        diagnostic_viewports[layer] = viewport
        preview_name = f'preview_scroll{layer}_frame{target:06d}.png'
        write_png_rgba(output/preview_name,384,224,viewport)
        detail['diagnostic_raw_scroll_viewport'] = preview_name
    if len(diagnostic_viewports)==3:
        diagnostic = compose_diagnostic_layers(diagnostic_viewports)
        diagnostic_name = f'preview_approx_composite_frame{target:06d}.png'
        write_png_rgba(output/diagnostic_name,384,224,diagnostic)
        overview['approximate_composite_png'] = diagnostic_name
        overview['unresolved_gaps'].append(
            'diagnostic 3->2->1 compositing does not prove actual CPS2 sprite/layer priority')
    # Carry the Wily atlas conventions without reusing Stage75's fake bounds.
    with (output / 'stage76_import_manifest.tsv').open('w', encoding='utf-8') as fp:
        fp.write('# Native MVC source; XRGA uses Stage75/76 texture format.\n')
        fp.write('# NOT a drop-in Stage75 manifest. Camera and loops unverified.\n')
        fp.write('SIZE\t384\t224\n')
        fp.write(f'SOURCE\t{gfx_entry[1]}\t{capture.session}\t{target}\n')
        for layer, detail in overview['atlas_layers'].items():
            fp.write(f'ATLAS\t{layer}\tbase\t0\t0\t{detail["file"]}\t-\t64\n')
            fp.write(f'MAP_BANK\t{layer}\t{detail["bank"]}\n')
        fp.write('COVERAGE\tunverified\n')
        fp.write('ANIMATION\tunverified\n')
        fp.write('PARALLAX\tunverified\n')
    (output / 'stage_manifest.json').write_text(json.dumps(overview,indent=2)+'\n',encoding='utf-8')
    print(f'Stage76-based importer: frames={len(frames)} map_pages={all_snapshots} '+
          f'palette_states={pal_count} OAM={oam_count} frame={target}')
    print('XRGA layers:', ', '.join(sorted(overview['atlas_layers'])))
    print('STATUS: INCOMPLETE, no falsely verified stage/animation loops')
    return overview


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--capture',type=Path,required=True,help='ZIP archive or stage_capture directory')
    ap.add_argument('--session',default='latest',help='session_XXXX or latest')
    ap.add_argument('--output',type=Path,required=True,help='NEW directory, never overwrite source or Stable')
    ap.add_argument('--frame',type=int,help='frame to reconstruct as 3 independent native tile atlases')
    args = ap.parse_args()
    source = Capture(args.capture,args.session)
    try:
        import_capture(source,args.output,args.frame)
    finally:
        source.close()


if __name__ == '__main__':
    main()
