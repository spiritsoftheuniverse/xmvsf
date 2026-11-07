#!/usr/bin/env python3
"""87.203 MVC1 offline sprite origin evidence: static 68000 literals, native
OBJ observations, original decoded pixels, and writer PC history. Evidence
classification only. No game execution, ROM patching or raw assets in ZIP.
"""
from __future__ import annotations
import argparse
import csv
import gzip
import json
import re
import struct
import sys
import zipfile
from collections import Counter, defaultdict
from functools import lru_cache
from pathlib import Path

FILES=('object_family_evidence.tsv','m68k_descriptor_matches.tsv',
       'm68k_runtime_overlap.tsv','owner_evidence_manifest.json',
       'owner_evidence_readme.txt')

class EvidenceError(ValueError): pass

def latest(root:Path):
    if (root/'sprite_writes.tsv').is_file(): return root
    if not root.is_dir(): return None
    dirs=sorted((p for p in root.iterdir() if p.is_dir()
                 and re.fullmatch(r'run_\d+',p.name)
                 and (p/'sprite_writes.tsv').is_file()),
                key=lambda p:int(p.name[4:]))
    return dirs[-1] if dirs else None

def observations(path:Path):
    opener=gzip.open if path.suffix=='.gz' else open
    if not path.is_file():raise EvidenceError('missing original sprite_observations.tsv.gz')
    variants={}
    total=0
    with opener(path,'rt',encoding='utf-8',newline='') as fp:
        source=csv.DictReader(fp,delimiter='\t')
        needed={'shot','sample','tile','attributes','priority','x','y','width_tiles','height_tiles'}
        if not source.fieldnames or not needed.issubset(source.fieldnames):
            raise EvidenceError('incomplete native sprite observation fields')
        for row in source:
            t,a,w,h=(int(row[k]) for k in ('tile','attributes','width_tiles','height_tiles'))
            pr=int(row['priority'])
            if not (0<=t<0x40000 and 0<=a<=0xffff and 0<=pr<8 and 1<=w<=16 and 1<=h<=16):
                raise EvidenceError('invalid CPS2 sprite values')
            key=(t,a,w,h)
            if key not in variants:
                variants[key]={'n':0,'shots':set(),'samples':set(),
                               'prios':Counter(),'positions':Counter()}
            v=variants[key]
            v['n']+=1
            v['shots'].add(row['shot'])
            v['samples'].add((row['shot'],row['sample']))
            v['prios'][pr]+=1
            v['positions'][(int(row['x']),int(row['y']))]+=1
            total+=1
    return variants,total

def writer_history(root:Path):
    run=latest(root)
    by_tile=Counter()
    priorities=Counter()
    raw=0;nonblank=0
    if run is None:return run,by_tile,priorities,raw,nonblank
    with (run/'sprite_writes.tsv').open('r',encoding='utf-8',newline='') as fp:
        rows=csv.DictReader(fp,delimiter='\t')
        if not rows.fieldnames or not {'tileword','yword','xword','attrword'}.issubset(rows.fieldnames):
            raise EvidenceError('invalid 68000 sprite writer trace format')
        for row in rows:
            raw+=1
            tw=int(row['tileword'],16);yw=int(row['yword'],16)
            xw=int(row['xword'],16);a=int(row['attrword'],16)
            if tw in (0,0x8000,0xffff) or yw&0x8000 or a>=0xff00:continue
            full=tw|((yw&0x6000)<<3)
            by_tile[full]+=1
            priorities[(xw>>13)&7]+=1
            nonblank+=1
    return run,by_tile,priorities,raw,nonblank

class Pixels:
    def __init__(self,path:Path|None):
        self.path=path if path and path.is_file() else None
        self.fp=self.path.open('rb') if self.path else None
        self.length=self.path.stat().st_size if self.path else 0
        self.mask=(1<<(max(1,self.length)-1).bit_length())-1
    @lru_cache(maxsize=150000)
    def tile(self,t:int):
        if not self.fp:return None
        off=(t<<7)&self.mask
        if off+128>self.length:return None
        self.fp.seek(off)
        raw=self.fp.read(128)
        return bool(any(raw)) if len(raw)==128 else None
    @lru_cache(maxsize=100000)
    def object(self,t:int,w:int,h:int):
        unknown=False
        for dy in range(h):
            for dx in range(w):
                sub=(t&~15)+(dy<<4)+((t+dx)&15)
                result=self.tile(sub)
                if result is True:return True
                if result is None:unknown=True
        return None if unknown else False
    def close(self):
        if self.fp:self.fp.close()

def role(v,opaque):
    if opaque is False:return 'NO_VISIBLE_SOURCE_PIXELS'
    fixed=v['positions'].most_common(1)[0][1]/v['n']
    if (len(v['shots'])>=5 and len(v['positions'])<=4
        and v['prios'][7]/v['n']>=.98 and fixed>=.35):
        return 'REPEATED_FIXED_INTERFACE_CANDIDATE'
    return 'UNRESOLVED_STAGE_FIGHTER_OR_FX'

