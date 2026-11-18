#!/usr/bin/env python3
"""Source-only regression for 87.440's reversible renderer installer."""
from __future__ import annotations
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / "recomp_port" / "apply_classic_renderer.py"
PORT = ROOT / "recomp" / "port"

def main():
    spec = importlib.util.spec_from_file_location("xmvsf_classic_patch", APP)
    assert spec and spec.loader
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    original = {file: file.read_text(encoding="utf-8") for file in (
        PORT / "frontend_sdl.cpp", PORT / "present.cpp", PORT / "present.h")}
    first = mod.patch(original)
    assert all("XMVSF_RECOMP_CLASSIC_87_440" in first[file] for file in first)
    assert mod.patch(first) == first, "patch is not idempotent"
    frontend = first[PORT / "frontend_sdl.cpp"]
    renderer = first[PORT / "present.cpp"]
    header = first[PORT / "present.h"]
    assert "SetClassicPixels(xmvsfClassic)" in frontend
    assert "const bool wantGpu = !xmvsfClassic" in frontend
    assert "const bool canShade = !xmvsfClassic" in frontend
    assert "XMVSF_RECOMP_CLASSIC_OFF" in frontend
    assert "SDL_LOGICAL_PRESENTATION_LETTERBOX" in renderer
    assert "SDL_SetRenderLogicalPresentation(m_renderer, 960, 540" in renderer
    assert "SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST)" in renderer
    assert "draw(m_texture, 1.0f)" in renderer
    assert "overlay();" in renderer
    assert "SDL_RenderPresent(m_renderer)" in renderer
    assert "bool m_classicPixels = false" in header
    assert "SDL_GPU_FILTER_LINEAR" in renderer, "upstream fallback must remain"
    assert "FitPicture(width, height" in frontend, "game viewport was modified"
    print("87.440 classic renderer: source anchors + nearest + logical letterbox + fallback + idempotence PASS")

if __name__ == "__main__":
    main()
