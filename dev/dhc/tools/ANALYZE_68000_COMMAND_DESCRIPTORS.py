#!/usr/bin/env python3
"""XMVSF Stage 87.170 read-only original 68000 command descriptor audit.

Reads PRIVATE host-word-swapped CpsCode and CpsRom from user's machine.
Exports derived descriptor structures and original code PC references ONLY.
Never labels a descriptor as a super solely from a matching ROM selector.
"""
from __future__ import annotations
import argparse, collections, csv, hashlib, json
from pathlib import Path

NAMES={0:'Wolverine',2:'Cyclops',4:'Storm',6:'Rogue',8:'Gambit',10:'Sabretooth',12:'Juggernaut',14:'Magneto',18:'Ryu',20:'Ken',22:'Chun-Li',24:'Dhalsim',26:'Zangief',28:'M. Bison',30:'Akuma',32:'Charlie',34:'Cammy'}
SIGNATURE=bytes.fromhex('45fa')
MATCHERS=(0x013C30,0x013F76,0x014096,0x014392,0x01458E)
SHARED_CALL=bytes.fromhex('4eb900013c30')
INDEXER=bytes.fromhex('302e0052d040207b')
MATCHER_HEADER=bytes.fromhex('4a2e0003661a4a2e029b6710302a0004b02e00d66606224a6000026270004e75')

def be16(b,pos):return int.from_bytes(b[pos:pos+2],'big')
def be32(b,pos):return int.from_bytes(b[pos:pos+4],'big')
def le16(b,pos):return int.from_bytes(b[pos:pos+2],'little')
def swap_words(b):
    if len(b)%2:raise ValueError('Odd-size opcode file')
    out=bytearray(b)
    for i in range(0,len(out),2):out[i],out[i+1]=out[i+1],out[i]
    return bytes(out)

