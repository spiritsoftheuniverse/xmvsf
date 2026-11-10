#!/usr/bin/env python3
"""TEST 87.248: source contracts for SDL controller recovery, no ROM needed.

The persistent menu and FBNeo game backend must not use a fixed device
enumeration from startup. A pad which starts invalid/absent, reconnects,
or changes enumeration index must be readable WITHOUT restarting XMVSF.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[3]
tree = root / "dev/stable/buildtree/src"
backend = (tree / "intf/input/sdl/inp_sdl2.cpp").read_text(encoding="utf-8")
controls = (tree / "burner/sdl/xmvsf_stage13_controls.h").read_text(encoding="utf-8")
boot = (tree / "burner/sdl/xmvsf_stage14_boot.h").read_text(encoding="utf-8")
training = (tree / "burner/sdl/xmvsf_stage23_ui.h").read_text(encoding="utf-8")
menus = (tree / "burner/sdl/xmvsf_stage24_3_menu_poll.h").read_text(encoding="utf-8")
handoff = (tree / "burner/sdl/xmvsf_stage25_4_input_handoff.h").read_text(encoding="utf-8")
main = (tree / "burner/sdl/main.cpp").read_text(encoding="utf-8")
build = (root / "dev/dhc/BUILD_TEST.sh").read_text(encoding="utf-8")

# Game backend: the SDL enumeration index can change when another pad
# detaches. Reopen by instance ID, not just by a count captured at init.
assert "static SDL_JoystickID xm248Instance[MAX_JOYSTICKS]" in backend
assert "static SDL_JoystickID xm248ListedInstance[MAX_JOYSTICKS]" in backend
assert "SDL_JoystickGetDeviceInstanceID(i)" in backend
assert "SDL_JoystickInstanceID(JoyList[i])==id" in backend
assert "SDLinpCloseJoystickSlot(i);" in backend
assert "if(forceRetry || retry || newlyListed)" in backend
assert "(Uint32)(now-xm248LastRetryAt)>=250u" in backend
assert "nJoystickCount=count;" in backend
assert "SDLinpRecheckJoysticks(true);" in backend
start = backend[backend.index("int SDLinpStart()"):backend.index("static int ReadJoystick()")]
assert "SDLinpRecheckJoysticks();" in start
poll = backend[backend.index("static int ReadJoystick()"):backend.index("int SDLinpJoyAxis(")]
assert "SDLinpRecheckJoysticks();" in poll
assert "SDL_GameControllerUpdate();" in poll
assert "SDL_GameControllerEventState(SDL_ENABLE);" in backend
assert "SDL_JoystickEventState(SDL_ENABLE);" in backend
assert "SDL_GameControllerEventState(SDL_IGNORE);" not in backend
assert "SDL_JoystickEventState(SDL_IGNORE);" not in backend
assert "if (!JoyList[i] || !SDL_JoystickGetAttached(JoyList[i])" in backend
assert "int buttons[MAX_JOYSTICKS][8]" in backend

# Shared menu helper: it closes a stale instance, then retry-opens the
# currently enumerated device, without modifying saved assignments.
assert "static inline bool xm248ReconcileMenuPad(" in controls
assert "instance==wanted && SDL_JoystickInstanceID(stick)==wanted" in controls
assert "if(!live)" in controls
assert "if(!stick)" in controls and "SDL_JoystickOpen(device)" in controls
assert "SDL_GameControllerGetAttached(pad)" in controls
assert "if(wanted<0)return changed;" in controls
# Saved player->physical-device ownership must also survive SDL index
# compaction when two identical-model controllers were initially connected.
assert "struct XM248RuntimeOwner" in controls
assert "extern XM248RuntimeOwner xm248RuntimeOwner[2];" in controls
assert "XM248RuntimeOwner xm248RuntimeOwner[2];" in main
assert "static inline int xm248FindAssignedDevice(" in controls
assert "SDL_JoystickGetDeviceInstanceID(i)==saved.instance" in controls
assert "xm248RuntimeOwner[other].instance" in controls
assert "otherLive && SDL_JoystickGetDeviceInstanceID(index)==otherInstance" in controls
assert "const int exact=xm13FindDevice(&p);" in controls
assert "const int device=xm248FindAssignedDevice(s,player);" in controls
assert "xm248FindAssignedDevice(&config,player);" in boot
assert "xm248FindAssignedDevice(&config,controlsPage);" in boot
assert "xm248FindAssignedDevice(&settings,0);" in training
assert "xm248FindAssignedDevice(&settings,i);" in menus
assert "xm248FindAssignedDevice(&xm13Config,xm14Source);" in backend

# Boot: attachment discovery continues even after the UI is visible.
assert "SDL_JoystickID menuInstances[8]" in boot
assert "(Uint32)(now-menuLastPadProbe)>=100u" in boot
assert "xm248ReconcileMenuPad(device,menuSticks[device]," in boot
assert "xm248ReconcileMenuPad(-1,menuSticks[i]" in boot
# Training: a first failed open is NOT the last attempt.
assert "SDL_JoystickID instance=-1;" in training
assert "(Uint32)(now-p.lastProbeAt)>=150u" in training
assert "deviceChanged=xm248ReconcileMenuPad(" in training
assert "if(first || deviceChanged)" in training
assert "p.previousA=a;p.previousB=b;p.previousUp=up;p.previousDown=down;" in training

# Arcade/Versus: refresh bindings and device identity in the same game.
assert "static inline void xm243RefreshPads(XM243Pad pads[2])" in menus
assert "xm243RefreshPads(xm243Pads);" in menus
assert "xm243RefreshPads(xm258ArcadePads);" in menus
assert "(Uint32)(now-xm243LastPadProbe)>=150u" in menus
assert "(Uint32)(now-xm258LastPadProbe)>=150u" in menus
assert "if(changed){" in menus
assert "xm243ReadPad(p,p.previousUp,p.previousDown,p.previousAccept," in menus
assert "p.mapping=settings.player[i];" in menus

# The handoff latch cannot make a newly valid controller unusable forever
# because another (even unassigned) pad/button never becomes neutral.
assert "xm25InputHandoff.beganAt=SDL_GetTicks();" in handoff
assert "(Uint32)(now-xm25InputHandoff.beganAt)>=400u" in handoff
assert "xm25InputHandoff.active=false;" in handoff

assert "test_controller_reconnect_248.py" in build
print("XMVSF 87.248 PASS: live SDL pad instance reconnection, menu reacquisition, bounded handoff")
