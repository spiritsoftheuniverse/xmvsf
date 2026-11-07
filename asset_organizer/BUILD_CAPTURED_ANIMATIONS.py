#!/usr/bin/env python3
"""Build a local-only MVC1 Wanted animation/scene-placement review from original captures.

Every displayed image comes from the original assembled-sprite gallery. Screen
coordinates are matched to native OAM observations; sequence ordering uses
sparse observed source frames, not assumed engine animation timing. The raw
SCROLL2 backdrop is diagnostic, not a native-priority-accurate stage capture.
Never edits gameplay/Stable, ROM bytes, aodata.json or source observations.
"""
from __future__ import annotations
import argparse
import base64
import collections
import csv
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys
import zipfile
import zlib

S = re.compile(r'<script type="application/json" id="original-catalog">(.*?)</script>',re.S)
PNG = b'\x89PNG\r\n\x1a\n'
TAGS=('character','effect','background','level_sprite')
MAX_GAP=210

class CaptureError(ValueError): pass


def capture_tree(root:Path):
    """Read ZIP or directory, with explicit internal path lookup (no extraction)."""
    archive=zipfile.ZipFile(root) if root.is_file() else None
    names=archive.namelist() if archive else [p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()]
    prefix='stage_capture/' if any(n.startswith('stage_capture/session_') for n in names) else ''
    sessions=sorted({m.group(1) for n in names if (m:=re.fullmatch(re.escape(prefix)+r'(session_\d+)/frames\.tsv',n))})
    if not sessions:raise CaptureError('no original frame sessions')
    def rd(name):return archive.read(prefix+name) if archive else (root/name).read_bytes()
    def stream(name):return archive.open(prefix+name) if archive else (root/name).open('rb')
    return archive,rd,stream,sessions


def read_camera(root:Path,targets:set[int]):
    archive,rd,stream,sessions=capture_tree(root)
    try:
        bounds=[]
        for s in sessions:
            text=rd(s+'/frames.tsv').decode('utf-8')
            lines=text.splitlines()
            if len(lines)<2:continue
            head=lines[0].split('\t');col=head.index('frame')
            a=int(lines[1].split('\t')[col]);b=int(lines[-1].split('\t')[col])
            bounds.append((s,a,b))
        if not bounds:raise CaptureError('no camera timestamps')
        active=max(bounds,key=lambda x:(x[2],x[0]));chain=[active]
        while active[1]>1:
            previous=[x for x in bounds if x[2]==active[1]-1 and x not in chain]
            if not previous:break
            active=max(previous,key=lambda x:x[0]);chain.append(active)
        chain=list(reversed(chain))
        cameras={}
        for s,start,end in chain:
            needed={frame for frame in targets if start<=frame<=end}
            if not needed:continue
            with io.StringIO(rd(s+'/frames.tsv').decode('utf-8')) as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    frame=int(row['frame'])
                    if frame in needed:
                        cameras[frame]={k:int(row[k]) for k in ('stage','phase','scroll1x','scroll1y','scroll2x','scroll2y','scroll3x','scroll3y')}
                        cameras[frame]['session']=s
        return cameras,[s for s,_,_ in chain]
    finally:
        if archive:archive.close()


def get_original(original:Path):
    files=[original] if original.is_file() else sorted(original.glob('run_*/mvc_program_data_host_words.bin'))
    if not files and (original/'mvc_program_data_host_words.bin').is_file():files=[original/'mvc_program_data_host_words.bin']
    if not files:raise CaptureError('private program READ source missing')
    data=files[-1].read_bytes();return data,hashlib.sha256(data).hexdigest()


def reviewed_records(program:bytes,review:dict):
    selected={}
    for obj in review['items']:
        if obj['wanted'] is not True:continue
        a=int(obj['romOffsetHex'],16)
        if not (a>=0 and a+10<=len(program) and a%2==0):raise CaptureError('selected ROM offset invalid')
        ctl=struct.unpack_from('<5H',program,a)
        if ctl[0]!=2 or not 0x35<=ctl[3]<=0x39:raise CaptureError(f'bad control record at {a:06X}')
        n=ctl[2]+1
        if not 3<=n<=32 or a+10+4*n>len(program):raise CaptureError('selected original descriptor count invalid')
        parts={struct.unpack_from('<HH',program,a+10+4*i) for i in range(n)}
        selected[obj['id']]={'pairs':parts,'count':n}
    return selected


