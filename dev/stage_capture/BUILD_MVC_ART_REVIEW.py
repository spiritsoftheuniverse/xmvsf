#!/usr/bin/env python3
"""Generate a local-only, single-file MVC1 sprite review HTML from original captures.

Does NOT change ROMs, graphics, source captures, emulator, or Stable.
Assumed categories are visual sorting hints, never claims about ROM semantics.
The generated HTML embeds original reconstructed artwork; DO NOT commit it.
"""
from __future__ import annotations
import argparse,base64,bisect,collections,csv,gzip,hashlib,json,struct,sys,zipfile
from pathlib import Path
from MVC_RGBA import RGBAImage, decode_original_tile
# The organizer's palette clustering is source-only and requires no image packages.
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'asset_organizer'))
from COLOR_GROUPING import palette, assign_groups

try:
    from TRACE_MVC_RECORD_LENGTHS import scan as scan_records
except ImportError:
    raise SystemExit('TRACE_MVC_RECORD_LENGTHS.py must be available beside exporter')

CATEGORIES=(
 ('character','Character / figure candidates'),
 ('stage','Stage / machinery candidates'),
 ('lettering','Lettering / HUD candidates'),
 ('effects','Effects / abstract candidates'),
 ('partial','Partial / unresolved fragments'),
)
SPECIAL={
    0x327A4E:'character',
    0x327326:'stage',
    0x328684:'lettering',
    0x327D4A:'partial',
    0x3356F8:'partial',
    0x33F426:'lettering', 0x33F4AC:'lettering', # MODE SELECT UI sign
    0x32888C:'lettering', # named text source sample
    0x2DF5D2:'effects',   # long abstract streak
    0x328E60:'stage', 0x328EA6:'stage', 0x328F26:'stage',
    0x328F6C:'stage', 0x328FEC:'stage', 0x329032:'stage', 0x329296:'stage',
}

class SourceError(ValueError): pass


def get_original(trace_root:Path) -> tuple[bytes,str]:
    files=sorted(trace_root.glob('run_*/mvc_program_data_host_words.bin')) if trace_root.is_dir() else []
    if trace_root.is_file():files=[trace_root]
    elif (trace_root/'mvc_program_data_host_words.bin').is_file():files=[trace_root/'mvc_program_data_host_words.bin']
    if not files: raise SourceError('missing private MVC1 program READ image')
    source=files[-1]
    data=source.read_bytes()
    if not (0<len(data)<=16*1024*1024 and len(data)%2==0):raise SourceError('invalid source program data')
    return data,hashlib.sha256(data).hexdigest()


def read_observations(path:Path):
    if not path.is_file():raise SourceError('missing original sprite observations')
    result=collections.defaultdict(list)
    opener=gzip.open if path.name.endswith('.gz') else open
    with opener(path,'rt',newline='',encoding='utf-8') as f:
        rows=csv.DictReader(f,delimiter='\t')
        required={'shot','frame','tile','attributes','priority','x','y','width_tiles','height_tiles'}
        if not required.issubset(rows.fieldnames or ()):raise SourceError('missing sprite observation fields')
        for r in rows:
            o={k:int(r[k]) for k in ('frame','tile','attributes','priority','x','y','width_tiles','height_tiles')}
            o['shot']=r['shot'];o['index']=int(r.get('index',r.get('object_index',0)))
            if not(0<=o['tile']<0x40000 and 0<=o['attributes']<=0xffff):raise SourceError('invalid source descriptor')
            o['pair']=(o['tile']&0xffff,o['attributes'])
            result[(o['shot'],o['frame'])].append(o)
    return result


