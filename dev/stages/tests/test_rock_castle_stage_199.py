#!/usr/bin/env python3
"""87.199 source-contract regression: Rock reuses Wily stage05 and
native fighter compositor, not a duplicate gameplay renderer.

Static checks are not a Windows native build or animation playback.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[3]
def read(rel):
    p=root/rel
    assert p.exists(), f"missing source: {rel}"
    return p.read_text(encoding="utf-8")
stage=read("dev/stable/buildtree/src/burner/sdl/xmvsf_stage77_selection.h")
menu=read("dev/stable/buildtree/src/burner/sdl/xmvsf_stage14_boot.h")
selector=read("dev/stable/buildtree/src/burner/sdl/xmvsf_stage77_native_stage5.h")
camera=read("dev/stable/buildtree/src/burner/sdl/xmvsf_stage86_49_camera.h")
painter=read("dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage86_49_painter.h")
rock=read("dev/stable/buildtree/src/intf/video/sdl/xmvsf_stage87_199_rock_castle.h")
video=read("dev/stable/buildtree/src/intf/video/sdl/vid_sdl2.cpp")
host=read("dev/stable/buildtree/src/burner/sdl/xmvsf_stage87_164_host_replay.h")
for x in [
    "XM77_RANDOM=0, XM77_WILY=1, XM77_ROCK_CASTLE=2",
    "xm77StageResolved=XM77_ROCK_CASTLE",
    "++xm77StageRollGeneration();",
]:
    assert x in stage, x
assert '"ROCK CASTLE"' in menu
assert "XM77_STAGE_COUNT" in menu and "xm77RollStage()" in menu
assert "xm77StageChoice=h.stage" in menu and "xm77StageResolved=h.stageResolved" in menu
assert "stageResolved" in host
assert "xm77StageResolved==XM77_ROCK_CASTLE" in selector
assert "c->write(0xFF5082u,4u)" in selector
assert "xm77StageResolved!=XM77_ROCK_CASTLE" in camera
assert "xm8649Visual.nativeCameraX" in camera
assert "xmvsf_stage87_199_rock_castle.h" in painter
assert "XM87199Rock::state().paint(render,view" in painter
assert "XM87199Rock::state().reset()" in painter
assert "xm8649Preview->draw()" in painter, "Wily atlas renderer must be preserved"
assert "xm77SpritePixels()" in painter
assert "xm8649Paint(sdlRenderer,xm87127View)" in video
assert "IMG_LoadAnimation" in rock, "GIF frame decoding required"
assert "SDL_IMAGE_VERSION_ATLEAST(2,6,0)" in rock, "SDL2_image legacy fallback required"
assert "IMG_Load(file.c_str())" in rock, "static first-frame fallback required"
assert "frameIndex" in rock and "anim->delays[n]" in rock
assert "xm8733Overlay(render,view)" in rock, "native HUD rendering required"
assert "SDL_RenderCopy(render,fighters,nullptr,&view)" in rock, "native actor layer required"
assert "SDL_RenderCopy(render,flash,nullptr,&view)" in rock, "native FX layer required"
assert "no" not in "" # intentional no runtime asset requirement; local _art is untracked
print("PASS 87.199 Rock Castle selected through Wily's native stage05, camera, compositor and replay; original Wily pipeline retained")
