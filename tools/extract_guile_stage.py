#!/usr/bin/env python3
"""Extract Guile Stage PSD layer pixels/geometry in place (stdlib only).
The source PSD remains untouched. Generated PNGs and manifest go in
_art/guilestage/extracted/. Re-run only when the PSD changes.

Supports 8-bit RGB PSD 1.0, raw, PackBits RLE, ZIP and ZIP-prediction channels,
Unicode names, Photoshop group dividers, per-layer alpha and pixel masks.
No Photoshop, pip packages, GitHub Actions or native build-cache deletion.
"""
from __future__ import annotations
import argparse
import hashlib
import os
from pathlib import Path
import re
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "_art" / "guilestage" / "guilestage.psd"
OUT = SOURCE.parent / "extracted"

class PSD:
    def __init__(self, raw: bytes):
        self.raw = memoryview(raw)
        self.pos = 0

    def take(self, n: int) -> bytes:
        if n < 0 or self.pos + n > len(self.raw):
            raise ValueError(f"PSD truncated at {self.pos}, length={n}")
        b = self.raw[self.pos:self.pos+n].tobytes()
        self.pos += n
        return b

    def skip(self, n: int) -> None:
        # Move over untouched PSD channel data without allocating a copy.
        # Joe-only refresh skips all unrelated layers this way.
        if n < 0 or self.pos + n > len(self.raw):
            raise ValueError(f"PSD truncated at {self.pos}, length={n}")
        self.pos += n

    def u8(self) -> int: return self.take(1)[0]
    def i16(self) -> int: return struct.unpack(">h", self.take(2))[0]
    def u16(self) -> int: return struct.unpack(">H", self.take(2))[0]
    def u32(self) -> int: return struct.unpack(">I", self.take(4))[0]
    def i32(self) -> int: return struct.unpack(">i", self.take(4))[0]

def packbits(data: bytes, wanted: int) -> bytes:
    out = bytearray()
    p = 0
    while p < len(data) and len(out) < wanted:
        n = data[p]
        p += 1
        if n <= 127:
            q = n + 1
            out += data[p:p+q]
            p += q
        elif n >= 129:
            q = 257 - n
            if p >= len(data): raise ValueError("Invalid PackBits repeat")
            out += bytes([data[p]]) * q
            p += 1
    if len(out) != wanted: raise ValueError(f"PackBits scanline {len(out)} != {wanted}")
    return bytes(out)

def decode_plane(payload: bytes, w: int, h: int) -> bytes:
    if w <= 0 or h <= 0 or w * h > 8192 * 8192:
        raise ValueError(f"Unsupported PSD layer size {w}x{h}")
    z = PSD(payload)
    compression = z.u16()
    data = z.take(len(payload)-2)
    if compression == 0:
        plane = data
    elif compression == 1:
        if len(data) < 2*h: raise ValueError("Short PSD RLE row table")
        counts = struct.unpack(">" + "H"*h, data[:2*h])
        pos = 2*h
        rows = []
        for n in counts:
            rows.append(packbits(data[pos:pos+n], w))
            pos += n
        plane = b"".join(rows)
    elif compression in (2, 3):
        plane = zlib.decompress(data)
        if compression == 3:
            b = bytearray(plane)
            for y in range(h):
                begin = y*w
                for x in range(1,w):
                    b[begin+x] = (b[begin+x]+b[begin+x-1]) & 255
            plane = bytes(b)
    else:
        raise ValueError(f"Unsupported PSD channel compression {compression}")
    if len(plane) != w*h:
        raise ValueError(f"Bad PSD channel length {len(plane)}; expected {w*h}")
    return plane

def png_rgba(w: int, h: int, pixels: bytes) -> bytes:
    def chunk(typ: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + typ + data + struct.pack(">I", zlib.crc32(typ+data)&0xffffffff)
    scan = b"".join(b"\0" + pixels[y*w*4:(y+1)*w*4] for y in range(h))
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", w,h,8,6,0,0,0)) +
            chunk(b"IDAT", zlib.compress(scan,6)) + chunk(b"IEND", b""))

