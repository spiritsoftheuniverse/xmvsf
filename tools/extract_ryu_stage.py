#!/usr/bin/env python3
"""Ryu stage 87.325: lossless PSD parallax, independent PNG ground.

The user-authored _art/ryustage/ryustage.png is the ONLY ground and
native world camera holder; this extractor never alters or regenerates it.
Extract only named Roof/Roof2/Roof3/Sky Top/Sky Bottom Photoshop layers
at full original bounds, preserving negative X/Y and overflow.
Unnamed construction layers and the deprecated PSD Ground are ignored.
The existing PSD/SHA warm cache is preserved (no Photoshop/pip required).
"""
from __future__ import annotations
import hashlib
from pathlib import Path
import re
import struct
import sys
import zlib
from extract_guile_stage import PSD, decode_plane, png_rgba

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "_art/ryustage/ryustage.psd"
OUT = ROOT / "_art/ryustage/extracted"
ROLES = {"roof", "roof2", "roof3", "skytop", "skybottom"}


def role(name: str) -> str:
    key = re.sub(r"[^a-z0-9]", "", name.lower())
    return key if key in ROLES else "other"


def layer_index(raw: bytes):
    p = PSD(raw)
    if p.take(4) != b"8BPS" or p.u16() != 1:
        raise ValueError("Ryu Stage requires PSD v1")
    p.skip(6)
    channels, height, width, depth, mode = p.u16(), p.u32(), p.u32(), p.u16(), p.u16()
    if (width, height) != (768, 471) or depth != 8 or mode not in (1, 3):
        raise ValueError(f"Expected original 768x471 8-bit RGB PSD; got {width}x{height}, depth={depth}, mode={mode}")
    p.skip(p.u32())
    p.skip(p.u32())
    whole = p.u32()
    if not whole:
        raise ValueError("PSD has no layers")
    info = p.u32()
    if not info:
        raise ValueError("PSD layer info is empty")
    end = p.pos + info
    count = abs(p.i16())
    if not 0 < count < 2048:
        raise ValueError(f"Unexpected layer count {count}")
    layers = []
    for i in range(count):
        top, left, bottom, right = p.i32(), p.i32(), p.i32(), p.i32()
        channel_count = p.u16()
        if channel_count > 64:
            raise ValueError(f"Invalid channel count {channel_count}")
        chans = [(p.i16(), p.u32()) for _ in range(channel_count)]
        if p.take(4) != b"8BIM":
            raise ValueError(f"Invalid layer {i} signature")
        p.skip(4)  # blend mode
        opacity, clipping, flags, pad = p.u8(), p.u8(), p.u8(), p.u8()
        extra_len = p.u32()
        extra_end = p.pos + extra_len
        mask = None
        mask_len = p.u32()
        if mask_len >= 20:
            mt, ml, mb, mr = p.i32(), p.i32(), p.i32(), p.i32()
            default, mask_flags = p.u8(), p.u8()
            mask = (mt, ml, mb, mr, default, mask_flags)
            p.skip(mask_len - 18)
        else:
            p.skip(mask_len)
        p.skip(p.u32()) # blending ranges
        nl = p.u8()
        name = p.take(nl).decode("latin1", errors="replace")
        p.skip((-(nl+1)) % 4)
        section = 0
        while p.pos + 12 <= extra_end:
            sig = p.raw[p.pos:p.pos+4].tobytes()
            if sig not in (b"8BIM", b"8B64"):
                break
            p.skip(4)
            key = p.take(4)
            size = p.u32()
            if sig == b"8B64":
                size = size << 32 | p.u32()
            if size > extra_end-p.pos:
                break
            payload = p.take(size)
            if key == b"luni" and len(payload) >= 4:
                n = struct.unpack(">I", payload[:4])[0]
                if n*2 <= len(payload)-4:
                    name = payload[4:4+n*2].decode("utf-16-be", errors="replace")
            elif key in (b"lsct", b"lsdk") and len(payload) >= 4:
                section = struct.unpack(">I", payload[:4])[0]
            if size&1 and p.pos < extra_end:
                p.skip(1)
        p.pos = extra_end
        layers.append({"index":i,"name":name,"section":section,"opacity":opacity,
                       "bounds":(left,top,right,bottom),"channels":chans,"mask":mask,
                       "visible":not bool(flags & 2)})
    if p.pos > end:
        raise ValueError("PSD layer index exceeds layer data")
    return p, width, height, mode, layers


