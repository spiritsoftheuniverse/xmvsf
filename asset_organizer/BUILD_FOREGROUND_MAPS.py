#!/usr/bin/env python3
"""Recover native MVC SCROLL1 foreground tilemaps, all referenced pixels and coordinates.

The captured SCROLL1 layer is missing from older SCROLL2-only previews.
Render each scroll layer separately from the original captured 64x64 map,
original fully decoded MVC CPS2 GFX and captured raw color RAM. Preserve native
cell coordinates and raw scroll values. Never infer ROM-level named stage IDs or
fixed native OBJ-vs-scroll priority. All outputs stay local, Git-ignored.
"""
from __future__ import annotations
import argparse
import base64
from collections import defaultdict,Counter
import csv
import hashlib
import io
import gzip
import json
from pathlib import Path
import struct
import sys
import zipfile

from BUILD_CAPTURED_ANIMATIONS import read_camera, capture_tree, chunk_png

STAGE_DIR=Path(__file__).resolve().parents[1]/'dev'/'stage_capture'
sys.path.insert(0,str(STAGE_DIR))
from BUILD_MVC_ART_REVIEW import Capture as OriginalCapture

DIM={1:8,2:16,3:32}
SHIFT={1:6,2:7,3:9}
COLOR={1:0x20,2:0x40,3:0x60}
STATUS='ORIGINAL_SCROLL_LAYER_DIAGNOSTIC_PRIORITY_NOT_CERTIFIED'

class ForegroundError(ValueError):pass


def map_at(layer:int,x:int,y:int):
    if not(1<=layer<=3 and 0<=x<64 and 0<=y<64):raise ForegroundError('bad CPS2 map cell')
    if layer==1:addr=((y&0x20)<<8)|(x<<7)|((y&0x1f)<<2)
    elif layer==2:addr=((y&0x30)<<8)|(x<<6)|((y&15)<<2)
    else:addr=((y&0x38)<<8)|(x<<5)|((y&7)<<2)
    return addr


