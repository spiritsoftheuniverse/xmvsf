#!/usr/bin/env python3
"""87.498: native 18-slot composition assets, FDBs, mirrored code, C++ navigation smoke."""
from __future__ import annotations
import pathlib
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
ART = ROOT / "_art/menu/characterselect"
SRC = ROOT / "recomp/port"
MIRROR = ROOT / "recomp_port"

def fdb(path: pathlib.Path, expected):
    with path.open("rb") as fh:
        magic, width, height, frames, fmt, stride, unused = struct.unpack(
            "<8s6I", fh.read(32))
        assert (magic, width, height, frames, fmt, stride, unused) == (
            b"XMFDBIN2", *expected, 2, expected[0] * 4, 0), path
        last_end = 32 + 16 * frames
        total = path.stat().st_size
        for _ in range(frames):
            offset, size, reserved = struct.unpack("<QII", fh.read(16))
            assert offset == last_end and size >= 8 and reserved == 0
            last_end = offset + size
            assert last_end <= total
        assert last_end == total

def main():
    for name in ["xmvsf_character_select_layout.h",
                 "xmvsf_character_select_backdrop.h",
                 "xmvsf_character_select_screen.h",
                 "xmvsf_menu_textures.h"]:
        assert (SRC / name).read_bytes() == (MIRROR / name).read_bytes(), name
    fdb(ART / "csbackground.fdb", (384, 224, 166))
    fdb(ART / "portraitbackground.fdb", (160, 160, 89))
    layout = (SRC / "xmvsf_character_select_layout.h").read_text()
    portraits = re.findall(r'"(layer_\d+_0x[0-9A-Fa-f]+_[^"]+\.png)"', layout)
    assert len(portraits) == 18
    all_portraits = list((ART / "portraits").glob("*.png"))
    assert len(all_portraits) >= 19
    for name in portraits:
        assert (ART / "portraits" / name).is_file(), name
    layers = ["Characters.png", "character grid.png", "1p Selector.png",
              "2p Selector.png", "Portrait Mask L.png",
              "Portrait Mask R.png", "Stage Brace.png"]
    for name in layers:
        image = ART / "screencomposition" / name
        with image.open("rb") as fh: head = fh.read(24)
        assert head[:8] == b"\x89PNG\r\n\x1a\n"
        assert struct.unpack_from(">II", head, 16) == (384, 224), image
    names = (ROOT / "_art/stagethumbs/levelnames.csv").read_text().splitlines()
    assert len(names) >= 18 and names[0] == "Apocalypse Now!"
    # Guard against mistakenly showing the brace before an actual signal.
    screen = (SRC / "xmvsf_character_select_screen.h").read_text()
    assert "stageVisible = false" in screen
    assert 'Cursor(p1Cursor_, 9, a)' in screen
    assert 'Cursor(p2Cursor_, 8, b)' in screen
    frontend = (SRC / "frontend_sdl.cpp").read_text()
    chooser = (ROOT / "recomp/games/xmvsf/src/choosing.cpp").read_text()
    game = (ROOT / "recomp/games/xmvsf/src/game.cpp").read_text()
    assert "homeTextures.ComposeCharacterSelect(" in frontend
    assert "ChoosingLayoutBeforeCpu(machine);" in game
    assert "machine.players[n] &= static_cast<uint16_t>(~kSticks);" in chooser
    assert "session::BeforeFrame(machine);\n        ChoosingLayoutBeforeCpu(machine);" in game
    assert "SetRam16(record + kCursor" in chooser
    gpp = shutil.which("g++")
    assert gpp, "UCRT64 g++ required for selection grid smoke"
    with tempfile.TemporaryDirectory(prefix="xmvsf_cs_layout_") as d:
        binary = pathlib.Path(d) / "select_layout.exe"
        subprocess.run([gpp, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(ROOT / "recomp_port/tests/test_character_select_grid.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("XMVSF 87.498 FDB packs, composition, 18 ROM IDs, selector geometry PASS")

if __name__ == "__main__":
    main()
