#!/usr/bin/env python3
"""Offline synthetic Motorola 68000 sprite assembly and camera regression."""
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
import TRACE_MVC_ROM_ASSEMBLIES as asm

class RomAssemblyTests(unittest.TestCase):
    def fixture(self,base):
        base=Path(base)
        base.mkdir(parents=True,exist_ok=True)
        trace=base/'m68k'/'run_0001';trace.mkdir(parents=True)
        prog=bytearray(0x400)
        for off,pairs in ((0x80,[(0xA6F7,0x0216),(0xA6E5,0x1116),(0xA6EC,0x1216)]),
                          (0x9A,[(0xA705,0x1216),(0xA6F7,0x0216),(0xA6E5,0x1116)])):
            for i,(tile,attr) in enumerate(pairs):
                struct.pack_into('<HH',prog,off+i*4,tile,attr)
        (trace/'mvc_program_data_host_words.bin').write_bytes(prog)
        obs=base/'observations.tsv.gz'
        with gzip.open(obs,'wt',encoding='utf-8',newline='') as f:
            fields=['shot','sample','frame','x','y','tile','attributes','priority','width_tiles','height_tiles']
            wr=csv.DictWriter(f,fieldnames=fields,delimiter='\t');wr.writeheader()
            for sample,frame,cx in ((0,100,100),(1,130,76)):
                for t,a,dx in ((0x1A6F7,0x0216,0),(0x1A6E5,0x1116,32),(0x1A6EC,0x1216,64)):
                    wr.writerow({'shot':'shot_0001','sample':sample,'frame':frame,
                                 'x':cx+dx,'y':100,'tile':t,'attributes':a,
                                 'priority':6,'width_tiles':((a>>8)&15)+1,
                                 'height_tiles':((a>>12)&15)+1})
        owner=base/'object_family_evidence.tsv'
        with owner.open('w',encoding='utf-8',newline='') as f:
            wr=csv.DictWriter(f,fieldnames=['tile_hex','attributes_hex','pixel_visibility','candidate_role'],delimiter='\t');wr.writeheader()
            for t,a in ((0x1A6F7,0x0216),(0x1A6E5,0x1116),(0x1A6EC,0x1216)):
                wr.writerow({'tile_hex':f'{t:05X}','attributes_hex':f'{a:04X}',
                             'pixel_visibility':'VISIBLE','candidate_role':'UNRESOLVED'})
        cam=base/'capture'
        for name,start,end,offset in [('session_0001',1,150,1000),
                                       ('session_0002',1,100,0),
                                       ('session_0003',101,150,0)]:
            d=cam/name;d.mkdir(parents=True)
            with (d/'frames.tsv').open('w',encoding='utf-8',newline='') as f:
                wr=csv.writer(f,delimiter='\t');wr.writerow(['frame',*asm.CAMERA])
                for frame in range(start,end+1):
                    camera2=(24 if frame>=130 else 0)+offset
                    wr.writerow([frame,0,0,camera2,0,0,0])
        return obs,trace.parent,owner,cam

    def test_native_descriptor_pairs_join_to_camera_and_rom(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td)
            obs,trace,owner,cam=self.fixture(base)
            output=base/'out'
            j=asm.report(obs,trace,owner,cam,output)
            self.assertEqual(j['object_observations'],6)
            self.assertEqual(j['rom_candidate_pair_runs'],2)
            self.assertEqual(j['runs_with_camera_motion_and_codrawwn_parts'],2)
            self.assertEqual(j['exact_descriptor_camera_lock_pairs'],3)
            self.assertEqual(j['distinct_camera_co_motion_cohorts_at_least_3_parts'],1)
            self.assertEqual(j['adjacent_candidate_pose_neighbors'],1)
            self.assertEqual(len(j['authoritative_camera_session_chain']),2)
            self.assertTrue(j['authoritative_camera_session_chain'][0].endswith('session_0002/frames.tsv'))
            self.assertEqual(j['conflicted_camera_frames_excluded'],0)
            self.assertFalse(j['observed_stage_ownership_verified'])
            with zipfile.ZipFile(output/'rom_assemblies_review_bundle.zip') as z:
                self.assertEqual(set(z.namelist()),set(asm.REPORTS))
                self.assertIsNone(z.testzip())
                self.assertFalse(any(x.endswith('.bin') or 'gfx' in x for x in z.namelist()))
            self.assertTrue((trace/'run_0001'/'mvc_program_data_host_words.bin').is_file())
            self.assertIn('000080',(output/'rom_candidate_assemblies.tsv').read_text())
            self.assertIn('00009E',(output/'rom_candidate_pose_neighbors.tsv').read_text())

    def test_visibility_rejection_and_sparse_sampling(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td);obs,trace,owner,cam=self.fixture(base)
            # Source coordinates still retained, but no unverified stage
            # candidate is exported when every native pixel is transparent.
            p=owner.read_text().replace('VISIBLE','ALL_TRANSPARENT')
            owner.write_text(p)
            r=asm.report(obs,trace,owner,cam,base/'out')
            self.assertEqual(r['rom_candidate_pair_runs'],0)
            self.assertEqual(r['exact_descriptor_camera_lock_pairs'],0)
            with self.assertRaisesRegex(asm.AssemblyError,'overlap'):
                asm.report(obs,trace,owner,cam,trace/'unsafe')

    def test_original_68000_word_endianness_and_nonmatching_pairs(self):
        blob=bytearray(48)
        struct.pack_into('<HHHH',blob,4,0xA6F7,0x0216,0xA6E5,0x1116)
        r=asm.candidate_runs(bytes(blob),{(0xA6F7,0x0216),(0xA6E5,0x1116)})
        self.assertEqual(r,[(4,[(0xA6F7,0x0216),(0xA6E5,0x1116)])])
        self.assertEqual(asm.signed_change(65400,64,65536),200)
        self.assertEqual(asm.signed_change(400,-496,1024),128)

if __name__=='__main__':unittest.main()