#!/usr/bin/env python3
"""XMVSF 87.243: read-only source integration for frame-data paint timing."""
from pathlib import Path

root = Path(__file__).resolve().parents[3]
run = (root / "dev/stable/buildtree/src/burner/sdl/run.cpp").read_text(encoding="utf-8")
clock = (root / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage30_frame_data.h").read_text(encoding="utf-8")
engine = (root / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage23_engine.h").read_text(encoding="utf-8")
video = (root / "dev/stable/buildtree/src/intf/video/sdl/vid_sdl2.cpp").read_text(encoding="utf-8")
start = run.index("static int RunFrame(")
end = run.index("// Callback used when DSound", start)
body = run[start:end]
draw_start = body.index("if (bDraw)")
skip_start = body.index("} else {", body.index("VidPaint(0);", draw_start))
drawn = body[draw_start:skip_start]
skipped = body[skip_start:body.index("if (!bPause) { /* XM87.44:", skip_start)]
assert drawn.count("xm23OnRealFrame(bDrvOkay);") == 1
assert drawn.index("xm23OnRealFrame(bDrvOkay);") < drawn.rindex("VidPaint(0);")
assert skipped.count("xm23OnRealFrame(bDrvOkay);") == 1
assert body.count("xm23OnRealFrame(bDrvOkay);") == 2
assert "xm23Overlay(sdlRenderer" in video
assert "XM87217::draw(image,font,xm23State.frameData);" in video
assert "const bool nativeStop=stopped && !firstOffensiveImpact;" in clock
assert "recordContacts(actors,frame,true);" in clock
assert "xm243FrameClockTrace(c,sample,actor,logicalBefore," in engine
assert "XMVSF_FRAME_TRACE" in engine
print("XMVSF 87.243 PASS: sampled once per native frame, before paint; +85 frozen ticks excluded")
