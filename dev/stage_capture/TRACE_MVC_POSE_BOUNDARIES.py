#!/usr/bin/env python3
"""Offline MVC1 Motorola 68000 candidate pose boundaries and sampled transitions.

Works exclusively from existing source: the original host-word-swapped program
READ image, native CPS2 OBJ observations, earlier pixel visibility evidence,
and the original camera record. Finds repeated (tile,attribute)*N followed
by a five-word $0002/control footer and reconstructs contiguous candidate
pose chains. Footer fields are preserved AS-IS, never called duration or
playback order without an independently decoded 68000 interpreter.

Distinct poses can be observed between 30-frame samples; intermediate frames,
exact animation timing, owner identity, and execution of a ROM table are NOT
verified. Nothing touches ROMs, Stable, the emulator, or source captures.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import json
from math import ceil
from pathlib import Path
import sys
import zipfile

import TRACE_MVC_ROM_ASSEMBLIES as source

VALID_CONTROLS = frozenset(range(0x35,0x3A))
OUTPUTS = ('rom_pose_records.tsv','rom_pose_sequences.tsv',
           'rom_pose_sample_matches.tsv','rom_pose_sample_transitions.tsv',
           'rom_pose_control_words.tsv','rom_pose_visual_review.tsv',
           'rom_pose_manifest.json',
           'rom_pose_readme.txt')

class PoseError(ValueError):
    pass


def words_after(blob:bytes,end:int):
    if end<0 or end+10>len(blob):return None
    return tuple(int.from_bytes(blob[end+i:end+i+2],'little')
                 for i in range(0,10,2))


def recover_records(blob:bytes,runs):
    """ROM cooccurrence runs + syntactically repeating five-word footer.

    A control-type word $0035..$0039 after $0002 is a *pattern*, not a
    confirmed instruction identifier. Requiring the next run exactly at
    footer_end gives independent neighboring-record structural evidence.
    """
    by_addr={a:tuple(pairs) for a,pairs in runs}
    records={}
    for a,pairs in runs:
        end=a+4*len(pairs)
        footer=words_after(blob,end)
        if footer is None or footer[0]!=2 or footer[3] not in VALID_CONTROLS:
            continue
        after=end+10
        records[a]={'addr':a,'pairs':tuple(pairs),'footer_at':end,
                    'footer':footer,'next':after if after in by_addr else None,
                    'pair_count':len(pairs)}
    # Only connect two records if BOTH have the same control-footer grammar;
    # an adjacent bare pair sequence alone is insufficient.
    for a,r in records.items():
        if r['next'] not in records:r['next']=None
    predecessors={r['next'] for r in records.values() if r['next'] is not None}
    sequences=[];seen=set()
    for start in sorted(records):
        if start in predecessors:continue
        chain=[];cur=start
        while cur in records and cur not in seen:
            chain.append(cur);seen.add(cur)
            cur=records[cur]['next']
        sequences.append(chain)
    if len(seen)!=len(records):
        raise PoseError('invalid candidate record adjacency cycle')
    sequences.sort(key=lambda chain:(-len(chain),chain[0]))
    return records,sequences


def read_owner_visibility(path:Path|None):
    return source.visibility_catalog(path)[0]


def allowed_pairs(grouped,visibility):
    """Keep exact original observed descriptors, remove pixel-confirmed blanks.

    If no decoded pixel evidence is supplied, do NOT claim original pixels
    were verified or silently remove any nonzero tile ID.
    """
    valid=set();full_by_pair=defaultdict(set)
    for frames in grouped.values():
        for obj_list in frames.values():
            for tile,attr,priority,w,h,x,y in obj_list:
                pair=(tile&0xffff,attr)
                full_by_pair[pair].add(tile)
                if tile not in (0,0xffff) and (not visibility or
                        visibility.get((tile,attr))=='VISIBLE'):
                    valid.add(pair)
    return valid,full_by_pair


def frame_support(grouped,records,sequences,camera,visibility,frame_gap=90):
    """Only observe sampled poses with distinctive pairs and a clear winner.

    No decoded footer timings, no fabricated interpolation. Within a single
    shot, an ambiguous pair shared by many poses cannot establish which pose
    the game displayed. This errs on the side of omitting weak transitions.
    """
    sample_rows=[];switch_rows=[];metrics={}
    for chain in sequences:
        if len(chain)<2:continue
        cid=f'{chain[0]:06X}'
        unique=[set(records[addr]['pairs']) for addr in chain]
        frequency=Counter(p for rec in unique for p in rec)
        distinctive=[{p for p in rec if frequency[p]==1} for rec in unique]
        union=set().union(*unique)
        matched=[]
        for shot,shot_frames in grouped.items():
            prior=None
            for frame,objects in sorted(shot_frames.items()):
                frame_pairs={(x[0]&0xffff,x[1]) for x in objects
                             if x[0] not in (0,0xffff) and
                             (not visibility or visibility.get((x[0],x[1]))=='VISIBLE')}
                if len(frame_pairs&union)<2:continue
                ranks=[]
                for idx,rec in enumerate(unique):
                    count=len(frame_pairs&rec)
                    ratio=count/len(rec)
                    uniq=len(frame_pairs&distinctive[idx])
                    if count>=2 and ratio>=0.6:
                        ranks.append((ratio,uniq,count,chain[idx]))
                if not ranks:continue
                ranks.sort(reverse=True)
                best=ranks[0]
                second=ranks[1] if len(ranks)>1 else None
                # Do not arbitrarily assign a record if two poses score the
                # same recall AND unique observed descriptors.
                resolved=bool(best[1]>=1 and (second is None or
                      (best[0],best[1])>(second[0],second[1])))
                verdict='DISTINCTIVE_SAMPLED_POSE' if resolved else 'AMBIGUOUS_SHARED_COMPONENTS'
                matched.append((shot,frame,best[3],resolved))
                sample_rows.append((cid,shot,frame,f'{best[3]:06X}',best[2],
                                    len(unique[chain.index(best[3])]),best[1],
                                    f'{best[0]:.3f}',verdict))
                if (prior is not None and resolved and prior[2] and
                    prior[0] != best[3] and 0<frame-prior[1]<=frame_gap):
                    a,b=prior[1],frame
                    # Always frame sample intervals; never assign a
                    # duration from the uninterpreted footer words.
                    cam='CAMERA_AVAILABLE' if a in camera and b in camera else 'NO_CAMERA'
                    switch_rows.append((cid,shot,a,b,
                         f'{prior[0]:06X}',f'{best[3]:06X}',b-a,cam,
                         'OBSERVED_BETWEEN_SAMPLES_NOT_EXACT_TIMING'))
                prior=(best[3],frame,resolved)
        metrics[cid]={'sample_hits':len(matched),
                      'resolved_sample_hits':sum(m[3] for m in matched),
                      'shots':len({m[0] for m in matched}),
                      'observed_switches':sum(row[0]==cid for row in switch_rows)}
    return sample_rows,switch_rows,metrics


def report(observations:Path,trace_root:Path,owner:Path|None,
           capture:Path|None,output:Path, visual_notes:Path|None=None)->dict:
    output=output.resolve()
    sources=[observations,trace_root]+([owner] if owner else [])+([capture] if capture else [])+([visual_notes] if visual_notes else [])
    for original in sources:
        src=original.resolve()
        if src==output or src in output.parents or output in src.parents:
            raise PoseError('output overlaps existing source evidence')
    private=source.find_private_data(trace_root)
    if private.stat().st_size>16*1024*1024:
        raise PoseError('invalid original program size')
    grouped,counts,frames,frame_prio,full_tiles,present=source.observations(observations)
    visibility=read_owner_visibility(owner)
    valid,full_by_pair=allowed_pairs(grouped,visibility)
    program=private.read_bytes()
    runs=source.candidate_runs(program,valid)
    records,chains=recover_records(program,runs)
    camera,selected_sessions,conflicts=source.read_camera(capture)
    matched_rows,switches,metrics=frame_support(grouped,records,chains,camera,visibility)
    controls=Counter(r['footer'][3] for r in records.values())
    linked={r['addr'] for r in records.values() if r['next'] is not None}
    chain_ids={a:f'{c[0]:06X}' for c in chains for a in c}
    chain_lengths={c[0]:len(c) for c in chains}
    output.mkdir(parents=True,exist_ok=True)
    def tsv(name,header,rows):
        with (output/name).open('w',newline='',encoding='utf-8') as f:
            wr=csv.writer(f,delimiter='\t');wr.writerow(header);wr.writerows(rows)
    record_rows=[]
    for a,r in sorted(records.items()):
        pairs=r['pairs'];footer=r['footer']
        pair_frames=set().union(*(frames.get(p,set()) for p in pairs))
        cotogether=set.intersection(*(frames.get(p,set()) for p in set(pairs)))
        priorities=sorted({pr for p in pairs for pr in range(8)
                           if frame_prio.get((p,pr))})
        record_rows.append((f'{a:06X}',chain_ids[a],len(pairs),
             len(set(pairs)),f'{r["footer_at"]:06X}',
             *(f'{word:04X}' for word in footer),
             f'{r["next"]:06X}' if r['next'] is not None else '',
             len(cotogether),len(pair_frames),
             ','.join(map(str,priorities)),
             'CONFIRMED_PIXEL_ELIGIBLE' if visibility else 'PIXELS_UNVERIFIED',
             'FOOTER_AND_NEXT_VALIDATED' if a in linked else 'FOOTER_ONLY',
             'UNDECODED_CONTROL_NOT_PROVEN_ANIMATION_FRAME'))
    tsv('rom_pose_records.tsv',
        ('record_offset_hex','chain_start_hex','tile_attribute_pairs',
         'distinct_pairs','footer_offset_hex','control_0_hex',
         'control_1_hex','control_2_hex','control_3_hex','control_4_hex',
         'contiguous_next_record_hex','all_parts_copresent_samples',
         'any_part_sample_frames','priority_channels',
         'original_pixel_visibility','boundary_evidence','timing_status'),
        record_rows)
    seq_rows=[]
    for chain in sorted(chains,key=lambda c:(-len(c),c[0])):
        full=set().union(*(set(records[a]['pairs']) for a in chain))
        cor=Counter(records[a]['footer'][3] for a in chain)
        hit=metrics.get(f'{chain[0]:06X}',{})
        seq_rows.append((f'{chain[0]:06X}',f'{chain[-1]:06X}',len(chain),
                         len(full),sum(len(records[a]['pairs']) for a in chain),
                         ','.join(f'{x:04X}:{cor[x]}' for x in sorted(cor)),
                         hit.get('sample_hits',0),hit.get('resolved_sample_hits',0),
                         hit.get('shots',0),hit.get('observed_switches',0),
                         'CONTIGUOUS_CONTROL_FOOTERS_UNVERIFIED_RUNTIME'))
    tsv('rom_pose_sequences.tsv',
        ('chain_start_hex','chain_last_record_hex','candidate_pose_records',
         'distinct_original_tile_attr_pairs','total_record_parts',
         'control_selectors','sampled_pose_candidates','distinctive_pose_samples',
         'observed_shots','observed_sampled_pose_changes','interpretation'),seq_rows)
    tsv('rom_pose_sample_matches.tsv',
        ('chain_start_hex','shot','sample_frame','best_rom_pose_record_hex',
         'matched_distinct_pairs','record_distinct_pairs',
         'record_unique_pairs_present','coverage_fraction','interpretation'),
        matched_rows)
    tsv('rom_pose_sample_transitions.tsv',
        ('chain_start_hex','shot','sample_from','sample_to','old_record_hex',
         'new_record_hex','sample_gap_frames','camera','timing_disclaimer'),
        switches)
    tsv('rom_pose_control_words.tsv',
        ('control_selector_hex','record_count','linked_to_next_record_count',
         'control_1_distinct','control_2_distinct','control_4_distinct',
         'verified_interpreter_semantics'),
        [(f'{ctl:04X}',n,
          sum(r['next'] is not None for r in records.values() if r['footer'][3]==ctl),
          len({r['footer'][1] for r in records.values() if r['footer'][3]==ctl}),
          len({r['footer'][2] for r in records.values() if r['footer'][3]==ctl}),
          len({r['footer'][4] for r in records.values() if r['footer'][3]==ctl}),
          'NO') for ctl,n in sorted(controls.items())])
    visual_rows=[]
    if visual_notes is not None and visual_notes.is_file():
        with visual_notes.open('r',encoding='utf-8',newline='') as f:
            rd=csv.DictReader(f,delimiter='\t')
            needed={'record_offset_hex','shot','sample_frame',
                    'visual_category','confidence','reviewer_note'}
            if not rd.fieldnames or not needed.issubset(rd.fieldnames):
                raise PoseError('visual notes must provide original address and review fields')
            for row in rd:
                try:a=int(row['record_offset_hex'],16)
                except ValueError as e:
                    raise PoseError('invalid manual visual review record address') from e
                if a not in records:
                    # Never apply a stale visual label to a different ROM.
                    continue
                visual_rows.append((f'{a:06X}',chain_ids[a],row['shot'],
                                    row['sample_frame'],row['visual_category'],
                                    row['confidence'],row['reviewer_note'],
                                    'HUMAN_IMAGE_REVIEW_NOT_NATIVE_ROM_IDENTITY'))
    tsv('rom_pose_visual_review.tsv',
        ('record_offset_hex','chain_start_hex','shot','sample_frame',
         'visual_category','confidence','reviewer_note','provenance'),
        visual_rows)
    manifest={
        'format':'MVC1_68000_CANDIDATE_POSE_RECORDS_V1',
        'original_rom_pair_runs':len(runs),'records_with_repeating_control_footers':len(records),
        'records_with_adjacent_validated_next':len(linked),
        'candidate_chains':len(chains),
        'multi_record_chains':sum(len(c)>=2 for c in chains),
        'longest_chain_records':max(map(len,chains),default=0),
        'controls_by_selector_hex':{f'{k:04X}':v for k,v in sorted(controls.items())},
        'camera_source_sessions':selected_sessions,
        'sampled_pose_match_rows':len(matched_rows),
        'distinctive_pose_matches':sum(x[-1]=='DISTINCTIVE_SAMPLED_POSE' for x in matched_rows),
        'observed_pose_changes_between_samples':len(switches),
        'pixel_visibility_source_used':bool(visibility),
        'manual_visual_notes_applied':len(visual_rows),
        'original_68000_animation_frame_boundary_verified':False,
        'original_68000_footer_semantics_verified':False,
        'original_duration_and_timing_verified':False,
        'stage_or_fighter_identity_verified':False,
        'all_original_object_observations_retained_in_sources':True,
        'uncertainty':'A repeating $0002 metadata/control block and exact next-record adjacency establish candidate data structure, not the engine interpretation. Distinctive poses observed 30+ frames apart give interval-only changes, not exact native timings.',
        'privacy':'No original ROM/data, decoded graphics, source observations, or raw program bytes included in report ZIP.'}
    (output/'rom_pose_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (output/'rom_pose_readme.txt').write_text(
        'MVC1 MOTOROLA 68000 ORIGINAL ROM POSE RECORD INVESTIGATION\n'
        f"{len(runs)} candidate tile/attribute runs; {len(records)} repeating-control records; "
        f"{len(linked)} consecutive links; {sum(len(c)>=2 for c in chains)} multi-record chains.\n"
        'Footer signature: $0002, raw_u1, raw_u2, selector $0035..$0039, raw_u4.\n'
        'These are candidate ROM pose boundaries, NOT engine-decoded script frames.\n'
        'Controls are retained as original words; do not call them animation durations or frame indices.\n'
        'Sample matches are labeled ambiguous unless distinctive exact descriptor pairs support one pose.\n'
        'Pose changes are observed only BETWEEN source samples; intermediate frame timing is unknown.\n'
        'Camera motion and exact decoded ink do not by themselves prove stage ownership.\n'
        'Optional visual descriptions are explicitly HUMAN observations, not decoded engine object IDs.\n'
        'Only metadata is exported; original program and graphics remain private locally.\n',encoding='utf-8')
    bundle=output/'rom_pose_boundaries_review.zip'
    with zipfile.ZipFile(bundle.with_suffix('.zip.tmp'),'w',zipfile.ZIP_DEFLATED) as z:
        for name in OUTPUTS:z.write(output/name,arcname=name)
    bundle.with_suffix('.zip.tmp').replace(bundle)
    return manifest


def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--observations',type=Path,required=True)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--owner-evidence',type=Path)
    p.add_argument('--capture-root',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--visual-notes',type=Path,help='optional separately maintained review labels')
    args=p.parse_args()
    try:
        result=report(args.observations,args.trace_root,args.owner_evidence,
                      args.capture_root,args.output,args.visual_notes)
        print('MVC_ROM_POSES records={records_with_repeating_control_footers} links={records_with_adjacent_validated_next} chains={multi_record_chains} sampled_switches={observed_pose_changes_between_samples}'.format(**result))
        print('MVC_ROM_POSES_BUNDLE',args.output/'rom_pose_boundaries_review.zip')
        return 0
    except (OSError,ValueError,zipfile.BadZipFile) as exc:
        print('MVC_ROM_POSES_ERROR',exc,file=sys.stderr)
        return 2

if __name__=='__main__':raise SystemExit(main())
