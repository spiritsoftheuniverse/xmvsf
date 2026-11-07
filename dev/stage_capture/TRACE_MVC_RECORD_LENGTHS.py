#!/usr/bin/env python3
"""Offline original MVC1 M68000 data-record length grammar survey.

Parses host-word-swapped original program READ bytes. Repeating five-word
$0002 metadata patterns with $0035..$0039 in word3 are connected only when
the next block exists EXACTLY at offset +10+4*(word2+1). This length equation
is established by source-data adjacency, NOT by an executed ROM interpreter.
No timing, stage identity, pose ownership or animation playback is inferred.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

SELECTORS = range(0x35,0x3A)
OUTPUTS = ('rom_structural_headers.tsv', 'rom_structural_regions.tsv',
           'rom_structural_terminals.tsv', 'rom_structural_manifest.json',
           'rom_structural_readme.txt')

class RecordError(ValueError): pass

def scan(blob:bytes):
    if not blob or len(blob)%2 or len(blob)>16*1024*1024:
        raise RecordError('invalid original host-word-swapped program image')
    w=struct.unpack('<%dH'%(len(blob)//2),blob)
    records={}
    for i in range(len(w)-4):
        if w[i]==2 and w[i+3] in SELECTORS:
            at=i*2
            # Do not infer a duration from word2. Treat word2+1 purely
            # as a candidate FOUR-BYTE BODY LENGTH and test adjacency.
            body_count=w[i+2]+1
            proposed=at+10+4*body_count
            records[at]=(tuple(w[i:i+5]),body_count,proposed)
    links={a:v[2] for a,v in records.items() if v[2] in records}
    parents=defaultdict(list)
    for a,other in links.items(): parents[other].append(a)
    if any(len(p)>1 for p in parents.values()):
        raise RecordError('ambiguous multiple predecessor links in original source')
    roots=sorted(records.keys()-parents.keys())
    regions=[]; seen=set()
    for root in roots:
        cur=root;region=[]
        while cur in records and cur not in seen:
            region.append(cur);seen.add(cur)
            cur=links.get(cur)
        regions.append(region)
    if len(seen)!=len(records):
        raise RecordError('record graph failed full coverage check')
    regions.sort(key=lambda v:v[0])
    return records,links,regions

def write_tsv(path,columns,rows):
    with path.open('w',newline='',encoding='utf-8') as f:
        writer=csv.writer(f,delimiter='\t')
        writer.writerow(columns)
        writer.writerows(rows)

def latest_original(root:Path)->Path:
    if root.is_file() and root.name=='mvc_program_data_host_words.bin': return root
    if (root/'mvc_program_data_host_words.bin').is_file():
        return root/'mvc_program_data_host_words.bin'
    if root.is_dir():
        runs=sorted((d for d in root.iterdir() if d.is_dir() and
                  re.fullmatch(r'run_\d+',d.name)),
                  key=lambda p:int(p.name[4:]),reverse=True)
        for run in runs:
            p=run/'mvc_program_data_host_words.bin'
            if p.is_file():return p
    raise RecordError('missing existing original MVC1 program READ image')

def report(trace_root:Path,pose_manifest:Path,output:Path)->dict:
    trace_root=trace_root.resolve()
    pose_manifest=pose_manifest.resolve()
    output=output.resolve()
    for src in (trace_root,pose_manifest,pose_manifest.parent):
        if output==src or output in src.parents or src in output.parents:
            raise RecordError('output overlaps private program or original pose evidence')
    source=latest_original(trace_root)
    blob=source.read_bytes()
    checksum=hashlib.sha256(blob).hexdigest()
    if not pose_manifest.is_file():
        raise RecordError('missing original checksum-guarded pose manifest')
    manifest=json.loads(pose_manifest.read_text(encoding='utf-8'))
    if manifest.get('format')!='MVC1_68000_CANDIDATE_POSE_RECORDS_V1' or\
       manifest.get('private_rom_sha256_for_note_matching')!=checksum:
        raise RecordError('STALE pose manifest for a different original ROM')
    rec,links,regions=scan(blob)
    rindex={a:(region[0],i) for region in regions for i,a in enumerate(region)}
    control_counts=Counter(rec[a][0][3] for a in rec)
    output.mkdir(parents=True,exist_ok=True)
    write_tsv(output/OUTPUTS[0],
        ('original_control_block_offset_hex','region_start_hex','index_in_region',
         'raw_word_0_hex','raw_word_1_hex','raw_word_2_hex','raw_word_3_hex',
         'raw_word_4_hex','body_start_offset_hex','inferred_4byte_body_units',
         'predicted_next_control_hex','validated_next_control_hex',
         'evidence_status'),
        ((f'{a:06X}',f'{rindex[a][0]:06X}',rindex[a][1],
          *(f'{v:04X}' for v in rec[a][0]),f'{a+10:06X}',rec[a][1],
          f'{rec[a][2]:06X}',f'{links[a]:06X}' if a in links else '',
          'SOURCE_ADJACENCY_VERIFIED_NOT_FRAME_TIMING' if a in links else
          'TERMINAL_OR_OTHER_GRAMMAR') for a in sorted(rec)))
    write_tsv(output/OUTPUTS[1],
        ('region_first_block_hex','region_last_block_hex',
         'header_count','validated_adjacencies','original_byte_span',
         'control_selector_counts','sampled_animation_frames_verified'),
        ((f'{v[0]:06X}',f'{v[-1]:06X}',len(v),len(v)-1,
          rec[v[-1]][2]-v[0],
          ','.join(f'{k:04X}:{n}' for k,n in sorted(Counter(rec[a][0][3] for a in v).items())),
          'NO') for v in regions))
    write_tsv(output/OUTPUTS[2],
        ('region_first_block_hex','terminal_block_hex','predicted_next_hex',
         'terminal_control_code_hex','reason'),
        ((f'{v[0]:06X}',f'{v[-1]:06X}',f'{rec[v[-1]][2]:06X}',
          f'{rec[v[-1]][0][3]:04X}',
          'NO_MATCHING_NEXT_FIVE_WORD_GRAMMAR_NOT_VERIFIED_ANIMATION_END')
          for v in regions))
    result={'format':'MVC1_ORIGINAL_M68K_DATA_RECORD_LENGTH_GRAMMAR_V1',
        'private_original_program_sha256':checksum,
        'five_word_control_grammar_blocks':len(rec),
        'exact_body_length_next_block_adjacencies':len(links),
        'contiguous_structural_regions':len(regions),
        'terminal_or_grammar_change_records':len(rec)-len(links),
        'multi_predecessor_conflicts':0,
        'longest_structural_region_records':max((len(c) for c in regions),default=0),
        'control_codes':{f'{k:04X}':v for k,v in sorted(control_counts.items())},
        'body_length_formula':'next_byte_offset = control_block_offset + 10 + 4*(raw_word_2 + 1)',
        'length_field_cpu_runtime_semantics_proven':False,
        'animation_timing_verified':False,
        'regions_are_separate_executed_animations':False,
        'stage_or_fighter_identification_verified':False,
        'privacy':'Metadata-only; original program, opcode image, decoded graphics and raw OAM not included.'}
    (output/OUTPUTS[3]).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    (output/OUTPUTS[4]).write_text(
       'MVC1 original 68000 five-word header-like blocks and structural adjacency.\n'
       'The old tile-pair-based exporter termed these blocks FOOTERS.\n'
       'Original code points TO the start of these blocks and separate render\n'
       'readers consume the first word, making HEADER-like interpretation likely.\n'
       'The third word equals the following descriptor-unit count minus one\n'
       'across observed exact header-to-header links. This is data grammar,\n'
       'not independently executed 68000 field semantics.\n'
       'Never call the contiguous regions complete animations or frame durations.\n'
       'The rare nonmatching successors may be sentinel/alternate format.\n'
       'No private ROM or graphics binaries distributed.\n',encoding='utf-8')
    zip_path=output/'rom_structural_records_review.zip'
    tmp=output/'rom_structural_records_review.zip.tmp'
    with zipfile.ZipFile(tmp,'w',zipfile.ZIP_DEFLATED) as z:
        for name in OUTPUTS:z.write(output/name,arcname=name)
    tmp.replace(zip_path)
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--pose-manifest',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    try:
        result=report(a.trace_root,a.pose_manifest,a.output)
        print('MVC_ROM_RECORD_LENGTHS blocks={five_word_control_grammar_blocks} links={exact_body_length_next_block_adjacencies} regions={contiguous_structural_regions}'.format(**result))
        print('MVC_ROM_RECORD_LENGTHS_ZIP',a.output/'rom_structural_records_review.zip')
        return 0
    except (OSError,ValueError,zipfile.BadZipFile) as e:
        print('MVC_ROM_RECORD_LENGTHS_ERROR',e,file=sys.stderr)
        return 2

if __name__=='__main__':raise SystemExit(main())
