#!/usr/bin/env python3
"""87.474: F10 M68K capture recording UI, without changing the capture."""
from pathlib import Path
from version_guard import require_matching_versions

ROOT = Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

def main():
    cap = read("recomp/port/xmvsf_m68k_capture.h")
    front = read("recomp/port/frontend_sdl.cpp")
    present_h = read("recomp/port/present.h")
    present_cpp = read("recomp/port/present.cpp")
    home = read("recomp_port/xmvsf_home.h")
    textures = read("recomp_port/xmvsf_menu_textures.h")
    assert cap == read("recomp_port/xmvsf_m68k_capture.h")
    assert "uint64_t FramesSeen() const { return framesSeen_; }" in cap
    assert "void SetF10Status(bool active, const std::string &message)" in present_h
    assert "void Presenter::DrawF10Status()" in present_cpp
    assert present_cpp.count("DrawF10Status();") == 3
    assert present_cpp.count("overlay();\n        DrawF10Status();") == 2
    assert "overlay();\n    DrawF10Status();" in present_cpp
    assert "SDL_RenderDebugText(m_renderer," in present_cpp
    assert "SDL_RenderFillRect(m_renderer, &panel);" in present_cpp
    assert "SDL_GetRenderScale(m_renderer, &beforeX, &beforeY);" in present_cpp
    assert "SDL_SetRenderScale(m_renderer, beforeX, beforeY);" in present_cpp
    assert 'f10Notice = "F10 SAVED";' in front
    assert 'f10Notice = "F10 FAILED";' in front
    assert "f10PreviousActive && !f10Recording" in front
    assert 'std::to_string(xmvsf_f10::Capture::Get().FramesSeen()) + " FRAMES"' in front
    assert 'presenter.SetF10Status(f10Recording, f10Label);' in front
    assert 'SDL_SetWindowTitle(window, f10Title.c_str());' in front
    assert "ev.key.scancode == SDL_SCANCODE_F10" in front
    assert "auto &f10 = xmvsf_f10::Capture::Get();" in front
    assert "f10.Toggle(*port.machine," in front
    assert "session::CompletingP1Selection()" not in front
    assert "if (session::EnteringTraining()) {" in front
    assert "selectReveal.Frame(SDL_GetTicks())" in front
    assert "homeAudio.Fight(!port.menu.muted && !audioLog)" in front
    assert "films_.Draw(renderer, now, filmReveal);" in textures
    assert require_matching_versions(home, textures, minimum=(87, 475)) >= (87, 474)
    print("XMVSF 87.475 F10 visible REC/FRAMES, SAVED/FAILED, title PASS")

if __name__ == "__main__":
    main()
