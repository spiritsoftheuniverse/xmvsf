#!/usr/bin/env python3
"""87.497: verify the EXACT Final Destination XMFDBIN2 pack, no rescale."""
from __future__ import annotations

import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "_art/menu/characterselect/csbackground"
PACK = ROOT / "_art/menu/characterselect/csbackground.fdb"
MAGIC = b"XMFDBIN2"
PNG = b"\x89PNG\r\n\x1a\n"
FIRST = 1124302
LAST = 1124467
COUNT = LAST - FIRST + 1
W, H = 384, 224


def sources():
    files = sorted(SOURCE.glob("csbackground*.png"))
    assert len(files) == COUNT, f"expected {COUNT} native frames, got {len(files)}"
    for i, path in enumerate(files, FIRST):
        assert path.name == f"csbackground0{i}.png", (
            f"missing/extra/out-of-order source frame {i}: {path.name}")
        with path.open("rb") as f:
            header = f.read(26)
        assert header.startswith(PNG) and header[12:16] == b"IHDR"
        w, h = struct.unpack_from(">II", header, 16)
        assert (w, h) == (W, H), f"{path.name} must remain native 384x224"
        assert header[24] == 8 and header[25] in (2, 6), (
            f"{path.name} must be 8-bit RGB/RGBA PNG")
    return files


def verify_pack(files):
    assert PACK.exists(), f"missing packed background {PACK}"
    with PACK.open("rb") as f:
        header = f.read(32)
        assert len(header) == 32
        magic, w, h, count, fmt, row, reserved = struct.unpack("<8s6I", header)
        assert (magic, w, h, count, fmt, row, reserved) == (
            MAGIC, W, H, COUNT, 2, W * 4, 0), "invalid XMFDBIN2 header"
        size = PACK.stat().st_size
        payload_start = 32 + COUNT * 16
        assert size >= payload_start
        index = []
        end = payload_start
        for i in range(COUNT):
            offset, nbytes, pad = struct.unpack("<QII", f.read(16))
            assert pad == 0 and nbytes >= 8 and offset == end
            assert offset + nbytes <= size
            index.append((offset, nbytes))
            end = offset + nbytes
        assert end == size, "trailing bytes or incomplete pack"
        for source, (offset, nbytes) in zip(files, index):
            f.seek(offset)
            packed = f.read(nbytes)
            assert packed == source.read_bytes(), (
                f"packed frame is stale or damaged: {source.name}")


def main():
    files = sources()
    original = (ROOT / "tools/pack_fd_video.ps1").read_text(encoding="utf-8")
    wrapper = (ROOT / "tools/pack_csbackground_fdb.ps1").read_text(encoding="utf-8")
    assert "'XMFDBIN2'" in original and "FrameGlob" in original
    assert "pack_fd_video.ps1" in wrapper
    assert "384" in wrapper and "224" in wrapper
    verify_pack(files)
    print(f"XMVSF 87.497 XMFDBIN2 native 384x224 frame payload integrity PASS ({COUNT} frames)")


if __name__ == "__main__":
    main()
