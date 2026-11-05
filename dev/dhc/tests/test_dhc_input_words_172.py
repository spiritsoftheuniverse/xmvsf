#!/usr/bin/env python3
"""No original ROM required: exact 68000 held/edge and facing transforms."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1] / 'tools'))
from ANALYZE_68000_INPUT_WORDS import (
    SITES, mirror_low_directions, input_tick, verify_opcode_sites,
    direction_evidence,
)


def main():
    assert mirror_low_directions(1) == 2
    assert mirror_low_directions(2) == 1
    assert mirror_low_directions(4) == 4
    assert mirror_low_directions(8) == 8
    assert mirror_low_directions(0x70) == 0x70
    assert mirror_low_directions(0x700) == 0x700
    assert mirror_low_directions(0xF000) == 0
    for mask in range(0x800):
        assert mirror_low_directions(mirror_low_directions(mask)) == (mask & 0x77F)
    # Numeric edge cases for held, freshly pressed, released, and facing flip.
    for current in range(128):
        for old in range(128):
            now=input_tick(current,old,True)
            raw=input_tick(current,old,False)
            assert raw['C4_raw_new_press']==(current & ~old & 0xFFFF)
            assert now['CA_facing_relative_new_press']==mirror_low_directions(raw['C4_raw_new_press'])
            assert now['CA_facing_relative_new_press']==(
                now['C6_facing_relative_held'] & ~now['C8_facing_relative_previous'])
    assert input_tick(0x30,0x30,False)['CA_facing_relative_new_press']==0
    assert input_tick(0,0x30,False)['CA_facing_relative_new_press']==0
    assert input_tick(0x30,0,False)['CA_facing_relative_new_press']==0x30

    # Construct a synthetic code image containing the known specific opcode
    # fragments. Deliberately no proprietary game data is required.
    image=bytearray(0x44000)
    for pc,(signature,_) in SITES.items():
        p=int(pc,16)
        fragment=bytes.fromhex(signature)
        image[p:p+len(fragment)]=fragment
    for pc,disp in ((0x10458,0x68),(0x10464,0x5C),(0x10470,0x50),
                    (0x1048C,0x34),(0x10498,0x28),(0x104A4,0x1C)):
        image[pc:pc+4]=b'\x61\x00'+disp.to_bytes(2,'big')
    assert len(verify_opcode_sites(bytes(image)))==len(SITES)
    image[0x10442]^=1
    try:verify_opcode_sites(bytes(image))
    except ValueError:pass
    else:raise AssertionError('Wrong 68000 opcode signature should reject')
    ryu=[
        {'fighter_id':'12','step_tokens':['0004','0006','0002']},
        {'fighter_id':'12','step_tokens':['0004','0005','0001']}
    ]
    assert direction_evidence(ryu)['fwd_record_count']==1
    print('PASS: 16 opcode sites, 6 branch targets, 2048 facing cases, 16384 input-edge pairs')

if __name__=='__main__':main()
