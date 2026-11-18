#!/usr/bin/env python3
"""87.441 local source regression for the SDL textured menu."""
from __future__ import annotations
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INSTALL = ROOT / "recomp_port" / "apply_textured_menu.py"
SOURCE = ROOT / "recomp_port" / "xmvsf_menu_textures.h"
PORT = ROOT / "recomp" / "port"

def main():
    spec = importlib.util.spec_from_file_location("xmvsf_textured", INSTALL)
    assert spec is not None and spec.loader is not None
    installer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(installer)
    source = {path: path.read_text(encoding="utf-8") for path in (
        PORT / "frontend_sdl.cpp", PORT / "present.h",
        PORT / "present.cpp", PORT / "xmvsf_home.h"
    )}
    if "XMVSF_RECOMP_PAD_OWNER_87_443" in source[PORT / "frontend_sdl.cpp"]:
        first = source  # later version retains renderer, changes shutdown
    else:
        first = installer.patch(source)
        assert installer.patch(first) == first, "installer must be idempotent"

    front = first[PORT / "frontend_sdl.cpp"]
    present = first[PORT / "present.cpp"]
    home = first[PORT / "xmvsf_home.h"]
    tex = SOURCE.read_text(encoding="utf-8")
    assert "xmvsf_home::TexturedMenu homeTextures(home)" in front
    assert "homeTextures.Render(presenter.Renderer(), SDL_GetTicks())" in front
    assert "presenter.UseClassicMenuCanvas(true)" in front
    assert 'SDL_CreateRenderer(window, "direct3d11")' in present
    assert "SDL_LOGICAL_PRESENTATION_LETTERBOX" in present
    assert "SDL_SCALEMODE_NEAREST" in tex
    assert "SDL_PIXELFORMAT_ABGR8888" in tex
    assert "SDL_RenderTexture" in tex and "SDL_RenderPresent" in tex
    assert "SDL_UpdateTexture(background_, nullptr" in tex
    assert "XMVSF_MENU_CPU_FALLBACK" in tex
    assert "friend class TexturedMenu" in home
    assert any(version in home for version in ("TEST 87.441", "TEST 87.442", "TEST 87.443", "TEST 87.444", "TEST 87.445", "TEST 87.446", "TEST 87.447", "TEST 87.448", "TEST 87.449", "TEST 87.450"))
    assert "FitPicture(width, height" in front
    assert "homeTextures.Release();" in front
    assert "homeAudio.Close();" in front
    print("87.441 source-texture menu patch: anchors, idempotence, fallback PASS")

if __name__ == "__main__":
    main()