def literal(code:bytes,pc:int):
    """Nearby 68000 MOVE.W #imm,D2 and MOVE.W #imm,D3 patterns.
    Local opcode motifs are NOT independently validated instruction flow."""
    result={'d2':None,'d3':None,'d2_kind':'UNKNOWN','d3_kind':'UNKNOWN'}
    if pc<0 or pc+4>len(code):return result
    def w(at):return struct.unpack_from('<H',code,at)[0]
    if w(pc)!=0x4890 or w(pc+2)!=0xf:return result
    e2=[];e3=[]
    for a in range(max(0,pc-48),pc-1,2):
        op=w(a)
        if op==0x343c and a+4<=pc:e2.append((a,'IMMEDIATE',w(a+2)))
        if op==0x343b:e2.append((a,'INDEXED_PC',None))
        if op in (0x342d,0x3429,0x3411,0x3407,0x3405,0x3406,0x342e,0x3428,0x3400):
            e2.append((a,'DYNAMIC',None))
        if op==0x363c and a+4<=pc:e3.append((a,'IMMEDIATE',w(a+2)))
        if op&0xff00==0x7600:e3.append((a,'MOVEQ',op&255))
    if e2 and pc-e2[-1][0]<=30:
        result['d2_kind']=e2[-1][1]
        result['d2']=e2[-1][2]
    if e3 and pc-e3[-1][0]<=30:
        result['d3_kind']=e3[-1][1]
        result['d3']=e3[-1][2]
    return result

def csv_out(path:Path,columns,rows):
    with path.open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f,delimiter='\t')
        writer.writerow(columns)
        writer.writerows(rows)

