#!/usr/bin/env python3
"""Local source-only regression for Wily-format MVC stage reconstruction."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile
import gzip

spec = importlib.util.spec_from_file_location(
    'stage_import', Path(__file__).with_name('BUILD_STAGE_FROM_CAPTURE.py'))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class StageImporterTests(unittest.TestCase):
    def test_every_layer_map_address_is_bijective(self):
        for layer in (1, 2, 3):
            addresses = {mod.index_for_xy(layer, x, y)
                         for x in range(64) for y in range(64)}
            self.assertEqual(addresses, set(range(4096)))

    def test_packed_pen_byte_order_and_flipping(self):
        gfx = bytearray(4096)
        gfx[:4] = bytes([0x12, 0x34, 0x56, 0x78])
        # ctv_do.h plots nibble sequence 7,8,5,6,3,4,1,2.
        palette = bytearray(6144)
        idx = ((0x20 << 4) | 7) ^ 15
        struct.pack_into('<H', palette, idx * 2, 0x0fff)
        tile = mod.tile_rgba(bytes(gfx), 0xfff, 0, 0, 0, bytes(palette), 1)
        self.assertEqual(tuple(tile[0:4]), (85, 85, 85, 255))
        self.assertEqual(tile[4*8:4*9], bytes(4))
        flipped = mod.tile_rgba(bytes(gfx), 0xfff, 0, 0, 0x20, bytes(palette), 1)
        self.assertNotEqual(tile[:32], flipped[:32])

    def test_import_and_reject_bad_oam(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)/'capture'
            session = root/'session_0001'
            session.mkdir(parents=True)
            (root/'latest.txt').write_text('session_0001\n')
            (session/'frames.tsv').write_text(
                'frame\tstage\tphase\tslices\tscroll1x\tscroll1y\tscroll2x\tscroll2y\tscroll3x\tscroll3y\tobjectBank\n'
                '1\t4294967295\t4294967295\t1\t0\t0\t0\t0\t0\t0\t1\n')
            filename = 'mvc1_decoded_gfx_00001000_test.bin'
            (session/filename).write_bytes(bytes([0xff])*4096)
            (session/'gfx_index.tsv').write_text(
                f'1\tmvscur1\t4096\t00000FFF\t0\t0\t0\t1234\t{filename}\n')
            regs = bytearray(256)
            for offset, bank in ((2, 0x9080), (4, 0x90c0), (6, 0x9100)):
                struct.pack_into('<H', regs, offset, bank)
            (session/'raster.bin').write_bytes(
                b'FRM1'+struct.pack('<III', 1, 0xffffffff, 1)
                +struct.pack('<I', 0)+regs+bytes(16))
            (session/'palettes.bin').write_bytes(
                b'PAL1'+struct.pack('<IIQ', 1, 0xffffffff, 0)+bytes(6144))
            (session/'oam.bin').write_bytes(
                b'OAM1'+struct.pack('<III', 1, 0xffffffff, 1)+bytes(8192))
            page = bytearray(16384)
            for layer in (1, 2, 3):
                struct.pack_into('<HH', page, 4*mod.index_for_xy(layer, 0, 0), 1, 0)
            (session/'maps.bin').write_bytes(
                b''.join(b'MAP1'+struct.pack('<IIIIIQ', 1, 0xffffffff, 0, layer, bank, 0)+page
                         for layer, bank in ((1,0x908000), (2,0x90c000), (3,0x910000))))
            (session/'row_scroll.bin').write_bytes(b'')
            capture = mod.Capture(root, 'latest')
            try:
                manifest = mod.import_capture(capture, Path(td)/'result')
            finally:
                capture.close()
            self.assertEqual(manifest['integrity']['map_snapshots'], 3)
            self.assertEqual(manifest['integrity']['oam_frames'], 1)
            self.assertEqual(len(manifest['atlas_layers']), 3)
            self.assertFalse(manifest['level_complete'])
            self.assertEqual(manifest['camera_preview_raw']['banks'],
                             [0x908000,0x90c000,0x910000])
            with gzip.open(Path(td)/'result'/'camera_timeline.tsv.gz','rt') as timeline:
                entries = timeline.read().splitlines()
                self.assertEqual(len(entries),2)
                self.assertIn('bank1',entries[0])
                self.assertIn('908000',entries[1])
            self.assertTrue((Path(td)/'result'/
                'preview_approx_composite_frame000001.png').is_file())
            for item in manifest['atlas_layers'].values():
                data = (Path(td)/'result'/item['file']).read_bytes()
                self.assertEqual(data[:4], b'XRGA')
                w,h = struct.unpack('<II',data[4:12])
                self.assertEqual(len(data), 12+w*h*4)
            (session/'oam.bin').write_bytes(
                b'BAD!'+struct.pack('<III', 1, 0xffffffff, 1)+bytes(8192))
            with self.assertRaisesRegex(ValueError, 'OAM'):
                mod.import_capture(mod.Capture(root,'latest'),Path(td)/'broken')

    def test_stage_viewport_wrap_and_valid_png(self):
        w=h=512
        pixels=bytearray(w*h*4)
        pixels[0:4]=b'\x10\x20\x30\xff'
        pixels[-4:]=b'\xaa\xbb\xcc\xff'
        atlas=b'XRGA'+struct.pack('<II',w,h)+pixels
        actual=mod.crop_native_viewport(atlas,511,511)
        self.assertEqual(actual[:4],bytes.fromhex('aabbccff'))
        self.assertEqual(actual[4*(384+1):4*(384+2)],bytes.fromhex('102030ff'))
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'preview.png'
            mod.write_png_rgba(path,384,224,actual)
            png=path.read_bytes()
            self.assertTrue(png.startswith(b'\x89PNG\r\n\x1a\n'))
            self.assertEqual(struct.unpack('>II',png[16:24]),(384,224))
        opaque=bytearray(len(actual))
        opaque[0:4]=b'\x11\x22\x33\xff'
        composite=mod.compose_diagnostic_layers({3:actual,1:bytes(opaque)})
        self.assertEqual(composite[:4],bytes.fromhex('112233ff'))

    def test_continuous_batches_ignore_overlapping_old_runs(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)/'stage_capture'
            root.mkdir()
            (root/'latest.txt').write_text('session_0004\n')
            header = 'frame\tstage\tphase\tslices\tscroll1x\n'
            records = {
                'session_0002': [1, 2, 3, 4, 5, 6, 7, 8, 9],
                'session_0003': [1, 2],
                'session_0004': [3, 4],
            }
            for session, frames in records.items():
                d = root/session
                d.mkdir()
                (d/'frames.tsv').write_text(
                    header+''.join(f'{frame}\t0\t0\t1\t0\n' for frame in frames))
                (d/'gfx_index.tsv').write_text(
                    f'{frames[0]}\tmvscur1\t4096\t00000FFF\t0\t0\t0\t1234\tdonor.bin\n')
                (d/'maps.bin').write_bytes(session.encode('ascii'))
                (d/'oam.bin').write_bytes(bytes(frames))
            for source in (root, Path(td)/'packed.zip'):
                if source.suffix == '.zip':
                    with zipfile.ZipFile(source,'w') as archive:
                        for file in root.rglob('*'):
                            if file.is_file():
                                archive.write(file, file.relative_to(root.parent))
                capture = mod.open_reconstruction_capture(source, 'latest')
                try:
                    self.assertIsInstance(capture, mod.ContinuousCapture)
                    self.assertEqual(capture.sessions,
                                     ['session_0003','session_0004'])
                    self.assertEqual(capture.session, 'session_0003..session_0004')
                    with capture.open('frames.tsv') as fp:
                        lines = fp.read().decode().splitlines()
                    self.assertEqual(lines.count(header.strip()), 1)
                    self.assertEqual([int(line.split('\t')[0]) for line in lines[1:]],
                                     [1, 2, 3, 4])
                    with capture.open('oam.bin') as fp:
                        self.assertEqual(fp.read(3), bytes([1, 2, 3]))
                        self.assertEqual(fp.read(1), bytes([4]))
                        self.assertEqual(fp.read(), b'')
                    self.assertEqual(capture.size('oam.bin'), 4)
                finally:
                    capture.close()
                one = mod.open_reconstruction_capture(source, 'session_0004')
                try:
                    self.assertEqual(one.session, 'session_0004')
                    with one.open('frames.tsv') as fp:
                        self.assertEqual(len(fp.readlines()), 3)
                finally:
                    one.close()

    def test_continuous_requires_consecutive_batch_frames(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root/'latest.txt').write_text('session_0002\n')
            for session, frames in [('session_0001',[1,2]),
                                    ('session_0002',[5,6])]:
                d = root/session
                d.mkdir()
                (d/'frames.tsv').write_text(
                    'frame\tslices\n'+''.join(f'{f}\t1\n' for f in frames))
                (d/'gfx_index.tsv').write_text(
                    f'{frames[0]}\tmvscur1\t4\t00000003\t0\t0\t0\t1234\ta.bin\n')
            latest = mod.open_reconstruction_capture(root,'latest')
            self.assertIsInstance(latest, mod.Capture)
            latest.close()
            with self.assertRaisesRegex(ValueError,'no contiguous'):
                mod.open_reconstruction_capture(root,'continuous')

    def test_zip_latest_session_resolution(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td)/'capture.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('stage_capture/latest.txt', 'session_0007\n')
                archive.writestr('stage_capture/session_0007/frames.tsv', 'frame\n1\n')
            capture = mod.Capture(path, 'latest')
            try:
                self.assertEqual(capture.session, 'session_0007')
                self.assertEqual(capture.size('frames.tsv'), 8)
            finally:
                capture.close()


if __name__ == '__main__':
    unittest.main()
