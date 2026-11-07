#!/usr/bin/env python3
"""Synthetic M68000 producer/consumer opcode and ROM-table tests. No ROM needed."""
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile
sys.path.insert(0,str(Path(__file__).parent))
import TRACE_MVC_POSE_CONSUMERS as m

def put(b,addr,*words):
    struct.pack_into('<'+'H'*len(words),b,addr,*words)

def putlong(b,addr,value):
    put(b,addr,value>>16,value&0xffff)

class ProducerConsumerTests(unittest.TestCase):
    def fixture(self,base):
        run=base/'private'/'run_0001';run.mkdir(parents=True)
        f=bytearray(0x100000);d=bytearray(0x100000)
        for split,selector,lookup,primary,correction in m.PRODUCERS:
            put(f,split,0x7000,0x1028,0x0063,0x0C00,0x00FF,0x6764,0x3200,0x0240,0x001f)
            # A6+0x52 -> D2; D2 *= 2. Indexed first table + upper-three selection.
            put(f,selector,0x342E,0x0052,0xD442,0x227C,primary>>16,primary&0xffff,
                0x2271,0x2000,0xEA49,0xD241,0xD241,0x2D71,0x1000,0x0040,
                0xD040,0x227B)
            displacement=correction-(selector+32)
            put(f,selector+32,0x2000|displacement,0x3031,0x0000)
            first=primary+23*4
            second=[first+32*(i if i<18 else i-18) for i in range(23)]
            for i,address in enumerate(second):
                putlong(d,primary+4*i,address)
                putlong(f,correction+4*i,0x1200)
            for i,address in enumerate(sorted(set(second))):
                for j in range(8):
                    target=0x2400+0x40*((i+j)%16)
                    putlong(d,address+4*j,target)
                    put(d,target,2,(i+j)%16,1,0x39,0x8900+(i+j)%16)
        for pc,ram,shape in m.CONSUMERS:
            put(f,pc,0x286E,0x0040,0x7000,0x301C)
            if pc==0x0BC0D6:
                put(f,pc+8,0x227C,ram>>16,ram&0xffff,0xD3C0,0x49EC,0x0002)
            elif shape=='FIRST_SECOND_THIRD':
                put(f,pc+8,0x43F9,ram>>16,ram&0xffff,0xD3C0,0x301C)
            else:
                put(f,pc+8,0x43F9,ram>>16,ram&0xffff,0xD3C0,0x49EC,0x0002)
        (run/'mvc_program_data_host_words.bin').write_bytes(d)
        (run/'mvc_opcodes_decrypted_host_words.bin').write_bytes(f)
        pose=base/'pose'/'rom_pose_manifest.json';pose.parent.mkdir()
        pose.write_text(json.dumps({'format':'MVC1_68000_CANDIDATE_POSE_RECORDS_V1',
             'private_rom_sha256_for_note_matching':hashlib.sha256(d).hexdigest()}))
        return run,pose

    def test_full_synthetic_tables_and_privacy(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);run,pose=self.fixture(base)
            orig=(run/'mvc_program_data_host_words.bin').read_bytes()
            result=m.report(run.parent,pose,base/'review')
            self.assertEqual(result['producer_motifs_verified'],2)
            self.assertEqual(result['consumer_motifs_verified'],3)
            self.assertEqual(result['indexed_leaf_occurrences'],368)
            self.assertEqual(result['distinct_pointer_target_blocks'],16)
            self.assertEqual(result['five_word_signature_leaf_occurrences'],368)
            self.assertFalse(result['actual_producer_consumer_runtime_flow_verified'])
            with zipfile.ZipFile(base/'review'/'pose_pointer_consumer_review.zip') as z:
                self.assertEqual(set(z.namelist()),set(m.FILES))
                self.assertIsNone(z.testzip())
                self.assertFalse(any(s.endswith('.bin') for s in z.namelist()))
            self.assertEqual(orig,(run/'mvc_program_data_host_words.bin').read_bytes())
            with (base/'review'/'pose_lookup_targets.tsv').open() as file:
                lines=list(csv.DictReader(file,delimiter='\t'))
            self.assertEqual(len(lines),368)
            self.assertEqual(lines[0]['five_word_block_address'],'002400')

    def test_opcode_tampering_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);run,pose=self.fixture(base)
            src=run/'mvc_opcodes_decrypted_host_words.bin'
            raw=bytearray(src.read_bytes());put(raw,0x7911E,0x4e75);src.write_bytes(raw)
            with self.assertRaisesRegex(m.AuditError,'unknown M68000 instruction'):
                m.report(run.parent,pose,base/'review')

    def test_rom_hash_and_output_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);run,pose=self.fixture(base)
            with self.assertRaisesRegex(m.AuditError,'output overlaps'):
                m.report(run.parent,pose,run/'overwrite')
            src=run/'mvc_program_data_host_words.bin'
            raw=bytearray(src.read_bytes());put(raw,0x3000,0xbeef);src.write_bytes(raw)
            with self.assertRaisesRegex(m.AuditError,'STALE'):
                m.report(run.parent,pose,base/'review')

if __name__=='__main__':unittest.main()
