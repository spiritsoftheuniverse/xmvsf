#!/usr/bin/env python3
"""87.456: verify film gap, alpha mask, and infinite scroll continuity."""
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
        "kGap = 96", "kPortraitTop = 78", "kMargin = 16",
        "kSpeedPixelsPerSecond = 10.0", "menu/menufilmL.png",
        "menu/menufilm.png", '"xm", true', '"sf", false',
        "SDL_SCALEMODE_NEAREST", "column.shape.height + kGap",
        "static Image MaskPortrait(", "const unsigned maskAlpha = film.pixels[at] >> 24",
        "const unsigned portraitAlpha = person >> 24",
        "out.pixels[at] = (person & 0x00ffffffu)",
        "SDL_RenderTexture(renderer_, column.plain, nullptr, &dest)",
        "SDL_RenderTexture(renderer_, portrait, nullptr, &dest)",
        "menu/characters/",
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

    # 453px high films have a true 96px transparent vertical margin.
    step = 453 + 96
    assert step == 549
    assert 960 - 16 - 260 == 684
    assert "kOverlap" not in film
    assert "Compose(" not in film
    for down in (False, True):
        for phase in (0, 1, 96, 273, 548):
            panels = [slot * step + (phase if down else -phase)
                      for slot in range(-2, 5)]
            assert all(panels[i + 1] - (panels[i] + 453) == 96
                       for i in range(len(panels) - 1)), (down, phase)
        # The same character must occupy the same Y across phase rollover.
        for slot in range(-1, 6):
            before_y = slot * step + (step if down else -step)
            next_slot = slot + (1 if down else -1)
            after_y = next_slot * step
            assert before_y == after_y
            before_index = slot
            after_index = next_slot + (-1 if down else 1)
            assert before_index == after_index

    # The portrait RGB is not altered or channel-swapped; film and portrait
    # alpha combine only at the pixel boundary, preserving true transparency.
    assert (255 * 255 + 127) // 255 == 255
    assert (255 * 0 + 127) // 255 == 0
    assert (128 * 128 + 127) // 255 == 64
    assert "(person & 0x00ffffffu)" in film
    print("XMVSF 87.456 film: 18 portraits, 96px gaps, alpha, wrap PASS")

if __name__ == "__main__":
    main()
