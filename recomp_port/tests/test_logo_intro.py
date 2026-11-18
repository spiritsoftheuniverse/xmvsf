#!/usr/bin/env python3
"""87.452 original XMVSF cold-boot logo timeline and SDL3 integration preflight.

Run before Ninja; never regenerates M68000 translation or modifies generated code.
"""
from __future__ import annotations
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "recomp_port"
LOCAL = ROOT / "recomp" / "port"

def require(text: str, needle: str, label: str):
    if needle not in text:
        raise AssertionError(f"{label}: missing {needle!r}")

def main():
    source = (PORT / "xmvsf_menu_textures.h").read_text(encoding="utf-8")
    mixer = (PORT / "xmvsf_menu_audio.h").read_text(encoding="utf-8")
    input_menu = (PORT / "xmvsf_home.h").read_text(encoding="utf-8")
    strokes = (PORT / "xmvsf_ii_strikes.h").read_text(encoding="utf-8")
    timeline = (PORT / "xmvsf_intro_timeline.h").read_text(encoding="utf-8")
    palette = (PORT / "xmvsf_palette_fx.h").read_text(encoding="utf-8")
    installer = (PORT / "apply_menu.py").read_text(encoding="utf-8")
    for filename in ("xmvsf_intro_timeline.h", "xmvsf_ii_strikes.h", "xmvsf_palette_fx.h"):
        assert (LOCAL / filename).read_bytes() == (PORT / filename).read_bytes(), filename
        require(installer, f'"{filename}"', "asset-free helper installer")
    require(palette, "logoPalettePulse(", "original warm/purple pulse")
    require(palette, "introLogoDither(", "source-pixel intro dither")
    require(palette, "backgroundHue(", "background intro strobe")
    require(source, "XMMenuColor::logoPalettePulse(", "rendered logo pulse")
    require(source, "XMMenuColor::introLogoDither(", "rendered intro logo")
    require(source, "XMMenuColor::backgroundHue(", "rendered intro background")
    require(source, "iiPainter_.draw(", "II animation")
    require(source, "RevealFonts(reveal)", "menu reveal")
    require(source, "DrawBackground(SDL_Renderer *r, std::uint64_t now, float introStrength = 0.0f)",
            "Options compatibility")
    require(source, "iiPainter_.release()", "SDL renderer shutdown")
    require(source, "TEST 87.452", "recomp version")
    require(mixer, 'crash_.Load("crash.wav")', "crash cue")
    require(mixer, 'welcome_.Load("xmvsf_welcome.wav")', "announcer cue")
    require(mixer, "XM87235Intro::MenuStart", "delayed menu music")
    require(input_menu, "XM87235Intro::State().InputAllowed(SDL_GetTicks())", "input gate")
    for name in ("logo_II_A.png","logo_II_B.png","logo_II_C.png","logo_II_D.png"):
        require(strokes,name,"original stroke asset")
        assert (ROOT / "_art" / name).is_file(), f"missing {name}"
    for name in ("crash.wav","xmvsf_welcome.wav"):
        assert (ROOT / "audio" / name).is_file(), f"missing {name}"
    for banned in ("SDL_RenderCopy(","IMG_Load(","SDL_FreeSurface(","SDL_Surface*"):
        assert banned not in strokes, f"unported SDL2 API: {banned}"
    require(strokes, "SDL_RenderTexture(", "SDL3 stroke rendering")
    require(strokes, "SDL_PIXELFORMAT_ABGR8888", "correct packed RGBA pixel format")

    test = r"""
#include "xmvsf_intro_timeline.h"
#include <cassert>
int main() {
    using namespace XM87235Intro;
    static_assert(AnnouncerStart==0u,"zero-offset cue");
    static_assert(LogoStart==1000u,"logo cue");
    static_assert(StrikeStart==2000u,"II reveal cue");
    static_assert(StrikeFrames==40u,"four source strokes, ten frames each");
    static_assert(MenuStart==2667u,"menu soundtrack cue");
    static_assert(TotalMs==3167u,"input unlock");
    BootClock clock;
    assert(clock.InputAllowed(123u));
    clock.Start(10000u);
    clock.Start(20000u); // reentering menu cannot replay intro
    assert(clock.Elapsed(10000u)==0u);
    assert(clock.Elapsed(11000u)==1000u);
    assert(!clock.InputAllowed(13166u));
    assert(clock.InputAllowed(13167u));
    assert(strokeFrame(2000u)==0u);
    assert(strokeFrame(2017u)==1u);
    assert(strokeFrame(2667u)==40u);
    assert(menuReveal(2667u)==0.f);
    assert(menuReveal(3167u)==1.f);
}
"""
    with tempfile.TemporaryDirectory(prefix="xmvsf_452_") as work:
        src = Path(work) / "timeline.cpp"
        exe = Path(work) / "timeline.exe"
        src.write_text(test, encoding="utf-8")
        subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror",
                        "-I",str(PORT),str(src),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print("XMVSF 87.452: intro timeline C++ and source-effect preflight PASS")

if __name__ == "__main__":
    main()
