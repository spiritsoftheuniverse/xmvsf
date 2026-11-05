#!/usr/bin/env python3
"""Original XMVSF 68000 DHC candidate control-flow reconstruction (read only).

Requires user-owned host-word-swapped CPS2 decrypted opcode/program dumps,
plus the previously derived Stage 87.170 canonical descriptor JSON and
Stage 87.171 alternate matcher CSV. Verifies ORIGINAL 68000 instruction
signatures, both native indexed dispatch levels and action 2E launch selector.
This is static code provenance; not a claim of live tag-in execution.
Never exports ROM/code bytes, installs patches or changes gameplay.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path

M68K_ACTION_DISPATCH = 0x01038E
D6_DISPATCH_SIG = bytes.fromhex('7000102E00D6303B00064EFB0002')
PHASE_DISPATCH_0 = bytes.fromhex('302E0008303B00064EFB0002')
PHASE_DISPATCH_1 = bytes.fromhex('302E0008323B00064EFB1002')

# Command source addresses from ROM-derived 87.170 and 87.171 evidence.
# These are NOT hardcoded DHC gameplay moves and are not written into the engine.
CASES = (
    dict(fighter='Ryu', fighter_id=0x12, matcher=0x013C30,
         matcher_call=0x0329EE, d6=0x0A, state_dispatch=0x0337BA,
         launch=0x033E0C, selectors=(0x08,0x0C),
         base_script=0x0D3070, label='Shinkuu Hadoken-related native path',
         notes='D1=08 except D1=0C if actor $A4=04; do not give both one move name'),
    dict(fighter='Ryu', fighter_id=0x12, matcher=0x013C30,
         matcher_call=0x032A4E, d6=0x0C, state_dispatch=0x0337BA,
         launch=0x033F80, selectors=(0x00,),
         base_script=0x0D2CFC, label='Shinkuu Tatsumaki-related native path',notes=''),
    dict(fighter='Ken', fighter_id=0x14, matcher=0x013C30,
         matcher_call=0x0346D0, d6=0x0C, state_dispatch=0x035356,
         launch=0x035A4E, selectors=(0x00,),
         base_script=0x0D6722, label='Shoryureppa-related native path',notes=''),
    dict(fighter='Ken', fighter_id=0x14, matcher=0x013C30,
         matcher_call=0x03472E, d6=0x0E, state_dispatch=0x035356,
         launch=0x035CB0, selectors=(0x02,),
         base_script=0x0D6976, label='Shinryuken-related native path',notes=''),
    dict(fighter='Akuma', fighter_id=0x1E, matcher=0x014392,
         matcher_call=0x04158A, d6=0x10, state_dispatch=0x04230A,
         launch=0x042C54, selectors=(0x0C,),
         base_script=0x0EB986, label='Shun Goku Satsu-related native path',
         notes='Alternate matcher 014392, NOT canonical Akuma matcher 0412CA / D6=0C'),
)


def unswap(data: bytes) -> bytes:
    if len(data) % 2:
        raise ValueError('Odd-sized original 68000 opcode image')
    result = bytearray(data)
    for i in range(0, len(data), 2):
        result[i],result[i+1] = result[i+1],result[i]
    return bytes(result)


def u16(b: bytes, at: int) -> int:
    if at<0 or at+2>len(b):
        raise ValueError(f'Word outside original code at {at:06X}')
    return (b[at]<<8) | b[at+1]


def le16(b: bytes, at: int) -> int:
    if at<0 or at+2>len(b):
        raise ValueError(f'Word outside original ROM data at {at:06X}')
    return b[at] | (b[at+1]<<8)


def signed16(v: int) -> int:
    return v-65536 if v&0x8000 else v


def require(code: bytes, at: int, pattern: bytes, meaning: str) -> None:
    if code[at:at+len(pattern)]!=pattern:
        raise ValueError(f'Original 68000 signature disagreement at {at:06X}: {meaning}')


def dispatch_table_index(code: bytes, dispatcher: int, d6: int) -> dict:
    if not 0<=d6<=0x7e or d6%2:
        raise ValueError('D6 table index must be even 68000 byte offset')
    require(code,dispatcher,D6_DISPATCH_SIG,'D6->native handler index')
    table=dispatcher+len(D6_DISPATCH_SIG)
    relative=signed16(u16(code,table+d6))
    target=table+relative
    # 68000 indexed PC base here is after the jump extension word,
    # i.e. the table first word. Using the extension address yields an
    # incorrect target two bytes early (the previous audit's ambiguity).
    if target%2 or target<0 or target+12>len(code):
        raise ValueError('Bad indexed character-state target')
    return {'d6_dispatch_pc':f'{dispatcher:06X}',
            'd6_table_pc':f'{table:06X}', 'd6_offset_bytes':d6,
            'd6_table_entry_pc':f'{table+d6:06X}',
            'd6_relative_offset':f'{relative&0xffff:04X}',
            'd6_target_pc':f'{target:06X}'}, target


def phase_table_index(code: bytes, pc: int, phase: int=0) -> dict:
    if code[pc:pc+len(PHASE_DISPATCH_0)] == PHASE_DISPATCH_0:
        kind='MOVE_W_TO_D0_AND_JMP_PC_D0'
    elif code[pc:pc+len(PHASE_DISPATCH_1)] == PHASE_DISPATCH_1:
        kind='MOVE_W_TO_D1_AND_JMP_PC_D1'
    else:
        raise ValueError(f'Unrecognized native +08 phase dispatch at {pc:06X}')
    table=pc+len(PHASE_DISPATCH_0)
    relative=signed16(u16(code,table+phase))
    target=table+relative
    if target%2 or not 0<=target<len(code):
        raise ValueError('Bad indexed phase target')
    return {'phase_dispatch_pc':f'{pc:06X}', 'phase_index_bytes':phase,
            'phase_table_pc':f'{table:06X}',
            'phase_table_entry_pc':f'{table+phase:06X}',
            'phase_relative_offset':f'{relative&0xffff:04X}',
            'phase_target_pc':f'{target:06X}',
            'phase_index_kind':kind}, target


def resolve_original_rom_script(rom: bytes, fighter: int, selector: int) -> int:
    # CPS2 program-data dump is host word-swapped, readable as LE words.
    # This is only script-pointer validation, not a callable native routine.
    t=0x8A946+fighter*2
    table=(le16(rom,t)<<16)|le16(rom,t+2)
    section=table+le16(rom,table+0x2E)
    return section+le16(rom,section+selector)


def original_native_matcher_call(code: bytes, matcher_call: int, matcher: int) -> dict:
    require(code,matcher_call,bytes.fromhex('4EB9')+matcher.to_bytes(4,'big'),
            'original character-specific matcher invocation')
    opcode=u16(code,matcher_call+6)
    if opcode&0xFF00 != 0x6700:
        raise ValueError(f'Expected BEQ immediately after match at {matcher_call+6:06X}')
    if opcode&0xff:
        displacement=(opcode&0xff)-(0x100 if opcode&0x80 else 0)
        reject=matcher_call+8+displacement
        size=2
    else:
        displacement=signed16(u16(code,matcher_call+8))
        reject=matcher_call+8+displacement
        size=4
    return {'matcher_pc':f'{matcher:06X}',
            'matcher_call_pc':f'{matcher_call:06X}',
            'failed_BEQ_pc':f'{matcher_call+6:06X}',
            'failure_target_pc':f'{reject:06X}',
            'failure_branch_size':size}


def accepted_state_setters(code: bytes, matcher_call: int, d6: int) -> dict:
    # This bounded scan is verified by exact opcode matches; control-flow
    # implications still depend on native branch guards and subroutine returns.
    start=matcher_call+8
    look=code[start:start+120]
    d6_pattern=bytes.fromhex('1D7C00')+bytes([d6])+bytes.fromhex('00D6')
    p=look.find(d6_pattern)
    if p<0:raise ValueError(f'Missing exact D6 assignment near {matcher_call:06X}')
    state_at=start+p
    mode_pattern=bytes.fromhex('1D7C002A00A0')
    q=code.find(mode_pattern,state_at+len(d6_pattern),state_at+48)
    if q<0:raise ValueError(f'Missing exact A0=2A assignment near {matcher_call:06X}')
    # Check mode and state writes refer to same actor context, but do not
    # claim unconditional reachability from the matcher.
    return {'D6_write_pc':f'{state_at:06X}',
            'A0_mode_2A_write_pc':f'{q:06X}',
            'state_writes_static_not_unconditional':True}


def verify_launch(code: bytes, entry: int, launch: int, selectors: tuple[int,...]) -> dict:
    if not entry <= launch < entry+240:
        raise ValueError('2E launch is not in bounded phase-zero body')
    if len(selectors)==1:
        selector=selectors[0]
        expected=bytes([0x70,0x2E,0x72,selector])+bytes.fromhex('4EF90001038E')
        require(code,launch,expected,'native action=2E/selector and original dispatcher JMP')
        next_pc=launch+4
        conditional=None
    elif selectors==(0x08,0x0C):
        expected=bytes.fromhex('702E72080C2E000400A46602720C4EF90001038E')
        require(code,launch,expected,'Ryu 2E08 or 2E0C conditional original launch')
        next_pc=launch+14
        conditional='CMPI.B #04,$A4(A6); when equal, set D1=0C; otherwise D1=08'
    else:
        raise ValueError('Unrecognized explicitly verified selector set')
    return {'launch_setup_pc':f'{launch:06X}',
            'launch_action':'2E',
            'launch_selectors':[f'{x:02X}' for x in selectors],
            'native_dispatch_jump_pc':f'{next_pc:06X}',
            'launch_within_phase_zero_body':True,
            'conditional_selector':conditional,
            'intraphase_conditions_not_fully_proven':True}


def analyze(opcodes: bytes, rom: bytes, canonical:dict, alternatives:list[dict]) ->dict:
    code=unswap(opcodes)
    if len(code)!=1048576 or len(rom)!=3670016:
        raise ValueError('Unexpected private CPS2 export sizes')
    # Cross-evidence 87.170 and 87.171 is verified by PC, fighter, state,
    # and signature. Avoid inventing a named move for every command.
    rows=[]
    for c in CASES:
        matcher=original_native_matcher_call(code,c['matcher_call'],c['matcher'])
        setters=accepted_state_setters(code,c['matcher_call'],c['d6'])
        state,phase_pc=dispatch_table_index(code,c['state_dispatch'],c['d6'])
        phase,entry=phase_table_index(code,phase_pc,0)
        launch=verify_launch(code,entry,c['launch'],c['selectors'])
        if c['matcher']==0x013C30:
            relevant=[e for e in canonical['entries']
                      if int(e['matcher_call_pc'],16)==c['matcher_call']]
        else:
            relevant=[e for e in alternatives
                      if int(e['matcher_pc'],16)==c['matcher'] and
                         int(e['setup_pc'],16)+8==c['matcher_call']]
        if len(relevant)!=1:raise ValueError(f'No unique native matcher descriptor at {c["matcher_call"]:06X}')
        rec=relevant[0]
        if (int(rec['fighter_id'],16),int(rec['native_d6_word'],16))!=(c['fighter_id'],c['d6']):
            raise ValueError(f'Original input descriptor disagrees at {c["matcher_call"]:06X}')
        original_scripts={f'{x:02X}':f'{resolve_original_rom_script(rom,c["fighter_id"],x):06X}' for x in c['selectors']}
        first_selector=f'{c["selectors"][0]:02X}'
        if int(original_scripts[first_selector],16)!=c['base_script']:
            raise ValueError(f'Original ROM table pointer mismatch for {c["fighter"]}: {first_selector}')
        rows.append({
            'fighter':c['fighter'],'fighter_id':f'{c["fighter_id"]:02X}',
            'previously_observed_move_label':c['label'],
            'native_command_state_D6':f'{c["d6"]:02X}',
            'descriptor_data_pc':rec['descriptor_data_pc'],
            'history_a6_offset':rec['history_a6_offset'],
            'raw_mask_word':rec.get('final_input_mask_word') or rec.get('raw_payload_words'),
            **matcher,**setters,**state,**phase,**launch,
            'all_ROM_script_pointers':original_scripts,
            'caveat':c['notes'],
            'proof_class':'STATIC_NATIVE_MATCHER_TO_STATE_TO_PHASE0_TO_ACTION_2E_SIGNATURES',
            'execution_not_recorded_for_DHC':True,
        })
    return {'schema':'xmvsf-87-174-native-d6-to-2e-static-path-v1',
            'opcode_sha256':hashlib.sha256(opcodes).hexdigest(),
            'rom_sha256':hashlib.sha256(rom).hexdigest(),
            'verified_examples':len(rows),
            'separate_matcher_families':sorted({r['matcher_pc'] for r in rows}),
            'main_result':'D6 command state selects an indexed character handler; +08 selects phase 0; phase 0 contains the native 2E action/selector dispatch. Original ROM script pointers verified.',
            'limitations':[
                'Syntactically verified branches/tables do not prove a live complete CPU execution trace, subroutine return, or all eligibility/cancel gates.',
                'No proof incoming DHC fighter can process a command at tag-in state.',
                'Ryu D6 00 belongs to a different native handler; Akuma D6 0C does NOT establish Shun Goku Satsu.',
                'Akuma alternate descriptor is not a named-move proof based on descriptor values alone; replayed selector 2E0C provides separate corroboration.',
                'No direct action=2E writing, command forcing, engine build, or Stable changes have been made.'
            ],
            'paths':rows,
            'original_rom_exported':False,
            'gameplay_changed':False}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--opcodes',required=True,type=Path)
    p.add_argument('--rom',required=True,type=Path)
    p.add_argument('--canonical-json',required=True,type=Path)
    p.add_argument('--alternate-csv',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path)
    args=p.parse_args()
    canonical=json.loads(args.canonical_json.read_text(encoding='utf8'))
    with args.alternate_csv.open(newline='',encoding='utf-8-sig') as f:
        alts=list(csv.DictReader(f))
    results=analyze(args.opcodes.read_bytes(),args.rom.read_bytes(),canonical,alts)
    args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'68000_d6_to_2e_native_paths.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf8')
    with (args.out/'68000_d6_to_2e_native_paths.csv').open('w',newline='',encoding='utf8') as f:
        columns=['fighter','native_command_state_D6','matcher_pc','matcher_call_pc','D6_write_pc','A0_mode_2A_write_pc','d6_dispatch_pc','d6_target_pc','phase_dispatch_pc','phase_target_pc','launch_setup_pc','launch_selectors','all_ROM_script_pointers','proof_class']
        w=csv.DictWriter(f,fieldnames=columns);w.writeheader()
        for x in results['paths']:
            w.writerow({k:','.join(x[k]) if isinstance(x.get(k),list) else json.dumps(x[k],sort_keys=True) if isinstance(x.get(k),dict) else x.get(k) for k in columns})
    print('Stage 87.174 verified native D6->action 2E paths:',results['verified_examples'])
    for row in results['paths']:
        print(row['fighter'], 'D6='+row['native_command_state_D6'],
              'phase0='+row['phase_target_pc'],'action 2E'+','.join(row['launch_selectors']),
              'selector PC='+row['launch_setup_pc'])
    print('Static provenance only; original DHC engine unchanged.')

if __name__=='__main__':main()
