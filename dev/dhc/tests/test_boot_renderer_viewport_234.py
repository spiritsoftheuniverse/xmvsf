#!/usr/bin/env python3
"""87.234 regression: preserve SDL's centered logical viewport at boot.

Source-order contract, not an SDL/Windows runtime assertion. The actual
native first frame and resize behavior still need local RUN_TEST verification.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
WIN = ROOT / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_52_window_state.h"
BOOT = ROOT / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage14_boot.h"
VIDEO = ROOT / "dev/stable/buildtree/src/intf/video/sdl/vid_sdl2.cpp"


def section(source: str, start: str, end: str) -> str:
    assert source.count(start) == 1, f"Missing or ambiguous {start}"
    return source.split(start, 1)[1].split(end, 1)[0]


def test_boot_canvas_order():
    win, boot, video = (p.read_text(encoding="utf-8") for p in (WIN, BOOT, VIDEO))
    reset = section(win, "static inline bool xm8652CenteredBootCanvas(", 
                    "// TEST 87.226: SDL_WINDOWPOS_CENTERED")
    # Reset stale logical size and physical viewport BEFORE calculating the
    # 960x540 aspect-fit viewport; never overwrite it afterward.
    ix_clear = reset.index("SDL_RenderSetLogicalSize(render,0,0)")
    ix_integer = reset.index("SDL_RenderSetIntegerScale(render,SDL_FALSE)")
    ix_full = reset.index("SDL_RenderSetViewport(render,nullptr)")
    ix_recalc = reset.index("SDL_RenderSetLogicalSize(render,960,540)")
    assert ix_clear < ix_full < ix_recalc
    assert ix_integer < ix_recalc
    assert reset.count("SDL_RenderSetViewport(render,nullptr)") == 1
    assert reset.count("SDL_RenderSetLogicalSize(render,960,540)") == 1
    assert "SDL_RenderGetViewport(render,&vp)" in reset
    assert "SDL_GetRendererOutputSize(render,&outW,&outH)" in reset
    assert "SDL_SetWindowPosition(" not in reset

    boot_start = section(boot, "static int xmvsfBootSelector(", 
                         "SDL_PumpEvents();")
    assert "xm8652CenteredBootCanvas(render," in boot_start
    assert boot_start.index("xm8652CenteredBootCanvas(render,") < boot_start.index("xm8656Present(render)")
    assert "SDL_RenderSetViewport(render,nullptr)" not in boot_start
    assert "SDL_GetRenderer(window)" in boot_start
    assert "xm8652CenterInitialWindow(window)" in boot_start

    handoff = section(video, "if(preserveMenuRenderer){", "}else{")
    assert "xm8652CenteredBootCanvas(sdlRenderer,\"game-to-menu\")" in handoff
    assert "SDL_RenderSetViewport(sdlRenderer,nullptr)" not in handoff
    assert "SDL_RenderSetLogicalSize(sdlRenderer,960,540)" not in handoff

    # The game reuses the same persistent SDL_Renderer and continues to own
    # its own aspect-fit source rectangle. This only corrects boot layout.
    assert "sdlRenderer = SDL_GetRenderer(sdlWindow)" in video
    assert "dstrect.x=(960-viewW)/2" in video
    assert "dstrect.y=(540-viewH)/2" in video


def aspect_fit(output_w, output_h, logical_w=960, logical_h=540):
    scale = min(output_w / logical_w, output_h / logical_h)
    width = logical_w * scale
    height = logical_h * scale
    return ((output_w - width) / 2, (output_h - height) / 2, width, height)


def test_expected_letterbox():
    for w, h in ((960,540), (1280,720), (1200,900), (1686,1064),
                 (3440,1440), (1000,540), (720,540)):
        x,y,vw,vh = aspect_fit(w,h)
        assert x >= 0 and y >= 0
        assert abs((2*x+vw)-w) < 1e-6
        assert abs((2*y+vh)-h) < 1e-6
        assert abs(vw/vh-16/9) < 1e-8
    assert aspect_fit(960,540) == (0,0,960,540)
    assert aspect_fit(1200,900)[1] > 0  # letterbox vertically
    assert aspect_fit(3440,1440)[0] > 0 # pillarbox horizontally


if __name__ == "__main__":
    test_boot_canvas_order()
    test_expected_letterbox()
    print("87.234 SDL centered boot logical viewport source-order: PASS")
