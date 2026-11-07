#!/usr/bin/env python3
"""Analyze PRIVATE MVC1 Motorola 68000 CPS2 sprite-writer traces offline.

Uses existing XM87.16 attribute-commit writer provenance. Group by real 68000
PC, object RAM bank/slot, original tile family and palette; correlate sampled
address registers with ROM space. Transitions are CANDIDATES, not proven
engine animation scripts. Never uploads decrypted game code or raw ROM.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

COLUMNS = {'frame','bank','slot','address','size','pc','a0','a1','a2',
           'a3','a4','a5','xword','yword','tileword','attrword'}
HEX = ('pc','a0','a1','a2','a3','a4','a5','xword','yword',
       'tileword','attrword','address')
RUN = re.compile(r'run_\d+\Z')
OPCODE_NAME='mvc_opcodes_decrypted_host_words.bin'
DATA_NAME='mvc_program_data_host_words.bin'
FILE_NAMES=('m68k_writer_pcs.tsv','m68k_object_families.tsv',
            'm68k_tile_transitions.tsv','m68k_rom_tile_references.tsv',
            'm68k_rom_tile_sequences.tsv','m68k_manifest.json','m68k_summary.txt')
# Raw opcode bytes in m68k_writer_code.tsv are intentionally LOCAL ONLY.

class TraceError(ValueError):
    pass

def source_run(root:Path)->Path|None:
    if (root/'sprite_writes.tsv').is_file():
        return root
    if not root.is_dir():
        return None
    runs=sorted((p for p in root.iterdir()
                 if p.is_dir() and RUN.fullmatch(p.name)),key=lambda p:int(p.name[4:]))
    return runs[-1] if runs else None

def hv(row:dict,field:str)->int:
    try:
        value=row[field].strip()
        if not value or not re.fullmatch(r'(?:0[xX])?[0-9a-fA-F]+',value):
            raise ValueError('bad hex')
        return int(value,16)
    except (ValueError,KeyError,TypeError) as e:
        raise TraceError('invalid hex '+field+' in native writer record') from e

def dv(row:dict,field:str)->int:
    try:return int(row[field],10)
    except (ValueError,KeyError,TypeError) as e:
        raise TraceError('invalid integer '+field+' in native writer record') from e

def read_window(opcodes:bytes,pc:int)->str:
    if pc<0 or pc>=len(opcodes):
        return ''
    data=opcodes[pc:min(pc+24,len(opcodes))]
    if len(data)%2:data=data[:-1]
    decoded=bytearray(data)
    for i in range(0,len(decoded)-1,2):
        decoded[i],decoded[i+1]=decoded[i+1],decoded[i]
    return decoded.hex().upper()

def asm_window(opcodes:bytes,pc:int)->str:
    raw=read_window(opcodes,pc)
    if not raw:return ''
    try:
        from capstone import Cs,CS_ARCH_M68K,CS_MODE_M68K_000,CS_MODE_BIG_ENDIAN
        md=Cs(CS_ARCH_M68K,CS_MODE_M68K_000|CS_MODE_BIG_ENDIAN)
        listing=[]
        for i in md.disasm(bytes.fromhex(raw),pc):
            listing.append(f'{i.address:06X} {i.mnemonic} {i.op_str}')
            if len(listing)>=4:break
        return ' ; '.join(listing)
    except (ImportError,ValueError,AttributeError):
        return ''

def write_tsv(path:Path,fields:tuple[str,...],rows):
    with path.open('w',encoding='utf-8',newline='') as fp:
        wr=csv.writer(fp,delimiter='\t')
        wr.writerow(fields)
        for row in rows: wr.writerow(row)

def analyze(root:Path,output:Path,pc_limit:int=600)->dict:
    root=root.resolve()
    output=output.resolve()
    if output==root or root in output.parents or output in root.parents:
        raise TraceError('output must never be inside or overwrite private trace root')
    if not 1<=pc_limit<=10000:
        raise TraceError('pc_limit outside 1..10000')
    run=source_run(root)
    output.mkdir(parents=True,exist_ok=True)
    instructions=b''
    program_data=b''
    program_bytes=0
    writer_count=0
    pcs=defaultdict(Counter)
    details=defaultdict(lambda:{'frames':set(),'tiles':set(),'families':set(),
                                'palettes':set(),'priorities':set(),'registers':Counter(),
                                'first':None,'last':None})
    families=Counter()
    transitions=Counter()
    last_slot={}
    warnings=[]
    capped=False
    native_tile_words=Counter()
    nonblank_priorities=Counter()
    if run is None or not (run/'sprite_writes.tsv').is_file():
        warnings.append('NO_M68K_WRITER_TRACE: an existing MVC ROM run must have used RUN_TEST.bat mvscur1 trace; no writer PCs can be invented from OAM history.')
    else:
        data=run/DATA_NAME
        code=run/OPCODE_NAME
        if data.is_file():
            if data.stat().st_size>16*1024*1024:raise TraceError('private ROM exceeds CPS2 size limit')
            program_data=data.read_bytes()
            program_bytes=len(program_data)
        if code.is_file():
            if code.stat().st_size>16*1024*1024:raise TraceError('decrypted code source larger than CPS2 limit')
            instructions=code.read_bytes()
        else:
            warnings.append('No private decrypted opcode dump; writer PCs can still be grouped, but code bytes are unavailable.')
        with (run/'sprite_writes.tsv').open('r',encoding='utf-8',newline='') as fp:
            reader=csv.DictReader(fp,delimiter='\t')
            if not reader.fieldnames or not COLUMNS.issubset(reader.fieldnames):
                raise TraceError('missing original XM87.16 writer columns')
            for row in reader:
                frame=dv(row,'frame')
                bank=dv(row,'bank')
                slot=dv(row,'slot')
                pc=hv(row,'pc')
                address=hv(row,'address')
                size=dv(row,'size')
                xword=hv(row,'xword')
                yword=hv(row,'yword')
                tileword=hv(row,'tileword')
                attr=hv(row,'attrword')
                if not(0<=bank<=1 and 0<=slot<1024 and
                       0x708000<=address<0x710000 and
                       size in (1,2,4) and 0<=pc<0x1000000):
                    raise TraceError('out-of-range native 68000 writer bank/slot/PC')
                tile=tileword|((yword&0x6000)<<3)
                family=tile&~15
                pal=attr&31
                pri=(xword>>13)&7
                if tileword not in (0,0x8000,0xffff) and not(yword&0x8000) and attr<0xff00:
                    native_tile_words[tileword]+=1
                    nonblank_priorities[pri]+=1
                who=details[pc]
                who['frames'].add(frame)
                who['tiles'].add(tile)
                who['families'].add(family)
                who['palettes'].add(pal)
                who['priorities'].add(pri)
                who['first']=frame if who['first'] is None else min(who['first'],frame)
                who['last']=frame if who['last'] is None else max(who['last'],frame)
                pcs[pc][(family,pal,pri)]+=1
                families[(pc,family,pal,pri)]+=1
                for reg in ('a0','a1','a2','a3','a4','a5'):
                    regval=hv(row,reg)
                    if 0<regval<program_bytes and regval%2==0:
                        who['registers'][(reg,regval)]+=1
                # Bank+slot is recycled, so a transition is only provisional.
                # Require a consecutive writer observation window to count.
                key=(bank,slot)
                old=last_slot.get(key)
                if old and frame>=old[0] and frame-old[0]<=30 and old[1]!=tile:
                    transitions[(pc,old[2],old[1],tile,pal,pri)]+=1
                last_slot[key]=(frame,tile,pc)
                writer_count+=1
        if writer_count>=1500000:
            capped=True
            warnings.append('XM87.16 writer recorder reached its 1,500,000 record cap; the rest of the runtime is unobserved.')
    ranked=sorted(details,key=lambda pc:(-sum(pcs[pc].values()),pc))
    p_rows=[]
    for pc in ranked:
        d=details[pc]
        common=pcs[pc].most_common(1)[0][0]
        reg,addr,count=(*(d['registers'].most_common(1)[0][0]),d['registers'].most_common(1)[0][1]) if d['registers'] else ('',0,0)
        p_rows.append((f'{pc:06X}',sum(pcs[pc].values()),d['first'],d['last'],
            len(d['frames']),len(d['tiles']),len(d['families']),
            ','.join(map(str,sorted(d['priorities']))),
            ','.join(map(str,sorted(d['palettes']))),
            f'{common[0]:05X}',common[1],common[2],reg,
            f'{addr:06X}' if reg else '',count))
    write_tsv(output/'m68k_writer_pcs.tsv',
              ('pc_hex','writer_events','first_frame','last_frame',
               'distinct_frames','distinct_tiles','distinct_16_tile_families',
               'observed_priorities','observed_palettes','main_family_hex',
               'main_palette','main_priority','most_common_rom_pointer_register',
               'most_common_rom_pointer_hex','rom_pointer_hits'),p_rows)
    family_rows=[(f'{pc:06X}',f'{family:05X}',pal,pri,n)
                 for (pc,family,pal,pri),n in sorted(families.items(),
                      key=lambda kv:(-kv[1],kv[0]))]
    write_tsv(output/'m68k_object_families.tsv',
              ('writer_pc_hex','tile_16_family_hex','palette','priority','events'),
              family_rows)
    transition_rows=[(f'{writer:06X}',f'{prior_pc:06X}',
                      f'{old:05X}',f'{new:05X}',pal,pri,n,
                      'UNVERIFIED_SLOT_REUSE_OR_ANIMATION')
                     for (writer,prior_pc,old,new,pal,pri),n in
                     sorted(transitions.items(),key=lambda kv:(-kv[1],kv[0]))]
    write_tsv(output/'m68k_tile_transitions.tsv',
              ('next_writer_pc','previous_writer_pc','old_tile_hex','new_tile_hex',
               'palette','priority','observations','interpretation'),transition_rows)
    write_tsv(output/'m68k_writer_code.tsv',
              ('writer_pc_hex','decrypted_opcode_bytes_big_endian','disassembly_if_capstone_installed'),
              [(f'{pc:06X}',read_window(instructions,pc),asm_window(instructions,pc))
               for pc in ranked[:pc_limit]])
    # Search the original 68000 program READ space for the tile word IDs
    # actually written to CPS2 OBJ. The data image is host-word-swapped:
    # a 68000 16-bit 0x6CD0 appears as bytes D0 6C. Only aligned matches
    # are indexed. Coincidence and text-glyph tables remain unverified.
    matches=[]
    sequences=[]
    if len(program_data)>1 and native_tile_words:
        count=len(program_data)//2
        words=struct.unpack('<'+str(count)+'H',program_data[:count*2])
        for i,word in enumerate(words):
            if word in native_tile_words:
                matches.append((f'{i*2:06X}',f'{word:04X}',native_tile_words[word]))
        i=0
        while i+3<count:
            if words[i] not in native_tile_words:
                i+=1
                continue
            j=i+1
            while (j<count and words[j] in native_tile_words
                   and words[j]==words[j-1]+1):
                j+=1
            if j-i>=4:
                sequences.append((f'{i*2:06X}',j-i,f'{words[i]:04X}',f'{words[j-1]:04X}'))
                i=j
            else:
                i+=1
    write_tsv(output/'m68k_rom_tile_references.tsv',
              ('rom_data_offset_hex','observed_tile_word_hex','observed_writer_events'),matches)
    write_tsv(output/'m68k_rom_tile_sequences.tsv',
              ('rom_data_offset_hex','consecutive_tile_count','first_tile_hex','last_tile_hex'),sequences)
    report={'format':'XMVSF_STAGE87_203_MVC_M68K_WRITER_PROVENANCE_V1',
            'source_run':str(run) if run else None,'private_code_captured':bool(instructions),
            'private_data_captured':program_bytes>0,'writer_records':writer_count,
            'unique_writer_pcs':len(pcs),'object_families':len(families),
            'candidate_tile_transitions':len(transitions),
            'pc_disassembly_windows':min(pc_limit,len(ranked)),
            'reported_handler_pc_equals_instruction_start_verified':False,
            'nonblank_sprite_priorities':dict(sorted(nonblank_priorities.items())),
            'observed_nonblank_tile_words':len(native_tile_words),
            'rom_aligned_tile_references':len(matches),
            'rom_consecutive_tile_sequences':len(sequences),
            'trace_cap_reached':capped,'warnings':warnings,
            'provenance':'Guest 68000 PC at committed CPS2 attribute-word writes; no code modifications.',
            'verified_animation_tables':False,
            'verified_stage_sprite_identity':False,
            'interleaving_verified_by_this_tool':False,
            'confidence_note':'SekGetPC(-1) at the handler may reference an instruction extension word, not the write opcode boundary. Raw opcode context is local-only. ROM tile matches and same-slot transitions are candidates, not proven animation tables.',
            'rom_binaries_in_report':False,
            'raw_opcode_windows_in_report':False,
            'local_private_opcode_windows_file':'m68k_writer_code.tsv'}
    (output/'m68k_manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    summary=[f'MVC1 M68K WRITER PROVENANCE: {writer_count} events',
             f'Source trace: {run or "NOT FOUND"}',
             f'Unique 68000 writer PCs: {len(pcs)}',
             f'Object family signatures: {len(families)}',
             f'Candidate same-slot transitions: {len(transitions)}',
             f'Private code present locally: {bool(instructions)}',
             f'Nonblank sprite priorities: {dict(sorted(nonblank_priorities.items()))}',
             f'Aligned ROM-data tile references: {len(matches)}',
             f'ROM tile runs of 4+ consecutive IDs: {len(sequences)}',
             'Note: reported handler PC may land on an instruction extension word.',
             'No ROM/decrypted opcode bytes are included in the review bundle.',
             'Animation-script and scene-actor ownership NOT verified.']
    summary.extend('WARNING '+w for w in warnings)
    (output/'m68k_summary.txt').write_text('\n'.join(summary)+'\n',encoding='utf-8')
    archive=output/'m68k_sprite_report.zip'
    with zipfile.ZipFile(archive.with_suffix('.zip.tmp'),'w',zipfile.ZIP_DEFLATED) as fp:
        for name in FILE_NAMES:
            fp.write(output/name,arcname=name)
    archive.with_suffix('.zip.tmp').replace(archive)
    print('\n'.join(summary))
    print('M68K_REPORT '+str(archive))
    return report

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True,help='private m68k_trace root or run_XXXX')
    p.add_argument('--output',type=Path,required=True,help='separate review-only output directory')
    p.add_argument('--pc-limit',type=int,default=600)
    args=p.parse_args()
    try:
        result=analyze(args.root,args.output,args.pc_limit)
        return 0 if result['writer_records'] else 2
    except (OSError,TraceError,ValueError,zipfile.BadZipFile) as exc:
        print('M68K_ANALYSIS_ERROR: '+str(exc),file=sys.stderr)
        return 2
if __name__=='__main__':
    raise SystemExit(main())
