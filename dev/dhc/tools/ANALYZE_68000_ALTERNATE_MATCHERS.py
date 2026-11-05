#!/usr/bin/env python3
"""Read-only CPS2 original 68000 alternate command matcher audit (Stage 87.171).

Reads private user-owned decrypted CpsCode + host-swapped CpsRom in memory.
Writes only derived offsets, scalar descriptor words, and instruction facts.
It does not map the moves or enable any DHC command.
"""
from __future__ import annotations
import argparse, collections, csv, hashlib, json
from pathlib import Path
from ANALYZE_68000_COMMAND_DESCRIPTORS import (
    MATCHERS, NAMES, extract, swap_words, le16, be16
)

# Derived only from the original alternate-matcher programs. Do not parse
# any variant by the canonical 013C30 n=3/n=5 assumption.
LAYOUTS = {
    0x013F76: (10, 'CLASS_0004_SINGLE_MASK'),
    0x014096: (14, 'CLASS_0028_TWO_HISTORY_MASKS_AND_FINAL_MASK'),
    0x014392: (16, 'CLASS_0004_THREE_HISTORY_WORDS_AND_FINAL_MASK'),
    0x01458E: (10, 'CLASS_0002_FINAL_MASK_WITH_DIRECTION_HISTORY'),
}
EXPECTED_FIRST = {0x013F76:4, 0x014096:0x28, 0x014392:4, 0x01458E:2}
# Known native opcode tokens checked at the original PC. The CpsCode image
# contains executable instructions; do NOT use its scrambled literal-table
# bytes as if they were CpsRom values.
READ_PROOFS = {
    0x013F76: (0x013FF2, '322e00cac252', '$00CA', 'MOVE.W (A6+CA),D1; AND.W (A2),D1'),
    0x014096: (0x0140D2, '322e00c6c252', '$00C6', 'MOVE.W (A6+C6),D1; AND.W (A2),D1'),
    0x014392: (0x014426, '322e00ca0241077fb252', '$00CA', 'MOVE.W (A6+CA),D1; ANDI.W #077F,D1; CMP.W (A2),D1'),
    0x01458E: (0x0145C8, '302e00c6', '$00C6', 'MOVE.W (A6+C6),D0'),
}
# These PCs are original 68000 opcode boundaries, not speculative guessed
# labels; comparing the full signature prevents false rule generalization.
EXACT_CODE = {
    0x0145CE: '0c400008671a72010c400002671272020c400004670a72030c400001',
    0x0145EE: '541330121740000217410003',
    0x0140F6: '322e00c6c26afffe',
    0x01410A: '322e00c6c26afffc',
    0x013FF2: '322e00cac252',
    0x014426: '322e00ca0241077fb252',
}
COMMON_GATE = bytes.fromhex('4a2e0003661a4a2e029b6710302a0004b02e00d66606224a6000')


def raw_word(rom, pos):
    if pos<0 or pos+2>len(rom):
        raise ValueError('Out-of-range data-ROM word %06X' % pos)
    return le16(rom,pos)