def decode_layer_tile(gfx:bytes,mask:int,offset:int,tid:int,attr:int,pal:bytes,layer:int)->bytes:
    dim=DIM[layer]
    start=(offset+(tid<<SHIFT[layer]))&mask
    stride=16 if dim==32 else 8
    if start+(dim-1)*stride+(dim//2)>len(gfx):
        raise ForegroundError('original scroll tile outside captured decoded GFX')
    if len(pal)!=6144:raise ForegroundError('bad source palette RAM')
    pens=[b'\0\0\0\0']
    for i in range(1,16):
        idx=((COLOR[layer]|(attr&31))*16+i)^15
        val=struct.unpack_from('<H',pal,idx*2)[0]
        brightness=15+2*((val>>12)&15)
        pens.append(bytes([min(255,((val>>shift)&15)*17*brightness//45)
                          for shift in (8,4,0)]+[255]))
    out=bytearray(dim*dim*4)
    xflip=bool(attr&32);yflip=bool(attr&64)
    for y in range(dim):
        row=start+(dim-1-y if yflip else y)*stride
        for x in range(dim):
            xx=dim-1-x if xflip else x
            v=gfx[row+(xx//8)*4+3-((xx%8)//2)]
            pen=(v>>4) if xx%2==0 else v&15
            out[(y*dim+x)*4:(y*dim+x+1)*4]=pens[pen]
    return bytes(out)


def paint_layer(page:bytes,layer:int,scrollx:int,scrolly:int,gfx:bytes,
                mask:int,offset:int,pal:bytes):
    if len(page)!=0x4000:raise ForegroundError('bad native map page')
    dim=DIM[layer]
    sx=scrollx%(64*dim);sy=scrolly%(64*dim)
    output=bytearray(384*224*4)
    cache={};visible=[]
    for ty in range(sy//dim,(sy+224+dim-1)//dim):
        for tx in range(sx//dim,(sx+384+dim-1)//dim):
            mx=tx%64;my=ty%64
            tile,attr=struct.unpack_from('<HH',page,map_at(layer,mx,my))
            key=(tile,attr)
            if key not in cache:cache[key]=decode_layer_tile(gfx,mask,offset,tile,attr,pal,layer)
            pixels=cache[key]
            left=tx*dim-sx;top=ty*dim-sy
            filled=0
            for dy in range(dim):
                syy=top+dy
                if not(0<=syy<224):continue
                for dx in range(dim):
                    sxx=left+dx
                    if not(0<=sxx<384):continue
                    sp=(dy*dim+dx)*4
                    if pixels[sp+3]:
                        dst=(syy*384+sxx)*4
                        output[dst:dst+4]=pixels[sp:sp+4]
                        filled+=1
            if filled:
                visible.append({'tileId':tile,'attributes':attr,'mapX':mx,'mapY':my,
                                'screenX':left,'screenY':top,
                                'originalMapByteOffset':map_at(layer,mx,my)})
    return bytes(output),visible


def composite(layers:dict[int,bytes], order=(3,2,1)):
    out=bytearray(384*224*4)
    for layer in order:
        src=layers.get(layer)
        if src is None:continue
        for at in range(0,len(src),4):
            if src[at+3]:out[at:at+4]=src[at:at+4]
    return bytes(out)


def source_shots(review:dict):
    groups=defaultdict(list)
    for item in review['items']:
        if item.get('wanted') is True:
            groups[item['sourceShot']].append(int(item['sourceFrame']))
    return {shot:sorted(frames) for shot,frames in groups.items()}


def captured_maps(root:Path,camera:dict,frame_list:list[int]):
    archive,rd,stream,sessions=capture_tree(root)
    try:
        result={}
        per_session=defaultdict(list)
        for f in frame_list:
            if f in camera:per_session[camera[f]['session']].append(f)
        for session,ts in per_session.items():
            targets=sorted(set(ts));i=0;recent={}
            with stream(session+'/maps.bin') as f:
                while i<len(targets):
                    header=f.read(32)
                    if not header:break
                    if len(header)!=32 or header[:4]!=b'MAP1':raise ForegroundError('invalid MAP1 packet')
                    at,stage,sl,layer,bank,_hash=struct.unpack('<IIIIIQ',header[4:])
                    raw=f.read(0x4000)
                    if len(raw)!=0x4000:raise ForegroundError('truncated captured map')
                    while i<len(targets) and targets[i]<at:
                        result[targets[i]]=dict(recent);i+=1
                    if i>=len(targets):break
                    # Source only has one raster slice in main stage shots.
                    # Map slice 0 is the top-of-frame region, explicit in metadata.
                    if sl==0 and layer in DIM:recent[layer]=(raw,at,bank)
                while i<len(targets):result[targets[i]]=dict(recent);i+=1
        return result
    finally:
        if archive:archive.close()



def scan_original_foreground_history(capture_path:Path,chain:list[str],
                                     scene_frames:dict[str,list[int]],
                                     full_review:dict):
    """Recover every recorded SCROLL1 map tile variant in a shot's AO time span.

    Source time windows are the min/max original AO observations for that shot,
    NOT a ROM-verified named-stage lifetime. No uncaptured state is fabricated.
    """
    scopes={}
    for shot in scene_frames:
        frames=[int(i['sourceFrame']) for i in full_review['items'] if i['sourceShot']==shot]
        if frames:scopes[shot]=(min(frames),max(frames))
    alltiles=set();histories=[];seen=Counter();event_totals=Counter()
    archive,rd,stream,_=capture_tree(capture_path)
    try:
        for session in chain:
            with stream(session+'/maps.bin') as fp:
                while True:
                    head=fp.read(32)
                    if not head:break
                    if len(head)!=32 or head[:4]!=b'MAP1':raise ForegroundError('invalid map source packet')
                    frame,stage,sl,layer,bank,_hash=struct.unpack('<IIIIIQ',head[4:])
                    body=fp.read(0x4000)
                    if len(body)!=0x4000:raise ForegroundError('truncated map history packet')
                    if sl!=0 or layer!=1:continue
                    for shot,(first,last) in scopes.items():
                        if first<=frame<=last:
                            histories.append((shot,frame,session,body,bank))
                            seen[shot]+=1
        # A sampled native SCROLL1 map is the complete 64x64 page at that time;
        # examine ALL 4096 cells, not just the 384x224 viewport.
        per_shot=defaultdict(list)
        for shot,frame,session,body,bank in histories:per_shot[shot].append((frame,session,body,bank))
        shots=[]
        for shot in sorted(scopes):
            entries=sorted(per_shot.get(shot,()),key=lambda x:(x[0],x[1]))
            previous=None; first_per_cell={};deltas=defaultdict(list)
            unique_ids=set();count=0
            for frame,session,body,bank in entries:
                for n in range(4096):
                    off=n*4;pair=struct.unpack_from('<HH',body,off)
                    unique_ids.add(pair[0]);alltiles.add(pair[0])
                    if previous is not None and body[off:off+4]!=previous[off:off+4]:
                        if n not in deltas:
                            old=struct.unpack_from('<HH',previous,off)
                            deltas[n].append({'observedFrame':None,'tileId':old[0],'attributes':old[1],
                                              'sourceStatus':'PREVIOUS_NATIVE_MAP_STATE'})
                        deltas[n].append({'observedFrame':frame,'tileId':pair[0],
                                          'attributes':pair[1],'session':session})
                        count+=1
                previous=body
            shots.append({'shot':shot,'sourceFrameRange':list(scopes[shot]),
                          'nativeMapSnapshotsScanned':len(entries),
                          'allOriginalPageTileIds':sorted(unique_ids),
                          'sourceMapCellChanges':count,
                          'changedCells':[{'mapByteOffset':n*4,'cellId':n,'variants':v}
                                          for n,v in sorted(deltas.items())]})
            event_totals[shot]=count
        return {'sourceFrameRangesAreOnlyObservedAOFrames':True,
                'totalNativeScroll1MapSnapshotsScanned':len(histories),
                'distinctOriginalScroll1TileIdsInEntireRecordedPages':len(alltiles),
                'totalMapCellChangeEvents':sum(event_totals.values()),
                'totalChangedMapCells':sum(len(s['changedCells']) for s in shots),
                'shots':shots},alltiles
    finally:
        if archive:archive.close()


def build(review_path:Path,capture_path:Path,output:Path):
    review=json.loads(review_path.read_text(encoding='utf-8'))
    if review.get('format')!='MVC1_ASSET_ORGANIZER_V2' or len(review.get('items',[]))!=review.get('sourceCatalogSize'):
        raise ForegroundError('invalid full AO review')
    shots=source_shots(review)
    if not shots:raise ForegroundError('no selected source scenes')
    samples={shot:frames[0] for shot,frames in shots.items()}
    camera,chain=read_camera(capture_path,set(samples.values()))
    historical,history_tiles=scan_original_foreground_history(capture_path,chain,shots,review)
    maps=captured_maps(capture_path,camera,list(samples.values()))
    gfx=OriginalCapture(capture_path)
    archive,rd,stream,_=capture_tree(capture_path)
    try:
        scenes=[];html=[];native_tile_ids=set()
        for shot,frame in sorted(samples.items()):
            if frame not in camera:continue
            cam=camera[frame];pages=maps.get(frame,{})
            state,palette=gfx.palette(frame)
            source_index=rd(cam['session']+'/gfx_index.tsv').decode('utf-8').strip().split('\t')
            if len(source_index)<9 or len(gfx.gfx)!=int(source_index[2]):
                raise ForegroundError('original decoded GFX source mismatch')
            offsets={k:int(source_index[3+k]) for k in DIM}
            mask=int(source_index[3],16)
            layers={};provenance={};visibility={}
            for layer in DIM:
                if layer not in pages:continue
                page,mapframe,bank=pages[layer]
                source,entries=paint_layer(page,layer,cam[f'scroll{layer}x'],cam[f'scroll{layer}y'],gfx.gfx,mask,offsets[layer],palette)
                layers[layer]=source
                provenance[str(layer)]={'mapSourceFrame':mapframe,'objectBank':bank,
                                         'scrollX':cam[f'scroll{layer}x'],
                                         'scrollY':cam[f'scroll{layer}y'],
                                         'graphicsOffset':offsets[layer],
                                         'visibleCellCount':len(entries),
                                         'visibleCells':entries}
                visibility[str(layer)]=sum(1 for a in source[3::4] if a)
                if layer==1:
                    native_tile_ids.update(x['tileId'] for x in entries)
            if 1 not in layers:continue
            joined=composite(layers)
            without1=composite(layers,(3,2))
            def datauri(src):return 'data:image/png;base64,'+base64.b64encode(chunk_png(384,224,src)).decode('ascii')
            scene={'shot':shot,'sourceFrame':frame,'session':cam['session'],
                   'stageCaptureId':cam['stage'],'stageIdentityFromRom':'UNAVAILABLE_0xFFFFFFFF',
                   'camera':cam,'layers':provenance,'opaquePixels':visibility,
                   'sourcePixelStatus':STATUS,
                   'visualNote':'Monster Rock Bar visual match; not a named ROM stage-ID match' if shot in ('shot_0002','shot_0012','shot_0013','shot_0014') else 'Original captured scene'}
            scenes.append(scene)
            html.append({'shot':shot,'frame':frame,'cam':cam,'before':datauri(without1),
                         'after':datauri(joined),'foreground':datauri(layers[1])})
        output.mkdir(parents=True,exist_ok=True)
        # Extract ALL original page-referenced source tiles across the recorded
        # shot window, including tiles that were offscreen in the preview.
        # Do not infer uncaptured, ROM-unreferenced animation frame IDs.
        all_original_tiles=native_tile_ids|history_tiles
        packet=bytearray()
        for tile in sorted(all_original_tiles):
            off=(offsets[1]+(tile<<6))&mask
            if off+64>len(gfx.gfx):raise ForegroundError('scroll1 tile outside decoded GFX')
            packet.extend(struct.pack('<I',tile));packet.extend(gfx.gfx[off:off+64])
        (output/'scroll1_tiles.bin').write_bytes(packet)
        report={'format':'MVC1_ORIGINAL_FOREGROUND_MAPS_V1',
                'sourceRomSha256':review['sourceRomSha256'],
                'reviewWanted':sum(x['wanted'] is True for x in review['items']),
                'sourceSessionChain':chain,'sourceScenes':len(scenes),
                'originalScroll1TileIdsRecovered':len(all_original_tiles),
                'originalViewportScroll1TileIds':len(native_tile_ids),
                'capturedMapHistory':{k:v for k,v in historical.items() if k!='shots'},
                'capturedHistoryFile':'foreground_tile_histories.json.gz',
                'scroll1TileArchiveFormat':'<LE u32 map tile ID><64 original decoded GFX bytes>',
                'scroll1IsNativeForeground':True,
                'nativePriorityToOBJVerified':False,
                'allFramesFromROMProven':False,
                'captureObservationScope':'One source frame per Wanted shot; tile bytes complete for visible source map cells, not all game levels or animation states',
                'scenes':scenes}
        (output/'foreground_maps.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        with gzip.open(output/'foreground_tile_histories.json.gz','wt',encoding='utf-8',compresslevel=7) as fp:
            json.dump(historical,fp,separators=(',',':'))
        import html as esc
        sections=[]
        for scene in html:
            ss=esc.escape(scene['shot']);ff=scene['frame']
            sections.append(f'<section><h2>{ss} · frame {ff}</h2><div class="three"><div><b>SCROLL3 + SCROLL2</b><img src="{scene["before"]}"></div><div><b>Restored near SCROLL1</b><img src="{scene["after"]}"></div><div><b>SCROLL1 isolated</b><img src="{scene["foreground"]}" class="fg"></div></div></section>')
        page='''<!DOCTYPE html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>MVC1 original foreground tiles</title><style>body{background:#101722;color:#eaf1ff;font:13px system-ui;margin:0}.shell{max-width:1550px;margin:auto;padding:24px}h1{font-size:22px}p{line-height:1.55;color:#b7c5d9}h2{font-size:15px;background:#1d2738;padding:8px;border:1px solid #374557}.three{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}.three>div{background:#1d2738;border:1px solid #374557;padding:8px}.three img{display:block;width:100%;image-rendering:pixelated;margin-top:5px}.fg{background:repeating-conic-gradient(#405069 0% 25%,#1d2738 0% 50%) 50%/20px 20px}section{margin:15px 0 25px}@media(max-width:900px){.three{grid-template-columns:1fr}}</style><div class="shell"><h1>MVC1 native layer restoration: foreground SCROLL1</h1><p>Left: original SCROLL3 and SCROLL2 without the missing near details. Center: original SCROLL1 restored over diagnostic background. Right: isolated transparent SCROLL1 showing tables, chairs and other props. The source ROM stage identity and native OBJ priority are unverified. Every source tile, coordinate, scroll and map timestamp is in foreground_maps.json; original decoded tile bytes in scroll1_tiles.bin and full observed tilemap-change history in foreground_tile_histories.json.gz.</p>'''+''.join(sections)+'</div></html>'
        (output/'foreground.html').write_text(page,encoding='utf-8')
        return report
    finally:
        gfx.close()
        if archive:archive.close()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--review',type=Path,required=True)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();r=build(a.review,a.capture,a.output)
    print('MVC FOREGROUND:',r['sourceScenes'],'scenes,',r['originalScroll1TileIdsRecovered'],'visible original SCROLL1 tile IDs')
    print('OUTPUT:',a.output)
if __name__=='__main__':main()