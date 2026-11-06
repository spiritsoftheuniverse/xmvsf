#!/usr/bin/env python3
"""Local source-only regression for Wily-format MVC stage reconstruction."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

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
            for item in manifest['atlas_layers'].values():
                data = (Path(td)/'result'/item['file']).read_bytes()
                self.assertEqual(data[:4], b'XRGA')
                w,h = struct.unpack('<II',data[4:12])
                self.assertEqual(len(data), 12+w*h*4)
            (session/'oam.bin').write_bytes(
                b'BAD!'+struct.pack('<III', 1, 0xffffffff, 1)+bytes(8192))
            with self.assertRaisesRegex(ValueError, 'OAM'):
                mod.import_capture(mod.Capture(root,'latest'),Path(td)/'broken')

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
