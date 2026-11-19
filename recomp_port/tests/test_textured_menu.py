#!/usr/bin/env python3
"""87.441 local source regression for the SDL textured menu."""
from __future__ import annotations
import importlib.util
from pathlib import Path
from version_guard import require_matching_versions

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
    # Whitespace-independent: the SDL3 renderer uses both normal and intro
    # background uploads. A Python source assertion must not require
    # formatting such as a space after the comma.
    import re
    background_upload = re.compile(
        r"\bSDL_UpdateTexture\s*\(\s*background_\s*,\s*nullptr\s*,"
    )
    assert background_upload.search(tex), "no SDL3 background texture upload"
    assert "XMVSF_MENU_CPU_FALLBACK" in tex
    assert "friend class TexturedMenu" in home
    require_matching_versions(home, tex, minimum=(87, 441))
    assert "FitPicture(width, height" in front
    assert "homeTextures.Release();" in front
    assert "homeAudio.Close();" in front
    print("87.441 source-texture menu patch: anchors, idempotence, fallback PASS")

if __name__ == "__main__":
    main()
