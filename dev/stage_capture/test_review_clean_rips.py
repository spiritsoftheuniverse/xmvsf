#!/usr/bin/env python3
"""Source-only safety tests for automated native MVC clean-rip review.

No source ROM, external screenshot libraries, network, or existing captures
are required; all test bursts are temporary synthetic CPS2 RGBA samples.
"""
from pathlib import Path
import csv
import importlib.util
import json
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from EXPORT_CLEAN_RIP import fnv64

spec = importlib.util.spec_from_file_location('mvc_clean_review',
                                               ROOT / 'REVIEW_CLEAN_RIPS.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class ReviewTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.parent = Path(self.tmp.name)
        self.root = self.parent / 'clean_rips'
        self.root.mkdir()
        self.output = self.parent / 'clean_rips_review'

    def shot(self, number, frames=4, complete=True):
        directory = self.root / f'shot_{number:04d}'
        directory.mkdir()
        with (directory / 'layers.tsv').open('w', newline='') as output:
            writer = csv.writer(output, delimiter='\t')
            writer.writerow(['sample','emulated_frame','layer','file','fnv64',
                             'width','height','scroll_x','scroll_y'])
            for sample in range(frames):
                for layer in range(4):
                    pixel = (sample*12, layer*25, 30, 255 if layer in (0,2) else 0)
                    raw = bytes(pixel) * (384*224)
                    name = f'l{layer}_{sample:06d}.rgba'
                    (directory / name).write_bytes(raw)
                    writer.writerow([sample, 100 + number*100 + sample*2,
                                     layer, name, f'{fnv64(raw):016x}',
                                     384, 224, sample*7, layer])
        if complete:
            (directory / 'CAPTURE_COMPLETE.txt').write_text(f'samples={frames}\n')
        return directory

    def test_sampled_and_full_validation_and_contact_sheet(self):
        self.shot(1)
        self.shot(2)
        status = self.parent / 'stage_capture' / 'status.log'
        quick = mod.review(self.root, self.output, status_log=status)
        self.assertEqual(quick['finished_bursts'], 2)
        self.assertEqual(quick['indexed_samples'], 8)
        self.assertFalse(quick['errors'])
        self.assertEqual(quick['shots'][0]['hash_checked_rgba_files'], 12)
        png = (self.output / 'clean_rips_contact_sheet.png').read_bytes()
        self.assertTrue(png.startswith(b'\x89PNG\r\n\x1a\n'))
        self.assertEqual(struct.unpack_from('>II',png,16)[0], 608)
        self.assertEqual(json.loads(
            (self.output/'clean_rips_review.json').read_text())['level_complete'], False)
        self.assertIn('CLEAN_RIP_REVIEW', status.read_text())
        full = mod.review(self.root, self.output, full=True)
        self.assertEqual(full['shots'][0]['hash_checked_rgba_files'], 16)
        self.assertEqual(full['shots'][0]['status'], 'FULL_VALIDATED')

    def test_partial_burst_and_corruption_stay_visible(self):
        self.shot(1, frames=2, complete=False)
        damaged = self.shot(2, frames=1)
        path = damaged / 'l0_000000.rgba'
        original = path.read_bytes()
        path.write_bytes(b'\xff\x00\x00\xff' + original[4:])
        report = mod.review(self.root, self.output)
        self.assertEqual(report['incomplete_bursts'], 1)
        self.assertEqual(len(report['errors']), 1)
        self.assertIn('FNV mismatch', report['errors'][0]['error'])
        self.assertTrue((self.output/'clean_rips_review.txt').is_file())
        self.assertEqual(path.read_bytes()[:4], b'\xff\x00\x00\xff')

    def test_completion_count_mismatch_fails_closed(self):
        directory = self.shot(1, frames=2)
        (directory / 'CAPTURE_COMPLETE.txt').write_text('samples=15\n')
        with self.assertRaisesRegex(mod.RipError, 'sample count'):
            mod.scan_shot(directory, full=True)

    def test_source_path_is_never_an_output(self):
        self.shot(1)
        with self.assertRaisesRegex(mod.RipError, 'sibling'):
            mod.review(self.root, self.root / 'review')
        with self.assertRaisesRegex(mod.RipError, 'sibling'):
            mod.review(self.root, self.root)
        self.assertFalse((self.root/'review').exists())


if __name__ == '__main__':
    unittest.main()
