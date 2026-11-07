#!/usr/bin/env python3
"""Synthetic no-ROM tests for 87.203 cross-capture 68000 ownership evidence."""
import csv
import gzip
import json
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
import TRACE_MVC_OBJECT_OWNERS as obj

FIELDS=['shot','sample','frame','x','y','tile','attributes','priority',
        'palette','width_tiles','height_tiles']
class OwnerEvidenceTests(unittest.TestCase):
    def fixture(self,root:Path):
        trace=root/'trace'/'run_0001'
        trace.mkdir(parents=True)
        code=bytearray(128)
        for at,word in [(0x20,0x207C),(0x22,0x0092),(0x24,0x4000),
                        (0x26,0x343C),(0x28,0x0100),
                        (0x2A,0x363C),(0x2C,0x001F),
                        (0x2E,0x4890),(0x30,0x000F)]:
            struct.pack_into('<H',code,at,word)
        (trace/'mvc_opcodes_decrypted_host_words.bin').write_bytes(code)
        (trace/'sprite_writes.tsv').write_text(
            'frame\tpc\txword\tyword\ttileword\tattrword\n'
            '10\t00002E\tE010\t2000\t0100\t001F\n'
            '11\t00002E\tC010\t2000\t0101\t001F\n',encoding='utf-8')
        emit=root/'emitters.tsv'
        with emit.open('w',newline='') as fp:
            wr=csv.writer(fp,delimiter='\t')
            wr.writerow(['movem_pc_hex','nearby_a0_base_hex'])
            wr.writerow(['00002E','924000'])
        observations=root/'sprite_observations.tsv.gz'
        with gzip.open(observations,'wt',newline='') as fp:
            wr=csv.writer(fp,delimiter='\t')
            wr.writerow(FIELDS)
            for i,shot in enumerate(['shot_0001','shot_0002','shot_0003',
                                     'shot_0004','shot_0005']):
                wr.writerow([shot,0,100+i,48,12,0x100,0x1F,7,31,1,1])
                wr.writerow([shot,0,100+i,8+i,80,0x101,0x1F,6,31,1,1])
        gfx=root/'gfx.bin'
        data=bytearray(0x10000)
        data[(0x101<<7)+5]=0xF0
        gfx.write_bytes(data)
        return observations,root/'trace',emit,gfx

    def test_exact_descriptor_match_and_opaque_pixels(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            obs,trace,emit,gfx=self.fixture(root)
            output=root/'report'
            result=obj.analyze(obs,trace,emit,output,gfx)
            self.assertEqual(result['objects_observed'],10)
            self.assertEqual(result['native_writer_nonblank'],2)
            self.assertEqual(result['native_writer_priority_counts'],{6:1,7:1})
            self.assertEqual(result['provisional_role_counts']['NO_VISIBLE_SOURCE_PIXELS'],5)
            self.assertEqual(result['pixel_visible_priority_counts'],{6:5})
            self.assertEqual(result['68000_descriptor_patterns'],1)
            self.assertEqual(result['literal_tile_and_attribute_pattern_matches'],1)
            owners=(output/'m68k_descriptor_matches.tsv').read_text()
            self.assertIn('00002E',owners)
            self.assertIn('924000',owners)
            self.assertIn('0100',owners)
            self.assertIn('NO_VISIBLE_SOURCE_PIXELS',owners)
            with zipfile.ZipFile(output/'m68k_owner_evidence_review.zip') as z:
                self.assertIsNone(z.testzip())
                self.assertEqual(set(z.namelist()),set(obj.FILES))
                self.assertFalse(any('gfx' in name or 'opcodes' in name
                                     or 'sprite_writes' in name for name in z.namelist()))
            self.assertTrue((trace/'run_0001'/'sprite_writes.tsv').is_file())

    def test_indexed_d2_invalidates_stale_literal(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            obs,trace,emit,gfx=self.fixture(root)
            p=trace/'run_0001'/'mvc_opcodes_decrypted_host_words.bin'
            code=bytearray(p.read_bytes())
            struct.pack_into('<HH',code,0x2A,0x343B,0x500E)
            struct.pack_into('<HH',code,0x2E,0x4890,0x000F)
            p.write_bytes(code)
            d=obj.literal(bytes(code),0x2E)
            self.assertIsNone(d['d2'])
            self.assertEqual(d['d2_kind'],'INDEXED_PC')
            report=obj.analyze(obs,trace,emit,root/'out',gfx)
            self.assertEqual(report['literal_tile_and_attribute_pattern_matches'],0)

    def test_no_source_overwrite_or_false_pixel_claims(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            obs,trace,emit,gfx=self.fixture(root)
            with self.assertRaisesRegex(obj.EvidenceError,'overlap'):
                obj.analyze(obs,trace,emit,trace/'unsafe',gfx)
            report=obj.analyze(obs,trace,emit,root/'no_gfx')
            self.assertFalse(report['original_decoded_pixels_checked'])
            self.assertIsNone(report['pixel_visible_priority_counts'])
            self.assertEqual(report['native_writer_nonblank'],2)

if __name__=='__main__':
    unittest.main()
