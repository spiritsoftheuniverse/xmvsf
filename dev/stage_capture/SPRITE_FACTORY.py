#!/usr/bin/env python3
"""87.203 offline CPS2 Sprite Factory: address original graphics by native OAM tile ID.

Preferred: Stage44/47 clean_rips/shot_XXXX sprites.tsv + tiles.bin + exact
emulator-effective 32-bit CpsPal palettes.bin. Alternative: Stage87.203
OAM + full decoded MVC gfx + raw palette RAM (colors are NOT certified).
Never modifies ROMs, capture streams, Stable, or gameplay. Identifying stage
scenery vs fighters/HUD is a REVIEW TASK, not inferred from a tile number.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys
import zipfile

from EXPORT_CLEAN_RIP import png_bytes

SHOT_NAME = re.compile(r'shot_\d{4}\Z')
RAW_TILE_SIZE = 128
PIXELS = 16
PALETTE_WORDS = 512
PAL_RECORD = 4 + 512 * 4
TILE_PACKET = 4 + RAW_TILE_SIZE
OAM_PACKET = 16 + 8192
RAW_PAL_PACKET = 20 + 6144
SAMPLE_FIELDS = ('sample', 'object_index', 'x', 'y', 'tile', 'attributes',
                 'priority', 'palette', 'width_tiles', 'height_tiles')
GLYPHS = {
    '0': ('111','101','101','101','111'),
    '1': ('010','110','010','010','111'),
    '2': ('111','001','111','100','111'),
    '3': ('111','001','111','001','111'),
    '4': ('101','101','111','001','001'),
    '5': ('111','100','111','001','111'),
    '6': ('111','100','111','101','111'),
    '7': ('111','001','001','001','001'),
    '8': ('111','101','111','101','111'),
    '9': ('111','101','111','001','111'),
    'A': ('010','101','111','101','101'),
    'B': ('110','101','110','101','110'),
    'C': ('011','100','100','100','011'),
    'D': ('110','101','101','101','110'),
    'E': ('111','100','110','100','111'),
    'F': ('111','100','110','100','100'),
    'P': ('110','101','110','100','100'),
    'T': ('111','010','010','010','010'),
    'N': ('101','111','111','111','101'),
    'X': ('101','101','010','101','101'),
    ':': ('0','1','0','1','0'),
    ' ': ('0','0','0','0','0'),
}

class SpriteError(Exception):
    pass


def effective_pen_color(palette: bytes, number: int, pen: int) -> tuple[int,int,int,int]:
    if pen == 0:
        return (0,0,0,0)
    if not (0 <= number < 32 and 0 <= pen < 16) or len(palette) != 2048:
        raise SpriteError('invalid effective CPS2 sprite palette')
    word = struct.unpack_from('<I', palette, (number*16+pen)*4)[0]
    # CpsPal pixel cache stores BurnHighCol 32-bpp values. Pixel bytes are
    # B,G,R,0 in the native draw buffer; native dual-clear uses [2],[1],[0].
    return ((word>>16)&255, (word>>8)&255, word&255, 255)


def raw_pen_color(palette: bytes, number: int, pen: int) -> tuple[int,int,int,int]:
    if pen == 0:
        return (0,0,0,0)
    if not (0 <= number < 32 and 0 <= pen < 16) or len(palette) != 6144:
        raise SpriteError('invalid raw CPS2 sprite palette')
    # Raw RAM is NOT the effective DMA cache. This preview is provisional.
    val=struct.unpack_from('<H', palette, ((number*16+pen)^15)*2)[0]
    brightness=15+2*((val>>12)&15)
    rgb=[min(255,((val>>shift)&15)*17*brightness//45) for shift in (8,4,0)]
    return (*rgb,255)


def decode_tile(packed: bytes, pen_colors: list[tuple[int,int,int,int]],
                flip_x: bool=False, flip_y: bool=False) -> bytes:
    if len(packed) != 128 or len(pen_colors) != 16:
        raise SpriteError('CPS2 16x16 tile requires exactly 128 bytes and 16 pens')
    out=bytearray(16*16*4)
    for y in range(16):
        sy=15-y if flip_y else y
        for x in range(16):
            sx=15-x if flip_x else x
            pos=sy*8+(sx//8)*4+3-((sx%8)//2)
            word=packed[pos]
            pen=(word>>4) if (sx%2)==0 else (word&15)
            at=(y*16+x)*4
            out[at:at+4]=bytes(pen_colors[pen])
    return bytes(out)


def sprite_tile_id(base: int, dx: int, dy: int) -> int:
    return (base & ~15) + (dy << 4) + ((base+dx)&15)


def compose_sprite(base:int, palette_num:int, width:int, height:int, attr:int,
                   fetch_tile, palette:bytes, effective:bool):
    if not (1<=width<=16 and 1<=height<=16):
        raise SpriteError('sprite dimensions outside hardware 1..16 tiles')
    xflip=bool(attr&0x20)
    yflip=bool(attr&0x40)
    colors=[effective_pen_color(palette,palette_num,p) if effective
            else raw_pen_color(palette,palette_num,p) for p in range(16)]
    pixel_w=width*16
    pixel_h=height*16
    canvas=bytearray(pixel_w*pixel_h*4)
    invalid=[]
    for dy in range(height):
        for dx in range(width):
            ident=sprite_tile_id(base,dx,dy)
            tile=fetch_tile(ident)
            if tile is None:
                invalid.append(ident)
                continue
            rgba=decode_tile(tile,colors,flip_x=xflip,flip_y=yflip)
            dest_x=(width-1-dx if xflip else dx)*16
            dest_y=(height-1-dy if yflip else dy)*16
            for iy in range(16):
                at=((dest_y+iy)*pixel_w+dest_x)*4
                canvas[at:at+64]=rgba[iy*64:(iy+1)*64]
    return pixel_w,pixel_h,bytes(canvas),invalid


def clean_tile_lookup(shot: Path):
    path=shot/'tiles.bin'
    if not path.is_file():
        raise SpriteError(f'{shot.name}: missing Stage47 tile archive')
    if path.stat().st_size % TILE_PACKET:
        raise SpriteError(f'{shot.name}: Stage47 tile archive truncated')
    tiles={}
    with path.open('rb') as stream:
        while True:
            block=stream.read(TILE_PACKET)
            if not block:
                break
            tile_id=struct.unpack_from('<I',block)[0]
            if tile_id>=0x40000:
                raise SpriteError('Stage47 tile ID outside 18-bit CPS2 range')
            if tile_id in tiles and tiles[tile_id]!=block[4:]:
                raise SpriteError('Stage47 repeated tile ID has conflicting bytes')
            tiles[tile_id]=block[4:]
    return tiles


def record(sample:int,frame:int,index:int,x:int,y:int,tile:int,attr:int,priority:int,
           palette:int,w:int,h:int,shot:str):
    if sample<0 or not (0<=index<1024 and 0<=tile<0x40000 and
       0<=palette<=31 and 0<=priority<=7 and 1<=w<=16 and 1<=h<=16):
        raise SpriteError('invalid source CPS2 sprite object fields')
    return {'shot':shot,'sample':sample,'frame':frame,'index':index,
            'x':x,'y':y,'tile':tile,'attributes':attr,'priority':priority,
            'palette':palette,'width_tiles':w,'height_tiles':h,
            'flip':(attr>>5)&3}


def load_shots(root:Path,selected_shot:str|None=None):
    if not root.is_dir():
        return []
    shots=sorted(p for p in root.iterdir() if p.is_dir() and SHOT_NAME.fullmatch(p.name))
    if selected_shot:
        shots=[p for p in shots if p.name==selected_shot]
    return shots


def get_clean_objects(shot:Path):
    frame_by_sample={}
    with (shot/'layers.tsv').open('r',encoding='utf-8',newline='') as frames:
        rows=csv.DictReader(frames,delimiter='\t')
        for row in rows:
            if int(row['layer'])==0:
                frame_by_sample[int(row['sample'])]=int(row['emulated_frame'])
    path=shot/'sprites.tsv'
    if not path.is_file():
        raise SpriteError(f'{shot.name}: missing sprites.tsv')
    observations=[]
    with path.open('r',encoding='utf-8',newline='') as fp:
        reader=csv.DictReader(fp,delimiter='\t')
        if not reader.fieldnames or not set(SAMPLE_FIELDS).issubset(reader.fieldnames):
            raise SpriteError(f'{shot.name}: missing CPS2 sprite TSV fields')
        for line in reader:
            try:
                values=[int(line[k]) for k in SAMPLE_FIELDS]
                sample=values[0]
                if sample not in frame_by_sample:
                    raise SpriteError(f'{shot.name}: sprite sample missing from layers.tsv')
                obs=record(sample,frame_by_sample[sample],*values[1:],shot=shot.name)
            except (ValueError,TypeError,KeyError) as e:
                raise SpriteError(f'{shot.name}: invalid sprite line: {e}') from e
            observations.append(obs)
    return observations


def clean_palette(shot:Path,sample:int):
    path=shot/'palettes.bin'
    if not path.is_file():
        raise SpriteError(f'{shot.name}: missing effective palette stream')
    with path.open('rb') as fp:
        fp.seek(sample*PAL_RECORD)
        data=fp.read(PAL_RECORD)
    if len(data)!=PAL_RECORD or struct.unpack_from('<I',data)[0]!=sample:
        raise SpriteError(f'{shot.name}: missing effective palette for sample {sample}')
    return data[4:]


def telemetry_input(path:Path,step:int):
    # Existing importer's checked capture wrapper handles ZIPs and exact
    # consecutive-batch chains. NEVER combine older overlapping ROM sessions.
    from BUILD_STAGE_FROM_CAPTURE import open_reconstruction_capture
    cap=open_reconstruction_capture(path,'latest')
    try:
        with cap.open('gfx_index.tsv') as f:
            fields=f.read().decode().strip().split('\t')
        if len(fields)!=9 or not fields[1].startswith('mvsc'):
            raise SpriteError('unexpected MVC graphics provenance')
        size=int(fields[2])
        mask=int(fields[3],16)
        with cap.open(fields[8]) as fp:
            gfx=fp.read()
        if len(gfx)!=size or size>64*1024*1024:
            raise SpriteError('bad decoded graphics size')
        with cap.open('palettes.bin') as fp:
            states=[]
            while True:
                head=fp.read(20)
                if not head:break
                if len(head)!=20 or head[:4]!=b'PAL1':
                    raise SpriteError('bad raw palette packet')
                frame=struct.unpack_from('<I',head,4)[0]
                data=fp.read(6144)
                if len(data)!=6144: raise SpriteError('truncated raw palette')
                states.append((frame,data))
        observations=[]
        selected_palettes={}
        index=0
        last=None
        frame_samples=0
        # OAM is 1024 original 8-byte slots, including unused records.
        with cap.open('oam.bin') as fp:
            while True:
                chunk=fp.read(OAM_PACKET)
                if not chunk:break
                if len(chunk)!=OAM_PACKET or chunk[:4]!=b'OAM1':
                    raise SpriteError('bad OAM frame packet')
                frame,stage,bank=struct.unpack_from('<III',chunk,4)
                if last is not None and frame<=last:
                    raise SpriteError('non-monotonic original OAM frame')
                last=frame
                if (frame-1)%step:continue
                while index<len(states) and states[index][0]<=frame:
                    selected_palettes[frame]=states[index][1]
                    index+=1
                if frame not in selected_palettes:
                    if index>0:selected_palettes[frame]=states[index-1][1]
                    else:continue
                raw=chunk[16:]
                count=0
                for i in range(1024):
                    xw,yw,tw,attr=struct.unpack_from('>HHHH',raw,i*8)
                    if yw&0x8000 or attr>=0xff00:break
                    if (xw|attr)==0:continue
                    tile=tw|((yw&0x6000)<<3)
                    w=((attr>>8)&15)+1
                    h=((attr>>12)&15)+1
                    priority=xw>>13
                    x=((xw&0x3ff)^0x200)-0x200
                    y=((yw&0x3ff)^0x200)-0x200
                    observations.append(record(frame_samples,frame,i,x,y,tile,attr,
                                               priority,attr&31,w,h,
                                               'telemetry'))
                    count+=1
                frame_samples+=1
        del states
        def tile_lookup(ident):
            offset=(ident<<7)&mask
            if offset+128>len(gfx):return None
            return gfx[offset:offset+128]
        return (observations,selected_palettes,tile_lookup,
                {'mode':'telemetry_raw_ram','source_batches':list(getattr(cap,'sessions',[cap.session])),
                 'effective_colors':False,'frame_samples':frame_samples,'source_file':str(path)})
    finally:
        cap.close()


def frequency_catalog(observations):
    variants={}
    for o in observations:
        key=(o['shot'],o['tile'],o['palette'],o['width_tiles'],
             o['height_tiles'],o['flip'])
        item=variants.get(key)
        if item is None:
            item={'shot':o['shot'],'tile':o['tile'],'palette':o['palette'],
                  'width_tiles':o['width_tiles'],'height_tiles':o['height_tiles'],
                  'flip':o['flip'],'attributes':o['attributes'],
                  'first_frame':o['frame'],'last_frame':o['frame'],
                  'observations':0,'sample_count':0,
                  'priority_counts':Counter(),'screen_positions':set(),
                  'frames':set(),'first_sample':o['sample']}
            variants[key]=item
        item['observations']+=1
        item['frames'].add(o['frame'])
        item['priority_counts'][o['priority']]+=1
        item['screen_positions'].add((o['x']//16,o['y']//16))
        item['first_frame']=min(o['frame'],item['first_frame'])
        item['last_frame']=max(o['frame'],item['last_frame'])
    for item in variants.values():
        item['sample_count']=len(item['frames'])
        item['positions_count']=len(item['screen_positions'])
        item['priority_mode']=item['priority_counts'].most_common(1)[0][0]
        item['priority_mask']=','.join(str(n) for n in sorted(item['priority_counts']))
        del item['screen_positions']
        del item['priority_counts']
        del item['frames']
    return list(variants.values())


def ranking(c):
    # Balanced look-up gallery, not a claim of scene/fighter classification.
    area=c['width_tiles']*c['height_tiles']
    return (c['sample_count'] * (1 + math.log2(1+area)),c['observations'],area)


def gallery_candidates(catalog,max_images,per_shot,tile_id,palette_id):
    selected=[c for c in catalog if (tile_id is None or c['tile']==tile_id)
              and (palette_id is None or c['palette']==palette_id)]
    groups=defaultdict(list)
    for item in selected:groups[item['shot']].append(item)
    for values in groups.values():values.sort(key=ranking,reverse=True)
    if tile_id is not None:
        # Exact tile selection should not be drowned out by other tiles.
        return sorted(selected,key=ranking,reverse=True)
    out=[]
    for i in range(per_shot):
        for shot in sorted(groups):
            if len(groups[shot])>i:
                out.append(groups[shot][i])
    # Return backup candidates. Empty/undecoded hardware objects are not
    # allowed to consume the caller's requested number of real PNGs.
    used={id(x) for x in out}
    for item in sorted(selected,key=ranking,reverse=True):
        if id(item) not in used:
            out.append(item)
            used.add(id(item))
    return out


def text_raster(image: bytearray,width:int,height:int,text:str,x:int,y:int):
    for ch in text.upper():
        glyph=GLYPHS.get(ch,GLYPHS[' '])
        for iy,row in enumerate(glyph):
            for ix,mark in enumerate(row):
                px=x+ix; py=y+iy
                if mark=='1' and 0<=px<width and 0<=py<height:
                    off=(py*width+px)*4
                    image[off:off+4]=b'\xff\xff\xff\xff'
        x+=max(len(row) for row in glyph)+1


def make_sheet(items:list[dict],size:int=4):
    cols=4
    rows=math.ceil(len(items)/cols)
    cellw,cellh=184,178
    W,H=cols*cellw,rows*cellh
    if rows>4:raise SpriteError('sheet pagination expected <=16 items')
    image=bytearray(W*H*4)
    for idx,item in enumerate(items):
        cx=(idx%cols)*cellw;cy=(idx//cols)*cellh
        w,h=item['pixel_width'],item['pixel_height']
        rgba=item['pixels']
        # Source artwork is scaled ONLY for the review contact sheet.
        scale=min(128/max(w,1),128/max(h,1),1)
        sw,sh=max(1,int(w*scale)),max(1,int(h*scale))
        ox=cx+(cellw-sw)//2
        oy=cy+8+(128-sh)//2
        for y in range(128):
            for x in range(160):
                at=((cy+8+y)*W+cx+12+x)*4
                checker=38 if ((x//8+y//8)%2) else 49
                image[at:at+4]=bytes((checker,checker,checker,255))
        for y in range(sh):
            sy=min(h-1,int(y/scale))
            for x in range(sw):
                sx=min(w-1,int(x/scale))
                src=(sy*w+sx)*4
                r,g,b,a=rgba[src:src+4]
                if a==0:continue
                at=((oy+y)*W+ox+x)*4
                image[at:at+4]=bytes((r,g,b,255))
        label=f"T{item['tile']:05X} P{item['palette']:02X} N{item['sample_count']:03d}"
        text_raster(image,W,H,label,cx+10,cy+148)
        text_raster(image,W,H,item['shot'][-4:].upper(),cx+10,cy+159)
    return png_bytes(W,H,bytes(image))


def create_factory(clean_root:Path|None,capture:Path|None,output:Path, *,
                   source:str='auto', tile:int|None=None,palette:int|None=None,
                   shot:str|None=None,max_images:int=96,per_shot:int=8,
                   step:int=120):
    if not(1<=max_images<=400 and 1<=per_shot<=32 and 1<=step<=10000):
        raise SpriteError('bad image/sample limits')
    output=output.resolve()
    if clean_root is not None:
        clean_root=clean_root.resolve()
    if capture is not None:
        capture=capture.resolve()
    if clean_root is not None and (clean_root==output or clean_root in output.parents or output in clean_root.parents):
        raise SpriteError('sprite output must not nest in or overwrite clean_rips')
    if capture is not None and capture.is_dir() and (capture==output or capture in output.parents or output in capture.parents):
        raise SpriteError('sprite output must not overwrite native telemetry')
    shots=load_shots(clean_root,shot) if clean_root is not None else []
    if source not in ('auto','clean','telemetry'):
        raise SpriteError('source must be auto, clean, or telemetry')
    use_clean=(source=='clean' or (source=='auto' and bool(shots)))
    if shot and not shots:
        raise SpriteError('selected shot_XXXX directory not found')
    if source=='clean' and not shots:
        raise SpriteError('no clean shot_XXXX directories found')
    if source=='telemetry' and capture is None:
        raise SpriteError('telemetry source not supplied')
    if not use_clean and capture is None:
        raise SpriteError('no clean shots or native telemetry capture; cannot invent sprite pixels')
    all_observations=[]
    errors=[]
    data_by_shot={}
    if use_clean:
        for directory in shots:
            try:
                observations=get_clean_objects(directory)
                if not observations:continue
                # Keep sampled data for ALL image identifications, not simply
                # the last 12 shot entries.
                tiles=clean_tile_lookup(directory)
                palfile=directory/'palettes.bin'
                if not palfile.is_file() or palfile.stat().st_size%PAL_RECORD:
                    raise SpriteError('missing or truncated effective palette samples')
                data_by_shot[directory.name]=(tiles,directory)
                all_observations.extend(observations)
            except (SpriteError,OSError,UnicodeDecodeError) as exc:
                errors.append({'shot':directory.name,'error':str(exc)})
        provenance={'mode':'effective_native_clean_rip','effective_colors':True,
                    'shot_dirs':list(data_by_shot),'source_root':str(clean_root)}
        palettes={}
    else:
        all_observations,palettes,gfx_lookup,provenance=telemetry_input(capture,step)
        errors=[]
    catalog=frequency_catalog(all_observations)
    catalog.sort(key=lambda c:(c['shot'],c['tile'],c['palette'],c['width_tiles'],c['height_tiles'],c['flip']))
    selected=gallery_candidates(catalog,max_images,per_shot,tile,palette)
    output.mkdir(parents=True,exist_ok=True)
    image_dir=output/'sprites'
    image_dir.mkdir(exist_ok=True)
    render=[]
    missing_tiles=Counter()
    attempts=0
    for item in selected:
        if len(render)>=max_images or attempts >= max(512,max_images*24):
            break
        attempts+=1
        if use_clean:
            tiles,shotdir=data_by_shot[item['shot']]
            lookup=lambda ident, tiles=tiles:tiles.get(ident)
            rawpal=clean_palette(shotdir,item['first_sample'])
        else:
            lookup=gfx_lookup
            rawpal=palettes.get(item['first_frame'])
            if rawpal is None:
                errors.append({'shot':'telemetry','error':'palette snapshot absent at sampled frame'})
                continue
        w,h,pixels,missing=compose_sprite(item['tile'],item['palette'],
                        item['width_tiles'],item['height_tiles'],item['attributes'],
                        lookup,rawpal,use_clean)
        if missing:
            missing_tiles[item['shot']]+=len(missing)
            # Never represent an incomplete decoded sprite as the original.
            continue
        if not any(pixels[3::4]):continue
        filename=(f"{item['shot']}_T{item['tile']:05x}_P{item['palette']:02d}"
                  f"_W{item['width_tiles']}_H{item['height_tiles']}"
                  f"_F{item['flip']}.png")
        (image_dir/filename).write_bytes(png_bytes(w,h,pixels))
        item['png']='sprites/'+filename
        item['png_sha256']=hashlib.sha256((image_dir/filename).read_bytes()).hexdigest()
        render.append({**item,'pixels':pixels,'pixel_width':w,'pixel_height':h})
    pages=[]
    for start in range(0,len(render),16):
        chunk=render[start:start+16]
        name=f'sprite_contact_{start//16+1:03d}.png'
        (output/name).write_bytes(make_sheet(chunk))
        pages.append({'file':name,'first_index':start,'count':len(chunk),
                      'tiles':[f"{x['tile']:05x}" for x in chunk]})
    with gzip.open(output/'sprite_observations.tsv.gz','wt',encoding='utf-8',newline='') as fp:
        writer=csv.writer(fp,delimiter='\t')
        writer.writerow(('shot','sample','frame','object_index','x','y','tile',
                         'attributes','priority','palette','width_tiles','height_tiles','flip'))
        for o in all_observations:
            writer.writerow([o[k] for k in ('shot','sample','frame','index','x','y','tile',
                                          'attributes','priority','palette',
                                          'width_tiles','height_tiles','flip')])
    columns=('shot','tile','palette','width_tiles','height_tiles','flip','priority_mode',
             'priority_mask','sample_count','observations','positions_count',
             'first_frame','last_frame','first_sample','png')
    with (output/'sprite_catalog.tsv').open('w',encoding='utf-8',newline='') as fp:
        wr=csv.writer(fp,delimiter='\t')
        wr.writerow(columns)
        for item in catalog:
            wr.writerow([item.get(col,'') for col in columns])
    manifest={'format':'XM87203_DIRECT_TILE_SPRITE_FACTORY_V1',
              'source':provenance,
              'objects_observed':len(all_observations),
              'unique_object_variants':len(catalog),
              'image_candidates_selected':attempts,
              'images_exported':len(render),
              'pages':pages,
              'missing_source_tiles_by_shot':dict(missing_tiles),
              'errors':errors,
              'scenery_actor_identity_verified':False,
              'sprite_animation_cycles_verified':False,
              'stage_identity_verified':False,
              'hardware_priority_verified':False,
              'source_pixels_verified':False,
              'original_graphics_tiles_decoded':True,
              'source_palette_effective':use_clean,
              'unresolved_gaps':[
                  'Tile ID alone does not distinguish background actors, fighters, effects or HUD.',
                  'Individual object animation tracks and original priority are not yet verified.',
                  'Source pixels are decoded from native CPS2 packed tiles; color cache is provisional for raw telemetry.'
                      if not use_clean else
                  'Correct emulator-effective 32-bit palette is used; compositing/blend effects are not yet reproduced.'
              ]}
    (output/'sprite_factory_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    # Small ZIP of indexed previews and evidence, never raw 32-MiB donor GFX.
    outzip=output/'sprite_factory_review_bundle.zip'
    with zipfile.ZipFile(outzip.with_suffix('.zip.tmp'),'w',compression=zipfile.ZIP_DEFLATED) as archive:
        for name in ('sprite_factory_manifest.json','sprite_catalog.tsv'):
            archive.write(output/name,arcname=name)
        for page in pages:
            archive.write(output/page['file'],arcname=page['file'])
        for sprite in render:
            archive.write(output/sprite['png'],arcname=sprite['png'])
    outzip.with_suffix('.zip.tmp').replace(outzip)
    print(f"SPRITE_FACTORY source={provenance['mode']} objects={len(all_observations)} variants={len(catalog)} sprites={len(render)} pages={len(pages)} errors={len(errors)}")
    print(f'SPRITE_FACTORY_BUNDLE {outzip}')
    print('NOTE: sprite graphics addressed directly, scene actors NOT yet classified')
    return manifest


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--clean-root',type=Path,help='existing Stage44/47 clean_rips directory')
    p.add_argument('--capture',type=Path,help='native stage_capture directory or ZIP fallback')
    p.add_argument('--output',type=Path,required=True,help='standalone new/refreshable review output')
    p.add_argument('--source',choices=['auto','clean','telemetry'],default='auto')
    p.add_argument('--shot',help='restrict clean source to one shot_XXXX')
    p.add_argument('--tile',type=lambda x:int(x,0),help='exact logical original CPS2 tile ID, e.g. 0x0B623')
    p.add_argument('--palette',type=lambda x:int(x,0),help='exact sprite palette 0..31')
    p.add_argument('--max-images',type=int,default=96)
    p.add_argument('--per-shot',type=int,default=8)
    p.add_argument('--step',type=int,default=120,help='raw telemetry sample spacing, frames')
    opts=p.parse_args()
    try:
        report=create_factory(opts.clean_root,opts.capture,opts.output,
                              source=opts.source,tile=opts.tile,palette=opts.palette,
                              shot=opts.shot,max_images=opts.max_images,
                              per_shot=opts.per_shot,step=opts.step)
        return 2 if report['errors'] else 0
    except (OSError,SpriteError,ValueError,zipfile.BadZipFile) as exc:
        print('SPRITE_FACTORY_ERROR '+str(exc),file=sys.stderr)
        return 2

if __name__=='__main__':
    raise SystemExit(main())