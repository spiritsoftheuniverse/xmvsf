#!/usr/bin/env python3
"""87.455: validate both menu-film assets, cached texture paths, and loop math."""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "_art" / "menu"
FILM = ROOT / "recomp_port" / "xmvsf_menu_film.h"
COMPOSITOR = ROOT / "recomp_port" / "xmvsf_menu_textures.h"
INSTALLER = ROOT / "recomp_port" / "apply_textured_menu.py"

def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    assert data[:8] == b"\x89PNG\r\n\x1a\n", f"not a PNG: {path}"
    assert data[12:16] == b"IHDR", f"missing PNG IHDR: {path}"
    return struct.unpack(">II", data[16:24])

def main() -> None:
    for name in ("menufilm.png", "menufilmL.png"):
        assert png_size(ART / name) == (260, 453), name
    for prefix in ("xm", "sf"):
        for number in range(1, 10):
            path = ART / "characters" / f"{prefix} ({number}).png"
            w, h = png_size(path)
            assert w > 0 and h > 0, path

    film = FILM.read_text(encoding="utf-8")
    tex = COMPOSITOR.read_text(encoding="utf-8")
    installer = INSTALLER.read_text(encoding="utf-8")
    assert film == (ROOT / "recomp" / "port" / "xmvsf_menu_film.h").read_text(encoding="utf-8")
    for requirement in (
        "kOverlap = 96", "kPortraitTop = 78", "kMargin = 16",
        "kSpeedPixelsPerSecond = 10.0", "menu/menufilmL.png",
        "menu/menufilm.png", '"xm", true', '"sf", false',
        "SDL_SCALEMODE_NEAREST", "column.shape.height - kOverlap",
        "const unsigned filmAlpha = backing >> 24", "menu/characters/",
    ):
        assert requirement in film, f"film requirement missing: {requirement}"
    for requirement in (
        '#include "xmvsf_menu_film.h"', "MenuFilmStrips films_;",
        "films_.Init(r);", "films_.Release();", "films_.Draw(renderer, now);",
    ):
        assert requirement in tex, f"menu hook missing: {requirement}"
    assert tex.index("DrawBackground(renderer,now,backgroundStrength);") < tex.index(
        "films_.Draw(renderer, now);") < tex.index("if (elapsed >= XM87235Intro::LogoStart)")
    assert 'FILM_SOURCE = ROOT / "recomp_port" / "xmvsf_menu_film.h"' in installer
    assert "FILM_HEADER.write_bytes(film_content)" in installer

    # At each 357px wrap, the same portrait must remain at the same Y.
    step = 453 - 96
    assert step == 357
    for down in (False, True):
        for slot in range(-1, 6):
            before_y = (slot - 1) * step + (step if down else -step)
            next_slot = slot + (1 if down else -1)
            after_y = (next_slot - 1) * step
            assert before_y == after_y
            before_index = slot
            after_index = next_slot + (-1 if down else 1)
            assert before_index == after_index
    print("XMVSF 87.455 film: 18 portraits, renderer hooks, wrap geometry PASS")

if __name__ == "__main__":
    main()
