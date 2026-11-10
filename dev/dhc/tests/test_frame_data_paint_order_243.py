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
assert "recordContacts(actors,samples,advancePlayer);" in clock
assert "xm41DrawFrameCard(r,card,e,f);" in (root / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage23_ui.h").read_text(encoding="utf-8")
assert "recordContacts(actors,samples,advancePlayer);" in clock
assert "recordContacts(actors,frame,true);" not in clock  # removed global-clock API
assert "xm255FrameClockTrace(c,sample,actor,logicalBefore," in engine
assert "xmvsf_frame_clock_255.tsv" in engine
assert "t.pendingOriginAction" in engine
assert "t.pendingOriginMode" in engine
assert "t.pendingNativeOnly" in engine
assert "events_created" in engine
assert "p1_contact_only" in engine and "p2_contact_only" in engine
assert "freeze_steps" in engine
assert "p1_super_contacts" in engine and "p2_super_contacts" in engine
assert "p1_super_linked_live" in engine and "p2_super_linked_live" in engine
assert "p1_super_deferred" in engine and "p2_super_deferred" in engine
assert "lastBoxFrame=e.hitboxLastFrame;" in engine
assert "xm23State.frameData.eventsCreated" in engine
assert "const bool nativeCandidateAccepted=" in clock
assert "objectOnlySuper) &&" in clock
assert "const bool objectOnlySuper=" in clock
assert "const bool contactWithoutBox=(hpFellNow || deferredHit) &&" in clock
assert "e.noBox=boxMissing;" in clock
assert "e.hitboxLastFrame=boxMissing?0u:frame;" in clock
assert "if(e.attackEnded && !landed[p] && contactExitGrace[p]==0u)" in clock
ui = (root / "dev/stable/buildtree/src/burner/sdl/xmvsf_stage23_ui.h").read_text(encoding="utf-8")
assert "43,48,57" in ui
assert "const unsigned long long base=startWall+" in ui
assert "const unsigned long long last=std::min(" in ui
assert "out.wrap.ticksPerRow=" in ui
assert "out.height=69+(int)out.wrap.rows()*18;" in ui
assert "const int h=xm41FrameCardLayout(e,f,cardWidth).height;" in ui
assert "const int x=x0+(int)(at-base);" in ui
assert "for(unsigned row=0;row<rows && row<maxRows;++row)" in ui
assert "f.wallTick(e.player,at)" in ui
assert "e.contactWallFrames[i]" in ui
assert "e.hitboxLastFrame" in ui
assert "e.lastProjectileFrame" in ui
assert "HIT / BOX ?" in ui
assert "PRE-BOX" in ui
assert "currentFrame:e.hitboxLastFrame" not in ui
assert "t.pendingOriginLeft=false;" in clock
assert "++eventsCreated;" in clock
assert "XMVSF_FRAME_TRACE" in engine
assert "const bool acceptedSuper=actors[p].valid &&" in clock
assert "const bool superRootReset=" not in clock
assert "family==0x2Eu && !t.superRootObserved;" in clock
assert "if(acceptedSuper){" in clock
assert "e.inputFrame=(unsigned long long)t.pending;" in clock
assert "e.inputWallFrame=t.pendingWallFrame;" in clock
assert "e.noBox=true;e.contactOnly=false;e.source=0;" in clock
assert "e.attackObserved=true;e.attackFamily=0x2Eu;" in clock
assert "t.superRootObserved=actors[p].valid && family==0x2Eu;" in clock
assert "p1_super_input_wall" in engine and "p2_super_input_wall" in engine
assert "p1_super_prebox" in engine and "p2_super_prebox" in engine
# 87.253: combo counter and native defender +85 reload are independent
# hit proofs after Training clamps the victim HP to 1.
assert "actor[p].comboKnown=true;" in engine
assert "p1_combo_proofs" in engine and "p2_combo_proofs" in engine
assert "p1_stop_proofs" in engine and "p2_stop_proofs" in engine
assert "const bool comboProof=" in clock and "const bool reloadProof=" in clock
assert "const int qd1[]={6,3,2},qd2[]={4,1,2};" in clock
assert "XM32_QCD" in clock
assert "++wallFrame[0];++wallFrame[1];" in clock
assert "recordWallTicks(advancePlayer);" in clock
assert "e.contactWallFrames" in clock
# 87.255: no inferred hit confirmation while defender HP can still
# fall; counter reloads cannot fabricate a new red tick before HP=1.
assert "const bool healthPinned=" in clock
assert "const bool comboProof=healthPinned && offense &&" in clock
assert "const bool reloadProof=healthPinned && offense &&" in clock
# The local release gate executes all C++ and Python regressions before
# the native build, reporting every failure in the same BUILD_TEST.log.
build_script = (root / "dev/dhc/BUILD_TEST.sh").read_text(encoding="utf-8")
assert "REGRESSION_FAILURES=()" in build_script
assert "run_required_python(){" in build_script
assert "test_frame_data_hit_proof_gate_255.cpp" in build_script
assert "REGRESSION SUMMARY:" in build_script
assert "Native build and TEST publishing blocked" in build_script
assert build_script.index("REGRESSION SUMMARY:") < build_script.index('cd "$TREE"')
print("XMVSF 87.255 PASS: independent native clocks, combo HP fallback, actual box duration and opt-in diagnostics")
