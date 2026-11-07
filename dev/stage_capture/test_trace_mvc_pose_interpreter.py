#!/usr/bin/env python3
"""Synthetic 68000 pose-interpreter leads tests; no copyrighted ROM needed."""
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).parent))
import TRACE_MVC_POSE_INTERPRETER as interp


class PoseInterpreterTests(unittest.TestCase):
    @staticmethod
    def put(blob, address, *words):
        struct.pack_into('<' + 'H' * len(words), blob, address, *words)

    def fixture(self, root):
        trace = root / 'trace' / 'run_0001'
        trace.mkdir(parents=True)
        original = bytearray(0x600)
        fetch = bytearray(0x400)
        # Adjacent records: 8-byte tile pairs, then a real ten-byte footer.
        self.put(original, 0x100, 0xA6F7, 0x0216, 0xA6E5, 0x1116,
                 2, 13, 2, 0x35, 0xFD68)
        self.put(original, 0x112, 0xA705, 0x1216, 0xA6E5, 0x1116,
                 2, 7, 1, 0x36, 0xFD72)
        # READ image contains an aligned 32-bit address, but it is not code.
        self.put(original, 0x310, 0x0000, 0x0112)
        for source_at in (0x340, 0x370):
            self.put(original, source_at,
                     0, 0x108, 0, 0x11A, 0, 0x108, 0, 0x11A)
        # Recognized MOVEA.L #$00000100,A0 in decrypted FETCH words.
        self.put(fetch, 0x80, 0x207C, 0x0000, 0x0100)
        # Unclassified exact pointer; must NOT be labeled an instruction.
        self.put(fetch, 0xC0, 0xDEAD, 0x0000, 0x0112)
        # CMPI.W #$35,D0, SUBI.W #$35,D0, CMPI.W #4,D0, indexed JMP.
        self.put(fetch, 0xF0, 0x0C40, 0x0035, 0x0440, 0x0035,
                 0x0C40, 0x0004, 0x4EFB, 0x0004)
        # Control site far away is only an independent static motif.
        self.put(fetch, 0x350, 0x0C41, 0x0036)
        self.put(fetch, 0x354, 0x7239)  # MOVEQ #$39,D1 (embedded 8-bit)
        (trace / interp.staging.BIN_CODE).write_bytes(fetch)
        (trace / interp.staging.BIN_DATA).write_bytes(original)
        poses = root / 'pose'
        poses.mkdir()
        with (poses / 'rom_pose_records.tsv').open('w', newline='', encoding='utf-8') as f:
            names = ['record_offset_hex', 'chain_start_hex', 'tile_attribute_pairs',
                     'footer_offset_hex', *('control_%d_hex' % i for i in range(5))]
            writer = csv.DictWriter(f, fieldnames=names, delimiter='\t')
            writer.writeheader()
            for start, footer, values in (
                (0x100, 0x108, (2, 13, 2, 0x35, 0xFD68)),
                (0x112, 0x11A, (2, 7, 1, 0x36, 0xFD72))
            ):
                row = {'record_offset_hex': f'{start:06X}',
                       'chain_start_hex': '000100',
                       'tile_attribute_pairs': 2,
                       'footer_offset_hex': f'{footer:06X}'}
                row.update({'control_%d_hex' % i: f'{v:04X}'
                            for i, v in enumerate(values)})
                writer.writerow(row)
        manifest = {
            'format': 'MVC1_68000_CANDIDATE_POSE_RECORDS_V1',
            'private_rom_sha256_for_note_matching': hashlib.sha256(original).hexdigest(),
            'records_with_repeating_control_footers': 2,
        }
        (poses / 'rom_pose_manifest.json').write_text(json.dumps(manifest), encoding='utf-8')
        return trace.parent, poses / 'rom_pose_records.tsv', original

    def test_report_pointer_classes_motifs_and_no_raw_bytes(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            trace, poses, original = self.fixture(root)
            out = root / 'output'
            m = interp.report(trace, poses, out)
            self.assertEqual(m['pose_records_verified_against_original_rom'], 2)
            self.assertEqual(m['recognized_fetch_address_load_motifs'], 1)
            self.assertEqual(m['read_pointer_blocks_min_four_refs'], 2)
            self.assertEqual(m['read_pointer_block_references'], 8)
            self.assertEqual(m['longest_read_pointer_block'], 4)
            self.assertGreaterEqual(m['selector_immediate_opcode_motifs'], 3)
            self.assertGreaterEqual(m['selector_near_indexed_jump_motifs'], 2)
            self.assertFalse(m['footer_field_semantics_decoded'])
            self.assertFalse(m['animation_frame_duration_verified'])
            pointers = (out / interp.OUTPUTS[0]).read_text(encoding='utf-8')
            self.assertIn('MOVEA_L_IMMEDIATE_A0', pointers)
            self.assertIn('ALIGNED_LONG_IN_READ_DATA', pointers)
            self.assertIn('UNCLASSIFIED_FETCH_WORDS', pointers)
            blocks = (out / interp.OUTPUTS[1]).read_text(encoding='utf-8')
            self.assertIn('000370', blocks)
            self.assertIn('000340', blocks)
            self.assertIn('OBSERVED_ALIGNED_POINTER_SUBSEQUENCE', blocks)
            motifs = (out / interp.OUTPUTS[2]).read_text(encoding='utf-8')
            self.assertIn('MOVEQ_IMMEDIATE_D1', motifs)
            leads = (out / interp.OUTPUTS[3]).read_text(encoding='utf-8')
            self.assertIn('JMP_PC_INDEXED', leads)
            self.assertIn('0000F8', leads)
            with zipfile.ZipFile(out / 'pose_footer_interpreter_leads.zip') as z:
                self.assertEqual(set(z.namelist()), set(interp.OUTPUTS))
                self.assertIsNone(z.testzip())
                self.assertFalse(any(n.endswith('.bin') for n in z.namelist()))
                self.assertNotIn(original, [z.read(n) for n in z.namelist()])
            self.assertEqual((trace / 'run_0001' / interp.staging.BIN_DATA).read_bytes(),
                             original)

    def test_stale_snapshot_and_tampered_footer_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            trace, poses, original = self.fixture(root)
            path = trace / 'run_0001' / interp.staging.BIN_DATA
            changed = bytearray(original)
            self.put(changed, 0x35A, 0xABCD)
            path.write_bytes(changed)
            with self.assertRaisesRegex(interp.InterpreterError, 'DIFFERENT'):
                interp.report(trace, poses, root / 'out')
            # Updating checksum does not circumvent exact source-field checks.
            manifest = poses.with_name('rom_pose_manifest.json')
            obj = json.loads(manifest.read_text())
            obj['private_rom_sha256_for_note_matching'] = hashlib.sha256(changed).hexdigest()
            manifest.write_text(json.dumps(obj))
            self.put(changed, 0x108 + 6, 0x0039)
            path.write_bytes(changed)
            obj['private_rom_sha256_for_note_matching'] = hashlib.sha256(changed).hexdigest()
            manifest.write_text(json.dumps(obj))
            with self.assertRaisesRegex(interp.InterpreterError, 'does not match'):
                interp.report(trace, poses, root / 'out')

    def test_bounds_and_safe_outputs(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            trace, poses, original = self.fixture(root)
            with self.assertRaisesRegex(interp.InterpreterError, 'overlaps'):
                interp.report(trace, poses, trace / 'unsafe')
            with self.assertRaisesRegex(interp.InterpreterError, 'overlaps'):
                interp.report(trace, poses, poses.parent / 'unsafe')
            with self.assertRaisesRegex(interp.InterpreterError, 'unaligned'):
                interp.word(original, 0x101)
            self.assertEqual(interp.signed8(0xFE), -2)


if __name__ == '__main__':
    unittest.main()
