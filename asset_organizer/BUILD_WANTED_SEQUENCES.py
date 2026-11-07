#!/usr/bin/env python3
"""Locate Wanted MVC1 artwork among adjacent original 68000 data records.

Metadata only. This associates saved user choices with source DATA adjacency;
it cannot identify executed animation order, character ownership, or timing.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'dev'/'stage_capture'))
from TRACE_MVC_RECORD_LENGTHS import scan, latest_original

TAGS={'character','effect','background','level_sprite'}


def build(review_path:Path, trace_root:Path, output:Path, max_gap:int=8)->dict:
    if max_gap<0 or max_gap>100: raise ValueError('max-gap out of range 0..100')
    source=latest_original(trace_root)
    if output.resolve()==source.resolve() or output.resolve()==review_path.resolve():
        raise ValueError('output must not overwrite an input')
    program=source.read_bytes()
    digest=hashlib.sha256(program).hexdigest()
    review=json.loads(review_path.read_text(encoding='utf-8'))
    if review.get('format')!='MVC1_ASSET_ORGANIZER_V2' or review.get('sourceRomSha256')!=digest:
        raise ValueError('aodata.json is missing, invalid or belongs to another original ROM')
    items=review.get('items')
    if not isinstance(items,list) or len(items)!=review.get('sourceCatalogSize'):
        raise ValueError('incomplete source review; use full aodata.json')
    seen=set();wanted={}
    for item in items:
        name=item['id'];offset=int(item['romOffsetHex'],16)
        if name!=f'rom_{offset:06X}' or name in seen: raise ValueError('duplicate/mismatched review ID')
        seen.add(name)
        tags=item.get('tags')
        if not isinstance(tags,list) or any(tag not in TAGS for tag in tags):
            raise ValueError('invalid category tags')
        if item.get('wanted') is True:
            wanted[offset]={'id':name,'tags':tags,'shot':item.get('sourceShot'),
                'frame':item.get('sourceFrame'), 'matched':item.get('componentsMatched'),
                'expected':item.get('componentsExpected')}
    records,links,regions=scan(program)
    coordinates={addr:(group[0],index,len(group)) for group in regions for index,addr in enumerate(group)}
    selected=sorted((addr,info) for addr,info in wanted.items() if addr in coordinates)
    unlinked=sorted(addr for addr in wanted if addr not in coordinates)
    groups=[];current=[];previous_region=None;previous_index=None
    for addr,info in sorted(selected,key=lambda x:(coordinates[x[0]][0],coordinates[x[0]][1])):
        root,index,length=coordinates[addr]
        if current and (root!=previous_region or index-previous_index>max_gap+1):
            groups.append(current);current=[]
        current.append((addr,info))
        previous_region=root;previous_index=index
    if current:groups.append(current)
    rows=[];seqs=[]
    for n,group in enumerate(groups,1):
        root=coordinates[group[0][0]][0]
        key=f'candidate_{n:03d}'
        seqs.append({'id':key,'regionStart':f'{root:06X}',
                     'wantedCount':len(group),'start':f'{group[0][0]:06X}',
                     'end':f'{group[-1][0]:06X}',
                     'wantedAssetIds':[info['id'] for _,info in group],
                     'tags':sorted({tag for _,info in group for tag in info['tags']})})
        for i,(addr,info) in enumerate(group):
            _,position,region_size=coordinates[addr]
            rows.append({'candidate':key,'record':f'{addr:06X}', 'regionStart':f'{root:06X}',
                'regionPosition':position,'regionLength':region_size,
                'wanted':True,'tags':info['tags'], 'sourceShot':info['shot'],
                'sourceFrame':info['frame'],'componentsMatched':info['matched'],
                'componentsExpected':info['expected'],
                'nextHeader':f'{links[addr]:06X}' if addr in links else None,
                'gapToNextWanted':coordinates[group[i+1][0]][1]-position-1 if i+1<len(group) else None})
    result={'format':'MVC1_WANTED_RECORD_ADJACENCY_V1','sourceRomSha256':digest,
        'selectionSource':'asset_organizer/aodata.json','reviewed':len(items),
        'wanted':len(wanted),'matchedWanted':len(selected),'unlinkedWantedOffsets':[f'{x:06X}' for x in unlinked],
        'structuralRegionCount':len(regions),'candidateGroupCount':len(seqs),
        'maximumUnselectedRecordsBetweenGroupMembers':max_gap,
        'disclaimer':'Structural record adjacency only; no executed sprite animation sequences or timing are established.',
        'groups':seqs,'records':rows}
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    tsv=output.with_suffix('.tsv')
    with tsv.open('w',newline='',encoding='utf-8') as f:
        columns=('candidate','record','regionStart','regionPosition','regionLength','tags','sourceShot','sourceFrame','componentsMatched','componentsExpected','nextHeader','gapToNextWanted')
        w=csv.DictWriter(f,fieldnames=columns,delimiter='\t',extrasaction='ignore');w.writeheader()
        for row in rows:w.writerow({**row,'tags':','.join(row['tags'])})
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--review',type=Path,default=Path(__file__).parent/'aodata.json')
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--output',type=Path,default=Path(__file__).parent/'wanted_sequences.json')
    p.add_argument('--max-gap',type=int,default=8)
    a=p.parse_args()
    result=build(a.review,a.trace_root,a.output,a.max_gap)
    print(f"MVC WANT: {result['wanted']} chosen, {result['matchedWanted']} located, {result['candidateGroupCount']} structural-neighbor clusters")
    print('Metadata:',a.output)

if __name__=='__main__':main()
