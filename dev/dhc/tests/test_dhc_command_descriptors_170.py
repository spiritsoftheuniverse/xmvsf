#!/usr/bin/env python3
"""No proprietary ROM required: exact 68000 descriptor byte-layout checks."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1] / 'tools'))
from ANALYZE_68000_COMMAND_DESCRIPTORS import decode_descriptor, swap_words, be16, le16
from struct import pack

def check(raw_words):
    buf=bytearray(0x100)
    start=0x40
    for i,w in enumerate(raw_words):
        buf[start+2*i:start+2*i+2]=pack('<H',w)
    assert decode_descriptor(buf,start)['descriptor_bytes']==2*len(raw_words)
    return decode_descriptor(buf,start)

a=check([3,0x1000,0x000C,0,2,4,6,0x0770])
assert a['step_count']==3 and a['native_d6_word']=='000C'
assert a['step_tokens']==['0002','0004','0006']
assert a['final_input_mask_word']=='0770'
b=check([5,0x1000,0x0008,0,2,6,4,5,1,0x0700])
assert b['step_count']==5 and b['step_tokens']==['0002','0006','0004','0005','0001']
assert b['final_input_mask_word']=='0700'
c=bytearray(0x50)
c[0x30:0x32]=pack('<H',0x0000)
try:
    decode_descriptor(c,0x30)
    raise AssertionError('zero-length descriptor accepted')
except ValueError:
    pass
assert swap_words(bytes.fromhex('11 22 33 44'))==bytes.fromhex('22 11 44 33')
print('PASS: 3- and 5-step descriptor layouts, reserved state, input masks, invalid length and host word swap')
# Alternate matcher special first word 0028 must not be interpreted
# as a forty-token canonical motion record.
from struct import pack as _pack
special=bytearray(0x40)
special[0:2]=_pack('<H',0x0028)
try:
    decode_descriptor(special,0)
    raise AssertionError('special 0028 record accepted as canonical count')
except ValueError:
    pass
print('PASS: special alternate 0028 header stays undecoded')
