#!/usr/bin/env python3
"""Read-only, original-ROM 68000 command acceptance / DHC-bridge audit.

Reads user-owned PRIVATE host-word-swapped opcode/data dumps and the derived
Stage 87.170 descriptor JSON. Exports no original opcode bytes. This is
static CPU instruction proof, not an emulator execution trace; in particular,
we DO NOT substitute forced 2E actions for the game's original acceptance.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path


MATCHER = 0x013C30
NATIVE_DISPATCH = 0x01038E


def unswap(raw: bytes) -> bytes:
    if len(raw) & 1:
        raise ValueError('68000 code has odd size')
    out = bytearray(raw)
    for i in range(0, len(raw), 2):
        out[i], out[i+1] = out[i+1], out[i]
    return bytes(out)


def s8(v: int) -> int:
    return v-0x100 if v & 0x80 else v


def s16(v: int) -> int:
    return v-0x10000 if v & 0x8000 else v


def branch_target(code:bytes, at:int) -> tuple[str,int,int]:
    """Exact 68000 branch PC-relative displacement (base is PC after opcode)."""
    op = int.from_bytes(code[at:at+2], 'big')
    if (op & 0xF000) != 0x6000:
        raise ValueError('Not a 68000 Bcc/BSR/BRA opcode at %06X'%at)
    cond = (op>>8)&15
    if op&0xFF:
        return ('%X'%cond,at+2+s8(op&0xFF),2)
    if at+4>len(code):raise ValueError('Truncated Bcc extension')
    return ('%X'%cond,at+2+s16(int.from_bytes(code[at+2:at+4], 'big')),4)


def require(code:bytes, at:int, hx:str, label:str) -> None:
    expected=bytes.fromhex(hx)
    if code[at:at+len(expected)] != expected:
        raise ValueError(f'Native opcode mismatch {label} at {at:06X}')


def find_state_setup(code:bytes, after:int, selector:int, horizon:int=128) -> dict:
    """Bounded *syntactic* forward look; only selected paths prove reachability."""
    w=code[after:after+horizon]
    static=bytes([0x1D,0x7C,0,selector,0,0xD6])
    p=w.find(static)
    mode=w.find(bytes.fromhex('1D7C002A00A0'))
    return {
        'direct_d6_immediate_pc': f'{after+p:06X}' if p>=0 else None,
        'native_mode_2A_immediate_pc': f'{after+mode:06X}' if mode>=0 else None,
        'scan_is_not_control_flow_proof': True,
    }


def matching_records(code:bytes, records:list[dict]) -> list[dict]:
    results=[]
    for x in records:
        at=int(x['matcher_call_pc'],16)
        require(code,at,'4EB900013C30', 'native canonical matcher JSR')
        bcc, target, size=branch_target(code,at+6)
        if bcc!='7':
            raise ValueError(f'Canonical checker lacks expected BEQ at {at+6:06X}')
        state=int(x['native_d6_word'],16)&0xff
        results.append({
            'fighter_id':x['fighter_id'], 'fighter_name':x['fighter_name'],
            'matcher_call_pc':x['matcher_call_pc'],
            'descriptor_data_pc':x['descriptor_data_pc'],
            'descriptor_expected_d6':x['native_d6_word'],
            'history_offset_a6':x['history_a6_offset'],
            'motion_tokens':x['step_tokens'],
            'final_input_mask':x['final_input_mask_word'],
            'matcher_failed_beq_pc':f'{at+6:06X}',
            'failed_branch_target_pc':f'{target:06X}',
            'beq_instruction_size':size,
            **find_state_setup(code,at+6+size,state),
            'status':'MATCHER_RETURN_BRANCH_STATIC_VERIFIED_POSTMATCH_WRITES_ARE_CANDIDATES',
        })
    return results


# For each selected success pathway, verify exact original instructions rather
# than trusting a nearby substring that may belong to an unrelated path.
SELECTED = [
 {'fighter':'Ryu','id':'12','call':0x03298A,'reject':0x0329DC,
  'd6':0x00,'d6_pc':0x0329A4,'mode_pc':0x0329B6,
  'guard_pc':0x032992,'success_pc':0x0329D8,
  'level':'DIRECT_LOCAL_SUCCESS_AND_REJECT_TRACE'},
 {'fighter':'Ken','id':'14','call':0x0346D0,'reject':0x03460A,
  'd6':0x0C,'d6_pc':0x0346E6,'mode_pc':0x0346F8,
  'guard_pc':0x0346DA,'success_pc':0x03471A,
  'level':'DIRECT_LOCAL_SUCCESS_AND_REJECT_TRACE'},
 {'fighter':'Ken','id':'14','call':0x03472E,'reject':0x03460A,
  'd6':0x0E,'d6_pc':0x034744,'mode_pc':0x034756,
  'guard_pc':0x034738,'success_pc':0x034778,
  'level':'DIRECT_LOCAL_SUCCESS_AND_REJECT_TRACE'},
 {'fighter':'Akuma','id':'1E','call':0x0412CA,'reject':0x041428,
  'd6':0x0C,'d6_pc':0x041316,'mode_pc':0x041328,
  'guard_pc':0x0412D4,'success_pc':0x041346,
  'level':'DIRECT_LOCAL_SUCCESS_WITH_EXTRA_CHORD_GATE'},
]


def verify_selected(code:bytes, records:list[dict]) -> list[dict]:
    results=[]
    for case in SELECTED:
        at=case['call']
        require(code,at,'4EB900013C30','selected canonical matcher call')
        condition, target,_=branch_target(code,at+6)
        if condition!='7' or target!=case['reject']:
            raise ValueError(f'Unexpected rejection destination at {at:06X}: {target:06X}')
        # Check continuation branches / success mutators from native code.
        require(code,case['d6_pc'],f'1D7C00{case["d6"]:02X}00D6', 'native state D6 update')
        require(code,case['mode_pc'],'1D7C002A00A0','native mode 2A transition')
        require(code,case['success_pc'],'70014E75','success return')
        if case['fighter']=='Ryu':
            require(code,case['guard_pc'],'4A2E03406644', 'additional Ryu state guard')
            require(code,case['reject'],'70004E75','Ryu rejection return')
        if case['fighter']=='Ken':
            require(code,case['guard_pc'],'4A2E000367064EB900013F10','Ken command guard and 013F10 call')
            require(code,case['reject'],'70004E75','Ken rejection return')
        if case['fighter']=='Akuma':
            require(code,case['guard_pc'],'426E000A4A2E0003','Akuma additional state guard')
            require(code,0x0412EC,'303C0070720C4EB900014488','Akuma first chord mask 0070')
            require(code,0x041300,'303C0700720C4EB900014488','Akuma fallback chord mask 0700')
            require(code,case['reject'],'70004E75','Akuma rejection return')
        observed=[r for r in records if r['fighter_id']==case['id'] and int(r['matcher_call_pc'],16)==at]
        if len(observed)!=1 or int(observed[0]['native_d6_word'],16)!=case['d6']:
            raise ValueError('Selected opcode state disagrees with ROM descriptor at %06X'%at)
        results.append({
            'fighter':case['fighter'],'fighter_id':case['id'],
            'matcher_call_pc':f'{at:06X}',
            'descriptor_data_pc':observed[0]['descriptor_data_pc'],
            'history_offset_a6':observed[0]['history_a6_offset'],
            'direction_tokens':observed[0]['step_tokens'],
            'final_input_mask':observed[0]['final_input_mask_word'],
            'branch_if_matcher_zero':'BEQ',
            'failed_branch_pc':f'{at+6:06X}',
            'failed_branch_target':f'{target:06X}',
            'state_d6':f'{case["d6"]:02X}',
            'state_write_pc':f'{case["d6_pc"]:06X}',
            'native_mode_value':'2A',
            'native_mode_write_pc':f'{case["mode_pc"]:06X}',
            'accepted_return_pc':f'{case["success_pc"]:06X}',
            'native_direct_action_2e_forced':False,
            'extra_chord_checker_pc':'014488' if case['fighter']=='Akuma' else None,
            'proof':case['level'],
            'limitation':'Static conditional control flow, not a recorded CPU execution path; no per-move super-name promotion.',
        })
    return results


CHORD_CHECKER_PC=0x014488
CHORD_CHECKER_OPCODES = (
 '322E00D0342E00C2B541342E00C2362E00C0B7428441'
 'C440B440670470004E7570014E75'
)


def chord_accept(saved_d0:int, previous_c2:int, current_c0:int, mask:int) -> bool:
    """Exact 014488 68000 XOR, OR, AND and CMP result for mask D0.

    This check uses actor $D0, $C2 and $C0; it does NOT directly consume
    $C6 or $CA. All values are 16-bit words.
    """
    d1=(saved_d0 ^ previous_c2)&0xffff
    d2=(previous_c2 ^ current_c0)&0xffff
    d2=(d2 | d1)&mask&0xffff
    return d2==(mask&0xffff)


def analyze(opcodes:bytes,rom:bytes,canonical:dict) -> dict:
    code=unswap(opcodes)
    if len(code)!=1048576:
        raise ValueError('Unexpected decrypted opcode dump length')
    if len(rom)!=3670016:
        raise ValueError('Unexpected private program ROM dump length')
    # Assert exact gate entry and all of the XOR/OR/AND/CMP chord checker.
    require(code,0x013C30,'4A2E0003661A4A2E029B6710302A0004B02E00D66606224A6000026270004E75',
            'canonical matcher actor gate')
    require(code,CHORD_CHECKER_PC,CHORD_CHECKER_OPCODES,'014488 raw-history button-group gate')
    rows=matching_records(code,canonical['entries'])
    selected=verify_selected(code,canonical['entries'])
    histogram=Counter(row['fighter_name'] for row in rows)
    direct=sum(row['direct_d6_immediate_pc'] is not None for row in rows)
    mode=sum(row['native_mode_2A_immediate_pc'] is not None for row in rows)
    # Fixed models to establish no false edge on holding or releasing inputs.
    chord_examples={
      'zero_mask_all_zero':chord_accept(0,0,0,0),
      'new_full_chord_0070':chord_accept(0,0,0x0070,0x0070),
      'missing_one_button_0070':chord_accept(0,0,0x0030,0x0070),
      'held_without_any_new_edge_0070':chord_accept(0x0070,0x0070,0x0070,0x0070),
      'history_part_and_new_part_0070':chord_accept(0x0030,0,0x0040,0x0070),
      'new_full_chord_0700':chord_accept(0,0,0x0700,0x0700),
    }
    assert chord_examples=={
      'zero_mask_all_zero':True,'new_full_chord_0070':True,
      'missing_one_button_0070':False,'held_without_any_new_edge_0070':False,
      'history_part_and_new_part_0070':True,'new_full_chord_0700':True,
    }, chord_examples
    return {
       'schema':'xmvsf-stage-87-173-native-command-acceptance-path-v1',
       'opcode_sha256':hashlib.sha256(opcodes).hexdigest(),
       'program_data_sha256':hashlib.sha256(rom).hexdigest(),
       'native_shared_matcher':'013C30',
       'canonical_matcher_calls_verified':len(rows),
       'matcher_call_then_beq_verified':sum(1 for x in rows if x['matcher_failed_beq_pc']),
       'bounded_candidate_d6_immediates':direct,
       'bounded_candidate_mode_2a_immediates':mode,
       'selected_control_flow_cases_verified':len(selected),
       'by_fighter':dict(sorted(histogram.items())),
       'selected_acceptance_paths':selected,
       'raw_history_chord_gate':{
          'pc':'014488','inputs':['00D0(A6) saved input word','00C2(A6) raw previous input','00C0(A6) raw current input'],
          'logic':'(((saved_D0 XOR prev_C2) OR (prev_C2 XOR current_C0)) AND requested_mask) == requested_mask',
          'returns_true_nonzero':True,
          'called_by_akuma_0412f2_and_041306':True,
          'input_groups_tested':['0070','0700'],
          'computed_examples':chord_examples,
          'note':'Exact 68000 data path; saved D0 lifetime and later engine transition remain unverified.'
       },
       'native_bridge_principle':(
          'Detect physical input; preserve facing-normalized C6/CA and raw C0/C2 input history; '
          'let selected fighter native 013C30/alternate matcher evaluate and update its own D6/A0=2A; '
          'observe native subsequent action-2E decision instead of writing a 2E selector.'
       ),
       'limits':[
         'No live CPU instruction trace showing the whole matcher-to-action-2E transition.',
         'D6 value is command-state, not automatically action-2E subselector.',
         'Correct incoming fighter ownership, attack eligibility, meter and timeline for DHC are not yet proven.',
         'Selected paths are native original code, not a runnable DHC input bridge or gameplay test.',
         'Akuma 0412CA descriptor D6=0C is NOT proven Shun Goku Satsu, even though an action-2E0C root exists elsewhere.',
         'Original ROM, original replays, Stable and 87.167 engine are not modified.',
       ],
       'raw_rom_exported':False,
       'native_gameplay_changed':False,
       'all_matcher_call_sites':rows,
    }


def main() -> None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--opcodes',required=True,type=Path)
    ap.add_argument('--rom',required=True,type=Path)
    ap.add_argument('--canonical-json',required=True,type=Path)
    ap.add_argument('--out',required=True,type=Path)
    a=ap.parse_args()
    evidence=analyze(a.opcodes.read_bytes(),a.rom.read_bytes(),json.loads(a.canonical_json.read_text()))
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8')
    print('PASS:',evidence['canonical_matcher_calls_verified'],'native canonical matches and BEQ return branches')
    print('PASS:',evidence['selected_control_flow_cases_verified'],'selected Ryu/Ken/Akuma state acceptance paths')
    print('PASS: native XOR/OR input chord mask at 014488; no forced 2E entries')
    print('Evidence:',a.out)

if __name__=='__main__':
    main()