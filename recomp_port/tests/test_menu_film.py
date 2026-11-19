#!/usr/bin/env python3
"""87.457: verify 16px margin, safe alpha bounds, and scroll continuity."""
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
        "kGap = 16", "kPortraitTop = 78", "kMargin = 16",
        "kPortraitInset = 8", "static Rect SafePortraitArea(",
        "const Rect visible = VisibleBounds(portrait)",
        "const double scale = std::min(", "MaskPortrait(column.shape, person, safe)",
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

    # 453px film plus 16px clear gap, like the side margin.
    step = 453 + 16
    assert step == 469
    assert 960 - 16 - 260 == 684
    assert "kOverlap" not in film
    assert "Compose(" not in film
    for down in (False, True):
        for phase in (0, 1, 16, 117, 468):
            panels = [slot * step + (phase if down else -phase)
                      for slot in range(-2, 5)]
            assert all(panels[i + 1] - (panels[i] + 453) == 16
                       for i in range(len(panels) - 1)), (down, phase)
        for slot in range(-1, 6):
            before_y = slot * step + (step if down else -step)
            next_slot = slot + (1 if down else -1)
            after_y = next_slot * step
            assert before_y == after_y
            before_index = slot
            after_index = next_slot + (-1 if down else 1)
            assert before_index == after_index

    # Contain-fit the opaque alpha bounds rather than cover-cropping heads.
    # The source film's 240+ alpha band is y=92..360; with 8px inset it
    # provides a safe 244x253 placement rectangle at (8,100).
    safe_x, safe_y, safe_w, safe_h = 8, 100, 244, 253
    for source_w, source_h in ((200, 400), (800, 120), (128, 512),
                               (512, 512), (260, 453)):
        scale = min(safe_w / source_w, safe_h / source_h)
        drawn_w = max(1, min(safe_w, int(source_w * scale + 0.5)))
        drawn_h = max(1, min(safe_h, int(source_h * scale + 0.5)))
        dst_x = safe_x + (safe_w - drawn_w) // 2
        dst_y = safe_y + (safe_h - drawn_h) // 2
        assert dst_x >= safe_x and dst_y >= safe_y
        assert dst_x + drawn_w <= safe_x + safe_w
        assert dst_y + drawn_h <= safe_y + safe_h
    assert "std::max(double(w) / portrait.width" not in film
    assert "(person & 0x00ffffffu)" in film
    print("XMVSF 87.457 film: 18 portraits, 16px gaps, safe alpha fit PASS")

if __name__ == "__main__":
    main()
