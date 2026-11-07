#!/usr/bin/env python3
"""Synthetic CPS2 ROM-record boundary and sample-transition tests. No ROM needed."""
from pathlib import Path
import csv
import gzip
import hashlib
import json
import struct
import sys
import tempfile
import unittest
import zipfile
sys.path.insert(0,str(Path(__file__).parent))
import TRACE_MVC_POSE_BOUNDARIES as pose

class PoseBoundaryTests(unittest.TestCase):
    def records(self):
        first=((0xA6F7,0x0216),(0xA6E5,0x1116))
        second=((0xA705,0x1216),(0xA6E5,0x1116))
        rom=bytearray(0x180)
        for a,pairs,foot in ((0x40,first,(2,13,2,0x35,0xFD68)),
                             (0x52,second,(2,7,1,0x35,0xFD72))):
            seq=[x for pair in pairs for x in pair]+list(foot)
            struct.pack_into('<'+'H'*len(seq),rom,a,*seq)
        return bytes(rom),[(0x40,list(first)),(0x52,list(second))]

    def test_repeating_footer_and_exact_contiguous_boundaries(self):
        raw,runs=self.records()
        records,chains=pose.recover_records(raw,runs)
        self.assertEqual(len(records),2)
        self.assertEqual(chains,[[0x40,0x52]])
        self.assertEqual(records[0x40]['next'],0x52)
        self.assertEqual(records[0x52]['footer'],(2,7,1,0x35,0xFD72))
        damaged=bytearray(raw)
        struct.pack_into('<H',damaged,0x40+8+6,0x0099)
        records,chains=pose.recover_records(bytes(damaged),runs)
        self.assertNotIn(0x40,records)
        self.assertEqual(chains,[[0x52]])

    def fixture(self,base):
        root=base/'m68k'/'run_0001';root.mkdir(parents=True)
        binary,_=self.records()
        (root/'mvc_program_data_host_words.bin').write_bytes(binary)
        obs=base/'sprite_observations.tsv.gz'
        pairs=((100,((0x1A6F7,0x0216),(0x1A6E5,0x1116)),100),
               (130,((0x1A705,0x1216),(0x1A6E5,0x1116)),76))
        with gzip.open(obs,'wt',newline='',encoding='utf-8') as f:
            fields=['shot','sample','frame','x','y','tile','attributes',
                    'priority','width_tiles','height_tiles']
            w=csv.DictWriter(f,fieldnames=fields,delimiter='\t');w.writeheader()
            for n,(frame,items,x) in enumerate(pairs):
                for i,(tile,attr) in enumerate(items):
                    w.writerow(dict(shot='shot_0001',sample=n,frame=frame,
                         x=x+i*16,y=100,tile=tile,attributes=attr,priority=6,
                         width_tiles=1,height_tiles=1))
        owner=base/'owner.tsv'
        with owner.open('w',newline='',encoding='utf-8') as f:
            w=csv.DictWriter(f,fieldnames=['tile_hex','attributes_hex','pixel_visibility','candidate_role'],delimiter='\t')
            w.writeheader()
            for tile,attr in ((0x1A6F7,0x0216),(0x1A705,0x1216),(0x1A6E5,0x1116)):
                w.writerow(dict(tile_hex=f'{tile:05X}',attributes_hex=f'{attr:04X}',
                                pixel_visibility='VISIBLE',candidate_role='UNRESOLVED'))
        cam=base/'cam'/'session_0001';cam.mkdir(parents=True)
        with (cam/'frames.tsv').open('w',newline='',encoding='utf-8') as f:
            w=csv.writer(f,delimiter='\t');w.writerow(['frame',*pose.source.CAMERA])
            for frame in (100,130):w.writerow([frame,0,0,24 if frame==130 else 0,0,0,0])
        return obs,root.parent,owner,cam.parent

    def test_complete_offline_pose_report_and_source_privacy(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td);obs,trace,owner,cam=self.fixture(base)
            notes=base/'review.tsv'
            with notes.open('w',newline='',encoding='utf-8') as f:
                w=csv.writer(f,delimiter='\t')
                w.writerow(['record_offset_hex','shot','sample_frame',
                            'visual_category','confidence','reviewer_note','rom_sha256'])
                good_sha=hashlib.sha256((trace/'run_0001'/'mvc_program_data_host_words.bin').read_bytes()).hexdigest()
                w.writerow(['000040','shot_0001',100,'ABSTRACT','LOW','Not engine verified',good_sha])
                w.writerow(['001234','shot_0001',100,'STALE','LOW','Must not be applied',good_sha])
                w.writerow(['000052','shot_0001',130,'WRONG_ROM','LOW','Do not propagate',('0'*64)])
            report=pose.report(obs,trace,owner,cam,base/'out',notes)
            self.assertEqual(report['manual_visual_notes_applied'],1)
            self.assertEqual(report['manual_visual_notes_skipped_checksum'],1)
            self.assertIn('ABSTRACT',(base/'out'/'rom_pose_visual_review.tsv').read_text())
            self.assertNotIn('STALE',(base/'out'/'rom_pose_visual_review.tsv').read_text())
            self.assertEqual(report['original_rom_pair_runs'],2)
            self.assertEqual(report['records_with_repeating_control_footers'],2)
            self.assertEqual(report['records_with_adjacent_validated_next'],1)
            self.assertEqual(report['multi_record_chains'],1)
            self.assertEqual(report['distinctive_pose_matches'],2)
            self.assertEqual(report['observed_pose_changes_between_samples'],1)
            self.assertFalse(report['original_68000_animation_frame_boundary_verified'])
            with zipfile.ZipFile(base/'out'/'rom_pose_boundaries_review.zip') as z:
                self.assertEqual(set(z.namelist()),set(pose.OUTPUTS))
                self.assertIsNone(z.testzip())
                self.assertFalse(any(n.endswith('.bin') or 'sprite_observations' in n
                                     for n in z.namelist()))
            self.assertEqual((trace/'run_0001'/'mvc_program_data_host_words.bin').read_bytes(),self.records()[0])
            transitions=(base/'out'/'rom_pose_sample_transitions.tsv').read_text()
            self.assertIn('OBSERVED_BETWEEN_SAMPLES',transitions)
            with self.assertRaisesRegex(pose.PoseError,'overlaps'):
                pose.report(obs,trace,owner,cam,trace/'unsafe')

    def test_ambiguous_shared_parts_not_declared_distinct_pose(self):
        raw,runs=self.records()
        rec,chains=pose.recover_records(raw,runs)
        grouped={'shot_0001':{100:[(0x1A6E5,0x1116,6,1,1,100,100),
                                    (0x1A705,0x1216,6,1,1,116,100)],
                              130:[(0x1A6E5,0x1116,6,1,1,76,100),
                                   (0x1A705,0x1216,6,1,1,92,100)]}}
        matches,trans,metrics=pose.frame_support(grouped,rec,chains,{}, {})
        self.assertEqual(len(trans),0)
        self.assertTrue(all(x[4]==2 for x in matches))

if __name__=='__main__':unittest.main()
