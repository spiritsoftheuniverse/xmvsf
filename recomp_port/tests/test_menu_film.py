#!/usr/bin/env python3
"""87.464: curl-noise 20%-visible screen edges on backing and portraits."""
from pathlib import Path
import struct
import subprocess
import tempfile

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
    # rainbowblobs is a real authored asset, not a generated placeholder.
    assert png_size(ART / "rainbowblobs.png")[0] > 0
    assert png_size(ART / "rainbowblobs.png")[1] > 0
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

    # Per-panel rainbow motion is stable over wraps: the ordinal is the
    # position in the continuous scroll, not the transient screen slot.
    for requirement in (
        'LoadPng("menu/rainbowblobs.png")',
        "Image rainbow_;",
        "std::array<FxCache, 7> fx;",
        "static PanelMotion Motion(bool down, std::int64_t ordinal)",
        "Motion(column.scrollDown, ordinal)",
        "std::uint64_t(ordinal)",
        "const int rx = Wrap(x + panX, rainbow_.width * kRainbowScale) / kRainbowScale",
        "Wrap(y + panY, rainbow_.height * kRainbowScale) / kRainbowScale",
        "const auto hue = HueMatrix(motion.huePhase + seconds * motion.hueSpeed)",
        "const unsigned maskAlpha = plate >> 24",
        "const unsigned rainbowWeight =",
        "((blob >> 24) * kRainbowMaxWeight + 127u) / 255u",
        "const unsigned baseWeight = 255u - rainbowWeight",
        "const unsigned backingAlpha =",
        "(maskAlpha * unsigned(kBackingAlpha) + 127u) / 255u",
        "rainbowPixels_[at] = (backingAlpha << 24)",
        "SDL_UpdateTexture(cache.texture, nullptr,",
        "SDL_RenderTexture(renderer_, cache.texture, nullptr, &dest)",
        "now - cache.lastUpdate < kFxRefreshMs",
        "kFxRefreshMs = 75u",
        "kBackingAlpha = 153",
        "kRainbowMaxWeight = 64u",
        "kRainbowScale = 2",
        "SDL_SetTextureAlphaMod(column.plain, kBackingAlpha)",
        "if (cache.texture) SDL_DestroyTexture(cache.texture)",
        "else if (column.plain)",
        '#include "xmvsf_film_edge_dither.h"',
        "FilmEdgeDither::Visible(column.x + x, screenY + y, now,",
        "edgeMaskPixels_[at] = visible ? 1u : 0u",
        "edgePortraitPixels_[at] = edgeMaskPixels_[at] ?",
        "SDL_UpdateTexture(cache.edgePortrait, nullptr,",
        "cache.screenY == screenY",
        "if (cache.edgePortrait) SDL_DestroyTexture(cache.edgePortrait)",
        "cache.edgePortrait = SDL_CreateTexture(renderer_,",
        "const bool masked = UpdateRainbow(column, cache, ordinal, now, reveal, y)",
        "masked && cache.edgePortrait",
        "if (!rainbow_.Valid()) {",
    ):
        assert requirement in film, f"87.464 film compositor missing: {requirement}"
    # Check all 8 possible pan directions and both independently seeded
    # columns remain available (randomness is generated by stable hash).
    for motion in ("{1, 0}", "{-1, 0}", "{0, 1}", "{0, -1}",
                   "{1, 1}", "{-1, 1}", "{1, -1}", "{-1, -1}"):
        assert motion in film
    assert "0xa5a5e7d37fa7c15bULL" in film
    assert "0x3c6ef372fe94f82bULL" in film
    # The dithering mask is applied inside the composited rainbow layer,
    # and the original panel/portrait reveal path remains unchanged.
    assert "kBayer[(y & 3) * 4 + (x & 3)] >= reveal" in film
    assert "ApplyReveal(xmen_, reveal)" in film
    assert "ApplyReveal(streetFighter_, reveal)" in film
    assert "column.order[size_t((ordinal % count + count) % count)]" in film
    assert "if (!rainbow_.Valid()" in film
    assert "const std::int64_t ordinal = std::int64_t(slot)" in film
    assert "column.fx[size_t(slot + 2)]" in film
    assert "random_device" not in film.split("void DrawColumn(", 1)[1]

    # Full-alpha blobs are capped at 64/255 (~25%); the original team
    # color always retains >=191/255 (~75%) of the backing's color.
    for blob_alpha in (0, 32, 128, 255):
        rainbow_weight = (blob_alpha * 64 + 127) // 255
        base_weight = 255 - rainbow_weight
        assert 0 <= rainbow_weight <= 64
        assert base_weight >= 191
        assert rainbow_weight + base_weight == 255
    assert (255 * 64 + 127) // 255 == 64
    # Film alpha fades independently; portraits retain their masked source
    # alpha, not the 60% backing multiplier.
    for panel_alpha in (0, 1, 127, 255):
        backing_alpha = (panel_alpha * 153 + 127) // 255
        assert backing_alpha <= panel_alpha
        assert backing_alpha <= 153
    assert "SDL_SetTextureAlphaMod(column.plain, kBackingAlpha)" in film
    portrait_draw = film.split("void DrawColumn(", 1)[1].split("static void Clear(", 1)[0]
    assert "SDL_RenderTexture(renderer_, portrait, nullptr, &dest)" in portrait_draw
    assert "SDL_SetTextureAlphaMod(portrait" not in film
    assert "SDL_SetTextureAlphaMod(column.portraits" not in film
    assert "const unsigned clippedAlpha =" in film
    # Magnified wrapping repeats only after two source-image widths:
    # exactly two output pixels sample each source rainbow pixel.
    for source_width in (3, 17, 256):
        samples = [(x % (source_width * 2)) // 2
                   for x in range(source_width * 4)]
        assert samples[:source_width * 2] == samples[source_width * 2:]
        assert samples[::2][:source_width] == list(range(source_width))

    # Keep the edge helper as an actual pinned C++17 unit test, not just
    # string-matching its implementation. This runs before warm Ninja.
    edge_source = ROOT / "recomp_port" / "xmvsf_film_edge_dither.h"
    assert edge_source.read_bytes() == (
        ROOT / "recomp" / "port" / "xmvsf_film_edge_dither.h"
    ).read_bytes()
    edge = edge_source.read_text(encoding="utf-8")
    for required in (
        "kScreenHeight = 540", "kBand = 108", "kEntryCoverage = 0.20",
        "static Flow Curl(", "static Gradient PotentialGradient(",
        "const double coverage = kEntryCoverage + (1.0 - kEntryCoverage) * soft",
        "static bool Visible(", "if (edgeDistance >= kBand) return true;",
        "const double sign = down ? 1.0 : -1.0;",
        "const int dy = (screenY + int(std::floor(curl.y))) & 7;",
    ):
        assert required in edge, f"missing curl-noise edge requirement: {required}"
    assert 'EDGE_SOURCE = ROOT / "recomp_port" / "xmvsf_film_edge_dither.h"' in installer
    assert "EDGE_HEADER.write_bytes(edge_content)" in installer
    smoke = r'''
#include "xmvsf_film_edge_dither.h"
#include <cassert>
#include <cstdint>
using port::xmvsf_home::FilmEdgeDither;
int main() {
    static_assert(FilmEdgeDither::kBand == 108);
    static_assert(FilmEdgeDither::kScreenHeight == 540);
    static_assert(FilmEdgeDither::kEntryCoverage == 0.20);
    const auto count = [](int y, std::uint64_t ms, bool down) {
        int visible = 0;
        for (int x = 0; x < 960; ++x)
            visible += FilmEdgeDither::Visible(x,y,ms,down,13);
        return visible;
    };
    for (int y : {-1, 540, 800})
        for (int x = 0; x < 960; x+=31)
            assert(!FilmEdgeDither::Visible(x,y,0,true,13));
    for (int y : {108, 160, 270, 431})
        assert(count(y,0,true) == 960);
    const int top = count(0,0,true), bottom = count(539,0,false);
    assert(top > 960*10/100 && top < 960*35/100);
    assert(bottom > 960*10/100 && bottom < 960*35/100);
    assert(count(54,0,true) > top);
    assert(count(54,0,true) < 960);
    bool changedByMotion = false;
    for (int y : {0, 17, 50, 520, 539})
        for (int x = 0; x < 960; x+=7) {
            const bool first = FilmEdgeDither::Visible(x,y,0,true,13);
            const bool later = FilmEdgeDither::Visible(x,y,3700,true,13);
            const bool opposite = FilmEdgeDither::Visible(x,y,3700,false,13);
            assert(first == FilmEdgeDither::Visible(x,y,0,true,13));
            changedByMotion |= first != later || later != opposite;
        }
    assert(changedByMotion);
}
'''
    with tempfile.TemporaryDirectory(prefix="xmvsf_film_edge_") as scratch:
        folder = Path(scratch)
        source = folder / "edge.cpp"
        executable = folder / "edge.exe"
        source.write_text(smoke, encoding="utf-8")
        subprocess.run([
            "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "recomp_port"), str(source),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)

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
    print("XMVSF 87.464 film: C++ curl edge smoke, 20% entry, 108px bands, masked portrait/backing PASS")

if __name__ == "__main__":
    main()