def rank_records(program:bytes,observations:dict,minimum_coverage=.75,limit=600):
    records,links,regions=scan_records(program)
    word=lambda a:struct.unpack_from('<H',program,a)[0]
    valid={};reverse=collections.defaultdict(list)
    for addr,(control,count,_) in records.items():
        if 3<=count<=32 and addr+10+4*count<=len(program):
            body=[(word(addr+10+4*i),word(addr+12+4*i)) for i in range(count)]
            unique=set(body)
            if len(unique)<3:continue
            valid[addr]={'body':body,'unique':unique,'ctl':control,'count':count}
            for pair in unique:reverse[pair].append(addr)
    best={}
    for (shot,frame),objects in observations.items():
        frame_pairs={o['pair'] for o in objects}
        matches=collections.Counter()
        for pair in frame_pairs:
            for addr in reverse.get(pair,()):matches[addr]+=1
        for addr,hits in matches.items():
            record=valid[addr]
            if hits<3:continue
            frac=hits/len(record['unique'])
            if frac<minimum_coverage:continue
            # Exact original descriptor matches, not approximations by graphics family.
            selected=[o for o in objects if o['pair'] in record['unique']]
            x0=min(o['x'] for o in selected);y0=min(o['y'] for o in selected)
            x1=max(o['x']+16*o['width_tiles'] for o in selected)
            y1=max(o['y']+16*o['height_tiles'] for o in selected)
            W=x1-x0;H=y1-y0
            if W<=0 or H<=0 or W>1600 or H>500 or W*H>250000:continue
            # Favor complete and distinctive matches over large grouped HUD duplicates.
            metric=(int(hits==len(record['unique'])),frac,hits,
                    -max(W,H),-W*H,-frame)
            prior=best.get(addr)
            if prior is None or metric>prior['metric']:
                best[addr]={'offset':addr,'shot':shot,'frame':frame,'hits':hits,
                    'expected':len(record['unique']),'body':record['body'],
                    'ctl':record['ctl'],'width':W,'height':H,
                    'coverage':frac,'objects':selected,'metric':metric}
    # All qualifying unique records, capped only when requested.
    ranked=sorted(best.values(),key=lambda a:(-int(a['coverage']==1),-a['hits'],a['offset']))
    return ranked[:limit] if limit else ranked, {'all_matching_records':len(best),'structural_records':len(records)}


