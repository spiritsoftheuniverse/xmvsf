#!/usr/bin/env python3
"""Offline CPS2 ALL-CHANNEL review from existing Stage44/47 MVC clean-rip files.

Preserves every observed priority-0..7 object in inventories; never guesses
scenery/fighter/HUD ownership. Bounded, unbiased per-channel *previews* use
original tile IDs + actual saved effective palettes. No ROM, game or build.
The native scrolls 1/2/3 and native no-OBJ scroll composite are also included.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import gzip
import json
from pathlib import Path
import sys
import zipfile

import SPRITE_FACTORY as sf
from EXPORT_CLEAN_RIP import SAFE_RGBA, fnv64, png_bytes

OBS_COLUMNS=('shot','sample','frame','index','x','y','tile','attributes',
             'priority','palette','width_tiles','height_tiles','flip')
INDEX_COLUMNS=('priority','shot','tile','palette','width_tiles','height_tiles',
               'flip','observations','first_frame','last_frame','distinct_frames',
               'first_object_index','sample','png')

class ChannelError(Exception): pass


def one_clean_background(shot: Path, layer: int, sample: int) -> tuple[bytes, dict]:
    if not 0 <= layer <= 3 or sample < 0:
        raise ChannelError('invalid scroll channel/sample')
    with (shot/'layers.tsv').open('r', newline='', encoding='utf-8') as fp:
        rows=csv.DictReader(fp,delimiter='\t')
        found=next((r for r in rows if int(r['layer'])==layer and
                    int(r['sample'])==sample),None)
    if found is None:
        raise ChannelError(f'{shot.name} lacks scroll {layer} sample {sample}')
    filename=found['file']
    match=SAFE_RGBA.fullmatch(filename)
    if not match or int(match.group(1))!=layer or int(match.group(2))!=sample:
        raise ChannelError('unsafe or mismatched native scroll filename')
    if (int(found['width']),int(found['height']))!=(384,224):
        raise ChannelError('native scroll dimensions not 384x224')
    raw=(shot/filename).read_bytes()
    if len(raw)!=384*224*4 or fnv64(raw)!=int(found['fnv64'],16):
        raise ChannelError(f'bad source byte count or FNV64: {filename}')
    return raw, {'shot':shot.name,'layer':layer,'sample':sample,
                 'native_frame':int(found['emulated_frame']),
                 'scroll_x':int(found['scroll_x']),
                 'scroll_y':int(found['scroll_y']),
                 'native_fnv64':found['fnv64']}


def variants_from_observations(observations: list[dict]) -> dict[int, list[dict]]:
    """Include actual occurrence priority in each identity (no modal collapse)."""
    variants={}
    for ob in observations:
        p=ob['priority']
        if p not in range(8):
            raise ChannelError(f'outside CPS2 priority 0..7: {p}')
        sig=(p,ob['shot'],ob['tile'],ob['palette'],ob['width_tiles'],
             ob['height_tiles'],ob['flip'])
        c=variants.get(sig)
        if c is None:
            c={'priority':p,'shot':ob['shot'],'tile':ob['tile'],
               'palette':ob['palette'],'width_tiles':ob['width_tiles'],
               'height_tiles':ob['height_tiles'],'flip':ob['flip'],
               'attributes':ob['attributes'],
               'first_frame':ob['frame'],'last_frame':ob['frame'],
               'first_object_index':ob['index'],'sample':ob['sample'],
               'observations':0,'frame_set':set(),'png':''}
            variants[sig]=c
        c['observations']+=1
        c['frame_set'].add(ob['frame'])
        c['first_frame']=min(c['first_frame'],ob['frame'])
        c['last_frame']=max(c['last_frame'],ob['frame'])
    per_priority={i:[] for i in range(8)}
    for c in variants.values():
        c['distinct_frames']=len(c.pop('frame_set'))
        per_priority[c['priority']].append(c)
    for p in range(8):
        per_priority[p].sort(key=lambda x:(x['shot'],x['first_frame'],x['tile'],
                                           x['palette'],x['flip']))
    return per_priority


def review_order(items: list[dict]) -> list[dict]:
    """Equal-time and equal-shot coverage; NO category/priority/motion ranking.

    A deterministic round-robin walks the entire unique-address inventory
    uniformly across each shot. All unselected objects remain in TSV.
    """
    groups=defaultdict(list)
    for c in items:
        groups[c['shot']].append(c)
    for group in groups.values():
        group.sort(key=lambda x:(x['first_frame'],x['first_object_index'],x['tile']))
    ordered={}
    for shot,group in groups.items():
        n=len(group)
        if n<2:
            ordered[shot]=group
        else:
            step=max(1,n//12)
            head=list(range(0,n,step))
            seen=set(head)
            ordered[shot]=[group[i] for i in head]+[v for i,v in enumerate(group) if i not in seen]
    result=[]
    maxlen=max((len(x) for x in ordered.values()),default=0)
    for i in range(maxlen):
        for shot in sorted(ordered):
            if i<len(ordered[shot]):result.append(ordered[shot][i])
    return result


def build(clean_root: Path, output: Path, *, max_previews_per_priority=32,
          background_shots=3) -> dict:
    clean_root=clean_root.resolve()
    output=output.resolve()
    if output==clean_root or output in clean_root.parents or clean_root in output.parents:
        raise ChannelError('review output must not overlap source captures')
    if not 1<=max_previews_per_priority<=128:
        raise ChannelError('per-priority previews must be 1..128')
    if not 1<=background_shots<=20:
        raise ChannelError('background shots must be 1..20')
    shots=sf.load_shots(clean_root)
    if not shots: raise ChannelError('no clean_rips/shot_XXXX directories found')
    observations=[]
    shot_data={}
    warnings=[]
    for shot in shots:
        try:
            obs=sf.get_clean_objects(shot)
            if not obs:
                warnings.append(f'{shot.name}: no sprites recorded')
                continue
            tile_lookup=sf.clean_tile_lookup(shot)
            palettes=shot/'palettes.bin'
            if not palettes.is_file() or palettes.stat().st_size%sf.PAL_RECORD:
                raise ChannelError('missing or truncated native effective palette records')
            shot_data[shot.name]=(shot,tile_lookup)
            observations.extend(obs)
        except (sf.SpriteError,OSError,ValueError,KeyError) as e:
            warnings.append(f'{shot.name}: no usable sprite source ({e})')
    if not shot_data:
        raise ChannelError('no valid native sprite tile/palette source shots')
    groups=variants_from_observations(observations)
    priority_counts=Counter(ob['priority'] for ob in observations)
    output.mkdir(parents=True, exist_ok=True)
    assets=[]
    channels=[]
    missing_tiles=Counter()
    contact_count=0
    for priority in range(8):
        folder=output/f'priority_{priority}'
        folder.mkdir(exist_ok=True)
        (folder/'sprites').mkdir(exist_ok=True)
        entries=groups[priority]
        exported=[]
        for item in review_order(entries):
            if len(exported)>=max_previews_per_priority:break
            shot,tiles=shot_data[item['shot']]
            try:
                palette=sf.clean_palette(shot,item['sample'])
                w,h,pixels,missing=sf.compose_sprite(
                    item['tile'],item['palette'],item['width_tiles'],
                    item['height_tiles'],item['attributes'],tiles.get,palette,True)
                if missing:
                    missing_tiles[priority]+=len(missing)
                    continue
                if not any(pixels[3::4]): continue
            except (sf.SpriteError,OSError,ValueError) as e:
                warnings.append(f'priority_{priority}: {item["shot"]}: {e}')
                continue
            fname=(f'{item["shot"]}_f{item["first_frame"]:08d}'
                   f'_t{item["tile"]:05x}_p{item["palette"]:02d}'
                   f'_w{item["width_tiles"]}h{item["height_tiles"]}'
                   f'_flip{item["flip"]}.png')
            rel=f'priority_{priority}/sprites/{fname}'
            (output/rel).write_bytes(png_bytes(w,h,pixels))
            item['png']=rel
            exported.append({**item,'pixels':pixels,'pixel_width':w,
                             'pixel_height':h,'sample_count':item['distinct_frames']})
            assets.append(rel)
        contacts=[]
        for start in range(0,len(exported),16):
            filename=f'priority_{priority}/contact_{start//16+1:03d}.png'
            (output/filename).write_bytes(sf.make_sheet(exported[start:start+16]))
            contacts.append(filename)
            assets.append(filename)
            contact_count+=1
        path=folder/'catalog.tsv'
        with path.open('w',newline='',encoding='utf-8') as fp:
            wr=csv.writer(fp,delimiter='\t')
            wr.writerow(INDEX_COLUMNS)
            for item in entries:
                wr.writerow([item.get(k,'') for k in INDEX_COLUMNS])
        assets.append(f'priority_{priority}/catalog.tsv')
        channels.append({'channel':priority,'type':'CPS2_OBJ_PRIORITY',
                         'object_observations':priority_counts[priority],
                         'unique_address_variants':len(entries),
                         'review_previews':len(exported),
                         'contact_sheets':contacts,
                         'all_source_objects_indexed':True,
                         'stage_ownership_verified':False})
    inventory='all_priority_observations.tsv.gz'
    with gzip.open(output/inventory,'wt',newline='',encoding='utf-8') as fp:
        wr=csv.writer(fp,delimiter='\t')
        wr.writerow(OBS_COLUMNS)
        for ob in observations:
            wr.writerow([ob[k] for k in OBS_COLUMNS])
    assets.append(inventory)
    backgrounds=[]
    usable=sorted(shot_data)
    selected=[]
    if usable:
        for i in range(min(background_shots,len(usable))):
            k=round(i*(len(usable)-1)/max(1,min(background_shots,len(usable))-1))
            if usable[k] not in selected:selected.append(usable[k])
    scroll_dir=output/'scroll_channels'
    scroll_dir.mkdir(exist_ok=True)
    for shot_name in selected:
        shot,_=shot_data[shot_name]
        with (shot/'layers.tsv').open('r',newline='',encoding='utf-8') as fp:
            rows=csv.DictReader(fp,delimiter='\t')
            samples=sorted({int(r['sample']) for r in rows if int(r['layer'])==0})
        if not samples:continue
        sample=samples[len(samples)//2]
        for layer in range(4):
            try:
                pixels,info=one_clean_background(shot,layer,sample)
                name=f'scroll_channels/{shot_name}_scroll{layer}.png'
                (output/name).write_bytes(png_bytes(384,224,pixels))
                info['png']=name
                info['native_pixels_hash_validated']=True
                backgrounds.append(info)
                assets.append(name)
            except (ChannelError,OSError,ValueError) as e:
                warnings.append(f'{shot_name} scroll{layer}: {e}')
    manifest={
        'format':'XM87203_ALL_CPS2_CHANNELS_V1',
        'source':'original Stage44/47 clean-rip tiles, effective palettes and OAM',
        'clean_root':str(clean_root),
        'sprite_channels':channels,
        'backgrounds':backgrounds,
        'total_observations':len(observations),
        'total_indexed_variants':sum(len(v) for v in groups.values()),
        'preview_limit_per_priority':max_previews_per_priority,
        'total_preview_contact_sheets':contact_count,
        'complete_per_occurrence_priority_index':inventory,
        'hud_fighter_effect_scenery_filters_applied':False,
        'all_eight_priorities_included_even_if_empty':True,
        'stage_identity_verified':False,
        'scenery_classification_verified':False,
        'native_obj_depth_and_blend_reproduction_verified':False,
        'limitations':[
            'Every captured object occurrence is indexed, but PNG previews are sampled for manageable file size.',
            'Sprite object tile assembly is direct and uses native effective palette; final on-screen OBJ priority occlusion and blend require native-render verification.',
            'Scroll channels 1/2/3 and native scroll-only composite (0) are original rendered pixels, not guessed scene reconstruction.',
            'No objects are called scenery merely because of their priority.',
        ],
        'warnings':warnings,
    }
    (output/'channel_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    assets.append('channel_manifest.json')
    summary=(f'All CPS2 OBJ priority channels: {len(observations)} observations, '
             f'{manifest["total_indexed_variants"]} unique per-priority address variants.\n'
             'No HUD, fighter, FX, or scenery filtering. All observations indexed.\n'
             'Only PNG previews are capped; use individual channel catalogs for all source variants.\n'
             'Scroll channels (0 original no-OBJ composite + 1/2/3) are separate.\n')
    for c in channels:
        summary+=(f'priority_{c["channel"]}: {c["object_observations"]} observations, '
                  f'{c["unique_address_variants"]} variants, '
                  f'{c["review_previews"]} PNG previews\n')
    if warnings:
        summary+='WARNINGS:\n'+'\n'.join(warnings[:100])+'\n'
    (output/'channel_summary.txt').write_text(summary,encoding='utf-8')
    assets.append('channel_summary.txt')
    out_zip=output/'sprite_all_channels_review_bundle.zip'
    tmp=out_zip.with_suffix('.zip.tmp')
    try:
        with zipfile.ZipFile(tmp,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as dest:
            for name in assets:
                dest.write(output/name,arcname=name)
        tmp.replace(out_zip)
    finally:
        if tmp.exists():tmp.unlink()
    print(summary,end='')
    print('SPRITE_CHANNELS_BUNDLE',out_zip)
    return manifest


def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--clean-root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--max-per-priority',type=int,default=32,
                   help='sampled PNG preview cap PER OBJ priority; all objects stay indexed')
    p.add_argument('--background-shots',type=int,default=3)
    args=p.parse_args()
    try:
        build(args.clean_root,args.output,
              max_previews_per_priority=args.max_per_priority,
              background_shots=args.background_shots)
        return 0
    except (sf.SpriteError,ChannelError,OSError,ValueError,zipfile.BadZipFile) as exc:
        print('SPRITE_CHANNELS_ERROR:',exc,file=sys.stderr)
        return 2
if __name__=='__main__':raise SystemExit(main())
