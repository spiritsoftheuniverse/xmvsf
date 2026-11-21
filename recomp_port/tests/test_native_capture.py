#!/usr/bin/env python3
"""87.489 F9 indexed native graphics inspector, annotated F6 source captures."""
from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile

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
    run = (ROOT / "RECOMP_RUN.bat").read_text(encoding="utf-8")

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
    assert menu.count("if (present) SDL_RenderPresent(renderer);") == 4
    assert "RenderStagePicker(SDL_Renderer *renderer" in menu

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
    assert 'nativeCapture.SaveMenu(presenter.Renderer(), "stages"' in front
    assert 'nativeCapture.SavePixels("home-fallback", homePicture.data(), 960, 540)' in front
    assert 'nativeCapture.SavePixels("ingame-menu", menuPicture.data(),' in front
    assert "port.Shown(nativeGamePixels);" in front
    assert 'nativeCapture.SavePixels("gameplay", nativeGamePixels.data(),' in front
    assert "port.ShownWidth(), Video::kHeight);" in front
    assert 'port.PlayerFolder() + "screenshots/"' not in front
    assert 'std::getenv("XMVSF_SCREENSHOT_DIR")' in front
    assert 'f6Folder && *f6Folder ? f6Folder : "../screenshots/"' in front

    # The launcher establishes the *absolute* repository root regardless of
    # where EEPROM/settings data are located; it also creates the folder at
    # start, so users can locate screenshots before pressing F6.
    assert 'set "XMVSF_SCREENSHOT_DIR=%PROJECT%\\screenshots"' in run
    assert 'mkdir "%XMVSF_SCREENSHOT_DIR%"' in run
    assert "std::filesystem::create_directories" in capture
    assert "F6 screenshots folder:" in capture
    assert "F6 capture requested ->" in capture

    # F12 is intentionally unchanged and still uses its original player
    # folder, while F6 gameplay goes through NativeCapture::SavePixels.
    game_capture = main_cpp.split("bool Port::Screenshot()", 1)[1].split("\n}\n", 1)[0]
    assert 'PlayerFolder() + "screenshots/"' in game_capture
    assert "Shown(shown)" in game_capture
    assert "WritePng(path.c_str(), shown.data()," in game_capture
    assert "ShownWidth()), Video::kHeight" in game_capture
    assert 'CAPTURE_HEADER.write_bytes(capture_content)' in installer
    # Compile actual typed interfaces. Don't assert source operators,
    # whitespace or particular line text: those are brittle preflights.
    compiler = shutil.which("g++")
    assert compiler, "UCRT64 g++ required for F6 provenance API smoke"
    smoke = CANONICAL / "tests/test_graphics_source_capture_smoke.cpp"
    subprocess.run([compiler, "-std=c++20", "-fsyntax-only", str(smoke)],
                   check=True)
    # Test F9's real pixel presentation and index/view behavior in C++,
    # without matching source code spelling or touching a ROM.
    with tempfile.TemporaryDirectory(prefix="xmvsf_f9_489_") as folder:
        inspector_smoke = CANONICAL / "tests/test_graphics_inspector_smoke.cpp"
        executable = Path(folder) / "graphics_inspector.exe"
        subprocess.run([compiler, "-std=c++20", str(inspector_smoke),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print("XMVSF 87.489 F9 indexed native presentation and F6 sources C++ smoke PASS")


if __name__ == "__main__":
    main()
