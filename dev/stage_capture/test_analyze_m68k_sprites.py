#!/usr/bin/env python3
"""Local 68000 write-provenance regression; synthetic data, no ROM required."""
import csv
import json
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import ANALYZE_M68K_SPRITES as m

FIELDS=('frame bank slot address size pc a0 a1 a2 a3 a4 a5 d0 d1 '
        'xword yword tileword attrword byte_offset value status '
        'p1_world_x p1_world_y p2_world_x p2_world_y p1_anim p2_anim').split()

class TraceTests(unittest.TestCase):
    def test_opcode_host_word_swap_and_absent_capstone(self):
        data=bytes.fromhex('00000000714e714e')
        self.assertEqual(m.read_window(data,4),'4E714E71')
        self.assertEqual(m.read_window(data,999),'')
    def write_trace(self,root):
        run=root/'run_0001'
        run.mkdir(parents=True)
        (run/m.OPCODE_NAME).write_bytes(bytes.fromhex('00000000714e714e')*32)
        program=bytearray(512)
        # Three observed native tile IDs appear as a linked table in the
        # original host-word-swapped ROM READ space (little within words).
        struct.pack_into('<HHH',program,64,0x120,0x121,0x122)
        (run/m.DATA_NAME).write_bytes(bytes(program))
        (run/'program_source.tsv').write_text(
            'frame\tdriver\trom_bytes\tdecrypted_bytes\tword_order\tstatus\n'
            '1\tmvscur1\t512\t256\thost_word_swapped\tCAPTURED\n')
        with (run/'sprite_writes.tsv').open('w',newline='') as f:
            wr=csv.DictWriter(f,fieldnames=FIELDS,delimiter='\t')
            wr.writeheader()
            for frame,tile,pc,bank in [(10,0x120,0x04,0),
                                       (11,0x121,0x04,0),
                                       (12,0x122,0x08,0),
                                       (14,0x120,0x04,1)]:
                row={col:'0' for col in FIELDS}
                row.update(frame=str(frame),bank=str(bank),slot='23',
                           address='7080BE',size='2',pc=f'{pc:08X}',
                           a0='00000040',xword=f'{(6<<13)|120:04X}',
                           yword='0000',tileword=f'{tile:04X}',
                           attrword='0402')
                wr.writerow(row)
        return run

    def test_real_pc_tile_family_palette_and_bank_transitions(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td)
            run=self.write_trace(base/'trace')
            output=base/'result'
            report=m.analyze(base/'trace',output)
            self.assertEqual(report['writer_records'],4)
            self.assertEqual(report['unique_writer_pcs'],2)
            self.assertTrue(report['private_code_captured'])
            self.assertTrue(report['private_data_captured'])
            self.assertEqual(report['candidate_tile_transitions'],2)
            self.assertEqual(report['observed_nonblank_tile_words'],3)
            self.assertEqual(report['nonblank_sprite_priorities'],{6:4})
            self.assertEqual(report['rom_aligned_tile_references'],3)
            self.assertEqual(report['rom_consecutive_tile_sequences'],0)
            self.assertFalse(report['reported_handler_pc_equals_instruction_start_verified'])
            self.assertIn('000040', (output/'m68k_rom_tile_references.tsv').read_text())
            self.assertTrue((output/'m68k_rom_tile_sequences.tsv').is_file())
            self.assertFalse(report['verified_animation_tables'])
            pcs=(output/'m68k_writer_pcs.tsv').read_text()
            self.assertIn('000004',pcs)
            self.assertIn('000008',pcs)
            self.assertIn('00120', (output/'m68k_object_families.tsv').read_text())
            self.assertIn('4E71', (output/'m68k_writer_code.tsv').read_text())
            bundle=output/'m68k_sprite_report.zip'
            with zipfile.ZipFile(bundle) as z:
                self.assertIsNone(z.testzip())
                self.assertEqual(set(z.namelist()),set(m.FILE_NAMES))
                self.assertFalse(any('decrypted' in n or 'program_data' in n
                                     or n=='sprite_writes.tsv' for n in z.namelist()))
            self.assertEqual(m.source_run(base/'trace'),run)
            with self.assertRaisesRegex(m.TraceError,'inside'):
                m.analyze(base/'trace',base/'trace'/'unsafe')

    def test_missing_trace_creates_honest_diagnostic(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td)
            report=m.analyze(base/'missing',base/'review')
            self.assertEqual(report['writer_records'],0)
            self.assertFalse(report['private_code_captured'])
            self.assertIn('NO_M68K_WRITER_TRACE',report['warnings'][0])
            self.assertTrue((base/'review'/'m68k_sprite_report.zip').exists())

    def test_latest_source_selection_keeps_old_run_intact(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)/'trace'
            a=self.write_trace(root)
            b=root/'run_0002'
            b.mkdir()
            (b/'sprite_writes.tsv').write_text('frame\n')
            self.assertEqual(m.source_run(root),b)
            self.assertTrue(a.is_dir())

if __name__=='__main__':
    unittest.main()
