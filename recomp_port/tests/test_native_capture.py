#!/usr/bin/env python3
"""87.458 read-only preflight: F6 captures each native UI/game source."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CANONICAL = ROOT / "recomp_port"
PORT = ROOT / "recomp" / "port"


def source(filename: str) -> str:
    a = (CANONICAL / filename).read_text(encoding="utf-8")
    b = (PORT / filename).read_text(encoding="utf-8")
    assert a == b, f"{filename}: canonical/deployed headers differ"
    return a


def main() -> None:
    capture = source("xmvsf_native_capture.h")
    menu = source("xmvsf_menu_textures.h")
    front = (PORT / "frontend_sdl.cpp").read_text(encoding="utf-8")
    main_cpp = (PORT / "main.cpp").read_text(encoding="utf-8")
    installer = (CANONICAL / "apply_textured_menu.py").read_text(encoding="utf-8")

    # Real menu pixels are captured offscreen *before* presenting; both
    # target switch and readback must exist. No OS/desktop screenshots.
    for line in (
        "SDL_TEXTUREACCESS_TARGET, kWidth, kHeight",
        "SDL_SetRenderTarget(renderer, target)",
        "SDL_RenderReadPixels(renderer, nullptr)",
        "SDL_SetRenderTarget(renderer, previous)",
        "SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ABGR8888)",
        "constexpr int kWidth = 960, kHeight = 540",
        "if (!requested_) return true;",
        "WritePng(path.c_str(), pixels, uint32_t(width), uint32_t(height))",
        "SDL_CreateDirectory(folder_.c_str())",
        'folder_ + name + ".png"',
    ):
        assert line in capture, f"missing native capture requirement: {line}"

    # Offscreen copies render the *same* menu/Options compositor, but never
    # present their capture targets to the visible window.
    assert "bool Render(SDL_Renderer *renderer, uint64_t now, bool present = true)" in menu
    assert "const PadOwners &owners, bool present = true)" in menu
    assert menu.count("if (present) SDL_RenderPresent(renderer);") == 3

    # F6 is caught before chat and controller-remapping handling, never
    # downgraded into the old previous-slot action.
    key = "ev.key.scancode == SDL_SCANCODE_F6"
    assert front.count(key) == 1
    assert front.index("while (SDL_PollEvent(&ev))") < front.index(key)
    assert front.index(key) < front.index("if (touch.Event(ev))")
    assert front.index(key) < front.index("options.HandleEvent(")
    assert "if (!ev.key.repeat) nativeCapture.Request();" in front
    assert "ev.key.scancode == SDL_SCANCODE_F7" in front
    assert "ev.key.mod & SDL_KMOD_SHIFT" in front
    assert "nativeCapture.SaveMenu(presenter.Renderer(), \"home\"" in front
    assert "nativeCapture.SaveMenu(presenter.Renderer(), \"options\"" in front
    assert 'nativeCapture.SavePixels("home-fallback", homePicture.data(), 960, 540)' in front
    assert 'nativeCapture.SavePixels("ingame-menu", menuPicture.data(),' in front
    assert "port.Screenshot(); // existing native" in front
    assert "nativeCapture.Consume();" in front

    # The upstream gameplay screenshot is already native, using the actual
    # frame width and 224 rows. The in-game menu has its own larger buffer.
    game_capture = main_cpp.split("bool Port::Screenshot()", 1)[1].split("\n}\n", 1)[0]
    assert "Shown(shown)" in game_capture
    assert "WritePng(path.c_str(), shown.data()," in game_capture
    assert "ShownWidth()), Video::kHeight" in game_capture
    assert 'CAPTURE_HEADER.write_bytes(capture_content)' in installer
    print("XMVSF 87.458 F6: native home, Options, ingame-menu and gameplay capture hooks PASS")


if __name__ == "__main__":
    main()