class Capture:
    """Read locally captured graphics + raw palette snapshots, no network/ROM execution."""
    def __init__(self,path:Path):
        self.path=path;self.zip=zipfile.ZipFile(path) if path.is_file() else None
        if self.zip:
            names=self.zip.namelist()
            paths=sorted({n.rsplit('/',1)[0] for n in names if n.endswith('/frames.tsv') and 'session_' in n})
        else:
            paths=sorted(str(p.relative_to(path)).replace('\\','/') for p in path.glob('session_*/frames.tsv'))
            paths=[p.rsplit('/',1)[0] for p in paths]
        if not paths:raise SourceError('no original stage capture sessions')
        bounds=[]
        for session in paths:
            text=self.read(session+'/frames.tsv').decode('utf-8').splitlines()
            if len(text)<2:continue
            head=text[0].split('\t');ix=head.index('frame')
            a=int(text[1].split('\t')[ix]);b=int(text[-1].split('\t')[ix])
            bounds.append((session,a,b))
        if not bounds:raise SourceError('no capture frames')
        # Latest frame-contiguous backwards chain, excluding older overlapping runs.
        current=max(bounds,key=lambda v:(v[2],v[0]));chosen=[current]
        while current[1]>1:
            priors=[b for b in bounds if b[2]==current[1]-1 and b not in chosen]
            if not priors:break
            current=max(priors,key=lambda v:v[0]);chosen.append(current)
        self.chain=list(reversed(chosen))
        last=self.chain[-1][0]
        gfx_candidates=[n for n in self.list() if n.startswith(last+'/mvc1_decoded_gfx_') and n.endswith('.bin')]
        if len(gfx_candidates)!=1:raise SourceError('expected one original decoded MVC1 gfx image in newest session')
        self.gfx=self.read(gfx_candidates[0]);self.mask=0x01ffffff
        if len(self.gfx)>64*1024*1024:raise SourceError('decoded gfx too large')
        self.palettes=[]
        self.palette_frames=[]
        for session,start,end in self.chain:
            raw=self.read(session+'/palettes.bin')
            size=6164
            if len(raw)%size:raise SourceError('invalid raw CPS2 palette snapshot stream')
            stamps=[]
            for i in range(len(raw)//size):
                if raw[i*size:i*size+4]!=b'PAL1':raise SourceError('invalid palette magic')
                stamps.append(struct.unpack_from('<I',raw,i*size+4)[0])
            self.palettes.append((session,start,end,raw,stamps))
            self.palette_frames.extend(stamps)
        self._last_colors=None
        self._palette_cache={}
        self._tile_cache={}

    def list(self):return self.zip.namelist() if self.zip else [str(x.relative_to(self.path)).replace('\\','/') for x in self.path.rglob('*') if x.is_file()]
    def read(self,name):return self.zip.read(name) if self.zip else (self.path/name).read_bytes()
    def close(self):
        if self.zip:self.zip.close()

    def palette(self,frame):
        # On the selected contiguous session chain, reuse the most recent
        # palette packet at-or-before this frame; DMA color conversion is approximate.
        for session,start,end,raw,stamps in reversed(self.palettes):
            if start<=frame<=end:
                idx=bisect.bisect_right(stamps,frame)-1
                if idx>=0:return (session,idx),raw[idx*6164+20:(idx+1)*6164]
                break
        # Palette can be unchanged since prior batch; take latest earlier packet.
        for session,start,end,raw,stamps in reversed(self.palettes):
            idx=bisect.bisect_right(stamps,frame)-1
            if idx>=0:return (session,idx),raw[idx*6164+20:(idx+1)*6164]
        raise SourceError('no palette at requested source frame')

    def pens(self,frame,number):
        state,pal=self.palette(frame)
        key=(state,number)
        if key not in self._palette_cache:
            colors=[bytes((0,0,0,0))]
            for i in range(1,16):
                value=struct.unpack_from('<H',pal,((number*16+i)^15)*2)[0]
                brightness=15+2*((value>>12)&15)
                colors.append(bytes([min(255,((value>>shift)&15)*17*brightness//45)
                                     for shift in (8,4,0)]+[255]))
            self._palette_cache[key]=tuple(colors)
        return state,self._palette_cache[key]

    def tile(self,ident,pen_state,pens,flipx,flipy):
        key=(ident,pen_state,flipx,flipy)
        image=self._tile_cache.get(key)
        if image is not None:return image
        start=(ident<<7)&self.mask
        if start+128>len(self.gfx):
            raise SourceError(f'original tile {ident:05X} outside captured graphics')
        image=decode_original_tile(self.gfx[start:start+128],pens,flipx,flipy)
        if len(self._tile_cache)>10000:self._tile_cache.clear()
        self._tile_cache[key]=image
        return image

    def sprite(self,o):
        attr=o['attributes'];num=attr&31
        state,pens=self.pens(o['frame'],num)
        tw=o['width_tiles'];th=o['height_tiles'];tile=o['tile']
        fx=bool(attr&32);fy=bool(attr&64)
        result=RGBAImage(tw*16,th*16)
        for dy in range(th):
            for dx in range(tw):
                ident=(tile&~15)+(dy<<4)+((tile+dx)&15)
                img=self.tile(ident,(state,num),pens,fx,fy)
                dest=((tw-1-dx if fx else dx)*16,(th-1-dy if fy else dy)*16)
                result.alpha_composite(img,dest)
        return result

    def compose(self,item):
        objects=item['objects'];x0=min(x['x'] for x in objects);y0=min(x['y'] for x in objects)
        w=item['width'];h=item['height']
        canvas=RGBAImage(w,h)
        for o in sorted(objects,key=lambda o:o['index']):
            canvas.alpha_composite(self.sprite(o),(o['x']-x0,o['y']-y0))
        box=canvas.getbbox()
        return canvas.crop(box) if box else canvas


def guess_category(item,img):
    known=SPECIAL.get(item['offset'])
    if known:return known,'visually reviewed reference'
    if item['coverage']<1:return 'partial','missing recorded components'
    if img.width>700 or img.height>300:return 'partial','very dispersed/large component layout (tentative)'
    w,h=img.size;ratio=w/max(h,1)
    if w>=120 and h<=48 and ratio>=4.5:return 'lettering','wide, flat silhouette (tentative)'
    if h>=76 and .3<=ratio<=1.6:return 'character','upright silhouette (tentative)'
    if w>=150 and 45<=h<=210 and ratio>=2.7:return 'stage','wide multi-part silhouette (tentative)'
    if w<96 and h<86:return 'effects','small compact silhouette (tentative)'
    if 70<=h<=240 and .5<=ratio<=1.7:return 'character','figure-like proportions (tentative)'
    return 'effects','uncategorized multi-part artwork (tentative)'


def encode_png(image):
    return 'data:image/png;base64,'+base64.b64encode(image.png_bytes()).decode('ascii')


def create_viewer(program_path:Path,observations_path:Path,capture_path:Path,
                  html_template:Path,output:Path,limit:int=0):
    source_path=program_path.resolve();capture_source=capture_path.resolve();observations_source=observations_path.resolve()
    target=output.resolve()
    for protected in (source_path,capture_source,observations_source):
        if target==protected or protected in target.parents or target in protected.parents:
            raise SourceError('review output overlaps original capture/source; refused')
    data,digest=get_original(program_path)
    obs=read_observations(observations_path)
    ranked,stats=rank_records(data,obs,limit=limit)
    if not ranked:raise SourceError('no original MVC sprite assemblies found')
    capture=Capture(capture_path)
    cards=[];issues=[]
    try:
        for idx,item in enumerate(ranked):
            try:
                image=capture.compose(item)
                category,rationale=guess_category(item,image)
                cards.append({
                    'id':f'rom_{item["offset"]:06X}',
                    'offset':f'{item["offset"]:06X}',
                    'category':category,'categoryEvidence':rationale,
                    'shot':item['shot'],'frame':item['frame'],
                    'componentsMatched':item['hits'],'componentsExpected':item['expected'],
                    'coverage':round(item['coverage'],4),
                    'selector':f'{item["ctl"][3]:04X}',
                    'nativeWidth':image.width,'nativeHeight':image.height,
                    'priorityChannels':sorted(set(o['priority'] for o in item['objects'])),
                    'dominantColors':palette(image),
                    'png':encode_png(image),
                })
            except (SourceError,ValueError,struct.error,IndexError) as exc:
                issues.append({'offset':f'{item["offset"]:06X}','reason':str(exc)})
    finally:capture.close()
    if not cards:raise SourceError('unable to reconstruct any original artwork')
    color_analysis=assign_groups(cards,threshold=.78)
    html=html_template.read_text(encoding='utf-8')
    payload={'format':'MVC1_ART_REVIEW_GALLERY_V1','romSha256':digest,
             'sourceObservations':observations_path.name,
             'paletteStatus':'APPROXIMATE_RAW_CPS2_PALETTE_NOT_NATIVE_EFFECTIVE_DMA',
             'classificationStatus':'TENTATIVE_VISUAL_HEURISTICS',
             'catalog':cards,'colorAnalysis':color_analysis,'categories':[{'id':k,'label':v} for k,v in CATEGORIES]}
    safe_json=json.dumps(payload,separators=(',',':')).replace('</','<\\/')
    if '__MVC_CATALOG__' not in html:raise SourceError('viewer HTML template missing catalog placeholder')
    # Optional review seed is local-only, never copied to public source control.
    # A source checksum and exact ID set must match before prior selections are embedded.
    review_seed=None
    saved=output.parent/'aodata.json'
    if saved.is_file():
        try:
            review_seed=json.loads(saved.read_text(encoding='utf-8'))
            required={x['id'] for x in cards}
            if (review_seed.get('format')!='MVC1_ASSET_ORGANIZER_V2' or
                review_seed.get('sourceRomSha256')!=digest or
                review_seed.get('sourceCatalogSize')!=len(cards) or
                not isinstance(review_seed.get('items'),list) or
                len(review_seed['items'])!=len(cards) or
                {x['id'] for x in review_seed['items']}!=required):
                raise SourceError('existing aodata.json does not match this gallery; original review preserved')
        except (ValueError,TypeError,KeyError,AttributeError) as e:
            raise SourceError('invalid existing aodata.json: '+str(e)) from e
    if '__AODATA_SEED__' in html:
        seed_json=json.dumps(review_seed,separators=(',',':')).replace('</','<\\/')
        html=html.replace('__AODATA_SEED__',seed_json)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(html.replace('__MVC_CATALOG__',safe_json),encoding='utf-8')
    manifest={k:v for k,v in payload.items() if k!='catalog'}
    manifest.update({'generatedCandidates':len(cards),'sourceFrameGroups':len(obs),
         'fullComponentMatchCards':sum(x['coverage']==1 for x in cards),
         'assumedCategoryCounts':dict(collections.Counter(x['category'] for x in cards)),
         'skipped':issues,'ranked':stats,
         'colorGroupCount':color_analysis['groupCount'],
         'colorMethod':color_analysis['method'],
         'originalArtCommittedToGit':False})
    output.with_suffix('.manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    return manifest


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace-root',type=Path,required=True)
    p.add_argument('--observations',type=Path,required=True)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--template',type=Path,default=Path(__file__).with_name('MVC_ART_REVIEW_TEMPLATE.html'))
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--limit',type=int,default=0,help='0=all qualifying artwork')
    args=p.parse_args()
    if args.limit<0:raise SystemExit('limit must not be negative')
    report=create_viewer(args.trace_root,args.observations,args.capture,args.template,args.output,args.limit)
    print('MVC_ART_REVIEW: {} artwork assemblies ({} complete)'.format(report['generatedCandidates'],report['fullComponentMatchCards']))
    print('HTML:',args.output)

if __name__=='__main__':main()