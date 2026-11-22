#!/usr/bin/env python3
"""87.496: verify the new selection font is the actual 144x115 PNG atlas."""
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
PNG = ROOT / "_art" / "sf2fontlarge.png"


def rgba_png(path: Path):
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", "missing source PNG"
    pos, bands = 8, []
    width = height = 0
    while pos + 12 <= len(data):
        length = struct.unpack_from(">I", data, pos)[0]
        kind = data[pos+4:pos+8]
        body = data[pos+8:pos+8+length]
        if kind == b"IHDR":
            width, height, depth, color = struct.unpack_from(">IIBB", body)
            assert (width, height, depth, color) == (144, 115, 8, 6), (
                "sf2fontlarge.png must remain the decoded 12-column "
                "12px-wide x 15px-row atlas"
            )
        elif kind == b"IDAT":
            bands.append(body)
        elif kind == b"IEND":
            break
        pos += length + 12
    assert bands
    source = zlib.decompress(b"".join(bands))
    stride, bpp = width * 4, 4
    pixels = bytearray(stride * height)
    for y in range(height):
        at = y * (stride + 1)
        filter_id = source[at]
        assert filter_id in range(5), "unsupported PNG row filter"
        for i in range(stride):
            v = source[at + 1 + i]
            a = pixels[y*stride+i-bpp] if i >= bpp else 0
            b = pixels[(y-1)*stride+i] if y else 0
            c = pixels[(y-1)*stride+i-bpp] if y and i >= bpp else 0
            if filter_id == 1: v += a
            elif filter_id == 2: v += b
            elif filter_id == 3: v += (a+b)//2
            elif filter_id == 4:
                p = a+b-c
                da, db, dc = abs(p-a), abs(p-b), abs(p-c)
                v += a if da <= db and da <= dc else b if db <= dc else c
            pixels[y*stride+i] = v & 255
    return width, height, pixels


def main():
    w, h, pixels = rgba_png(PNG)
    # This is a different layout from sf2font.png (16x12x12).
    # Confirm real nontransparent ink in representative glyph cells:
    # A/L, M/X, Y/Z, lowercase, countdown 0/9, and name punctuation.
    cells = {"A": 0, "L": 11, "M": 12, "X": 23, "Y": 24,
             "Z": 25, "a": 26, "z": 51, ".": 52,
             "0": 60, "9": 69, "-": 79}
    for char, index in cells.items():
        x0, y0 = (index % 12) * 12, (index // 12) * 15
        x1, y1 = min(x0 + 12, w), min(y0 + 15, h)
        count = sum(pixels[(y*w+x)*4+3] > 12
                    for y in range(y0, y1) for x in range(x0, x1))
        assert count, f"glyph {char!r} at cell {index} is blank"
    home = (ROOT/"recomp/port/xmvsf_home.h").read_text(encoding="utf-8")
    sdl = (ROOT/"recomp/port/xmvsf_menu_textures.h").read_text(encoding="utf-8")
    assert 'selectionLarge_.LoadSelection("sf2fontlarge.png");' in home
    assert "selectionLargeFont_ = MakeTexture(r, menu_.selectionLarge_.sheet);" in sdl
    assert "CharacterSelectText(SDL_Renderer *r" in sdl
    assert "CharacterSelectCountdown(SDL_Renderer *r" in sdl
    assert sdl.count("selectionLargeFont_") >= 5
    assert (ROOT/"recomp_port/xmvsf_home.h").read_bytes() == (
        ROOT/"recomp/port/xmvsf_home.h").read_bytes()
    assert (ROOT/"recomp_port/xmvsf_menu_textures.h").read_bytes() == (
        ROOT/"recomp/port/xmvsf_menu_textures.h").read_bytes()
    assert (ROOT/"recomp_port/xmvsf_character_select_font.h").read_bytes() == (
        ROOT/"recomp/port/xmvsf_character_select_font.h").read_bytes()
    gpp = shutil.which("g++")
    assert gpp, "UCRT64 g++ required for font mapping smoke"
    with tempfile.TemporaryDirectory(prefix="xmvsf_font496_") as folder:
        exe = Path(folder) / "font_atlas.exe"
        subprocess.run([gpp, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(ROOT/"recomp_port/tests/test_sf2fontlarge_cpp.cpp"),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("XMVSF 87.496 12x15 SF2 font atlas, glyphs, SDL registry, mirrors PASS")


if __name__ == "__main__":
    main()