def normalize(name: str) -> str:
    return re.sub(r"[^a-z0-9]+", " ", name.lower()).strip()

def role_for(parts: list[str]) -> str:
    # Characters is one exact Photoshop GROUP, not a fuzzy layer heuristic.
    parents = [normalize(p) for p in parts[:-1]]
    name = normalize(parts[-1])
    if "characters" in parents:
        return "actor"
    if name == "ground":
        return "ground"
    if name in ("high sky", "highsky", "sky") or "cloud" in name:
        return "sky"
    if name in ("bg", "background") or "background" in parents:
        return "bg"
    return "other"

def actor_name(parts: list[str]) -> str:
    # Literal Photoshop names: ladyL1/ladyL2 => ladyL; Joe1/Joe2 => Joe.
    name = re.sub(r"(?i)(?:[\s_-]*(?:frame|cel))?[\s_-]*\d+$", "", parts[-1]).strip()
    return name or parts[-1]

def sanitize_field(text: str) -> str:
    return text.replace("\t", " ").replace("\r", " ").replace("\n", " ")

def padded_rgba(pixels: bytes, w: int, h: int, x: int, y: int,
                canvas: tuple[int, int, int, int]) -> bytes:
    left, top, right, bottom = canvas
    tw, th = right - left, bottom - top
    if tw <= 0 or th <= 0 or tw * th > 8192 * 8192:
        raise ValueError(f"Invalid padded actor frame {canvas}")
    out = bytearray(tw * th * 4)
    # Preserve all original pixels; no clipping to a tight bounding box.
    if w and h:
        dx, dy = x - left, y - top
        if dx < 0 or dy < 0 or dx + w > tw or dy + h > th:
            raise ValueError(f"Actor frame {x},{y} + {w}x{h} exceeds {canvas}")
        for row in range(h):
            start = (dy + row) * tw * 4 + dx * 4
            out[start:start+w*4] = pixels[row*w*4:(row+1)*w*4]
    return bytes(out)