def extract(raw: bytes) -> list[str]:
    p, cw, ch, mode, layers = layer_index(raw)
    records = [f"CANVAS\t{cw}\t{ch}", "FORMAT\t1",
               "SOURCE_SHA256\t"+hashlib.sha256(raw).hexdigest(),
               "EXTRACTOR_SHA256\t"+hashlib.sha256(Path(__file__).read_bytes()).hexdigest()]
    found = {}
    pending: dict[str, bytes] = {}
    for layer in layers:
        name, section = layer["name"],layer["section"]
        x, y, right, bottom = layer["bounds"]
        w, h = right-x, bottom-y
        planes = {}
        for ident,length in layer["channels"]:
            data = p.take(length)
            if w<=0 or h<=0 or length < 2:
                continue
            if ident in (0,1,2,-1):
                planes[ident] = decode_plane(data,w,h)
            elif ident == -2 and layer["mask"]:
                mt,ml,mb,mr,_,_=layer["mask"]
                if mr>ml and mb>mt:
                    planes[-2] = decode_plane(data,mr-ml,mb-mt)
        if section in (1,2,3) or w<=0 or h<=0 or not planes:
            continue
        # The new PNG is sole Ground. PSD 'Layer 2' is an opaque
        # construction raster that would hide the sky and the parallax.
        # Preserve the PSD, but export only the five explicitly named
        # sky/roof roles. Hidden named roles remain supported.
        if role(name) not in ROLES:
            continue
        if w>4096 or h>4096 or w*h>8000000:
            raise ValueError(f"Unreasonably large Ryu raster layer {name}: {w}x{h}")
        label = role(name)
        if label in found:
            raise ValueError(f"Duplicate Ryu stage layer role: {name}")
        found[label] = layer["index"]
        n = w*h
        gray = planes.get(0,bytes(n))
        red = gray
        green = planes.get(1,gray) if mode==3 else gray
        blue = planes.get(2,gray) if mode==3 else gray
        alpha = planes.get(-1,bytes([255])*n)
        rgba = bytearray(n*4)
        rgba[0::4]=red;rgba[1::4]=green;rgba[2::4]=blue
        mask = layer["mask"]
        mask_plane = planes.get(-2)
        opacity = layer["opacity"]
        if mask and mask_plane:
            mt,ml,mb,mr,default,flags=mask
            mw,mh=mr-ml,mb-mt
            composite=bytearray(n)
            for yy in range(h):
                for xx in range(w):
                    a=(y+yy-mt)*mw+(x+xx-ml)
                    mx,my=x+xx-ml,y+yy-mt
                    mask_a=mask_plane[a] if 0<=mx<mw and 0<=my<mh else default
                    if flags&4:mask_a=255-mask_a
                    composite[yy*w+xx]=alpha[yy*w+xx]*opacity*mask_a//(255*255)
            rgba[3::4]=composite
        else:
            rgba[3::4]=alpha if opacity==255 else alpha.translate(
                bytes(j*opacity//255 for j in range(256)))
        filename=f"{layer['index']:04d}.png"
        pending[filename] = png_rgba(w,h,bytes(rgba))
        clean_name=name.replace("\t"," ").replace("\r"," ").replace("\n"," ")
        records.append(f"LAYER\t{label}\t{layer['index']}\t{x}\t{y}\t{w}\t{h}\t{filename}\t{clean_name}")
        print(f"RYU_LAYER {clean_name!r} role={label} PSD=({x},{y})..({right},{bottom}) image={w}x{h} canvas=768x471")
    needed=ROLES-set(found)
    if needed:
        raise ValueError("Missing required named PSD layers: "+", ".join(sorted(needed)))
    if not pending:
        raise ValueError("No Ryu layers were exported")
    OUT.mkdir(parents=True,exist_ok=True)
    # Write all PNGs only after every required role has been verified.
    for name,payload in pending.items():
        (OUT/name).write_bytes(payload)
    (OUT/"manifest.tsv").write_text("\n".join(records)+"\n",encoding="utf-8")
    print(f"RYU_EXTRACTED {len(pending)} textures; all original PSD coordinates retained")
    return records


def main() -> int:
    if not SRC.is_file():
        print(f"RYU_PSD_MISSING: {SRC}",file=sys.stderr)
        return 2
    try:
        raw=SRC.read_bytes()
        sha=hashlib.sha256(raw).hexdigest()
        script_sha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        manifest=OUT/"manifest.tsv"
        if manifest.is_file():
            rows=manifest.read_text(encoding="utf-8").splitlines()
            images=[OUT / row.split("\t")[7] for row in rows
                    if row.startswith("LAYER\t") and len(row.split("\t"))>=9]
            if (f"SOURCE_SHA256\t{sha}" in rows and
                f"EXTRACTOR_SHA256\t{script_sha}" in rows and
                f"CANVAS\t768\t471" in rows and images and
                all(p.is_file() and p.stat().st_size>0 for p in images)):
                print(f"RYU_CACHE_HIT {len(images)} PSD layers")
                return 0
        extract(raw)
        return 0
    except (ValueError,struct.error,zlib.error,OverflowError) as e:
        print(f"RYU_PSD_EXTRACT_FAILED: {e}",file=sys.stderr)
        return 1


if __name__=="__main__":
    sys.exit(main())
