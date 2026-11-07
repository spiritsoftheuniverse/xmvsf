#!/usr/bin/env python3
"""Synthetic original M68000 header length survey tests. No ROM needed."""
import hashlib,json,struct,sys,tempfile,unittest,zipfile,csv
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
import TRACE_MVC_RECORD_LENGTHS as m

def put(blob,offset,*values):struct.pack_into('<'+'H'*len(values),blob,offset,*values)

class StructuralGrammarTests(unittest.TestCase):
    def example(self,root):
        private=root/'private'/'run_0001';private.mkdir(parents=True)
        blob=bytearray(0x600)
        # Each header is followed by word2+1 four-byte descriptors.
        put(blob,0x40,2,8,2,0x35,0xFA11)
        put(blob,0x4A,0xA6F7,0x0216,0xA6E5,0x1116,0xA705,0x1216)
        put(blob,0x56,2,9,0,0x37,0xFB22)
        put(blob,0x60,0x1C10,0x0400)
        put(blob,0x64,2,10,1,0x39,0xFC33)
        put(blob,0x6E,0x1C12,0x0100,0x1C13,0x0200)
        # A second singleton: a deliberately unrelated $0002 header.
        put(blob,0x180,2,0,0,0x38,0xFBAA)
        (private/'mvc_program_data_host_words.bin').write_bytes(blob)
        pose=root/'pose'/'rom_pose_manifest.json';pose.parent.mkdir()
        pose.write_text(json.dumps({'format':'MVC1_68000_CANDIDATE_POSE_RECORDS_V1',
          'private_rom_sha256_for_note_matching':hashlib.sha256(blob).hexdigest()}))
        return private,pose

    def test_header_length_chain_and_preserves_all_controls(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);run,pose=self.example(root)
            blob=(run/'mvc_program_data_host_words.bin').read_bytes()
            rec,links,regions=m.scan(blob)
            self.assertEqual(len(rec),4)
            self.assertEqual(links,{0x40:0x56,0x56:0x64})
            self.assertEqual(regions,[[0x40,0x56,0x64],[0x180]])
            result=m.report(run.parent,pose,root/'review')
            self.assertEqual(result['five_word_control_grammar_blocks'],4)
            self.assertEqual(result['exact_body_length_next_block_adjacencies'],2)
            self.assertEqual(result['contiguous_structural_regions'],2)
            self.assertEqual(result['control_codes'],{'0035':1,'0037':1,'0038':1,'0039':1})
            self.assertFalse(result['animation_timing_verified'])
            with zipfile.ZipFile(root/'review'/'rom_structural_records_review.zip') as z:
                self.assertEqual(set(z.namelist()),set(m.OUTPUTS))
                self.assertIsNone(z.testzip())
                self.assertFalse(any(n.endswith('.bin') for n in z.namelist()))
            with (root/'review'/'rom_structural_headers.tsv').open() as f:
                rows=list(csv.DictReader(f,delimiter='\t'))
            self.assertEqual(rows[0]['body_start_offset_hex'],'00004A')
            self.assertEqual(rows[0]['inferred_4byte_body_units'],'3')
            self.assertEqual(blob,(run/'mvc_program_data_host_words.bin').read_bytes())

    def test_source_hash_and_output_overlap_guard(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);run,pose=self.example(root)
            with self.assertRaisesRegex(m.RecordError,'output overlaps'):
                m.report(run.parent,pose,run/'unsafe')
            original=run/'mvc_program_data_host_words.bin'
            altered=bytearray(original.read_bytes());put(altered,0x20,0x1234)
            original.write_bytes(altered)
            with self.assertRaisesRegex(m.RecordError,'STALE'):
                m.report(run.parent,pose,root/'review')

    def test_empty_and_misaligned_rejected(self):
        with self.assertRaisesRegex(m.RecordError,'invalid original'):
            m.scan(b'')
        with self.assertRaisesRegex(m.RecordError,'invalid original'):
            m.scan(b'\x00')

if __name__=='__main__':unittest.main()
