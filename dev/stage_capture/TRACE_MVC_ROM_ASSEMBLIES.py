#!/usr/bin/env python3
"""MVC1 68000 READ-data sprite assemblies crosschecked with CPS2 OAM/camera.

This is an OFFLINE evidence extractor, not a ROM-script decoder. Consecutive
(tileword, attrword) pairs in the original Motorola 68000 program data are
candidate multi-part sprite records. Same-frame co-occurrence and camera-locked
motion are independent corroboration, not proof of stage or animation ownership.
No ROM/program, native pixels, private code, or source observations in ZIP.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import gzip
import io
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

REQUIRED=('shot','frame','x','y','tile','attributes','priority','width_tiles','height_tiles')
CAMERA=('scroll1x','scroll1y','scroll2x','scroll2y','scroll3x','scroll3y')
REPORTS=('rom_candidate_assemblies.tsv','rom_assembly_pairs.tsv',
         'camera_locked_objects.tsv','camera_co_motion_groups.tsv',
         'rom_candidate_pose_neighbors.tsv','rom_assembly_manifest.json',
         'rom_assembly_readme.txt')
class AssemblyError(ValueError):pass

def signed_change(old:int,new:int,mod:int)->int:
    return ((new-old+mod//2)%mod)-mod//2

def find_private_data(root:Path)->Path:
    if root.is_file() and root.name=='mvc_program_data_host_words.bin':return root
    if (root/'mvc_program_data_host_words.bin').is_file():
        return root/'mvc_program_data_host_words.bin'
    if root.is_dir():
        runs=sorted((p for p in root.iterdir() if p.is_dir() and
                     re.fullmatch(r'run_\d+',p.name)),
                    key=lambda p:int(p.name[4:]),reverse=True)
        for run in runs:
            p=run/'mvc_program_data_host_words.bin'
            if p.is_file():return p
    raise AssemblyError('existing MVC1 68000 original program READ data is missing')

def observations(path:Path):
    if not path.is_file():raise AssemblyError('original observations are missing')
    grouped=defaultdict(lambda:defaultdict(list))
    pair_counts=Counter()
    present=defaultdict(set)
    pair_frame=defaultdict(set)
    pair_frame_priority=defaultdict(set)
    full_tile_by_pair=defaultdict(set)
    if path.suffix=='.gz':opened=gzip.open(path,'rt',newline='',encoding='utf-8')
    else:opened=path.open('r',newline='',encoding='utf-8')
    with opened as f:
        reader=csv.DictReader(f,delimiter='\t')
        if not reader.fieldnames or not set(REQUIRED).issubset(reader.fieldnames):
            raise AssemblyError('missing native sprite observation columns')
        for row in reader:
            shot=row['shot'];frame=int(row['frame']);tile=int(row['tile'])
            attr=int(row['attributes']);prio=int(row['priority'])
            w=int(row['width_tiles']);h=int(row['height_tiles'])
            if not(0<=tile<0x40000 and 0<=attr<0x10000 and
                   0<=prio<=7 and 1<=w<=16 and 1<=h<=16):
                raise AssemblyError('out-of-range original OBJ fields')
            obj=(tile,attr,prio,w,h,int(row['x']),int(row['y']))
            grouped[shot][frame].append(obj)
            pair=(tile&0xffff,attr)
            pair_counts[pair]+=1
            pair_frame[pair].add((shot,frame))
            pair_frame_priority[(pair,prio)].add((shot,frame))
            full_tile_by_pair[pair].add(tile)
            present[(shot,frame)].add(pair)
    return grouped,pair_counts,pair_frame,pair_frame_priority,full_tile_by_pair,present

def visibility_catalog(path:Path|None):
    vis={}
    labels={}
    if path is None or not path.is_file():return vis,labels
    with path.open('r',encoding='utf-8',newline='') as f:
        reader=csv.DictReader(f,delimiter='\t')
        if not reader.fieldnames or not {'tile_hex','attributes_hex','pixel_visibility','candidate_role'}.issubset(reader.fieldnames):
            raise AssemblyError('owner evidence TSV is missing pixel visibility fields')
        for r in reader:
            k=(int(r['tile_hex'],16),int(r['attributes_hex'],16))
            vis[k]=r['pixel_visibility']
            labels[k]=r['candidate_role']
    return vis,labels

def read_camera(root:Path|None):
    """Choose the current *contiguous capture-session suffix*, not all history.

    Prior MVC sessions overlap newer sessions at the same frame numbers, but
    are different playthroughs. Never mix their camera records with the 17-shot
    observations derived from the latest complete session chain.
    """
    if root is None or not root.exists():return {},[],0
    camera={}
    entries=[]
    if root.is_file():
        if not zipfile.is_zipfile(root):raise AssemblyError('capture must be a directory or ZIP')
        with zipfile.ZipFile(root) as z:
            for name in sorted(n for n in z.namelist() if n.endswith('/frames.tsv')):
                entries.append((name,z.read(name).decode('utf-8')))
    else:
        paths=sorted(root.glob('session_*/frames.tsv'))
        if not paths and (root/'frames.tsv').is_file():paths=[root/'frames.tsv']
        entries=[(str(p),p.read_text(encoding='utf-8')) for p in paths]
    candidates=[]
    for name,body in entries:
        rows=list(csv.DictReader(io.StringIO(body),delimiter='\t'))
        if not rows:continue
        if not set(CAMERA).union({'frame'}).issubset(rows[0]):
            raise AssemblyError('capture frames.tsv is missing CPS2 scroll columns')
        first=int(rows[0]['frame']);last=int(rows[-1]['frame'])
        session=re.search(r'session_(\d+)',name)
        ordinal=int(session.group(1)) if session else 0
        candidates.append((ordinal,first,last,name,rows))
    if not candidates:return {},[],0
    latest=max(candidates,key=lambda c:c[0])
    selected=[]
    seen=set()
    current=latest
    while current and current[0:3] not in seen:
        selected.append(current)
        seen.add(current[0:3])
        if current[1]<=1:break
        prior=[c for c in candidates if c[0]<current[0] and c[2]==current[1]-1]
        current=max(prior,key=lambda c:c[0]) if prior else None
    selected.reverse()
    for _,first,last,name,rows in selected:
        for row in rows:
            frame=int(row['frame'])
            if frame in camera:raise AssemblyError('selected native capture sessions overlap')
            camera[frame]=tuple(int(row[k]) for k in CAMERA)
    return camera,[s[3] for s in selected],0

def camera_motion(grouped,camera,visibility, min_delta=8,tolerance=3):
    """Require one occurrence of the EXACT descriptor in each adjacent sample.

    This deliberately misses animated sprites that change their tile ID; it
    cannot accidentally assign such changes to a ROM animation script.
    """
    events=[]
    for shot,frames in sorted(grouped.items()):
        prior=None
        for frame,objs in sorted(frames.items()):
            if prior is not None:
                old_frame,old_objs=prior
                if frame-old_frame>90 or frame not in camera or old_frame not in camera:
                    prior=(frame,objs);continue
                a,b=camera[old_frame],camera[frame]
                shifts=[(signed_change(a[2*i],b[2*i],65536),
                         signed_change(a[2*i+1],b[2*i+1],65536)) for i in range(3)]
                if not any(abs(dx)+abs(dy)>=min_delta for dx,dy in shifts):
                    prior=(frame,objs);continue
                lo=defaultdict(list);hi=defaultdict(list)
                for o in old_objs:lo[o[:5]].append(o)
                for o in objs:hi[o[:5]].append(o)
                for key,left in lo.items():
                    right=hi.get(key,())
                    if len(left)!=1 or len(right)!=1:continue
                    src=left[0];dst=right[0]
                    if visibility and visibility.get((src[0],src[1]))!='VISIBLE':continue
                    dx=signed_change(src[5],dst[5],1024)
                    dy=signed_change(src[6],dst[6],1024)
                    if abs(dx)+abs(dy)<min_delta:continue
                    matching=[i+1 for i,(sx,sy) in enumerate(shifts)
                              if abs(sx)+abs(sy)>=min_delta and
                              abs(dx+sx)+abs(dy+sy)<=tolerance]
                    if matching:
                        events.append({'shot':shot,'from':old_frame,'to':frame,
                                       'tile':src[0],'attr':src[1],
                                       'priority':src[2],'x0':src[5],'y0':src[6],
                                       'x1':dst[5],'y1':dst[6],
                                       'screen_dx':dx,'screen_dy':dy,
                                       'layers':tuple(matching),
                                       'residual':min(abs(dx+shifts[i-1][0])+
                                                      abs(dy+shifts[i-1][1]) for i in matching)})
            prior=(frame,objs)
    return events

def candidate_runs(program:bytes,valid:set[tuple[int,int]]):
    if not program or len(program)%2 or len(program)>16*1024*1024:
        raise AssemblyError('invalid original Motorola 68000 program data length')
    words=struct.unpack('<%dH'%(len(program)//2),program)
    i=0;runs=[]
    while i+3<len(words):
        if (words[i],words[i+1]) in valid and (words[i+2],words[i+3]) in valid:
            begin=i;j=i
            while j+1<len(words) and (words[j],words[j+1]) in valid:
                j+=2
            pairs=[(words[k],words[k+1]) for k in range(begin,j,2)]
            runs.append((begin*2,pairs))
            i=j
        else:i+=1
    return runs

def report(observations_path:Path,trace:Path,owner_tsv:Path|None,
           camera_source:Path|None,output:Path):
    output=output.resolve()
    sources=[observations_path,trace]+([owner_tsv] if owner_tsv else [])+([camera_source] if camera_source else [])
    for source in sources:
        source=source.resolve()
        if source==output or source in output.parents or output in source.parents:
            raise AssemblyError('output must not overlap or modify original input paths')
    private=find_private_data(trace)
    if private.stat().st_size>16*1024*1024:raise AssemblyError('unexpected MVC program size')
    (groups,counts,frames,frame_prio,full_tiles,present)=observations(observations_path)
    vis,labels=visibility_catalog(owner_tsv)
    camera,chosen_sessions,conflicts=read_camera(camera_source)
    motions=camera_motion(groups,camera,vis)
    motion_by_pair=Counter((e['tile']&0xffff,e['attr']) for e in motions)
    motion_full=Counter((e['tile'],e['attr']) for e in motions)
    cohorts=defaultdict(list)
    for ev in motions:
        cohorts[(ev['shot'],ev['from'],ev['to'],ev['layers'],
                 ev['screen_dx'],ev['screen_dy'])].append(ev)
    strong_cohorts=[(key,arr) for key,arr in cohorts.items()
                    if len({(x['tile'],x['attr']) for x in arr})>=3]
    eligible={p for p in counts if (any(vis.get((t,p[1]))=='VISIBLE'
                  for t in full_tiles[p]) if vis else p[0] not in (0,0xffff))}
    runs=candidate_runs(private.read_bytes(),eligible)
    outputs=[];pair_rows=[]
    for address,pairs in runs:
        unique=set(pairs)
        visible=[p for p in unique if any(vis.get((t,p[1]))=='VISIBLE'
                                             for t in full_tiles[p])]
        camera_hits=sum(motion_by_pair[p] for p in unique)
        coframes=set()
        copro=Counter()
        if len(unique)>=2:
            # Pairwise intersections are not complete animation-frame proof:
            # long runs may include many separate sprite poses.
            for pi,p in enumerate(pairs):
                for q in pairs[pi+1:]:
                    if p==q:continue
                    both=frames[p].intersection(frames[q])
                    coframes.update(both)
                    for pr in range(8):
                        n=len(frame_prio[(p,pr)] & frame_prio[(q,pr)])
                        if n:copro[pr]+=n
        if camera_hits and len(visible)>=2 and coframes:
            tier='CAMERA_AND_ROM_PAIR_COHERENT_CANDIDATE'
        elif len(visible)>=2 and coframes:
            tier='CO_DRAWN_ROM_PAIR_CANDIDATE'
        else:tier='ROM_PAIR_SEQUENCE_ONLY'
        if len(unique)==1:
            tier='REPEATED_SINGLE_PAIR_SEQUENCE'
        outputs.append({'offset':address,'pairs':pairs,'visible':len(visible),
                        'camera_hits':camera_hits,'coframes':len(coframes),
                        'copro':copro,'tier':tier})
        for i,p in enumerate(pairs):
            full=sorted(full_tiles[p])
            pixel='VISIBLE' if p in visible else 'UNKNOWN_OR_TRANSPARENT'
            pair_rows.append((f'{address:06X}',i,f'{p[0]:04X}',f'{p[1]:04X}',
                              ','.join(f'{x:05X}' for x in full[:12]),
                              counts[p],len(frames[p]),motion_by_pair[p],pixel))
    output.mkdir(parents=True,exist_ok=True)
    def tsv(name,head,rows):
        with (output/name).open('w',newline='',encoding='utf-8') as f:
            writer=csv.writer(f,delimiter='\t');writer.writerow(head);writer.writerows(rows)
    rank={'CAMERA_AND_ROM_PAIR_COHERENT_CANDIDATE':0,
          'CO_DRAWN_ROM_PAIR_CANDIDATE':1,
          'ROM_PAIR_SEQUENCE_ONLY':2,
          'REPEATED_SINGLE_PAIR_SEQUENCE':3}
    outputs.sort(key=lambda d:(rank[d['tier']],-d['camera_hits'],
                               -d['coframes'],-d['visible'],d['offset']))
    tsv('rom_candidate_assemblies.tsv',
        ('rom_address_hex','pair_count','distinct_pairs','visible_distinct_pairs',
         'co_drawn_sample_frames','camera_lock_events','same_priority_pair_coappearances',
         'priority_channels','classification','first_pair','last_pair'),
        [(f"{x['offset']:06X}",len(x['pairs']),len(set(x['pairs'])),x['visible'],
          x['coframes'],x['camera_hits'],sum(x['copro'].values()),
          ','.join(str(n) for n in sorted(x['copro'])),x['tier'],
          f"{x['pairs'][0][0]:04X}/{x['pairs'][0][1]:04X}",
          f"{x['pairs'][-1][0]:04X}/{x['pairs'][-1][1]:04X}") for x in outputs])
    tsv('rom_assembly_pairs.tsv',
        ('rom_run_offset_hex','pair_index','tileword_hex','attributes_hex',
         'observed_full_tile_ids','observations','sample_frames',
         'camera_lock_events','pixel_visibility'),pair_rows)
    tsv('camera_locked_objects.tsv',
        ('shot','from_frame','to_frame','full_tile_hex','attributes_hex','priority',
         'x_before','y_before','x_after','y_after','observed_screen_dx',
         'observed_screen_dy','compatible_scroll_layers','pixel_residual'),
        [(x['shot'],x['from'],x['to'],f"{x['tile']:05X}",f"{x['attr']:04X}",
          x['priority'],x['x0'],x['y0'],x['x1'],x['y1'],x['screen_dx'],
          x['screen_dy'],','.join(map(str,x['layers'])),x['residual']) for x in motions])
    tsv('camera_co_motion_groups.tsv',
        ('shot','from_frame','to_frame','compatible_layers','screen_dx','screen_dy',
         'distinct_exact_sprite_parts','full_tiles','evidence'),
        [(key[0],key[1],key[2],','.join(map(str,key[3])),key[4],key[5],
          len(arr),','.join(f"{x['tile']:05X}" for x in arr),
          'MATCHED_CAMERA_MOTION_NOT_PROVEN_ACTOR')
         for key,arr in sorted(strong_cohorts)])
    neighbors=[]
    ascending=sorted(outputs,key=lambda x:x['offset'])
    for a,b in zip(ascending,ascending[1:]):
        sep=b['offset']-(a['offset']+4*len(a['pairs']))
        shared=set(a['pairs'])&set(b['pairs'])
        if 0<=sep<=64 and shared:
            neighbors.append((f"{a['offset']:06X}",f"{b['offset']:06X}",sep,
                              len(shared),','.join(f'{v:04X}/{at:04X}' for v,at in sorted(shared)[:10]),
                              'PROXIMATE_SHARED_PAIRS_POSSIBLE_POSE_NEIGHBOR_NOT_TIMING'))
    tsv('rom_candidate_pose_neighbors.tsv',
        ('run_a_offset_hex','run_b_offset_hex','bytes_between_runs',
         'shared_exact_pairs','sample_shared_pairs','interpretation'),neighbors)
    manifest={'format':'MVC1_68000_ROM_PAIR_ASSEMBLY_CROSSCHECK_V1',
              'object_observations':sum(counts.values()),
              'sampled_frames':len(present),'captured_camera_frames':len(camera),
              'conflicted_camera_frames_excluded':conflicts,
              'authoritative_camera_session_chain':chosen_sessions,
              'candidate_pairs_with_visible_artwork':len(eligible),
              'distinct_source_tile_attribute_pairs':len(counts),
              'rom_candidate_pair_runs':len(outputs),
              'runs_with_co_drawn_visible_parts':sum(x['visible']>=2 and x['coframes']>0 for x in outputs),
              'runs_with_camera_motion_and_codrawwn_parts':sum(x['tier']=='CAMERA_AND_ROM_PAIR_COHERENT_CANDIDATE' for x in outputs),
              'exact_descriptor_camera_lock_pairs':len(motions),
              'distinct_camera_co_motion_cohorts_at_least_3_parts':len(strong_cohorts),
              'adjacent_candidate_pose_neighbors':len(neighbors),
              'native_pixel_source_checks_available':bool(vis),
              'observed_stage_ownership_verified':False,
              'original_animation_script_boundaries_verified':False,
              '68000_upstream_routine_execution_verified':False,
              'frame_sampling_note':'Sparse 30-frame samples hide many intermediate animation frames; constant exact descriptors are the only camera motion anchors.',
              'cooccurrence_note':'ROM runs may concatenate multiple poses; coappearance of any two pieces is not proof that the whole run draws in one frame.',
              'source_privacy':'No original program, decrypted code, gfx bytes or raw OAM observations included in review ZIP.'}
    (output/'rom_assembly_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (output/'rom_assembly_readme.txt').write_text(
        'MVC1 ORIGINAL MOTOROLA 68000 DATA SPRITE ASSEMBLIES\n'
        f"Original samples: {manifest['object_observations']}; candidate contiguous pair runs: {len(outputs)}\n"
        f"Camera-locked exact descriptor pairs: {len(motions)}; groups of 3+ co-moving parts: {len(strong_cohorts)}\n"
        f"Camera plus ROM co-draw candidate runs: {manifest['runs_with_camera_motion_and_codrawwn_parts']}\n"
        'The strongest signal combines full tile+attribute pairs, same-frame cooccurrence, and nonzero screen motion that exactly negates a recorded scroll-plane change.\n'
        'All stage/actor ownership, 68000 execution, and animation timing are still unverified.\n'
        'Private original 68000 READ data remains only on the local machine.\n',encoding='utf-8')
    bundle=output/'rom_assemblies_review_bundle.zip'
    with zipfile.ZipFile(bundle.with_suffix('.tmp'),'w',zipfile.ZIP_DEFLATED) as z:
        for name in REPORTS:z.write(output/name,arcname=name)
    bundle.with_suffix('.tmp').replace(bundle)
    return manifest

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--observations',required=True,type=Path)
    ap.add_argument('--trace-root',required=True,type=Path)
    ap.add_argument('--owner-evidence',type=Path)
    ap.add_argument('--capture-root',type=Path)
    ap.add_argument('--output',required=True,type=Path)
    a=ap.parse_args()
    try:
        r=report(a.observations,a.trace_root,a.owner_evidence,a.capture_root,a.output)
        print('M68K_ROM_ASSEMBLIES runs={rom_candidate_pair_runs} camera_locked={exact_descriptor_camera_lock_pairs} candidates={runs_with_camera_motion_and_codrawwn_parts}'.format(**r))
        print('M68K_ROM_ASSEMBLY_BUNDLE',a.output/'rom_assemblies_review_bundle.zip')
        return 0
    except (OSError,ValueError,zipfile.BadZipFile) as exc:
        print('M68K_ROM_ASSEMBLY_ERROR',str(exc),file=sys.stderr)
        return 2
if __name__=='__main__':raise SystemExit(main())