#!/usr/bin/env python3
"""Synthetic ROM-free tests for original 68000 alternate descriptor layouts."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from ANALYZE_68000_ALTERNATE_MATCHERS import (
    LAYOUTS, decode_alternate_record, EXPECTED_FIRST,
)

for matcher,(size,kind) in sorted(LAYOUTS.items()):
    length = size//2
    words = [EXPECTED_FIRST[matcher], 0x9000, 0x000C, 0]
    words += ([0x0010,0x0010,0x0002,0x0100] if length==8 else
              [0x0004,0x0008,0x0700] if length==7 else
              [0x0070])
    assert len(words)==length
    raw = b''.join(word.to_bytes(2,'little') for word in words)
    parsed = decode_alternate_record(raw,0,matcher)
    assert parsed['record_bytes']==size
    assert len(parsed['raw_payload_words'])==length-4
    assert parsed['native_d6_word']=='000C'
    assert parsed['reserved_word']=='0000'
    # A class-0028 7-word record is NOT a 40-token motion.
    if matcher==0x014096:
        assert parsed['first_record_word']=='0028'
        assert parsed['record_bytes']==14
    for invalid in [raw[:-2], b'']:
        try: decode_alternate_record(invalid,0,matcher)
        except ValueError: pass
        else: raise AssertionError('Truncated variant accepted')
    wrong = bytearray(raw);wrong[0:2]=(0x0005).to_bytes(2,'little')
    try: decode_alternate_record(wrong,0,matcher)
    except ValueError: pass
    else: raise AssertionError('Invalid class accepted')
    wrong=bytearray(raw);wrong[6:8]=(1).to_bytes(2,'little')
    try: decode_alternate_record(wrong,0,matcher)
    except ValueError: pass
    else: raise AssertionError('Nonzero reserved word accepted')
print('PASS: four alternate matcher formats, bounded sizes and reject paths')