def analyze(observation_file:Path,trace_root:Path,emitters_file:Path,output:Path,
            gfx_file:Path|None=None):
    output=output.resolve()
    for source in (observation_file,trace_root,emitters_file,gfx_file):
        if source is None:continue
        source=source.resolve()
        if output==source or output in source.parents or source in output.parents:
            raise EvidenceError('review output may not overlap an original source')
    variants,total=observations(observation_file)
    run,writer_tiles,writer_prios,raw_writes,nonblank=writer_history(trace_root)
    code=b''
    if run:
        op=run/'mvc_opcodes_decrypted_host_words.bin'
        if op.is_file():
            if op.stat().st_size>0x1000000:raise EvidenceError('program opcode dump too large')
            code=op.read_bytes()
    pixels=Pixels(gfx_file)
    bylow=defaultdict(list)
    summary=Counter()
    prio_all=Counter();prio_visible=Counter()
    family_rows=[]
    try:
        for (t,a,w,h),v in sorted(variants.items()):
            opaque=pixels.object(t,w,h)
            classification=role(v,opaque)
            summary[classification]+=v['n']
            for pr,count in v['prios'].items():
                prio_all[pr]+=count
                if opaque is True:prio_visible[pr]+=count
            xy,count=v['positions'].most_common(1)[0]
            family_rows.append((f'{t:05X}',f'{a:04X}',w,h,v['n'],
                                len(v['shots']),len(v['samples']),
                                ','.join(map(str,sorted(v['prios']))),
                                xy[0],xy[1],count,len(v['positions']),
                                ('VISIBLE' if opaque is True else
                                 'TRANSPARENT' if opaque is False else 'UNKNOWN'),
                                classification,writer_tiles[t]))
            bylow[t&0xffff].append((t,a,v,classification))
    finally:
        pixels.close()
    matches=[]
    if code and emitters_file.is_file():
        with emitters_file.open('r',newline='',encoding='utf-8') as fp:
            rdr=csv.DictReader(fp,delimiter='\t')
            if not rdr.fieldnames or not {'movem_pc_hex','nearby_a0_base_hex'}.issubset(rdr.fieldnames):
                raise EvidenceError('invalid original staging emitter index')
            for row in rdr:
                pc=int(row['movem_pc_hex'],16)
                d=literal(code,pc)
                entries=bylow.get(d['d2'],[]) if d['d2'] is not None else []
                # Compare exact attribute only if 68000 literal is known.
                entries=[e for e in entries if d['d3'] is None or e[1]==d['d3']]
                if entries:
                    t,a,v,classification=max(entries,key=lambda e:e[2]['n'])
                    hits=sum(e[2]['n'] for e in entries)
                    shots=len(set().union(*(e[2]['shots'] for e in entries)))
                    lead=f'{t:05X}'
                    attrs=f'{a:04X}'
                else:
                    hits=shots=0
                    lead=attrs=''
                    classification='NO_EXACT_LITERAL_MATCH_OR_DYNAMIC'
                matches.append((f'{pc:06X}',row['nearby_a0_base_hex'],
                    d['d2_kind'],f"{d['d2']:04X}" if d['d2'] is not None else '',
                    d['d3_kind'],f"{d['d3']:04X}" if d['d3'] is not None else '',
                    len(entries),hits,shots,lead,attrs,classification,
                    'STATIC_PATTERN_NOT_VERIFIED_RUNTIME_WRITER'))
    runtime=[]
    for t,n in writer_tiles.most_common():
        entries=[e for e in bylow[t&0xffff] if e[0]==t]
        hits=sum(e[2]['n'] for e in entries)
        shots=len(set().union(*(e[2]['shots'] for e in entries))) if entries else 0
        runtime.append((f'{t:05X}',n,hits,shots,
                        ','.join(sorted({e[3] for e in entries})) if entries
                        else 'NOT_IN_EARLIER_STAGE_CAPTURE'))
    output.mkdir(parents=True,exist_ok=True)
    csv_out(output/'object_family_evidence.tsv',
      ('tile_hex','attributes_hex','width_tiles','height_tiles','object_observations',
       'shots','sample_frames','priority_channels','top_x','top_y','top_position_count',
       'unique_positions','source_pixels','provisional_role','run1_writer_tile_events'),family_rows)
    csv_out(output/'m68k_descriptor_matches.tsv',
      ('movem_pc_hex','staging_buffer_hex','tile_operand','tileword_hex',
       'attribute_operand','attribute_hex','matching_variants',
       'matching_observations','matching_shots','top_full_tile_hex',
       'top_attributes_hex','provisional_role','writer_execution_certainty'),matches)
    csv_out(output/'m68k_runtime_overlap.tsv',
      ('full_tile_hex','run1_writer_events','earlier_stage_observations',
       'earlier_stage_shots','provisional_role'),runtime)
    report={'format':'MVC1_M68K_OBJECT_OWNER_EVIDENCE_V1',
        'objects_observed':total,'distinct_tile_attribute_size_variants':len(variants),
        'native_writer_records':raw_writes,'native_writer_nonblank':nonblank,
        'native_writer_priority_counts':dict(sorted(writer_prios.items())),
        'all_observation_priority_counts':dict(sorted(prio_all.items())),
        'pixel_visible_priority_counts':dict(sorted(prio_visible.items()))
                                      if pixels.path else None,
        'original_decoded_pixels_checked':bool(pixels.path),
        '68000_descriptor_patterns':len(matches),
        'literal_tile_and_attribute_pattern_matches':sum(
            bool(e[6] and e[3] and e[5]) for e in matches),
        'provisional_role_counts':dict(summary),
        'verified_stage_actor_owner':False,
        'upstream_descriptor_writer_executed_in_run1_verified':False,
        'limits':['Source pixels validated only with decoded MVC1 GFX file',
                  '0 ink is transparent: blank OAM records are not visible sprites',
                  'The run_1 M68K write trace captured only priority-7 nonblank writes',
                  'Tile low 16 bits need original Y word for the high graphics bank',
                  'Fixed repeated priority-7 geometry only suggests interface',
                  'Code scanning is static; control flow and animation scripts remain unverified',
                  'No original encrypted code, decrypted opcodes or game graphics are exported.']}
    (output/'owner_evidence_manifest.json').write_text(
        json.dumps(report,indent=2)+'\n',encoding='utf-8')
    (output/'owner_evidence_readme.txt').write_text(
        f"MVC1 68000 sprite-origin evidence\n"
        f"Objects: {total}, variants: {len(variants)}, writer events: {nonblank}\n"
        f"Pixel visibility checked: {bool(pixels.path)}\n"
        f"Static descriptor patterns: {len(matches)}\n"
        "No stage/fighter ownership automatically verified.\n"
        "No raw ROMs, sprites, or opcodes in this review ZIP.\n",encoding='utf-8')
    zipname=output/'m68k_owner_evidence_review.zip'
    with zipfile.ZipFile(zipname.with_suffix('.tmp'),'w',zipfile.ZIP_DEFLATED) as z:
        for name in FILES:z.write(output/name,arcname=name)
    zipname.with_suffix('.tmp').replace(zipname)
    print('M68K_OBJECT_OWNERS observed={objects_observed} variants={distinct_tile_attribute_size_variants} matched_emitters={literal_tile_and_attribute_pattern_matches}'.format(**report))
    print('M68K_OBJECT_OWNERS_BUNDLE',zipname)
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--observations',type=Path,required=True)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--staging-emitters',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--gfx',type=Path)
    p.add_argument('--capture-root',type=Path,
                   help='optional native stage_capture root to find existing decoded MVC GFX')
    args=p.parse_args()
    gfx=args.gfx
    if gfx is None and args.capture_root and args.capture_root.is_dir():
        candidates=sorted(args.capture_root.glob('session_*/mvc1_decoded_gfx_*.bin'))
        if candidates:gfx=candidates[0]
    try:analyze(args.observations,args.trace_root,args.staging_emitters,args.output,gfx)
    except (OSError,ValueError,zipfile.BadZipFile) as exc:
        print('M68K_OBJECT_OWNERS_ERROR',str(exc),file=sys.stderr)
        return 2
    return 0

if __name__=='__main__':raise SystemExit(main())