def fighter_regions(code):
    tables=[]
    for pc in range(0,len(code)-12,2):
        if code[pc:pc+8]!=INDEXER:continue
        if code[pc+10:pc+12] not in (b'\x4e\x90',b'\x4e\xd0'):continue
        table=pc+8+int.from_bytes(code[pc+8:pc+10],'big',signed=True)
        if not 0<=table<len(code)-76:continue
        ptr=[be32(code,table+4*i) for i in range(19)]
        if sum(0x1E000<=v<0x48000 and v%2==0 for v in ptr[:18])>=15:
            tables.append((pc,table,ptr))
    if len(tables)<10:
        raise ValueError('Original native fighter dispatcher indexers absent')
    boundaries=[]
    for fighter in NAMES:
        if fighter==16:continue
        candidates=[t[2][fighter//2] for t in tables
                    if 0x1E000<=t[2][fighter//2]<0x48000]
        if not candidates:raise ValueError(f'No fighter handler for ID {fighter:X}')
        boundaries.append((min(candidates),fighter))
    boundaries.sort()
    return tables,boundaries

def which_fighter(pc,boundaries):
    found=None
    for lower,fighter in boundaries:
        if lower>pc:break
        found=fighter
    return found if found is not None and pc<0x48000 else None

def decode_descriptor(rom,pc):
    """Decode the canonical 013C30 3/5-token records only.

    Native first word selects a matcher handler; 3- and 5-token layouts are
    supported by repeated contiguous tables, but raw token/flag meaning
    remains unproven. Other matcher variants use DIFFERENT record formats.
    """
    if not 0<=pc<len(rom)-12:
        raise ValueError('Descriptor pointer outside private program data')
    n=le16(rom,pc)
    if n not in (3,5):
        raise ValueError('Unexpected 013C30 descriptor class %04X at %06X'%(n,pc))
    if pc+8+2*(n+1)>len(rom):
        raise ValueError('Truncated descriptor at %06X'%pc)
    return {
        'record_kind':'CANONICAL_013C30_3_OR_5_TOKEN',
        'first_record_word':f'{n:04X}',
        'step_count':n,
        'flags_word':f'{le16(rom,pc+2):04X}',
        'native_d6_word':f'{le16(rom,pc+4):04X}',
        'reserved_word':f'{le16(rom,pc+6):04X}',
        'step_tokens':[f'{le16(rom,pc+8+2*i):04X}' for i in range(n)],
        'final_input_mask_word':f'{le16(rom,pc+8+2*n):04X}',
        'descriptor_bytes':8+2*(n+1)
    }

def extract(code,rom):
    if code[0x13C30:0x13C30+len(MATCHER_HEADER)]!=MATCHER_HEADER:
        raise ValueError('Shared 013C30 gate opcode bytes changed; review required')
    tables,boundaries=fighter_regions(code)
    records=[]
    alternates=[]
    for pc in range(0,len(code)-14,2):
        if code[pc:pc+2]!=SIGNATURE or code[pc+4:pc+6]!=b'\x47\xee' or code[pc+8:pc+10]!=b'\x4e\xb9':
            continue
        matcher=be32(code,pc+10)
        if matcher not in MATCHERS:
            continue
        disp=int.from_bytes(code[pc+2:pc+4],'big',signed=True)
        target=pc+2+disp  # 68000 PC-relative extension-word base
        if not 0<=target<len(rom)-12:continue
        history=be16(code,pc+6)
        fighter=which_fighter(pc,boundaries)
        if matcher!=0x013C30:
            # Do NOT treat 0028 as a count, or the 2/4 variant's payload
            # as the canonical record layout. Preserve raw words only.
            alternates.append({
                'fighter_id':f'{fighter:02X}' if fighter is not None else None,
                'fighter_name':NAMES.get(fighter,'OUTSIDE_KNOWN_FIGHTER_SECTION'),
                'setup_pc':f'{pc:06X}',
                'matcher_call_pc':f'{pc+8:06X}',
                'matcher_pc':f'{matcher:06X}',
                'descriptor_data_pc':f'{target:06X}',
                'history_a6_offset':f'{history:04X}',
                'first_record_word':f'{le16(rom,target):04X}',
                'preview_words':[f'{le16(rom,target+2*i):04X}' for i in range(8)],
                'classification':'ALTERNATE_MATCHER_FORMAT_NOT_YET_DECODED'
            })
            continue
        desc=decode_descriptor(rom,target)
        records.append({
            'fighter_id':f'{fighter:02X}' if fighter is not None else None,
            'fighter_name':NAMES.get(fighter,'OUTSIDE_KNOWN_FIGHTER_SECTION'),
            'setup_pc':f'{pc:06X}',
            'matcher_call_pc':f'{pc+8:06X}',
            'matcher_pc':f'{matcher:06X}',
            'descriptor_data_pc':f'{target:06X}',
            'history_a6_offset':f'{history:04X}',
            **desc,
            'status':'NATIVE_DESCRIPTOR_STRUCTURAL_PROOF_NOT_SUPER_NAME_PROOF'
        })
    return tables,boundaries,records,alternates

def report(code_bytes,rom_bytes):
    code=swap_words(code_bytes)
    tables,boundaries,records,alternates=extract(code,rom_bytes)
    lengths=collections.Counter(x['step_count'] for x in records if x['step_count'] is not None)
    flags=collections.Counter(x['flags_word'] for x in records)
    masks=collections.Counter(x['final_input_mask_word'] for x in records)
    matchers=collections.Counter(x['matcher_pc'] for x in records)
    kinds=collections.Counter(x['record_kind'] for x in records)
    fighter_counts=collections.Counter(x['fighter_name'] for x in records)
    unknown=[x for x in records if x['reserved_word']!='0000']
    missing=[x for x in records if x['fighter_id'] is None]
    return {
        'schema':'xmvsf-68000-native-command-descriptor-audit-v1',
        'opcode_sha256':hashlib.sha256(code_bytes).hexdigest(),
        'program_data_sha256':hashlib.sha256(rom_bytes).hexdigest(),
        'shared_native_matcher_pc':'013C30',
        'shared_matcher_gate_verified':True,
        'native_actor_indexers':len(tables),
        'native_command_descriptor_calls':len(records),
        'alternate_matcher_calls':len(alternates),
        'all_five_matcher_calls':len(records)+len(alternates),
        'alternate_matchers':alternates,
        'calls_per_matcher':dict(sorted(matchers.items())),
        'descriptor_record_kinds':dict(sorted(kinds.items())),
        'fighter_section_counts':dict(sorted(fighter_counts.items())),
        'step_count_histogram':dict(sorted(lengths.items())),
        'flags_word_histogram':dict(sorted(flags.items())),
        'input_mask_histogram':dict(sorted(masks.items())),
        'unexpected_nonzero_reserved_fields':unknown,
        'unscoped_descriptor_calls':missing,
        'matcher_gates':[
          {'at':'013C30','op':'TST.B +0003(A6)','decision':'BNE to 013C50 (normal descriptor evaluation)'},
          {'at':'013C36','op':'TST.B +029B(A6)','decision':'BEQ to 013C4C (return D0=0)'},
          {'at':'013C3C','op':'MOVE.W +0004(A2),D0','decision':'fetch descriptor native-state word'},
          {'at':'013C40','op':'CMP.B +00D6(A6),D0','decision':'compare descriptor low byte to actor D6'},
          {'at':'013C46','op':'MOVEA.L A2,A1; BRA.W to 013EAE','decision':'matching-state continuation'},
          {'at':'013C50','op':'MOVEQ #0,D0; MOVE.B (A3),D0','decision':'read actor-relative command-history state index'},
          {'at':'013C54','op':'MOVEA.L A2,A1; ADDA.W D0,A2; ADDQ.W #8,A2','decision':'select indexed descriptor step after eight-byte header'},
          {'at':'013C5A','op':'MOVE.W (A1),D1; ADD.W D1,D1','decision':'use descriptor class 0003/0005 to select matcher dispatch table entry'},
          {'at':'013C5E','op':'MOVE.W (PC,D1.W),D1; LEA (PC,D1.W),A0','decision':'read matcher branch-table/target offset; code/data bus selection requires care'},
          {'at':'013C68','op':'MOVE.W 0(A0,D0.W),D1; JMP 0(A0,D1.W)','decision':'dispatch to a step-specific matcher branch'}
        ],
        'important_limit':'Descriptor step token values and flags are preserved as raw words; semantics, runtime history transitions and whether an entry is a super are not yet independently proven. Do not auto-promote to DHC.',
        'entries':records
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--opcodes',type=Path,required=True)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    args=p.parse_args()
    data=report(args.opcodes.read_bytes(),args.rom.read_bytes())
    args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'68000_command_descriptor_audit.json').write_text(json.dumps(data,indent=2)+'\n')
    cols=('fighter_id','fighter_name','setup_pc','descriptor_data_pc','history_a6_offset','step_count','record_kind','first_record_word','matcher_pc','flags_word','native_d6_word','reserved_word','step_tokens','final_input_mask_word')
    with (args.out/'68000_command_descriptors.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,cols);w.writeheader()
        for row in data['entries']:
            w.writerow({k: '|'.join(row[k]) if k=='step_tokens' else row[k] for k in cols})
    print('Canonical 013C30 calls:',data['native_command_descriptor_calls'])
    print('Other matcher variants (unparsed):',data['alternate_matcher_calls'])
    print('Native fighter indexers:',data['native_actor_indexers'])
    print('Canonical 3/5-token histogram:',data['step_count_histogram'])
    print('Unexpected reserved words:',len(data['unexpected_nonzero_reserved_fields']))
    print('Unscoped calls:',len(data['unscoped_descriptor_calls']))
    print('Report:',args.out)
if __name__=='__main__': main()
