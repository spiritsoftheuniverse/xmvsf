#!/usr/bin/env python3
"""Convert original CPS2 render-pass captures into pixel-exact RGBA PNGs.

Uses the Stage44/45 dual-clear renderer, not guessed tilemap palettes/camera.
All files are local-only; no ROM graphics are committed to GitHub.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib

MAGIC = b"\x89PNG\r\n\x1a\n"
SAFE_RGBA = re.compile(r"l([0-3])_(\d{6})\.rgba\Z", re.ASCII)
FNV_BASE = 14695981039346656037
FNV_PRIME = 1099511628211


def fnv64(data: bytes) -> int:
    result = FNV_BASE
    for byte in data:
        result = ((result ^ byte) * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return result


def png_bytes(width: int, height: int, rgba: bytes) -> bytes:
    if width < 1 or height < 1 or width > 2048 or height > 2048:
        raise ValueError("invalid unfiltered CPS2 image dimensions")
    if len(rgba) != width * height * 4:
        raise ValueError("truncated native RGBA frame")

    def chunk(name: bytes, data: bytes) -> bytes:
        body = name + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    scanlines = b"".join(b"\x00" + rgba[y * width * 4:(y + 1) * width * 4]
                         for y in range(height))
    return MAGIC + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) + \
        chunk(b"IDAT", zlib.compress(scanlines, 6)) + chunk(b"IEND", b"")


def export(input_dir: Path, output_dir: Path) -> dict:
    input_dir = input_dir.resolve()
    output_dir = output_dir.resolve()
    if not input_dir.is_dir():
        raise ValueError("native source shot directory not found")
    if output_dir == input_dir or input_dir in output_dir.parents or output_dir in input_dir.parents:
        raise ValueError("refuse to nest output in source or overwrite source")
    if output_dir.exists():
        raise FileExistsError(f"output directory already exists: {output_dir}")
    if not re.fullmatch(r"shot_\d{4}", input_dir.name):
        raise ValueError("select one original Stage44 shot_XXXX directory")
    index_path = input_dir / "layers.tsv"
    with index_path.open("r", newline="", encoding="utf-8") as fp:
        rows = list(csv.DictReader(fp, delimiter="\t"))
    if not rows:
        raise ValueError("no native frame records")
    required = {"sample", "emulated_frame", "layer", "file", "fnv64",
                "width", "height", "scroll_x", "scroll_y"}
    if not required.issubset(rows[0]):
        raise ValueError("missing Stage44 source layer fields")

    frame_groups = {}
    verified = []
    hashes = {}
    for row in rows:
        sample, frame, layer = (int(row[k]) for k in ("sample", "emulated_frame", "layer"))
        width, height = int(row["width"]), int(row["height"])
        name = row["file"]
        match = SAFE_RGBA.fullmatch(name)
        if not match or int(match.group(1)) != layer or int(match.group(2)) != sample:
            raise ValueError("unsafe or inconsistent source filename")
        if width != 384 or height != 224:
            raise ValueError("unexpected CPS2 viewport dimensions")
        path_in = input_dir / name
        data = path_in.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        expected = int(row["fnv64"], 16)
        if fnv64(data) != expected:
            raise ValueError(f"native FNV64 mismatch: {name}")
        if len(data) != width * height * 4:
            raise ValueError(f"native RGBA length mismatch: {name}")
        if sample not in frame_groups:
            frame_groups[sample] = {"native_frame": frame, "layers": {}}
        group = frame_groups[sample]
        if group["native_frame"] != frame or layer in group["layers"]:
            raise ValueError("duplicate layer record or out-of-sync source frame")
        verified.append((sample, layer, digest, data, width, height, row))
        group["layers"][layer] = None
    samples = sorted(frame_groups)
    if samples != list(range(len(samples))):
        raise ValueError("sample sequence has gaps")
    if any(set(v["layers"]) != set(range(4)) for v in frame_groups.values()):
        raise ValueError("each native sample requires composite + all three isolated scroll layers")
    if any(frame_groups[b]["native_frame"] <= frame_groups[a]["native_frame"]
           for a, b in zip(samples, samples[1:])):
        raise ValueError("source emulated frame timeline is not increasing")

    output_dir.mkdir(parents=True, exist_ok=False)
    png_dir = output_dir / "frames"
    png_dir.mkdir()
    repeated = {}
    all_timestamps = []
    png_count = 0
    for sample, layer, digest, data, width, height, row in verified:
        # Deduplicate only within the *same* layer, preserving temporal sample
        # references even when the source artwork remains unchanged.
        key = (layer, digest)
        if key not in repeated:
            name = f"l{layer}_{sample:06d}.png"
            blob = png_bytes(width, height, data)
            (png_dir / name).write_bytes(blob)
            repeated[key] = {"path": "frames/" + name,
                             "png_sha256": hashlib.sha256(blob).hexdigest(),
                             "rgba_sha256": digest}
            png_count += 1
        frame_groups[sample]["layers"][layer] = {
            **repeated[key],
            "scroll_x": int(row["scroll_x"]),
            "scroll_y": int(row["scroll_y"]),
            "native_fnv64": row["fnv64"],
        }
    for sample in samples:
        all_timestamps.append({"sample": sample, **frame_groups[sample]})

    original_marker = input_dir / "CAPTURE_COMPLETE.txt"
    finished = original_marker.is_file()
    manifest = {
        "format": "XMVSF_STAGE87_CPS2_CLEAN_RIP_V1",
        "method": "Kawaks-style original FBNeo Cps2Layers dual-clear isolation",
        "source_session": input_dir.name,
        "emulator_rendered": True,
        "separate_layers": [0, 1, 2, 3],
        "composite_excludes_gameplay_sprites": True,
        "rgba_transparency": "original two-fill equality mask, preserves native black pixels",
        "source_gfx_recalculated": False,
        "native_frames": len(samples),
        "distinct_pngs": png_count,
        "duplicates_deduplicated": len(verified) - png_count,
        "burst_finished": finished,
        "level_complete": False,
        "stage_boundaries_verified": False,
        "animation_cycles_verified": False,
        "scenery_sprite_actors_separated": False,
        "unresolved_gaps": [
            "camera excursion does not prove whole-stage geography",
            "source OAM includes fighters and HUD; background sprite actors not identified",
            "complete animation periods and stage identity not proven",
        ],
        "frames": all_timestamps,
    }
    (output_dir / "clean_rip_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    with (output_dir / "timeline.tsv").open("w", newline="", encoding="utf-8") as fp:
        writer = csv.writer(fp, delimiter="\t")
        writer.writerow(("sample", "emulated_frame", "layer", "png", "scroll_x", "scroll_y", "sha256"))
        for item in all_timestamps:
            for layer, desc in sorted(item["layers"].items()):
                writer.writerow((item["sample"], item["native_frame"], layer, desc["path"],
                                 desc["scroll_x"], desc["scroll_y"], desc["rgba_sha256"]))
    print(f"CLEAN_RIP_EXPORTED source={input_dir.name} native_samples={len(samples)} "
          f"distinct_png={png_count} original_layers=4 finished={int(finished)}")
    print("INCOMPLETE: these are source-clean frames; whole-stage coverage and animation loops unverified")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True, help="existing clean_rips/shot_XXXX")
    parser.add_argument("--output", type=Path, required=True, help="NEW output directory")
    args = parser.parse_args()
    export(args.input, args.output)


if __name__ == "__main__":
    main()
