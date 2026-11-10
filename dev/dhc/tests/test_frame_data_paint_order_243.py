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
assert "const bool nativeSetup=" in clock
assert "const bool cinematicStop=nativeSetup && bothAnimationsStatic &&" in clock
assert "!offensiveObjectPresent && !healthDroppedNow;" in clock
assert "const bool healthDroppedNow=" in clock
assert "const bool offensiveObjectPresent=" in clock
assert "previousAnimPtr[p]!=a.animPtr" in clock
assert "previousAnimTimer[p]!=a.animTimer" in clock
assert "xm30NativeAttackFamily(a1)==0x2Cu" in clock
assert "xm30NativeAttackFamily(a1)==0x2Eu" in clock
assert "const bool nativeNewAttack=" in clock
assert "t.pendingNativeOnly=true;" in clock
assert "t.pendingOriginMode=t.lastNativeMode;" in clock
assert "advancePlayer[p]=stops[p]==0u || firstImpact;" in clock
assert "const unsigned long long frame=++playerFrame[p];" in clock
assert "recordContacts(actors,advancePlayer);" in clock
assert "xm41DrawFrameCard(r,card,e,f.playerFrame[e.player]);" in (root / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage23_ui.h").read_text(encoding="utf-8")
assert "recordContacts(actors,frame,true);" not in clock  # removed global-clock API
assert "xm250FrameClockTrace(c,sample,actor,logicalBefore," in engine
assert "xmvsf_frame_clock_250.tsv" in engine
assert "t.pendingOriginAction" in engine
assert "t.pendingOriginMode" in engine
assert "t.pendingNativeOnly" in engine
assert "events_created" in engine
assert "p1_contact_only" in engine and "p2_contact_only" in engine
assert "freeze_steps" in engine
assert "lastBoxFrame=e.hitboxLastFrame;" in engine
assert "xm23State.frameData.eventsCreated" in engine
assert "const bool nativeCandidateAccepted=" in clock
assert "(meleeRise || projectileRise || contactWithoutBox) &&" in clock
assert "const bool contactWithoutBox=hpFellNow &&" in clock
assert "e.noBox=contactWithoutBox;" in clock
assert "e.hitboxLastFrame=contactWithoutBox?0u:frame;" in clock
assert "if(e.attackEnded && !landed[p] && contactExitGrace[p]==0u)" in clock
ui = (root / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage23_ui.h").read_text(encoding="utf-8")
assert "e.hitboxLastFrame" in ui
assert "e.lastProjectileFrame" in ui
assert "HIT CONFIRMED - BOX UNKNOWN" in ui
assert "currentFrame:e.hitboxLastFrame" not in ui
assert "t.pendingOriginLeft=false;" in clock
assert "++eventsCreated;" in clock
assert "XMVSF_FRAME_TRACE" in engine
print("XMVSF 87.250 PASS: independent native clocks, combo HP fallback, actual box duration and opt-in diagnostics")
