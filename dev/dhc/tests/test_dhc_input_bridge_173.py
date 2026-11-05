#!/usr/bin/env python3
"""Synthetic/no-ROM checks for the 87.173 read-only CPU-branch and chord logic."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1] / 'tools'))
from ANALYZE_68000_DHC_INPUT_BRIDGE import (branch_target, chord_accept, require, unswap)


def run():
    code=bytearray(0x100)
    def put(pc,hexwords):
        blob=bytes.fromhex(hexwords)
        code[pc:pc+len(blob)]=blob
        assert len(code)==0x100
    put(0x10,'674A')       # BEQ.B +0x4A, base next opcode 0x12
    put(0x20,'6700FF32')   # BEQ.W -0xCE, base extension pc 0x22
    put(0x30,'66FE')       # BNE.B -2 -> self branch
    assert branch_target(code,0x10)==('7',0x5C,2)
    assert branch_target(code,0x20)==('7',0x22-0xCE,4)
    assert branch_target(code,0x30)==('6',0x30,2)
    for mask in (0x0070,0x0700):
        bits=[1<<i for i in range(16) if mask&(1<<i)]
        for s in range(8):
            for p in range(8):
                for c in range(8):
                    def expand(v):return sum(bit for i,bit in enumerate(bits) if v&(1<<i))
                    saved,previous,current=map(expand,(s,p,c))
                    should=all(((saved ^ previous) | (previous ^ current)) & bit
                               for bit in bits)
                    assert chord_accept(saved,previous,current,mask)==should, (mask,s,p,c)
    assert chord_accept(0x0030,0,0x0040,0x0070)
    assert not chord_accept(0x0070,0x0070,0x0070,0x0070)
    require(bytes.fromhex('70014e75'),0,'70014e75','success return fixture')
    try:require(bytes.fromhex('70004e75'),0,'70014e75','tamper fixture')
    except ValueError:pass
    else: raise AssertionError('tampering must fail closed')
    assert unswap(bytes.fromhex('1234abcd'))==bytes.fromhex('3412cdab')
    print('PASS: short/long 68000 BEQ/BNE destinations')
    print('PASS: 1,024 independent mask/historical-input combinations')
    print('PASS: byte-swap and source-code tamper refusal')

if __name__=='__main__':run()