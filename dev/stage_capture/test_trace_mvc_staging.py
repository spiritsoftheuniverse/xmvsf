#!/usr/bin/env python3
"""Synthetic read-only Motorola 68000 staging analysis tests; no ROM required."""
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
import TRACE_MVC_STAGING as m

class StagingTests(unittest.TestCase):
    def fixture(self,base):
        run=base/'run_0001'
        run.mkdir(parents=True)
        code=bytearray(0x8000)
        data=bytearray(0x8000)
        def words(at,seq):
            for i,w in enumerate(seq):
                struct.pack_into('<H',code,at+i*2,w)
        words(0x4B32,[0x287C,0x0092,0x4000,0x1E10,0x6A16,0x287C,
                     0x0092,0x5000,0x0207,0x00F0,0x0C07,0x0080,
                     0x6706,0x287C,0x0092,0x6000])
        words(0x0300,[0x207C,0x0092,0x5800])
        words(0x0314,[0x343B,0x500E])
        words(0x0322,[0x4890,0x000F])
        words(0x0380,[0x4EF9,0x0000,0x4B0E])
        struct.pack_into('<H',code,0x0330,0x1234)
        struct.pack_into('<H',data,0x0330,0x5678)
        (run/m.BIN_CODE).write_bytes(code)
        (run/m.BIN_DATA).write_bytes(data)
        return run

    def test_exact_dispatch_and_descriptor(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td)
            run=self.fixture(base/'private')
            result=m.create_report(base/'private',base/'review')
            self.assertTrue(result['dispatcher_bank_select_opcode_match'])
            self.assertGreaterEqual(result['staging_pointer_motifs'],4)
            self.assertEqual(result['descriptor_movem_motifs'],1)
            self.assertEqual(result['nearby_staging_pointer_emitters'],1)
            self.assertEqual(result['indexed_tile_lookup_emitters'],1)
            self.assertEqual(result['dispatcher_direct_call_motifs'],1)
            desc=(base/'review'/'staging_descriptor_emitters.tsv').read_text()
            for expected in ('000322','925800','000314','000324'):
                self.assertIn(expected,desc)
            with zipfile.ZipFile(base/'review'/'m68k_staging_report.zip') as archive:
                self.assertIsNone(archive.testzip())
                self.assertEqual(set(archive.namelist()),set(m.OUTPUTS))
                self.assertFalse(any('.bin' in n or 'opcode' in n for n in archive.namelist()))
            self.assertEqual(m.latest_run(base/'private'),run)
            with self.assertRaisesRegex(m.AnalysisError,'private trace'):
                m.create_report(base/'private',base/'private'/'overlap')

    def test_source_is_required_and_signature_is_verified(self):
        with tempfile.TemporaryDirectory() as td:
            base=Path(td)
            with self.assertRaisesRegex(m.AnalysisError,'missing existing'):
                m.create_report(base/'missing',base/'output')
            run=self.fixture(base/'private')
            code=bytearray((run/m.BIN_CODE).read_bytes())
            struct.pack_into('<H',code,0x4B32,0)
            (run/m.BIN_CODE).write_bytes(code)
            result=m.create_report(base/'private',base/'review')
            self.assertFalse(result['dispatcher_bank_select_opcode_match'])

    def test_unsigned_signed_index_offsets(self):
        self.assertEqual(m.signed8(0x0E),14)
        self.assertEqual(m.signed8(0xA6),-90)
        with self.assertRaises(m.AnalysisError):
            m.word(b'\0\0',2)

if __name__=='__main__':
    unittest.main()