def extract(data: bytes, out: Path, actor_only: str | None = None) -> list[str]:
    p = PSD(data)
    if p.take(4) != b"8BPS" or p.u16() != 1:
        raise ValueError("Guile stage needs a Photoshop PSD (not PSB)")
    p.skip(6)
    channels, height, width, depth, mode = p.u16(), p.u32(), p.u32(), p.u16(), p.u16()
    if depth != 8 or mode not in (1,3) or not 1 <= channels <= 64 or width*height > 8192*8192:
        raise ValueError(f"Unsupported PSD: {width}x{height}, channels={channels}, depth={depth}, mode={mode}")
    p.skip(p.u32())  # color data
    p.skip(p.u32())  # image resources
    layer_mask_length = p.u32()
    if not layer_mask_length: raise ValueError("PSD has no layers")
    end = p.pos + layer_mask_length
    layer_info_len = p.u32()
    if not layer_info_len: raise ValueError("PSD has no editable layers")
    info_end = p.pos + layer_info_len
    layer_count = abs(p.i16())
    if layer_count > 16384: raise ValueError("Unreasonable PSD layer count")
    layers: list[dict] = []
    for index in range(layer_count):
        top,left,bottom,right = p.i32(),p.i32(),p.i32(),p.i32()
        n = p.u16()
        if n > 256: raise ValueError("Unreasonable PSD channel count")
        channels_info = [(p.i16(),p.u32()) for _ in range(n)]
        sig = p.take(4)
        blend = p.take(4)
        opacity,clipping,flags,filler = p.u8(),p.u8(),p.u8(),p.u8()
        if sig != b"8BIM": raise ValueError("Malformed PSD layer")
        # Read the length before computing the absolute end. Python evaluates
        # the left operand of "p.pos + p.u32()" first, which points FOUR bytes
        # before the extra payload and truncates the final 8BIM record.
        extra_length = p.u32()
        extra_end = p.pos + extra_length
        mask = None
        mask_len = p.u32()
        if mask_len >= 20:
            mt,ml,mb,mr = p.i32(),p.i32(),p.i32(),p.i32()
            default = p.u8()
            mask_flags = p.u8()
            mask = (mt,ml,mb,mr,default,mask_flags)
            p.skip(mask_len-18)
        else:
            p.skip(mask_len)
        p.skip(p.u32())  # blending ranges
        name_len = p.u8()
        name = p.take(name_len).decode("latin-1",errors="replace")
        p.skip((-(name_len+1)) % 4)
        # Extra data is a *bounded* container, not necessarily a stream of
        # additional-layer-info records all the way to its last byte. Adobe
        # and third-party PSD writers may leave word/long padding in the tail.
        # A trailing 1..11 bytes isn't a new 12-byte 8BIM record.
        if p.pos > extra_end:
            raise ValueError(
                f"PSD layer {index} ({name!r}) name/mask exceeds its extra data: "
                f"pos={p.pos} end={extra_end}")
        section = 0
        while p.pos + 12 <= extra_end:
            signature = p.raw[p.pos:p.pos+4].tobytes()
            if signature not in (b"8BIM", b"8B64"):
                # Trailing alignment bytes or an unrecognized extension.
                # Never interpret them as a new record's length.
                break
            p.skip(4)
            key = p.take(4)
            length = p.u32()
            if signature == b"8B64":
                if p.pos + 4 > extra_end:
                    break
                length = (length << 32) | p.u32()
            if length > extra_end-p.pos:
                # The declared block would cross its enclosing extra-data
                # boundary. This can be an optional extension with different
                # padding; stop only metadata parsing, not the entire PSD.
                print(
                    f"GUILE PSD metadata tail layer={index} name={name!r} "
                    f"key={key!r} length={length} remaining={extra_end-p.pos}",
                    file=sys.stderr)
                break
            if key in (b"luni", b"lsct", b"lsdk"):
                blob = p.take(length)
                if key == b"luni" and len(blob) >= 4:
                    count = struct.unpack(">I",blob[:4])[0]
                    if count*2 <= len(blob)-4:
                        name = blob[4:4+count*2].decode("utf-16-be",errors="replace")
                elif key in (b"lsct", b"lsdk") and len(blob) >= 4:
                    section = struct.unpack(">I",blob[:4])[0]
            else:
                p.skip(length)
            if (length & 1) and p.pos < extra_end:
                p.skip(1)
        # Precisely restore the enclosing layer-record boundary, including
        # any tail padding, before parsing the next layer. Do not read extra
        # data from the following layer or its channel payloads.
        p.pos = extra_end
        layers.append(dict(index=index,top=top,left=left,bottom=bottom,right=right,
                           channels=channels_info,opacity=opacity,visible=not (flags&2),
                           name=name,section=section,mask=mask,blend=blend))
    if p.pos > info_end: raise ValueError("PSD layer info overflow")
    # Resolve Photoshop's section dividers before exporting any pixels.
    # Some PSD writers record the folder delimiters in the opposite order.
    # Select whichever traversal actually contains the most raster members
    # under the literal "Characters" group; never silently skip its frames.
    def group_paths(reverse: bool) -> dict[int,list[str]]:
        stack: list[str] = []
        paths: dict[int,list[str]] = {}
        order = range(len(layers)-1,-1,-1) if reverse else range(len(layers))
        for i in order:
            layer = layers[i]
            section = layer["section"]
            if section in (1,2):
                stack.append(layer["name"])
            elif section == 3:
                if stack:
                    stack.pop()
            else:
                paths[i] = stack + [layer["name"]]
        return paths
    forward = group_paths(False)
    backward = group_paths(True)
    def char_count(paths: dict[int,list[str]]) -> int:
        return sum(role_for(parts) == "actor" for parts in paths.values())
    reverse_groups = char_count(backward) > char_count(forward)
    paths = backward if reverse_groups else forward
    print(f"GUILE PSD GROUP_ORDER={'reverse' if reverse_groups else 'forward'} "
          f"Characters_members={char_count(paths)}")
    ground = None
    actors: dict[str,list[dict]] = {}
    # Photoshop animation names are case-insensitive. ladyL1 and LadyL2,
    # for example, are frames of one actor, not two static characters.
    # Preserve the first seen spelling in the manifest for readability.
    actor_aliases: dict[str,str] = {}
    for i,layer in enumerate(layers):
        if layer["section"] in (1,2,3):
            layer["role"] = "folder"
            continue
        parts = paths.get(i,[layer["name"]])
        layer["path"] = parts
        layer["role"] = role_for(parts)
        if layer["role"] == "other":
            layer["role"] = "bg"
        if layer["role"] == "ground" and layer["right"] > layer["left"] and layer["bottom"] > layer["top"]:
            if ground is None:
                ground = layer
        if layer["role"] == "actor":
            spelled_name = actor_name(parts)
            layer["actor"] = actor_aliases.setdefault(spelled_name.casefold(), spelled_name)
            actors.setdefault(layer["actor"],[]).append(layer)
    if ground is None:
        raise ValueError("Guile PSD has no pixel layer named ground; stage needs its exact bounds")
    # Photoshop stores the occupied rectangle of a raster layer, not a
    # guaranteed full-stage rectangle. Combining artwork into Ground can
    # expand that occupied rectangle beyond the earlier 1016-pixel reference.
    # Keep original coordinates, expand the transparent frame if needed,
    # and never crop, rescale, or throw away merged ground artwork.
    raw_gl,raw_gt,raw_gr,raw_gb = (
        ground["left"],ground["top"],ground["right"],ground["bottom"])
    if raw_gl >= raw_gr or raw_gt >= raw_gb:
        raise ValueError(f"Guile ground has invalid source bounds "
                         f"({raw_gl},{raw_gt},{raw_gr},{raw_gb})")
    # The originally agreed 1016x471 frame is a MINIMUM/reference, not
    # a clipping boundary. The exported frame is the union of that frame
    # and the actual Ground raster layer. Never substitute the PSD canvas.
    gl,gt = min(0,raw_gl),min(0,raw_gt)
    gr,gb = max(1016,raw_gr),max(471,raw_gb)
    gw,gh = gr-gl,gb-gt
    # The camera stays centered on the original 1016px authored stage.
    # Extra pixels introduced by a merged Ground remain preserved offscreen
    # but must not shift all characters or the viewport by 4+ pixels.
    view_x = (1016-768)//2 - gl
    if gw*gh > 8192*8192 or gw < 768 or gh < 471:
        raise ValueError(f"Unsafe Guile ground frame ({gl},{gt},{gr},{gb})")
    pad_ground = (gl,gt,gr,gb) != (raw_gl,raw_gt,raw_gr,raw_gb)
    print(f"GUILE PSD GROUND SOURCE ({raw_gl},{raw_gt},{raw_gr},{raw_gb}) "
          f"frame=({gl},{gt},{gr},{gb}) centered_crop={view_x} "
          "pixels=verbatim")
    if not actors:
        raise ValueError("Guile PSD has no raster layers inside the Characters group")
    # Every animation group owns one identical transparent, ground-anchored
    # frame rectangle. Expand it if an original child extends beyond ground,
    # rather than silently dropping even one pixel.
    actor_bounds: dict[str, tuple[int,int,int,int]] = {}
    for key, frames in actors.items():
        bounds = (
            min([gl]+[f["left"] for f in frames if f["right"]>f["left"]]),
            min([gt]+[f["top"] for f in frames if f["bottom"]>f["top"]]),
            max([gr]+[f["right"] for f in frames if f["right"]>f["left"]]),
            max([gb]+[f["bottom"] for f in frames if f["bottom"]>f["top"]]),
        )
        actor_bounds[key] = bounds
    records: list[str] = [
        f"CANVAS\t{width}\t{height}",
        "FORMAT\t3",
        f"GROUND\t{gl}\t{gt}\t{gw}\t{gh}",
        f"VIEW\t768\t471\t{view_x}",
    ]
    out.mkdir(parents=True,exist_ok=True)
    target_actor: str | None = None
    positions: dict[int, int] = {}
    pending_pngs: dict[str, bytes] = {}
    if actor_only is not None:
        matches = [key for key in actors if key.casefold() == actor_only.casefold()]
        if len(matches) != 1:
            raise ValueError(f"PSD has no unique Characters actor {actor_only!r}")
        target_actor = matches[0]
        previous = out / "manifest.tsv"
        if not previous.is_file():
            raise ValueError("Joe-only export requires an existing full manifest; first run full extraction")
        previous_rows = previous.read_text(encoding="utf-8").splitlines()
        for header in records:
            if header not in previous_rows:
                raise ValueError(f"Stage geometry changed ({header}); run full extraction")
        records = [line for line in previous_rows if not line.startswith("PARTIAL_ACTOR\t")]
        previous_layers: dict[int, list[str]] = {}
        for index,line in enumerate(records):
            fields = line.split("\t")
            if fields[0] != "LAYER":
                continue
            if len(fields) < 16:
                raise ValueError("Existing stage manifest is incomplete; run full extraction")
            psd_index = int(fields[3])
            if psd_index in previous_layers or not 0 <= psd_index < len(layers):
                raise ValueError("Stage layer indexing changed; run full extraction")
            previous_layers[psd_index] = fields
            if fields[1] == "actor" and fields[2].casefold() == target_actor.casefold():
                positions[psd_index] = index
            else:
                layer = layers[psd_index]
                old_bounds = [str(layer[key]) for key in ("left","top","right","bottom")]
                old_path = "/".join(sanitize_field(x) for x in layer["path"])
                if (fields[1] != layer["role"] or
                    fields[2] != (layer.get("actor") if layer["role"] == "actor" else "-") or
                    fields[9] != sanitize_field(layer["name"]) or
                    fields[10] != old_path or fields[11:15] != old_bounds or
                    fields[15] != str(int(layer["visible"])) or
                    not (out / fields[8]).is_file()):
                    raise ValueError(f"Non-Joe layer {psd_index} changed or PNG missing; run full extraction")
        current_target = {layer["index"] for layer in actors[target_actor]}
        if not positions or set(positions) != current_target:
            raise ValueError("Joe frame indices/count changed; run full extraction")
        if len(previous_layers) != len([line for line in records if line.startswith("LAYER\t")]):
            raise ValueError("Existing stage manifest has duplicate layers; run full extraction")
        # No new non-Joe raster may silently disappear from the old manifest.
        # Targeted extraction is valid only when the original layer roster
        # and all geometry still match the previously exported stage.
        for layer in layers:
            if layer["role"] == "folder":
                continue
            w = layer["right"] - layer["left"]
            h = layer["bottom"] - layer["top"]
            if (layer["role"] == "actor" or
                    (layer["visible"] and w > 0 and h > 0)):
                if layer["index"] not in previous_layers:
                    raise ValueError(
                        f"New stage layer {layer['index']} detected; run full extraction")
        print(f"GUILE PSD JOE ONLY: {len(positions)} frame(s); all other PNGs and stage layers preserved")
    count = 0
    actor_count = 0
    for layer in layers:
        # The PSD channel stream is sequential. Skip every unrelated
        # payload verbatim: no decode, PNG rewrite or mask processing.
        if target_actor is not None and not (
                layer["role"] == "actor" and layer["actor"] == target_actor):
            for _, length in layer["channels"]:
                p.skip(length)
            continue
        planes = {}
        w,h = layer["right"]-layer["left"],layer["bottom"]-layer["top"]
        # Always consume this layer's entire channel block, even if the
        # group marker has no pixels or the artwork is hidden.
        for ident,length in layer["channels"]:
            payload = p.take(length)
            if w>0 and h>0 and ident in (0,1,2,-1,-2) and length>=2:
                if ident == -2 and layer["mask"]:
                    mt,ml,mb,mr,_,_=layer["mask"]
                    mw,mh=mr-ml,mb-mt
                    if mw>0 and mh>0:
                        try: planes[-2] = decode_plane(payload,mw,mh)
                        except ValueError: pass
                else:
                    planes[ident] = decode_plane(payload,w,h)
        role = layer["role"]
        if role in ("folder","folder_end"):
            continue
        if role != "actor" and (not layer["visible"] or w<=0 or h<=0 or not planes):
            continue
        if role == "actor":
            # Export even hidden and empty child layers; the animation's
            # membership is the literal PSD Characters folder, not visibility.
            actor_count += 1
        size = w*h
        gray = planes.get(0,b"\0"*size)
        red = gray
        green = planes.get(1,gray) if mode == 3 else gray
        blue = planes.get(2,gray) if mode == 3 else gray
        alpha = planes.get(-1,b"\xff"*size)
        opacity = layer["opacity"]
        mask = layer["mask"]
        mask_plane = planes.get(-2)
        # Slice assignments run in C instead of making four Python writes
        # for every pixel. The first build may extract many large PSD layers.
        rgba = bytearray(size*4)
        rgba[0::4] = red
        rgba[1::4] = green
        rgba[2::4] = blue
        if mask is None or mask_plane is None:
            rgba[3::4] = (alpha if opacity == 255
                         else alpha.translate(bytes((i*opacity//255)
                                                     for i in range(256))))
        else:
            mt,ml,mb,mr,default,mflags = mask
            mw,mh = mr-ml,mb-mt
            # Build mask alpha in its own plane, then interleave once.
            composite = bytearray(size)
            for y in range(h):
                ty=layer["top"]+y-mt
                for x in range(w):
                    i=y*w+x
                    tx=layer["left"]+x-ml
                    mask_value=(mask_plane[ty*mw+tx]
                                if 0<=tx<mw and 0<=ty<mh else default)
                    if mflags&4: mask_value=255-mask_value
                    composite[i]=alpha[i]*opacity*mask_value//(255*255)
            rgba[3::4] = composite
        filename = f"{layer['index']:04d}.png"
        if role == "actor":
            actor = layer["actor"]
            bounds = actor_bounds[actor]
            texture = padded_rgba(bytes(rgba),w,h,layer["left"],layer["top"],bounds)
            image_x,image_y = bounds[0],bounds[1]
            image_w,image_h = bounds[2]-bounds[0],bounds[3]-bounds[1]
        elif role == "ground" and pad_ground:
            # Restore transparent padding; never scale trimmed pixels to fit.
            # Original layer bounds remain recorded in the manifest columns.
            actor = "-"
            texture = padded_rgba(bytes(rgba),w,h,layer["left"],layer["top"],
                                  (gl,gt,gr,gb))
            image_x,image_y = gl,gt
            image_w,image_h = gw,gh
        else:
            actor = "-"
            texture = bytes(rgba)
            image_x,image_y = layer["left"],layer["top"]
            image_w,image_h = w,h
        png_data = png_rgba(image_w,image_h,texture)
        if target_actor is None:
            (out/filename).write_bytes(png_data)
        else:
            pending_pngs[filename] = png_data
            print(f"GUILE PSD JOE FRAME {layer['name']}: "
                  f"source=({layer['left']},{layer['top']},{layer['right']},{layer['bottom']}) "
                  f"padded=({image_x},{image_y},{image_w},{image_h})")
        # The first nine columns are the renderer contract. Additional columns
        # index the exact PSD layer name, hierarchy, ORIGINAL pixel bounds and
        # visibility. The PNG coordinates may describe a padded actor canvas.
        original = (layer["left"],layer["top"],layer["right"],layer["bottom"])
        path = "/".join(sanitize_field(x) for x in layer["path"])
        new_record = (
            f"LAYER\t{role}\t{sanitize_field(actor)}\t{layer['index']}\t"
            f"{image_x}\t{image_y}\t{image_w}\t{image_h}\t{filename}\t"
            f"{sanitize_field(layer['name'])}\t{path}\t"
            f"{original[0]}\t{original[1]}\t{original[2]}\t{original[3]}\t"
            f"{int(layer['visible'])}"
        )
        if target_actor is None:
            records.append(new_record)
        else:
            records[positions[layer["index"]]] = new_record
        count += 1
    if target_actor is not None:
        if count != len(actors[target_actor]) or set(pending_pngs) != {
                f"{layer['index']:04d}.png" for layer in actors[target_actor]}:
            raise ValueError("Joe-only PNG inventory incomplete; no files were replaced")
        # Commit the PNGs after successful decoding of EVERY Joe frame.
        # Keep the original full-export SHA as a stale marker; a later
        # normal build will correctly revalidate/re-export the whole PSD.
        for filename,payload in pending_pngs.items():
            pending = out / (filename + ".joe.pending")
            pending.write_bytes(payload)
            os.replace(pending, out / filename)
        records.append(f"PARTIAL_ACTOR\t{target_actor}\t{hashlib.sha256(data).hexdigest()}")
        pending_manifest = out / "manifest.tsv.joe.pending"
        pending_manifest.write_text("\n".join(records)+"\n",encoding="utf-8")
        os.replace(pending_manifest, out / "manifest.tsv")
        print(f"GUILE PSD JOE ONLY DONE: {count} frames rewritten; "
              "all other stage PNGs untouched; full SHA validation deferred")
        return records
    if not count:
        raise ValueError("PSD contains no rasterizable stage layers")
    if actor_count != sum(len(frames) for frames in actors.values()):
        raise ValueError(
            f"Characters extraction incomplete: exported {actor_count} of "
            f"{sum(len(frames) for frames in actors.values())} Photoshop children")
    records.insert(1,f"SOURCE_SHA256\t{hashlib.sha256(data).hexdigest()}")
    records.insert(2,f"EXTRACTOR_SHA256\t{hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}")
    records.insert(2,f"SUMMARY\t{count}\t{actor_count}")
    for key,frames in actors.items():
        records.append(f"ACTOR\t{sanitize_field(key)}\t{len(frames)}")
    (out/"manifest.tsv").write_text("\n".join(records)+"\n",encoding="utf-8")
    print(f"GUILE PSD INDEX ground=({gl},{gt},{gr},{gb}) visible=768x471 centered_crop={view_x} actor_groups={len(actors)}")
    for key,frames in actors.items():
        print(f"GUILE PSD ACTOR {key}: {len(frames)} / {len(frames)} frames, bounds={actor_bounds[key]}")
    return records

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,default=SOURCE)
    ap.add_argument("--output",type=Path,default=OUT)
    ap.add_argument("--actor",metavar="NAME",
                    help="re-extract only this actor's layers (e.g. Joe); "
                         "requires the existing full manifest")
    args=ap.parse_args()
    if not args.source.is_file():
        print(f"GUILE PSD missing: {args.source}",file=sys.stderr)
        return 2
    raw=args.source.read_bytes()
    manifest=args.output/"manifest.tsv"
    digest=hashlib.sha256(raw).hexdigest()
    extractor_digest=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if manifest.is_file():
        cached = manifest.read_text(encoding="utf-8")
        if (not args.actor and "PARTIAL_ACTOR\t" not in cached and
                f"SOURCE_SHA256\t{digest}" in cached and
                f"EXTRACTOR_SHA256\t{extractor_digest}" in cached and
                "FORMAT\t3" in cached):
            # A manifest can survive the deletion of exported PNGs.
            # Regenerate instead of serving a partly missing animation.
            exports = []
            for line in cached.splitlines():
                parts = line.split("\t")
                if len(parts) >= 9 and parts[0] == "LAYER":
                    exports.append(args.output / parts[8])
            if exports and all(path.is_file() for path in exports):
                print(f"GUILE PSD unchanged: {manifest} ({len(exports)} layers)")
                return 0
    try:
        rows=extract(raw,args.output,args.actor)
    except (ValueError,struct.error,zlib.error) as ex:
        print(f"GUILE PSD extraction failed: {ex}",file=sys.stderr)
        return 1
    if not args.actor:
        print(f"GUILE PSD extracted: {rows[0]}, {rows[2]}, {manifest}")
    return 0

if __name__=="__main__":
    sys.exit(main())
