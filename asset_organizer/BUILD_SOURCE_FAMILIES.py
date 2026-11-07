#!/usr/bin/env python3
"""Recover all native source records and referenced GFX tiles around Wanted MVC assets.

A Wanted record SEEDS its entire verified contiguous five-word ROM-data region.
This recovers uncaptured records and every packed native sprite tile they cite,
without marking those records Wanted or interpreting data adjacency as executed
animation timing.  Explicit Unwanted decisions ALWAYS remain rejected.

Outputs are PRIVATE / LOCAL: source descriptions and an original decoded-GFX
packed tile archive, plus an optional tile-contact HTML. Never commit outputs.
No external Python libraries or game/ROM mutation required.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys

STAGE_DIR = Path(__file__).resolve().parents[1] / 'dev' / 'stage_capture'
sys.path.insert(0, str(STAGE_DIR))
from TRACE_MVC_RECORD_LENGTHS import scan, latest_original

FMT='MVC1_RECOVERED_SOURCE_FAMILIES_V1'
TAGS={'character','effect','background','level_sprite'}

class RecoveryError(ValueError): pass


def review_info(path:Path,program:bytes):
    review=json.loads(path.read_text(encoding='utf-8'))
    if review.get('format')!='MVC1_ASSET_ORGANIZER_V2' or review.get('sourceRomSha256')!=hashlib.sha256(program).hexdigest():
        raise RecoveryError('review ROM checksum or format mismatch')
    items=review.get('items')
    if not isinstance(items,list) or len(items)!=review.get('sourceCatalogSize') or not items:
        raise RecoveryError('incomplete review items')
    byaddr={}
    for item in items:
        if not isinstance(item,dict) or not isinstance(item.get('romOffsetHex'),str):
            raise RecoveryError('bad review item')
        addr=int(item['romOffsetHex'],16)
        if item.get('id')!=f'rom_{addr:06X}' or addr in byaddr:
            raise RecoveryError('duplicate or mismatched review item ID')
        if item.get('wanted') is not None and type(item['wanted']) is not bool:
            raise RecoveryError('bad review decision')
        tags=item.get('tags')
        if not isinstance(tags,list) or len(tags)!=len(set(tags)) or any(t not in TAGS for t in tags):
            raise RecoveryError('bad review tags')
        byaddr[addr]=item
    return review,byaddr


def expanded_ids(tile:int,attr:int,hi:int):
    """Exact CPS2 OBJ 16x16 addressing from OAM width/height and bank."""
    w=((attr>>8)&15)+1;h=((attr>>12)&15)+1
    base=(hi<<16)|tile
    return [((base&~15)+(dy<<4)+((base+dx)&15)) for dy in range(h) for dx in range(w)]


def read_pair_banks(observations:Path, wanted_pairs:set[tuple[int,int]]):
    import gzip
    banks=defaultdict(Counter)
    opener=gzip.open if observations.suffix=='.gz' else open
    with opener(observations,'rt',encoding='utf-8',newline='') as fp:
        rows=csv.DictReader(fp,delimiter='\t')
        if not {'tile','attributes'}.issubset(rows.fieldnames or []):
            raise RecoveryError('missing captured OAM source fields')
        for row in rows:
            tile=int(row['tile']);attr=int(row['attributes'])
            pair=tile&65535,attr
            if pair in wanted_pairs:
                banks[pair][tile>>16]+=1
    return banks


def tile_contact(record,frame:int,capture, bank:int):
    """Preview first native 16x16 tile of EACH descriptor, not false sprite placement."""
    from MVC_RGBA import RGBAImage
    pairs=record['descriptorPairs']
    cols=min(8,len(pairs));rows=(len(pairs)+7)//8
    img=RGBAImage(cols*20, rows*20)
    for i,(tile,attr) in enumerate(pairs):
        state,pens=capture.pens(frame,attr&31)
        full=(bank<<16)|tile
        src=capture.tile(full,(state,attr&31),pens,bool(attr&32),bool(attr&64))
        img.alpha_composite(src,((i%8)*20,(i//8)*20))
    import base64
    return 'data:image/png;base64,'+base64.b64encode(img.png_bytes()).decode('ascii')


def build(review_path:Path, trace_root:Path, observations:Path, capture_path:Path | None,
          output_dir:Path, *, previews:bool=True):
    out=output_dir.resolve()
    for source in (review_path,trace_root,observations,capture_path):
        if source is None:continue
        s=source.resolve()
        if out==s or (s.is_dir() and s in out.parents) or (out.is_dir() and out in s.parents):
            raise RecoveryError('recovery outputs overlap original inputs')
    source=latest_original(trace_root)
    program=source.read_bytes();checksum=hashlib.sha256(program).hexdigest()
    review,byaddr=review_info(review_path,program)
    records,links,regions=scan(program)
    wanted={a for a,r in byaddr.items() if r['wanted'] is True}
    selected_regions=[g for g in regions if wanted.intersection(g)]
    included=[a for g in selected_regions for a in g]
    all_pairs={struct.unpack_from('<HH',program,a+10+4*i)
               for a in included for i in range(records[a][1])}
    bank_evidence=read_pair_banks(observations,all_pairs)
    # Modal original bank for each region: missing captures must never be
    # represented as independently proved tile-bank usage.
    out.mkdir(parents=True,exist_ok=True)
    allids=set();groupdata=[];flat=[]
    image_capture=None
    if capture_path is not None:
        from BUILD_MVC_ART_REVIEW import Capture
        image_capture=Capture(capture_path)
    try:
        for group in selected_regions:
            root=group[0];seeds=[a for a in group if a in wanted]
            seen=Counter()
            for a in group:
                for i in range(records[a][1]):
                    pair=struct.unpack_from('<HH',program,a+10+4*i)
                    seen.update(bank_evidence.get(pair,{}))
            bank,support=seen.most_common(1)[0] if seen else (None,0)
            # Do not make up graphics-bank assignments for an unobserved family.
            if bank is None or (len(seen)>1 and support*20<sum(seen.values())*19):
                bank=None
            seedinfo=[byaddr[a] for a in seeds]
            first_seed_frame=int(seedinfo[0]['sourceFrame'])
            groupdata.append({'regionStart':f'{root:06X}',
                              'sourceRecordCount':len(group), 'explicitWanted':len(seeds),
                              'inferredBankHigh':bank, 'bankEvidence':dict(seen),
                              'referenceSourceFrame':first_seed_frame,
                              'sourceRecordOffsets':[f'{a:06X}' for a in group]})
            for index,a in enumerate(group):
                item=byaddr.get(a)
                status=('WANTED' if item and item['wanted'] is True else
                        'UNWANTED' if item and item['wanted'] is False else
                        'UNREVIEWED' if item else 'SOURCE_DISCOVERED')
                n=records[a][1]
                pairs=[list(struct.unpack_from('<HH',program,a+10+4*i)) for i in range(n)]
                ids=sorted(set(t for tword,attr in pairs for t in expanded_ids(tword,attr,bank))) if bank is not None else []
                allids.update(ids)
                data={'id':f'rom_{a:06X}','romOffsetHex':f'{a:06X}',
                      'regionStartHex':f'{root:06X}','indexInRegion':index,
                      'selectorHex':f'{records[a][0][3]:04X}',
                      'status':status,'explicitDecision':item['wanted'] if item else None,
                      'tags':(item['tags'] if item else []),
                      'semanticType':('UI' if item and not item['tags'] else 'USER_TAGS' if item else 'UNASSIGNED_SOURCE'),
                      'colorGroupOverride':item.get('colorGroupOverride') if item else None,
                      'sourceShot':item.get('sourceShot') if item else None,
                      'sourceFrame':item.get('sourceFrame') if item else None,
                      'descriptorCount':len(pairs),'descriptorPairs':pairs,
                      'tileBankHigh':bank,'tileBankEvidence':'CAPTURED_PAIR_MATCH' if bank is not None and any(tuple(p) in bank_evidence for p in pairs) else 'INFERRED_FROM_REGION' if bank is not None else 'UNRESOLVED',
                      'allExpandedTileIds':ids,
                      'nextOriginalRecordHex':f'{links[a]:06X}' if a in links else None}
                # UI contact is a descriptor-tile index only, not an animation or
                # native tile-placement claim. Recovered data always includes all tiles.
                if previews and image_capture is not None and bank is not None:
                    try:
                        frame=int(item['sourceFrame']) if item is not None else first_seed_frame
                        data['contactPng']=tile_contact(data,frame,image_capture,bank)
                    except (IndexError,ValueError) as e:
                        data['contactError']=str(e)
                flat.append(data)
    finally:
        if image_capture is not None:
            # Keep GFX bytearray available for archive extraction before close.
            gfx=image_capture.gfx
            mask=image_capture.mask
            image_capture.close()
        else:
            gfx=None;mask=None
    # Only exact captured decoded GFX bytes; never fake or extrapolate pixels.
    if gfx is not None:
        tmp=out/'recovered_sprite_tiles.bin.tmp'
        with tmp.open('wb') as f:
            for ident in sorted(allids):
                start=(ident<<7)&mask
                if start+128>len(gfx):raise RecoveryError('native tile ID outside decoded source')
                f.write(struct.pack('<I',ident));f.write(gfx[start:start+128])
        tmp.replace(out/'recovered_sprite_tiles.bin')
    source_records=sum(v['status']=='SOURCE_DISCOVERED' for v in flat)
    summary={'format':FMT,'sourceRomSha256':checksum,'reviewFormat':review['format'],
             'selected':len(wanted),'sourceRegions':len(selected_regions),
             'regionsOriginalRecords':len(flat),
             'sourceDiscovered':source_records,
             'uiSourceAssets':[x['id'] for x in review['items'] if not x['tags']],
             'explicitUnwantedInRegions':sum(x['status']=='UNWANTED' for x in flat),
             'uniqueOriginal16x16TileIds':len(allids),
             'original16x16TilesExtracted':len(allids) if gfx is not None else 0,
             'tileArchiveFormat':'<LE u32 tile id><128 byte packed CPS2 tile>; repeated ascending ID',
             'sourceRegionIsVerifiedAnimation':False,'animationTimingVerified':False,
             'unobservedFramesHaveVerifiedPositions':False,
             'newCandidatesAutomaticallyWanted':False,
             'paletteStatus':'RAW_PALETTE_APPROXIMATION_WHEN_CONTACTS_INCLUDED',
             'uiRule':'All explicitly untagged reviewed entries are user-classified UI, without modifying aodata tags',
             'regionEvidence':groupdata,
             'records':[{k:v for k,v in x.items() if k!='contactPng'} for x in flat]}
    (out/'family_recovery.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
    with (out/'family_recovery.tsv').open('w',encoding='utf-8',newline='') as f:
        cols=('id','romOffsetHex','regionStartHex','indexInRegion','status','semanticType','tileBankEvidence','descriptorCount','tileBankHigh','sourceFrame')
        w=csv.DictWriter(f,fieldnames=cols,extrasaction='ignore',delimiter='\t');w.writeheader();w.writerows(flat)
    if previews:
        import html
        def escape(s):return html.escape(str(s),quote=True)
        colors={'WANTED':'#306c4c','UNWANTED':'#63333e','SOURCE_DISCOVERED':'#314c67','UNREVIEWED':'#4c4f5a'}
        blocks=[]
        for g in groupdata:
            subset=[x for x in flat if x['regionStartHex']==g['regionStart']]
            tiles=[]
            for x in subset:
                label=escape(x['id']);note=escape(x['status']);markup=x.get('contactPng')
                art=f'<img loading="lazy" src="{markup}" alt="Original first tiles of every descriptor">' if markup else '<span>Bank unresolved</span>'
                tiles.append(f'<article style="border-color:{colors[x["status"]]}" title="{label}">{art}<b>{label}</b><small>{note} · {x["descriptorCount"]} parts</small></article>')
            blocks.append(f'<section><h2>Source region {escape(g["regionStart"])} · {len(subset)} candidate records</h2><div class="grid">{"".join(tiles)}</div></section>')
        heading=f'{len(wanted)} Wanted → {len(flat)} original source records; {source_records} uncaptured candidates; {len(allids)} native 16×16 tiles'
        page='''<!DOCTYPE html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>MVC1 Recovered Source Families</title><style>body{margin:0;background:#101722;color:#e4edff;font:14px system-ui}.frame{padding:18px;max-width:1600px;margin:auto}h2{background:#1d293c;padding:12px;border:1px solid #375071}.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(155px,1fr));gap:8px}article{border:2px solid #3a4a5d;background:#1d2836;padding:8px;display:flex;flex-direction:column;gap:5px;min-height:110px}article img{max-height:86px;object-fit:contain;object-position:left center;image-rendering:pixelated}small{color:#c0cadd}h1{font-size:20px}p{line-height:1.6;color:#acbcce}</style><div class="frame"><h1>Recovered source records · '''+escape(heading)+'''</h1><p>Every first tile is original captured CPS2 data. These grids are NOT reconstructed animation frames or correct in-game geometry. Adjacent five-word records are structural source candidates, NOT certified animation sequences. Blue = newly discovered, green = Wanted, red = explicitly rejected. Complete source descriptor and exact referenced tile IDs are saved to family_recovery.json, with packed originals in recovered_sprite_tiles.bin.</p>'''+''.join(blocks)+'</div></html>'
        (out/'family_recovery.html').write_text(page,encoding='utf-8')
    return summary


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--review',type=Path,required=True)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--observations',type=Path,required=True)
    p.add_argument('--capture',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--no-preview',action='store_true')
    a=p.parse_args();r=build(a.review,a.trace_root,a.observations,a.capture,a.output,previews=not a.no_preview)
    print('MVC SOURCE FAMILIES:',r['selected'],'Wanted,',r['sourceDiscovered'],'discovered,',r['explicitUnwantedInRegions'],'rejected,',r['uniqueOriginal16x16TileIds'],'original tiles')
    print('OUTPUT:',a.output)

if __name__=='__main__':main()