#!/usr/bin/env python3
"""Extract Reaper Sigma's PSD animation layers as lossless RGBA PNGs.

No external dependency or Photoshop is required. All original PSD layer
positions, alpha, masks and opacity are retained in a SHA-cached manifest;
PNG pixels are cropped only to the visible alpha rectangle, so an actor's
bottom-most visible pixel can be anchored 140 world pixels above the floor.
"""
from __future__ import annotations
import hashlib
import os
from pathlib import Path
import re
import struct
import sys
from extract_guile_stage import PSD, decode_plane, png_rgba

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "_art/sigmastage/reapersigma.psd"
OUT = SOURCE.parent / "extracted"
REQUIRED = (
    tuple(f"idle{i}" for i in range(1, 4)) +
    tuple(f"smash{i}" for i in range(1, 5)) +
    tuple(f"slashrun{i}" for i in range(5, 8)) +
    ("ball1body",) + tuple(f"ball{i}" for i in range(2, 10))
)

def normal(text: str) -> str:
    return re.sub("[^a-z0-9]", "", text.lower())

def frame_role(parts: list[str]) -> str | None:
    leaf = normal(parts[-1])
    parents = [normal(x) for x in parts[:-1]]
    if leaf.startswith("reapersigma"):
        leaf = leaf[len("reapersigma"):]
    if leaf.startswith("frame"):
        leaf = leaf[5:]
    if leaf.isdigit() and parents:
        parent = "".join(parents)
        if "slashrun" in parent or ("slash" in parent and "run" in parent):
            leaf = "slashrun" + leaf
        elif "smash" in parent:
            leaf = "smash" + leaf
        elif "idle" in parent:
            leaf = "idle" + leaf
        elif "ball" in parent:
            leaf = "ball" + leaf
    if leaf.startswith("slashrunframe"):
        leaf = "slashrun" + leaf[13:]
    if leaf.startswith("slashrun"):
        pass
    elif leaf.startswith("slash") and leaf[5:].isdigit():
        leaf = "slashrun" + leaf[5:]
    elif leaf.startswith("run") and leaf[3:].isdigit():
        leaf = "slashrun" + leaf[3:]
    if leaf in REQUIRED:
        return leaf
    if "ball" in leaf and "body" in leaf and ("1" in leaf or not re.search(r"\d",leaf)):
        return "ball1body"
    return None

def role_for_raster(parts: list[str]) -> str | None:
    """A Photoshop animation frame may be a group of raster sublayers.

    The group itself has no channel pixels; never treat its lsct marker as
    an image. For a genuine raster, a named leaf wins, then the nearest
    named ancestor group. This supports e.g. ball2/Layer 1 + ball2/Glow.
    """
    direct = frame_role(parts)
    if direct:
        return direct
    for name in reversed(parts[:-1]):
        candidate = frame_role([name])
        if candidate:
            return candidate
    return None

def read_layers(raw: bytes):
    p = PSD(raw)
    if p.take(4) != b"8BPS" or p.u16() != 1:
        raise ValueError("Reaper Sigma requires an ordinary PSD v1 file (not PSB)")
    p.skip(6)
    channels, height, width, depth, mode = p.u16(),p.u32(),p.u32(),p.u16(),p.u16()
    if mode not in (1,3) or depth != 8 or not 1 <= channels <= 64 or width*height > 8192*8192:
        raise ValueError(f"Unsupported PSD {width}x{height} mode={mode}, depth={depth}")
    p.skip(p.u32())  # color-mode data
    p.skip(p.u32())  # image resources
    layer_length = p.u32()
    if not layer_length:
        raise ValueError("Reaper Sigma PSD has no editable layers")
    layer_info_length = p.u32()
    if not layer_info_length:
        raise ValueError("Reaper Sigma PSD layer record is empty")
    info_end=p.pos+layer_info_length
    count=abs(p.i16())
    if not 1<=count<=4096:
        raise ValueError(f"Invalid Reaper Sigma layer count {count}")
    layers=[]
    for i in range(count):
        top,left,bottom,right=p.i32(),p.i32(),p.i32(),p.i32()
        n=p.u16()
        if n>64:
            raise ValueError(f"Invalid layer channel count {n} at {i}")
        chan=[(p.i16(),p.u32()) for _ in range(n)]
        if p.take(4)!=b"8BIM":
            raise ValueError(f"Invalid Reaper Sigma layer signature at {i}")
        p.skip(4) # blend mode
        opacity,clipping,flags,filler=p.u8(),p.u8(),p.u8(),p.u8()
        extra_len=p.u32()
        extra_end=p.pos+extra_len
        mask=None
        mask_len=p.u32()
        if mask_len>=20:
            mt,ml,mb,mr=p.i32(),p.i32(),p.i32(),p.i32()
            default,mask_flags=p.u8(),p.u8()
            mask=(mt,ml,mb,mr,default,mask_flags)
            p.skip(mask_len-18)
        else:
            p.skip(mask_len)
        p.skip(p.u32()) # blending ranges
        name_len=p.u8()
        name=p.take(name_len).decode("latin1",errors="replace")
        p.skip((-(name_len+1))%4)
        if p.pos>extra_end:
            raise ValueError(f"Invalid layer metadata size for {name!r}")
        section=0
        while p.pos+12<=extra_end:
            sig=p.raw[p.pos:p.pos+4].tobytes()
            if sig not in (b"8BIM",b"8B64"):
                break
            p.skip(4)
            key=p.take(4)
            sz=p.u32()
            if sig==b"8B64":
                sz=sz<<32|p.u32()
            if sz>extra_end-p.pos:
                break
            data=p.take(sz)
            if key==b"luni" and len(data)>=4:
                chars=struct.unpack(">I",data[:4])[0]
                if chars*2<=len(data)-4:
                    name=data[4:4+chars*2].decode("utf-16-be",errors="replace")
            if key in (b"lsct",b"lsdk") and len(data)>=4:
                section=struct.unpack(">I",data[:4])[0]
            if sz&1 and p.pos<extra_end:
                p.skip(1)
        p.pos=extra_end
        layers.append(dict(index=i,name=name,section=section,
                           bounds=(left,top,right,bottom),
                           channels=chan,opacity=opacity,mask=mask))
    if p.pos>info_end:
        raise ValueError("Reaper Sigma PSD layer-info overflow")
    def path_map(backward: bool):
        stack=[]
        found={}
        order=range(len(layers)-1,-1,-1) if backward else range(len(layers))
        for i in order:
            layer=layers[i]
            if layer["section"] in (1,2):
                stack.append(layer["name"])
            elif layer["section"]==3:
                if stack:stack.pop()
            else:
                found[i]=stack+[layer["name"]]
        return found
    forward,backward=path_map(False),path_map(True)
    score=lambda paths:sum(role_for_raster(names) is not None for names in paths.values())
    paths=backward if score(backward)>score(forward) else forward
    return p,width,height,mode,layers,paths

