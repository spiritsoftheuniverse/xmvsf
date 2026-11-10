#!/usr/bin/env python3
"""TEST 87.249: menu Xbox/DS4Windows versus raw Fighter Pi source integration."""
from pathlib import Path
root=Path(__file__).resolve().parents[3]
src=root/"dev/stable/buildtree/src/burner/sdl"
backend=(root/"dev/stable/buildtree/src/intf/input/sdl/inp_sdl2.cpp").read_text(encoding="utf-8")
controls=(src/"xmvsf_stage13_controls.h").read_text(encoding="utf-8")
boot=(src/"xmvsf_stage14_boot.h").read_text(encoding="utf-8")
mapping=(src/"xmvsf_stage13_1_game_bindings.h").read_text(encoding="utf-8")
main=(src/"main.cpp").read_text(encoding="utf-8")
policy=(src/"xmvsf_stage249_controller_policy.h").read_text(encoding="utf-8")
build=(root/"dev/dhc/BUILD_TEST.sh").read_text(encoding="utf-8")
assert "struct XM249SessionPad" in controls
assert "extern XM249SessionPad xm249SessionPad[2];" in controls
assert "XM249SessionPad xm249SessionPad[2];" in main
assert "xm249SessionPad[0]=XM249SessionPad{};" in boot
assert "xm249SessionPad[1]=XM249SessionPad{};" in boot
assert "if(accept && ev.type==SDL_CONTROLLERBUTTONDOWN)" in boot
assert "xm249SelectSessionPad(0,ev.cbutton.which);" in boot
assert "xm249SelectSessionPad(0,ev.jbutton.which);" in boot
assert boot.index("xm249SelectSessionPad(0,ev.cbutton.which);") < boot.index("if(screen==5)")
assert "if(!SDL_IsGameController(found))return false;" in controls
assert "SDL_JoystickGetDeviceInstanceID(i)==owner.instance" in controls
assert "const int fromMenu=xm249FindSessionDevice(player);" in controls
assert "if(fromMenu>=0)return fromMenu;" in controls
assert "XM249Policy::needsVirtualMap(" in mapping
assert "xm249SessionPad[player].selected" in mapping
assert "XM249Policy::needsVirtualMap(savedActive,fromMenu)" in controls
assert "XM249Policy::useSemanticPad(" in controls
assert "XM249Policy::deliver(" in controls
assert "SDL_GameControllerGetButton(pad,names[action])" in controls
assert "SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y" in controls
assert "SDL_CONTROLLER_BUTTON_LEFTSHOULDER" in controls
assert "SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B" in controls
assert "SDL_CONTROLLER_BUTTON_RIGHTSHOULDER" in controls
assert "SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START" in controls
assert "if(!s || player<0 || player>1)return -1;" in controls
# The gameplay callback must actually reach xm13ReadInput instead of
# returning xm15SyntheticButton=0 when the saved mapping is inactive.
assert "if(XM249Policy::needsVirtualMap(" in backend
assert "xm249SessionPad[xm14Source].selected))" in backend
assert backend.index("if(XM249Policy::needsVirtualMap(") < backend.index("xm13ReadInput(&xm13Config,xm14Source,xm15Sub")
assert "XM87.249 PAD_GAME_OWNER" in backend
assert "XM249Policy::needsVirtualMap" in policy
assert "XM249Policy::useSemanticPad" in policy
assert "XM249Policy::deliver" in policy
assert "test_controller_session_249.cpp" in build
assert "test_controller_session_249.py" in build
# This is a read-only ROM-facing input path; no saved mappings are written.
segment=controls[controls.index("static inline bool xm249SelectSessionPad("):
                 controls.index("static void xm13SetDefaultButtons(")]
assert "xm13Save" not in segment
# Still preserve the underlying recovery code for initially invalid devices.
assert "SDLinpRecheckJoysticks();" in backend
assert "SDL_GameControllerEventState(SDL_ENABLE);" in backend
print("XMVSF 87.249 PASS: Xbox/DS4Windows menu-to-CPS2 P1 routing, Fighter Pi preserved")
