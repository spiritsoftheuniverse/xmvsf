#!/usr/bin/env python3
"""ROM-free 68000 indexed-dispatch and input-acceptance regression checks."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from ANALYZE_68000_D6_TO_2E_PATHS import (
    D6_DISPATCH_SIG,PHASE_DISPATCH_0,PHASE_DISPATCH_1,
    dispatch_table_index, phase_table_index, original_native_matcher_call,
    accepted_state_setters, verify_launch, resolve_original_rom_script, unswap,
)


class NativeDispatchTests(unittest.TestCase):
    def setUp(self):
        self.buf=bytearray(4096)
    def put(self,at,data):
        self.buf[at:at+len(data)]=data
        self.assertEqual(len(self.buf),4096)  # no accidental bytearray resizing
    def test_original_68000_pc_index_base(self):
        at=0x100;self.put(at,D6_DISPATCH_SIG)
        table=at+len(D6_DISPATCH_SIG)
        self.put(table+0x0C,(0x50).to_bytes(2,'big'))
        first,target=dispatch_table_index(bytes(self.buf),at,0x0C)
        self.assertEqual(target,table+0x50)
        self.assertEqual(first['d6_table_entry_pc'],f'{table+0x0C:06X}')
        self.assertEqual(first['d6_target_pc'],f'{table+0x50:06X}')
        self.put(target,PHASE_DISPATCH_0)
        self.put(target+len(PHASE_DISPATCH_0),(0x1A).to_bytes(2,'big'))
        second,entry=phase_table_index(bytes(self.buf),target)
        self.assertEqual(entry,target+len(PHASE_DISPATCH_0)+0x1a)
        self.assertEqual(second['phase_index_kind'],'MOVE_W_TO_D0_AND_JMP_PC_D0')
        self.put(entry+0x10,bytes.fromhex('702E72024EF90001038E'))
        proof=verify_launch(bytes(self.buf),entry,entry+0x10,(2,))
        self.assertEqual(proof['native_dispatch_jump_pc'],f'{entry+0x14:06X}')
        with self.assertRaises(ValueError):
            verify_launch(bytes(self.buf),entry,entry+0x10,(4,))
    def test_d1_phase_table(self):
        p=0x220
        self.put(p,PHASE_DISPATCH_1)
        self.put(p+len(PHASE_DISPATCH_1),(0x2A).to_bytes(2,'big'))
        proof,target=phase_table_index(bytes(self.buf),p)
        self.assertEqual(target,p+len(PHASE_DISPATCH_1)+0x2A)
        self.assertEqual(proof['phase_index_kind'],'MOVE_W_TO_D1_AND_JMP_PC_D1')
    def test_short_and_word_beq_fallback(self):
        p=0x100; matcher=0x13c30
        self.put(p,bytes.fromhex('4EB900013C306712'))
        row=original_native_matcher_call(bytes(self.buf),p,matcher)
        self.assertEqual(row['failure_target_pc'],'00011A')
        self.put(p,bytes.fromhex('4EB900013C306700FF00'))
        row=original_native_matcher_call(bytes(self.buf),p,matcher)
        self.assertEqual(row['failure_target_pc'],'000008')
    def test_acceptance_and_tamper(self):
        p=0x100
        self.put(p,bytes.fromhex('4EB900013C306708'))
        self.put(p+12,bytes.fromhex('1D7C000C00D6'))
        self.put(p+24,bytes.fromhex('1D7C002A00A0'))
        row=accepted_state_setters(bytes(self.buf),p,0x0C)
        self.assertEqual(row['D6_write_pc'],'00010C')
        self.assertEqual(row['A0_mode_2A_write_pc'],'000118')
        with self.assertRaises(ValueError):
            accepted_state_setters(bytes(self.buf),p,0x0A)
        self.put(p,b'\0')
        with self.assertRaises(ValueError):
            original_native_matcher_call(bytes(self.buf),p,0x13c30)
    def test_ryu_conditional_selector_and_jump_pc(self):
        p=0x300
        self.put(p,bytes.fromhex('702E72080C2E000400A46602720C4EF90001038E'))
        row=verify_launch(bytes(self.buf),p,p,(0x08,0x0C))
        self.assertEqual(row['native_dispatch_jump_pc'],'00030E')
        self.assertIsNotNone(row['conditional_selector'])
    def test_host_word_swap_and_rom_table(self):
        original=bytes.fromhex('70 2e 72 0c 4e f9')
        self.assertEqual(unswap(bytes.fromhex('2e70 0c72 f94e')),original)
        data=bytearray(0x8c000)
        def w(addr,value):data[addr:addr+2]=value.to_bytes(2,'little')
        fighter=0x12
        anchor=0x8a946+fighter*2
        w(anchor,0);w(anchor+2,0x9000)
        w(0x902e,0x20)
        w(0x9020+0x08,0x40)
        self.assertEqual(resolve_original_rom_script(data,fighter,0x08),0x9060)

if __name__=='__main__':
    unittest.main(verbosity=2)