def extract(raw: bytes, src_sha: str, code_sha: str):
    p,cw,ch,mode,layers,paths=read_layers(raw)
    pending={}
    records=[f"FORMAT\t1",f"CANVAS\t{cw}\t{ch}",
             f"SOURCE_SHA256\t{src_sha}",f"EXTRACTOR_SHA256\t{code_sha}"]
    # Each role may contain several alpha-bearing sublayers within a PSD
    # folder. Preserve their original canvas-space bounds for composition.
    fragments: dict[str,list[tuple[int,int,int,int,bytes,str]]] = {}
    for i,layer in enumerate(layers):
        x,y,right,bottom=layer["bounds"]
        w,h=right-x,bottom-y
        # lsct=1/2/3 are Photoshop folder/start/end markers, not pixels.
        # Their names (ball2, etc.) identify a FRAME GROUP, and its
        # descendant raster layers contain the art.
        role=(role_for_raster(paths.get(i,[layer["name"]]))
              if layer["section"] not in (1,2,3) else None)
        # Channel payload is sequential in the PSD, including folders,
        # invisible source frames, and unrelated/guide layers.
        planes={}
        for ident,length in layer["channels"]:
            if length<0 or length>len(raw):
                raise ValueError(f"Invalid PSD channel length in {layer['name']}")
            if not role:
                p.skip(length)
                continue
            payload=p.take(length)
            if w<=0 or h<=0 or w*h>8192*8192 or length<2:
                continue
            if ident in (0,1,2,-1):
                planes[ident]=decode_plane(payload,w,h)
            elif ident==-2 and layer["mask"]:
                mt,ml,mb,mr,*_=layer["mask"]
                if mr>ml and mb>mt:
                    planes[-2]=decode_plane(payload,mr-ml,mb-mt)
        if not role:
            continue
        if w<=0 or h<=0 or not planes:
            # Placeholder rasters and empty group thumbnails are legal.
            # Reject only when the entire required animation role has
            # no visible source, after inspecting all of its sublayers.
            continue
        n=w*h
        red=planes.get(0,bytes(n))
        green=planes.get(1,red) if mode==3 else red
        blue=planes.get(2,red) if mode==3 else red
        alpha=planes.get(-1,bytes([255])*n)
        opacity=layer["opacity"]
        rgba=bytearray(n*4)
        rgba[0::4]=red
        rgba[1::4]=green
        rgba[2::4]=blue
        mask=layer["mask"]
        mask_data=planes.get(-2)
        if mask and mask_data:
            mt,ml,mb,mr,default,flags=mask
            mw,mh=mr-ml,mb-mt
            result=bytearray(n)
            for yy in range(h):
                for xx in range(w):
                    tx,ty=x+xx-ml,y+yy-mt
                    a=mask_data[ty*mw+tx] if 0<=tx<mw and 0<=ty<mh else default
                    if flags&4:a=255-a
                    result[yy*w+xx]=alpha[yy*w+xx]*opacity*a//65025
            rgba[3::4]=result
        else:
            rgba[3::4]=alpha if opacity==255 else alpha.translate(
                bytes(j*opacity//255 for j in range(256)))
        # Ignore Photoshop's transparent excess; bounds of actual pixels
        # define the sprite foot, not a full-size opaque PSD canvas.
        used=rgba[3::4]
        left0,top0,right0,bottom0=w,h,-1,-1
        for yy in range(h):
            row=used[yy*w:(yy+1)*w]
            if not any(row):
                continue
            top0=min(top0,yy)
            bottom0=yy
            first=next(j for j,a in enumerate(row) if a)
            last=w-1-next(j for j,a in enumerate(reversed(row)) if a)
            left0=min(left0,first)
            right0=max(right0,last)
        if right0<left0 or bottom0<top0:
            continue
        tw,th=right0-left0+1,bottom0-top0+1
        trimmed=b"".join(rgba[(yy*w+left0)*4:(yy*w+left0+tw)*4]
                         for yy in range(top0,bottom0+1))
        fragments.setdefault(role,[]).append(
            (x+left0,y+top0,tw,th,trimmed,layer["name"]))
    absent=[role for role in REQUIRED if role not in fragments]
    if absent:
        print("REAPER_SIGMA_PSD_LAYER_INVENTORY "+
              repr([(layer["index"],layer["name"],layer["section"],
                     paths.get(i,[])) for i,layer in enumerate(layers)]))
        raise ValueError("Missing visible Reaper Sigma animation roles: "+
                         ", ".join(absent))
    # Composite sublayers in PSD layer order with Porter-Duff source-over,
    # correctly accounting for each layer's per-pixel alpha and position.
    # Photoshop stores layers top-first; draw them in reverse order.
    for role in REQUIRED:
        parts=fragments[role]
        left=min(p[0] for p in parts)
        top=min(p[1] for p in parts)
        right=max(p[0]+p[2] for p in parts)
        bottom=max(p[1]+p[3] for p in parts)
        tw,th=right-left,bottom-top
        if tw<=0 or th<=0 or tw*th>8000000:
            raise ValueError(f"Unsafe frame {role}: {tw}x{th}")
        output=bytearray(tw*th*4)
        for px,py,pw,ph,data,source_name in reversed(parts):
            for yy in range(ph):
                for xx in range(pw):
                    src=(yy*pw+xx)*4
                    a=data[src+3]
                    if not a:
                        continue
                    dst=((py-top+yy)*tw+(px-left+xx))*4
                    prev=output[dst+3]
                    oa=a+(prev*(255-a)+127)//255
                    if oa:
                        for ch in range(3):
                            n=data[src+ch]*a*255 + output[dst+ch]*prev*(255-a)
                            output[dst+ch]=(n+oa*255//2)//(oa*255)
                        output[dst+3]=oa
        filename=role+".png"
        pending[filename]=png_rgba(tw,th,bytes(output))
        records.append(f"FRAME\t{role}\t{filename}\t{left}\t{top}\t{tw}\t{th}")
        print(f"REAPER_SIGMA_FRAME {role} layers={len(parts)} "
              f"visible=({left},{top}) {tw}x{th}")
    OUT.mkdir(parents=True,exist_ok=True)
    for filename,png in pending.items():
        tmp=OUT/(filename+".tmp")
        tmp.write_bytes(png)
        os.replace(tmp,OUT/filename)
    manifest=OUT/"manifest.tsv"
    tmp=OUT/"manifest.tsv.tmp"
    tmp.write_text("\n".join(records)+"\n",encoding="utf-8")
    os.replace(tmp,manifest)
    print(f"REAPER_SIGMA_EXPORTED {len(pending)} frames from {SOURCE} "
          f"size={cw}x{ch}; transparent bounds preserved")

def main() -> int:
    if not SOURCE.is_file():
        print(f"REAPER_SIGMA_PSD_MISSING {SOURCE}",file=sys.stderr)
        return 2
    try:
        raw=SOURCE.read_bytes()
        src_sha=hashlib.sha256(raw).hexdigest()
        code_sha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        manifest=OUT/"manifest.tsv"
        if manifest.is_file():
            rows=manifest.read_text(encoding="utf-8").splitlines()
            frames={}
            for row in rows:
                fields=row.split("\t")
                if len(fields)>=7 and fields[0]=="FRAME":
                    frames[fields[1]]=fields[2]
            if (f"SOURCE_SHA256\t{src_sha}" in rows and
                f"EXTRACTOR_SHA256\t{code_sha}" in rows and
                all(role in frames for role in REQUIRED) and
                all((OUT/frames[r]).is_file() and (OUT/frames[r]).stat().st_size>0
                    for r in REQUIRED)):
                print(f"REAPER_SIGMA_CACHE_HIT {len(frames)} frames")
                return 0
        extract(raw,src_sha,code_sha)
        return 0
    except (ValueError,struct.error,OverflowError,EOFError) as error:
        print(f"REAPER_SIGMA_EXTRACT_FAILED {error}",file=sys.stderr)
        return 1

if __name__=="__main__":
    raise SystemExit(main())