def decode_alternate_record(rom: bytes, pos: int, matcher: int) -> dict:
    """One bounded native variant record; never interpret header as step count."""
    if matcher not in LAYOUTS:
        raise ValueError('Unknown alternate matcher %06X' % matcher)
    size, kind = LAYOUTS[matcher]
    if pos < 0 or pos + size > len(rom):
        raise ValueError('Truncated native variant record at %06X' % pos)
    words = [raw_word(rom, pos+2*i) for i in range(size//2)]
    if words[0] != EXPECTED_FIRST[matcher]:
        raise ValueError('Unsupported record class %04X at %06X' % (words[0],pos))
    if words[3] != 0:
        raise ValueError('Native reserved field changed at %06X' % pos)
    if words[2] > 0xFF:
        raise ValueError('D6 state word outside byte range at %06X' % pos)
    return {
        'first_record_word': f'{words[0]:04X}',
        'flags_word': f'{words[1]:04X}',
        'native_d6_word': f'{words[2]:04X}',
        'reserved_word': f'{words[3]:04X}',
        'raw_payload_words': [f'{v:04X}' for v in words[4:]],
        'record_bytes': size,
        'record_kind': kind,
        'classification': 'NATIVE_DESCRIPTOR_RAW_WORDS_NOT_VERIFIED_MOVE_COMMAND',
    }

def audit(raw_opcodes: bytes, rom: bytes) -> dict:
    code = swap_words(raw_opcodes)
    _,_,_,alternates = extract(code,rom)
    checks = []
    for pc,signature in EXACT_CODE.items():
        want=bytes.fromhex(signature)
        if code[pc:pc+len(want)]!=want:
            raise ValueError('Native 68000 instruction bytes changed at %06X' % pc)
        checks.append({'pc':f'{pc:06X}','opcode_hex':signature,
                       'status':'EXACT_USER_ROM_OPCODE_MATCH'})
    per_matcher=[]
    for matcher,(size,kind) in LAYOUTS.items():
        if code[matcher:matcher+len(COMMON_GATE)]!=COMMON_GATE:
            raise ValueError('Alternate matcher native state/actor gate changed at %06X'%matcher)
        proof_pc,proof_hex,source,summary=READ_PROOFS[matcher]
        if code[proof_pc:proof_pc+len(bytes.fromhex(proof_hex))]!=bytes.fromhex(proof_hex):
            raise ValueError('Native matcher input read changed at %06X'%proof_pc)
        per_matcher.append({
            'matcher_pc':f'{matcher:06X}',
            'actor_state_gate':'CMP.W descriptor(+4),actor($00D6) under $0003/$029B gates',
            'state_gate_exact_opcode_prefix':COMMON_GATE.hex(),
            'input_read_pc':f'{proof_pc:06X}',
            'native_input_word':source,
            'input_opcode_hex':proof_hex,
            'input_instruction_description':summary,
            'record_bytes':size,
            'record_classification':kind,
            'first_record_word':f'{EXPECTED_FIRST[matcher]:04X}',
            'classification':'NATIVE_CONTROL_FLOW_AND_RAW_INPUT_MASK_ONLY'
        })
    records=[]
    for entry in alternates:
        matcher=int(entry['matcher_pc'],16)
        assert matcher in LAYOUTS
        size,kind=LAYOUTS[matcher]
        pos=int(entry['descriptor_data_pc'],16)
        variant = decode_alternate_record(rom, pos, matcher)
        records.append({
            'fighter_id':entry['fighter_id'],
            'fighter_name':entry['fighter_name'],
            'matcher_pc':entry['matcher_pc'],
            'setup_pc':entry['setup_pc'],
            'matcher_call_pc':entry['matcher_call_pc'],
            'descriptor_data_pc':entry['descriptor_data_pc'],
            'history_a6_offset':entry['history_a6_offset'],
            **variant,
            'native_input_word':READ_PROOFS[matcher][2],
        })
    if len(records)!=12 or sorted(collections.Counter(int(x['matcher_pc'],16) for x in records).values())!=[1,2,2,7]:
        raise ValueError('Native alternate matcher census drift; stop without guessing')
    # Five-way progress-state dispatch is encoded directly in native opcodes.
    def switch(base_pc, table_pc, entries):
        offsets=[be16(code,table_pc+2*i) for i in range(entries)]
        # A CPS2 opcode-fetch buffer and native program-data buffer may
        # disagree in encrypted code-page regions. Preserve this mismatch:
        # do not certify a runtime indirect branch from opcode bytes alone.
        data_words=[raw_word(rom,table_pc+2*i) for i in range(entries)]
        return {
            'matcher_pc':f'{base_pc:06X}',
            'opcode_image_table_pc':f'{table_pc:06X}',
            'program_data_table_words':[f'{w:04X}' for w in data_words],
            'opcode_table_equals_data_rom':data_words==offsets,
            'runtime_memory_bus_choice':'UNVERIFIED',
            'opcode_image_branch_candidates':[
                {'history_index_byte':i*2,
                 'relative_offset':f'{offset:04X}',
                 'candidate_target_pc':f'{table_pc+offset:06X}'}
                for i,offset in enumerate(offsets)
            ]
        }
    branch_tables={
        '014096':switch(0x014096,0x0140C8,5),
        '01458E':switch(0x01458E,0x0145BA,5),
    }
    # 01458E exact original comparisons: D0=$C6; D1 starts at zero, then
    # maps four accepted values to 0..3; otherwise resets history.
    direction_values=[(8,0),(2,1),(4,2),(1,3)]
    return {
        'schema':'xmvsf-68000-alternate-native-command-matchers-v1',
        'opcode_sha256':hashlib.sha256(raw_opcodes).hexdigest(),
        'data_rom_sha256':hashlib.sha256(rom).hexdigest(),
        'alternate_matchers':per_matcher,
        'all_exact_code_checks':checks,
        'alternate_calls':len(records),
        'counts_per_matcher':dict(sorted(collections.Counter(x['matcher_pc'] for x in records).items())),
        'records':records,
        'progress_switches':branch_tables,
        'zangief_01458e_input_word_comparison':[{'C6_value':f'{v:04X}', 'encoded_class':idx}
                                                for v,idx in direction_values],
        'zangief_recorded_history_writes':[
            {'pc':'0145EE','meaning':'ADDQ.B #2,(A3) — advance history state'},
            {'pc':'0145F0','meaning':'MOVE.W (A2),D0'},
            {'pc':'0145F2','meaning':'MOVE.B D0,$0002(A3)'},
            {'pc':'0145F6','meaning':'MOVE.B D1,$0003(A3)'}
        ],
        'important_limit':'C6/CA bit semantics and individual super command acceptance remain unproven. These are original opcode and descriptor facts, not DHC gameplay commands.',
        'rom_data_exported':False,
        'gameplay_changed':False,
    }


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--opcodes',type=Path,required=True)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    args=p.parse_args()
    report=audit(args.opcodes.read_bytes(),args.rom.read_bytes())
    args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'68000_alternate_matcher_audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    with (args.out/'68000_alternate_matchers.csv').open('w',newline='',encoding='utf-8') as f:
        fields=['fighter_id','fighter_name','matcher_pc','setup_pc','descriptor_data_pc','history_a6_offset','native_input_word','first_record_word','flags_word','native_d6_word','reserved_word','raw_payload_words','record_bytes','record_kind']
        w=csv.DictWriter(f,fieldnames=fields);w.writeheader()
        for e in report['records']:
            w.writerow({k:'|'.join(e[k]) if k=='raw_payload_words' else e[k] for k in fields})
    print('PASS: alternate matcher opcode and data-ROM audit')
    print('Native matcher calls:',len(report['records']))
    print('Matchers:',report['counts_per_matcher'])
    print('Report:',args.out)

if __name__=='__main__':main()