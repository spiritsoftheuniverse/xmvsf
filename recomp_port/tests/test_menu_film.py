#!/usr/bin/env python3
"""87.460: verify 80px-closer overlapping films, random order, dither and 2x speed."""
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
        "kGap = -64", "kPortraitTop = 78", "kMargin = 16",
        "kPortraitInset = 8", "static Rect SafePortraitArea(",
        "const Rect visible = VisibleBounds(portrait)",
        "const double scale = std::min(", "MaskPortrait(column.shape, person, safe)",
        "kSpeedPixelsPerSecond = 20.0", "menu/menufilmL.png",
        "menu/menufilm.png", '"xm", true', '"sf", false',
        "SDL_SCALEMODE_NEAREST", "column.shape.height + kGap",
        "static Image MaskPortrait(", "const unsigned maskAlpha = film.pixels[at] >> 24",
        "const unsigned portraitAlpha = person >> 24",
        "out.pixels[at] = (person & 0x00ffffffu)",
        "SDL_RenderTexture(renderer_, column.plain, nullptr, &dest)",
        "SDL_RenderTexture(renderer_, portrait, nullptr, &dest)",
        "menu/characters/", "std::shuffle(column.order.begin(), column.order.end(), random)",
        "std::iota(column.order.begin(), column.order.end(), size_t(0))",
        "column.portraitImages.push_back(std::move(clipped))",
        "column.order[size_t((ordinal % count + count) % count)]",
        "void ApplyReveal(Column &column, int level)",
        "if (column.revealLevel == level) return;",
        "constexpr uint8_t bayer[16]", "SDL_UpdateTexture(texture, nullptr, pixels, source.width * 4)",
        "for (int slot = -2; slot <= 4; ++slot)",
        "DrawColumn(xmen_, now, reveal)",
    ):
        assert requirement in film, f"film requirement missing: {requirement}"
    for requirement in (
        '#include "xmvsf_menu_film.h"', "MenuFilmStrips films_;",
        "films_.Init(r);", "films_.Release();", "films_.Draw(renderer, now, filmReveal);",
    ):
        assert requirement in tex, f"menu hook missing: {requirement}"
    assert tex.index("DrawBackground(renderer,now,backgroundStrength);") < tex.index(
        "films_.Draw(renderer, now, filmReveal);") < tex.index("if (elapsed >= XM87235Intro::LogoStart)")
    assert "elapsed, XM87235Intro::TotalMs, XM87235Intro::MenuMs" in tex
    assert "const int filmReveal = !clock.started ? 16 :" in tex
    timeline = (ROOT / "recomp_port" / "xmvsf_intro_timeline.h").read_text(encoding="utf-8")
    assert "static constexpr std::uint32_t MenuMs=500u;" in timeline
    assert "static constexpr std::uint32_t TotalMs=MenuStart+MenuMs;" in timeline
    assert 'FILM_SOURCE = ROOT / "recomp_port" / "xmvsf_menu_film.h"' in installer
    assert "FILM_HEADER.write_bytes(film_content)" in installer

    # Move panels 80px closer: old 469px pitch -> 389px.
    # Their tapered tips overlap by 64px; no invisible gap remains.
    step = 453 - 64
    assert step == 389
    assert 469 - step == 80
    assert 960 - 16 - 260 == 684
    assert "Compose(" not in film
    for down in (False, True):
        for phase in (0, 1, 64, 194, 388):
            panels = [slot * step + (phase if down else -phase)
                      for slot in range(-2, 5)]
            assert all(panels[i + 1] - (panels[i] + 453) == -64
                       for i in range(len(panels) - 1)), (down, phase)
        for slot in range(-2, 5):
            before_y = slot * step + (step if down else -step)
            next_slot = slot + (1 if down else -1)
            after_y = next_slot * step
            assert before_y == after_y
            before_index = slot
            after_index = next_slot + (-1 if down else 1)
            assert before_index == after_index

    # The same Bayer threshold order as menu text; zero means invisible,
    # sixteen reveals every original pixel without alpha/channel changes.
    bayer = (0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5)
    for level in range(17):
        assert sum(t < level for t in bayer) == level
    assert "if (!column.shape.Valid() || reveal == 0) return;" in film
    assert "const uint32_t *pixels = source.pixels.data();" in film
    assert "ditherPixels_[at] = bayer[(y & 3) * 4 + (x & 3)] < level ?" in film
    # Clock: text reaches full visibility at 3167ms, film is dark until
    # that instant, then takes 500ms to finish its own 16-step dither.
    menu_start, text_duration = 2667, 500
    text_complete = menu_start + text_duration
    def film_level(elapsed: int) -> int:
        return min(16, max(0, int(16 * min(1.0, max(
            0.0, (elapsed - text_complete) / text_duration)))))
    assert all(film_level(t) == 0 for t in (0, menu_start, 3100, 3167))
    assert 0 < film_level(3417) < 16
    assert film_level(3667) == 16

    # Shuffling occurs once on load, not on every scroll frame.
    load = film.split("void LoadColumn(", 1)[1].split("void DrawColumn(", 1)[0]
    draw = film.split("void DrawColumn(", 1)[1].split("static void Clear(", 1)[0]
    assert "std::shuffle(" in load
    assert "std::shuffle(" not in draw
    assert "std::random_device{}()" in load
    assert "column.order.resize(column.portraits.size())" in load

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
    print("XMVSF 87.460 film: 18 shuffled portraits, 389px pitch, 20px/s, post-text Bayer PASS")

if __name__ == "__main__":
    main()