def lookup_positions(observations:Path,wanted:dict,ids:dict):
    """Recheck best sample's exact source OAM tile+attribute pairs and bounds."""
    seek={(obj['sourceShot'],int(obj['sourceFrame'])) for obj in ids.values()}
    byframe=collections.defaultdict(list)
    opener=gzip.open if observations.name.endswith('.gz') else open
    with opener(observations,'rt',encoding='utf-8',newline='') as fp:
        for row in csv.DictReader(fp,delimiter='\t'):
            key=(row['shot'],int(row['frame']))
            if key not in seek:continue
            byframe[key].append({'pair':(int(row['tile'])&65535,int(row['attributes'])),
                'x':int(row['x']),'y':int(row['y']),'w':int(row['width_tiles'])*16,
                'h':int(row['height_tiles'])*16,'priority':int(row['priority']),
                'index':int(row.get('object_index',row.get('index',0)))})
    results={}
    for id,obj in ids.items():
        source=byframe.get((obj['sourceShot'],int(obj['sourceFrame'])),())
        matching=[o for o in source if o['pair'] in wanted[id]['pairs']]
        distinct={o['pair'] for o in matching}
        if len(distinct)<3:raise CaptureError('selected sprite no longer matches original OAM')
        x=min(o['x'] for o in matching);y=min(o['y'] for o in matching)
        right=max(o['x']+o['w'] for o in matching)
        bottom=max(o['y']+o['h'] for o in matching)
        results[id]={'screenX':x,'screenY':y,'screenWidth':right-x,'screenHeight':bottom-y,
                     'matchedPairs':len(distinct),'expectedPairs':len(wanted[id]['pairs']),
                     'nativePriorities':sorted({o['priority'] for o in matching}),
                     'note':'Uncropped source-object bounding box; PNG may trim transparent margins'}
    return results


def group_takes(review:dict,adjacency:dict,cards:dict,camera:dict,positions:dict,max_gap:int=MAX_GAP):
    if not 0<=max_gap<=900:raise CaptureError('invalid split gap')
    selection={x['id']:x for x in review['items'] if x['wanted'] is True}
    takes=[]
    for family in adjacency['groups']:
        buckets=collections.defaultdict(list)
        for id in family['wantedAssetIds']:
            if id not in selection:continue
            item=selection[id]
            # Multi-tag assets can appear in more than one preview group; ID remains stable.
            tags=[t for t in TAGS if t in item['tags']] or ['untagged']
            for tag in tags:buckets[(item['sourceShot'],tag)].append(item)
        for (shot,tag),items in sorted(buckets.items()):
            ordered=sorted(items,key=lambda x:(int(x['sourceFrame']),int(x['romOffsetHex'],16)))
            sub=[]
            def emit():
                if not sub:return
                seq=len(takes)+1
                frames=[]
                for ob in sub:
                    id=ob['id'];sc=int(ob['sourceFrame']);pos=positions[id]
                    cam=camera.get(sc)
                    frames.append({'id':id,'romOffsetHex':ob['romOffsetHex'],'shot':shot,
                        'sourceFrame':sc,'screen':pos,'camera':cam,
                        'scroll2Candidate':({'x':(pos['screenX']+cam['scroll2x'])%1024,
                                             'y':(pos['screenY']+cam['scroll2y'])%1024} if cam else None),
                        'png':cards[id]['png'],'nativeWidth':cards[id]['nativeWidth'],
                        'nativeHeight':cards[id]['nativeHeight'],
                        'componentsMatched':ob['componentsMatched'],
                        'componentsExpected':ob['componentsExpected']})
                takes.append({'id':f'take_{seq:03d}','family':family['id'],
                    'shot':shot,'tag':tag,'frameCount':len(frames),
                    'firstSourceFrame':frames[0]['sourceFrame'],
                    'lastSourceFrame':frames[-1]['sourceFrame'],
                    'classification':'SAMPLED_CANDIDATE_SEQUENCE' if len(frames)>1 else 'SINGLE_CAPTURED_POSE',
                    'frames':frames})
            for item in ordered:
                if sub and int(item['sourceFrame'])-int(sub[-1]['sourceFrame'])>max_gap:
                    emit();sub=[]
                sub.append(item)
            emit()
    return takes


