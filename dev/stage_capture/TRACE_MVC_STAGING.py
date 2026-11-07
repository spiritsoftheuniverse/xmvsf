#!/usr/bin/env python3
"""Read-only MVC1 68000 CPS2 sprite staging investigation.

Input: locally captured host-word-swapped decrypted FETCH instructions and
original READ program bytes. Look for exact opcode motifs that load sprite
staging addresses, write four-word OBJ descriptors and index candidate
graphic tables. Static motifs are NOT evidence of instruction execution,
animation identity, or true instruction boundaries. No ROM bytes exported.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

BIN_CODE='mvc_opcodes_decrypted_host_words.bin'
BIN_DATA='mvc_program_data_host_words.bin'
OUTPUTS=('staging_buffer_references.tsv','staging_descriptor_emitters.tsv',
         'staging_indexed_tile_lookups.tsv','staging_dispatcher_calls.tsv',
         'staging_analysis.json','staging_readme.txt')
DISPATCH=0x004B0E
RUN=re.compile(r'run_\d+\Z')
class AnalysisError(Exception):
    pass

def latest_run(root:Path)->Path|None:
    if (root/BIN_CODE).is_file():return root
    if not root.is_dir():return None
    dirs=sorted((p for p in root.iterdir()
                 if p.is_dir() and RUN.fullmatch(p.name)),
                key=lambda p:int(p.name[4:]))
    return next((p for p in reversed(dirs)
                 if (p/BIN_CODE).is_file()),None)

def word(blob:bytes,off:int)->int:
    if off<0 or off+2>len(blob):
        raise AnalysisError('word outside program')
    return struct.unpack_from('<H',blob,off)[0]

def signed8(n:int)->int:
    return n-256 if n&128 else n

def write_tsv(path:Path,fields:tuple[str,...],rows):
    with path.open('w',encoding='utf-8',newline='') as f:
        wr=csv.writer(f,delimiter='\t')
        wr.writerow(fields)
        wr.writerows(rows)

def parse_program(code:bytes,data:bytes)->dict:
    if len(code)%2 or len(data)%2 or not code:
        raise AnalysisError('invalid CPS2 host-word-swapped source sizes')
    ptrs=[];caller=[];emitters=[];indexed=[];references=[]
    for pc in range(0,len(code)-6,2):
        opcode=word(code,pc)
        kind=None;reg=None
        # MOVEA.L #imm,Ax / LEA abs.l,Ax. These exact opcode motifs
        # may occur within embedded data; use them only as static leads.
        if opcode in (0x207C+(r<<9) for r in range(8)):
            kind='MOVEA_L_IMMEDIATE';reg=(opcode-0x207C)//0x200
        elif opcode in (0x41F9+(r<<9) for r in range(8)):
            kind='LEA_ABSOLUTE_LONG';reg=(opcode-0x41F9)//0x200
        if kind is not None:
            addr=(word(code,pc+2)<<16)|word(code,pc+4)
            if 0x924000<=addr<0x927000:
                ptrs.append((pc,addr,reg,kind))
        if opcode in (0x4EB9,0x4EF9):
            dest=(word(code,pc+2)<<16)|word(code,pc+4)
            if dest in (DISPATCH,0x004B0A):
                caller.append((pc,dest,'JSR_ABS_LONG' if opcode==0x4EB9 else 'JMP_ABS_LONG'))
        # MOVEM.W D0-D3,(A0). Four words match native x/y/tile/attr.
        if opcode==0x4890 and word(code,pc+2)==0x000F:
            emitter={'pc':pc,'base':None,'base_pc':None,'distance':None,
                     'lookup_pc':None,'table_base':None,
                     'confidence':'UNRESOLVED_A0'}
            prior=[p for p in ptrs if p[2]==0 and p[0]<pc and pc-p[0]<=64]
            if prior:
                p=prior[-1]
                emitter.update(base=p[1],base_pc=p[0],distance=pc-p[0],
                               confidence='STATIC_NEARBY_A0_LITERAL_NOT_EXECUTION_PROOF')
            possible=[]
            for q in range(max(0,pc-40),pc-2,2):
                if word(code,q)==0x343B:
                    ext=word(code,q+2)
                    idx=('A' if ext&0x8000 else 'D')+str((ext>>12)&7)
                    disp=signed8(ext&255)
                    base=q+2+disp
                    possible.append((q,idx,disp,base))
            if possible:
                q,idx,disp,base=possible[-1]
                emitter.update(lookup_pc=q,table_base=base)
                eq=None
                if 0<=base and base+16<=min(len(code),len(data)):
                    eq=all(word(code,base+2*i)==word(data,base+2*i) for i in range(8))
                indexed.append((pc,q,idx,disp,base,
                                'MATCH' if eq else 'DIFFERENT' if eq is False else 'OUT_OF_RANGE',
                                'UNVERIFIED_PC_RELATIVE_TABLE'))
            emitters.append(emitter)
    # Source-specific branch signature: MOVEA.L #$924000,A4; test type;
    # MOVEA.L #$925000,A4; compare high nibble $80; otherwise $926000.
    signature=[0x287C,0x0092,0x4000,0x1E10,0x6A16,0x287C,
               0x0092,0x5000,0x0207,0x00F0,0x0C07,0x0080,
               0x6706,0x287C,0x0092,0x6000]
    at=0x004B32
    verified=(at+len(signature)*2<=len(code) and
              all(word(code,at+i*2)==x for i,x in enumerate(signature)))
    for pc,addr,reg,kind in ptrs:
        references.append((f'{pc:06X}',f'{addr:06X}',f'A{reg}',kind,
                           'EXACT_OPCODE_MOTIF_CANDIDATE'))
    erows=[]
    for e in emitters:
        erows.append((f"{e['pc']:06X}",
                      f"{e['base']:06X}" if e['base'] is not None else '',
                      f"{e['base_pc']:06X}" if e['base_pc'] is not None else '',
                      e['distance'] if e['distance'] is not None else '',
                      f"{e['lookup_pc']:06X}" if e['lookup_pc'] is not None else '',
                      f"{e['table_base']:06X}" if e['table_base'] is not None else '',
                      e['confidence']))
    irows=[(f'{pc:06X}',f'{q:06X}',idx,disp,f'{base:06X}',equal,status)
           for pc,q,idx,disp,base,equal,status in indexed]
    calls=[(f'{pc:06X}',f'{dest:06X}',kind,'CODE_MOTIF_NOT_RUNTIME_CALL')
           for pc,dest,kind in caller]
    return {'references':references,'emitters':erows,'indexed':irows,
            'calls':calls,'dispatcher_verified':verified,
            'stats':{'staging_pointer_motifs':len(references),
                     'descriptor_movem_motifs':len(emitters),
                     'nearby_staging_pointer_emitters':sum(e['base'] is not None for e in emitters),
                     'indexed_tile_lookup_emitters':len(indexed),
                     'dispatcher_direct_call_motifs':len(caller)},
            'examples':{'native_buffer_select_dispatcher_hex':'004B0E',
                        'descriptor_builder_pc_hex':'006B94',
                        'descriptor_builder_buffer_hex':'925800',
                        'descriptor_builder_indexed_lookup_hex':'006B8C',
                        'native_descriptor_copy_loop_hex':'02483C-02484C'}}

def create_report(root:Path,output:Path)->dict:
    root=root.resolve();output=output.resolve()
    if root==output or root in output.parents or output in root.parents:
        raise AnalysisError('report must not modify private trace or its parent folder')
    run=latest_run(root)
    if run is None:
        raise AnalysisError('missing existing MVC decrypted 68000 program snapshot')
    code_path=run/BIN_CODE;data_path=run/BIN_DATA
    if not data_path.is_file():
        raise AnalysisError('missing original program READ image')
    if code_path.stat().st_size>16*1024*1024 or data_path.stat().st_size>16*1024*1024:
        raise AnalysisError('unexpected oversized program image')
    code=code_path.read_bytes();data=data_path.read_bytes()
    provenance=run/'program_source.tsv'
    driver=None;status=None
    if provenance.is_file():
        with provenance.open('r',encoding='utf-8',newline='') as f:
            for row in csv.DictReader(f,delimiter='\t'):
                driver=row.get('driver')
                status=row.get('status')
                if driver and driver.startswith('mvsc'):
                    break
    if driver and not driver.startswith('mvsc'):
        raise AnalysisError('original program snapshot is not an MVC CPS2 driver')
    parsed=parse_program(code,data)
    output.mkdir(parents=True,exist_ok=True)
    for name,fields,key in (
        ('staging_buffer_references.tsv',('opcode_pc_hex','staging_address_hex','address_register','instruction','confidence'),'references'),
        ('staging_descriptor_emitters.tsv',('movem_pc_hex','nearby_a0_base_hex','a0_load_pc_hex','byte_distance','nearest_index_lookup_pc_hex','possible_table_base_hex','confidence'),'emitters'),
        ('staging_indexed_tile_lookups.tsv',('descriptor_write_pc_hex','lookup_pc_hex','index_register','pc_relative_displacement','candidate_table_base_hex','fetch_vs_read_eight_words','confidence'),'indexed'),
        ('staging_dispatcher_calls.tsv',('callsite_pc_hex','dispatcher_pc_hex','instruction','confidence'),'calls')):
        write_tsv(output/name,fields,parsed[key])
    m={'format':'MVC1_CPS2_STATIC_M68K_STAGING_DISCOVERY_V1',
       'source_driver':driver,'source_snapshot_status':status,
       'source_run':str(run),'decrypted_fetch_bytes_local':len(code),
       'original_program_bytes_local':len(data),
       'code_sha256':hashlib.sha256(code).hexdigest(),
       'data_sha256':hashlib.sha256(data).hexdigest(),
       'dispatcher_bank_select_opcode_match':parsed['dispatcher_verified'],
       **parsed['stats'],**parsed['examples'],
       'source_pipeline':'68000 object builder motifs -> staging RAM -> final OBJ copier -> native sprite renderer',
       'staging_buffers_are_hardware_priorities':False,
       'stage_prop_ownership_verified':False,
       'animation_tables_verified':False,
       'staging_writer_instructions_executed_in_run1_verified':False,
       'program_binaries_exported':False,
       'method_note':'Exact static opcode motifs, NOT instruction-boundary-verified exhaustive disassembly or execution. Nearby pointer/PC-relative table links require runtime corroboration.',
       'pc_relative_note':'Decrypted FETCH and original READ streams differ for CPS2 code; indexed lookup table bytes require separate runtime validation.'}
    (output/'staging_analysis.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8')
    text=['MVC1 CPS2 68000 STAGING INVESTIGATION',
          f"Dispatcher opcode triplet verified: {parsed['dispatcher_verified']}",
          'Bank routing: $924000 for nonnegative type, $925000 for high nibble $80, otherwise $926000 for negative type.',
          f"MOVEM.W D0-D3,(A0) motifs: {len(parsed['emitters'])}",
          f"Emitters with nearby A0 staging literal: {m['nearby_staging_pointer_emitters']}",
          f"Nearby indexed tile lookups: {len(parsed['indexed'])}",
          f"Direct dispatcher call motifs: {len(parsed['calls'])}",
          '$006B8C indexes candidate tile data; $006B94 stores four words into staging RAM at $925800.',
          'Neither execution of these routines nor stage/actor ownership is established by run_1.',
          'All exports are metadata: no original program or decrypted instruction bytes.']
    (output/'staging_readme.txt').write_text('\n'.join(text)+'\n',encoding='utf-8')
    dest=output/'m68k_staging_report.zip'
    with zipfile.ZipFile(dest.with_suffix('.tmp'),'w',zipfile.ZIP_DEFLATED) as z:
        for name in OUTPUTS:z.write(output/name,arcname=name)
    dest.with_suffix('.tmp').replace(dest)
    return m

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    try:
        m=create_report(args.root,args.output)
        print('M68K_STAGING_REPORT refs={staging_pointer_motifs} emitters={descriptor_movem_motifs} indexed={indexed_tile_lookup_emitters} callers={dispatcher_direct_call_motifs} dispatcher={dispatcher_bank_select_opcode_match}'.format(**m))
        print('M68K_STAGING_BUNDLE',args.output/'m68k_staging_report.zip')
        return 0 if m['dispatcher_bank_select_opcode_match'] else 2
    except (OSError,ValueError,AnalysisError,zipfile.BadZipFile) as exc:
        print('M68K_STAGING_ERROR',str(exc),file=sys.stderr)
        return 2

if __name__=='__main__':
    raise SystemExit(main())