def safe_json(data):return json.dumps(data,separators=(',',':'),ensure_ascii=False).replace('</','<\\/')


def chunk_png(width:int,height:int,rgba:bytes):
    if len(rgba)!=width*height*4:raise CaptureError('bad captured background pixels')
    def ch(k,b):
        x=k+b;return struct.pack('>I',len(b))+x+struct.pack('>I',zlib.crc32(x)&0xffffffff)
    scan=b''.join(b'\0'+rgba[i*width*4:(i+1)*width*4] for i in range(height))
    return PNG+ch(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+ch(b'IDAT',zlib.compress(scan,5))+ch(b'IEND',b'')


def tile_rgba(gfx:bytes,mask:int,offset:int,tile:int,attrs:int,palette:bytes,layer:int=2)->bytes:
    """Original decoded CPS2 16px scroll2 nibble layout; raw colors approximate."""
    size=16;start=(offset+(tile<<7))&mask
    if start+size*8>len(gfx):return bytes(size*size*4)
    pal_base=(0x40|(attrs&31))*16
    pens=[b'\0\0\0\0']
    for i in range(1,16):
        at=2*((pal_base+i)^15)
        if at+2>len(palette):raise CaptureError('invalid scroll palette offset')
        val=struct.unpack_from('<H',palette,at)[0];brightness=15+2*((val>>12)&15)
        pens.append(bytes([min(255,((val>>shift)&15)*17*brightness//45) for shift in (8,4,0)]+[255]))
    out=bytearray(size*size*4)
    fx=bool(attrs&32);fy=bool(attrs&64)
    for y in range(size):
        ry=start+(15-y if fy else y)*8
        for x in range(size):
            srcx=15-x if fx else x
            v=gfx[ry+(srcx//8)*4+3-(srcx%8)//2]
            pen=(v>>4)&15 if srcx%2==0 else v&15
            off=(y*16+x)*4
            out[off:off+4]=pens[pen]
    return bytes(out)

def captured_scroll2_previews(capture:Path,requested:set[int],camera:dict):
    """Render original SCROLL2 tilemap at selected source frames.

    This is explicitly a DIAGNOSTIC composition: ignores raster splits,
    priority and other scroll layers. No inferred world-coordinate certainty.
    """
    if not requested:return {},'NO_LEVEL_SPRITES'
    arc,rd,stream,sessions=capture_tree(capture)
    try:
        chain=sorted({camera[f]['session'] for f in requested if f in camera})
        if not chain:return {},'CAMERA_MISSING'
        earliest=min(requested);latest=max(requested)
        maps={}
        # Sources may contain early-overlapping sessions. Use only the camera-selected session for a frame.
        for session in chain:
            targets=sorted(f for f in requested if f in camera and camera[f]['session']==session)
            if not targets:continue
            last_page=None;previous=0;index=0
            with stream(session+'/maps.bin') as fp:
                while index<len(targets):
                    head=fp.read(32)
                    if not head:break
                    if len(head)!=32 or head[:4]!=b'MAP1':raise CaptureError('bad original map packet')
                    frame,stage,sl,layer,bank,hashed=struct.unpack('<IIIIIQ',head[4:])
                    if frame<previous:raise CaptureError('non-monotonic map snapshots')
                    previous=frame
                    body=fp.read(0x4000)
                    if len(body)!=0x4000:raise CaptureError('truncated map snapshot')
                    while index<len(targets) and targets[index]<frame:
                        if last_page is not None:maps[targets[index]]=last_page
                        index+=1
                    if index>=len(targets):break
                    if layer==2 and frame<=targets[-1]:last_page=(body,bank,session,frame)
                while index<len(targets):
                    if last_page is not None:maps[targets[index]]=last_page
                    index+=1
        if not maps:return {},'NO_SCROLL2_MAP_SNAPSHOT'
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'dev'/'stage_capture'))
        from BUILD_MVC_ART_REVIEW import Capture as OriginalCapture
        gfx=OriginalCapture(capture)
        rendered={}
        try:
            for frame,(page,bank,session,snap_frame) in maps.items():
                state,palette=gfx.palette(frame)
                cam=camera[frame];scroll_x=cam['scroll2x']%1024;scroll_y=cam['scroll2y']%1024
                # Asset source is decoded CPS2 GFX; index offset is explicitly captured.
                raw_index=rd(session+'/gfx_index.tsv').decode().split('\t')
                if len(raw_index)<9:continue
                offset=int(raw_index[5]);mask=int(raw_index[3],16)
                scene=bytearray(384*224*4)
                cache={}
                for ty in range(scroll_y//16,(scroll_y+224+15)//16+1):
                    for tx in range(scroll_x//16,(scroll_x+384+15)//16+1):
                        ix=tx%64;iy=ty%64
                        addr=(((iy&0x30)<<8)|(ix<<6)|((iy&0x0f)<<2))
                        tile,attr=struct.unpack_from('<HH',page,addr)
                        key=(tile,attr)
                        if key not in cache:cache[key]=tile_rgba(gfx.gfx,mask,offset,tile,attr,palette,2)
                        pixels=cache[key]
                        left=tx*16-scroll_x;top=ty*16-scroll_y
                        for dy in range(16):
                            sy=top+dy
                            if not(0<=sy<224):continue
                            for dx in range(16):
                                sx=left+dx
                                if not(0<=sx<384):continue
                                source=(dy*16+dx)*4;target=(sy*384+sx)*4
                                if pixels[source+3]:scene[target:target+4]=pixels[source:source+4]
                rendered[frame]={'png':'data:image/png;base64,'+base64.b64encode(chunk_png(384,224,bytes(scene))).decode(),
                                  'sourceMapFrame':snap_frame,'cameraScroll2':[cam['scroll2x'],cam['scroll2y']],
                                  'status':'ORIGINAL_SCROLL2_DIAGNOSTIC_NOT_NATIVE_COMPOSITE'}
        finally:gfx.close()
        return rendered,'ORIGINAL_SCROLL2_DIAGNOSTIC'
    finally:
        if arc:arc.close()


def build(review_path:Path,adjacency_path:Path,gallery_path:Path,observations:Path,capture:Path,
          trace_root:Path,template_path:Path,output:Path,with_backdrops=True):
    for source in (review_path,adjacency_path,gallery_path,observations,capture,trace_root,template_path):
        src=source.resolve();out=output.resolve()
        if src==out or (src.is_dir() and src in out.parents):
            raise CaptureError('output would overwrite original input')
    gallery=gallery_path.read_text(encoding='utf-8');match=S.search(gallery)
    if not match:raise CaptureError('original embedded artwork catalog missing')
    catalog=json.loads(match.group(1));cards={x['id']:x for x in catalog['catalog']}
    review=json.loads(review_path.read_text(encoding='utf-8'))
    if review.get('format')!='MVC1_ASSET_ORGANIZER_V2' or len(review.get('items',[]))!=review.get('sourceCatalogSize'):
        raise CaptureError('incomplete aodata.json')
    prog,digest=get_original(trace_root)
    if not(digest==review.get('sourceRomSha256')==catalog['romSha256']):raise CaptureError('original ROM checksum mismatch')
    if set(x['id'] for x in review['items'])!=set(cards):raise CaptureError('review assets differ from original gallery')
    adjacent=json.loads(adjacency_path.read_text(encoding='utf-8'))
    if adjacent.get('sourceRomSha256')!=digest:raise CaptureError('stale Wanted adjacency file')
    wanted={x['id']:x for x in review['items'] if x['wanted'] is True}
    if len(wanted)!=adjacent.get('wanted'):raise CaptureError('adjacency report does not match current review')
    direct=reviewed_records(prog,review)
    pos=lookup_positions(observations,direct,wanted)
    cams,chain=read_camera(capture,{int(x['sourceFrame']) for x in wanted.values()})
    takes=group_takes(review,adjacent,cards,cams,pos)
    backdrop_frames={int(x['sourceFrame']) for x in wanted.values()}
    backgrounds={};background_method='DISABLED'
    if with_backdrops:
        try:backgrounds,background_method=captured_scroll2_previews(capture,backdrop_frames,cams)
        except (CaptureError,ValueError,OSError,IndexError,struct.error) as e:
            background_method='UNAVAILABLE: '+str(e)[:120]
    # Keep artwork local in generated HTML; JSON manifest has no decoded sprites/images.
    for take in takes:
        for frame in take['frames']:
            if frame['sourceFrame'] in backgrounds:
                frame['background']=backgrounds[frame['sourceFrame']]
    public={'format':'MVC1_CAPTURED_POSE_PLACEMENTS_V1','sourceRomSha256':digest,
        'selectionSource':'asset_organizer/aodata.json','reviewed':len(review['items']),
        'wanted':len(wanted),'originalCandidateRegions':len(adjacent['groups']),
        'previewTakes':len(takes),'multiFrameTakes':sum(len(x['frames'])>1 for x in takes),
        'singleFrameTakes':sum(len(x['frames'])==1 for x in takes),
        'originalCameraSessionChain':chain,'cameraFrameMatches':sum(int(x['sourceFrame']) in cams for x in wanted.values()),
        'backgroundMethod':background_method,'backgroundFrames':len(backgrounds),
        'stageIdentity':'UNKNOWN: captured stage fields use sentinel values, do not treat shot IDs as stage IDs',
        'placementMeaning':'Screen X/Y from exact recorded OAM pairs, confirmed for the recorded source shot/frame. SCROLL2 sums are diagnostic candidate tilemap coordinates, not verified world anchors.',
        'animationMeaning':'Frame order follows sparse observed native frame numbers. Playback hold time is a review convenience, NOT original animation duration.',
        'takes':[{k:v for k,v in take.items() if k!='frames'}|{'frames':[
            {k:v for k,v in frame.items() if k not in ('png','background')} for frame in take['frames']]} for take in takes]}
    outdir=output.parent;outdir.mkdir(parents=True,exist_ok=True)
    metadata=output.with_name('captured_animations.json')
    metadata.write_text(json.dumps(public,indent=2)+'\n',encoding='utf-8')
    template=template_path.read_text(encoding='utf-8')
    if '__CAPTURED_ANIMATIONS__' not in template:raise CaptureError('animation HTML template missing payload placeholder')
    payload={'meta':{k:v for k,v in public.items() if k!='takes'},'takes':takes}
    output.write_text(template.replace('__CAPTURED_ANIMATIONS__',safe_json(payload)),encoding='utf-8')
    return public


def main():
    p=argparse.ArgumentParser(description=__doc__)
    root=Path(__file__).resolve().parent
    p.add_argument('--review',type=Path,default=root/'aodata.json')
    p.add_argument('--adjacency',type=Path,default=root/'wanted_sequences.json')
    p.add_argument('--gallery',type=Path,default=root/'index.html')
    p.add_argument('--observations',type=Path,required=True)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--template',type=Path,default=root/'CAPTURED_ANIMATIONS_TEMPLATE.html')
    p.add_argument('--output',type=Path,default=root/'animations.html')
    p.add_argument('--no-backdrops',action='store_true')
    a=p.parse_args()
    report=build(a.review,a.adjacency,a.gallery,a.observations,a.capture,a.trace_root,a.template,a.output,not a.no_backdrops)
    print('MVC animation review: {} Wanted, {} candidate takes ({} multiframe); {} mapped scroll2 backdrops'.format(
        report['wanted'],report['previewTakes'],report['multiFrameTakes'],report['backgroundFrames']))
    print('Generated:',a.output)

if __name__=='__main__':main()