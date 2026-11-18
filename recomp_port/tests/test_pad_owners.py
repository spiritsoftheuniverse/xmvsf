#!/usr/bin/env python3
"""87.443 local source regression: controller identity and menu press edges."""
from __future__ import annotations
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "recomp" / "port"
INSTALLER = ROOT / "recomp_port" / "apply_pad_owners.py"
SOURCE = ROOT / "recomp_port" / "xmvsf_pad_owners.h"
OPTIONS = ROOT / "recomp_port" / "xmvsf_options.h"
RENDER = ROOT / "recomp_port" / "xmvsf_menu_textures.h"
HOME = ROOT / "recomp_port" / "xmvsf_home.h"

def main():
    spec = importlib.util.spec_from_file_location("xmvsf_pad_owner", INSTALLER)
    assert spec and spec.loader
    patcher = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(patcher)
    text = (PORT / "frontend_sdl.cpp").read_text(encoding="utf-8")
    assert patcher.patch(text) == text, "device router did not install idempotently"
    assert (PORT / "xmvsf_pad_owners.h").read_bytes() == SOURCE.read_bytes()
    for needed in (
        "previousMenuButtons = menuNow", "menuNow & ~previousMenuButtons",
        "owners.Refresh(gamepads)", "owners.ReadRaw(p, Table())",
        "owners.Gamepad(p)", "owners.AdditionalGamepad(p - 2)",
        "owners.RawMenu(Table())", "owners.Event(ev)",
        "owners.LoadSavedProfiles(g_bindings, Table())",
        "options.HandleEvent(", "port.opt.settingsPath, owners",
        "homeTextures.RenderOptions", "owners.Close()",
    ):
        assert needed in text, f"missing native hook: {needed}"
    assert "menuRepeat.Step(menuNow)" not in text

    cpp = SOURCE.read_text(encoding="utf-8")
    for needed in (
        "SDL_GetJoystickName(js)", "SDL_GetJoystickGUID(js)",
        "SDL_GetJoystickSerial(js)", "SDL_GetGamepadID(gp)",
        "SDL_GetJoystickID(js)", "SDL_GetJoysticks(&count)",
        "SDL_EVENT_JOYSTICK_ADDED", "SDL_EVENT_JOYSTICK_REMOVED",
        "SDL_GetJoystickHat(js", "SDL_GetJoystickButton(js",
        "SDL_GetJoystickAxis(js", "SameIdentity",
        "reserve BOTH players", "recomp::Ini ini(",
        "RawSetting(", "PopulateProfile(",
        "LoadSavedProfiles(", "AdditionalGamepad("
    ):
        assert needed in cpp, f"missing ownership invariant: {needed}"

    options = OPTIONS.read_text(encoding="utf-8")
    for needed in (
        "owners.AssignNext(", "owners.PopulateProfile(", "owners.SaveRaw(",
        "owners.RawJoystick(", "SDL_EVENT_JOYSTICK_BUTTON_DOWN",
        "SDL_EVENT_JOYSTICK_AXIS_MOTION", "SDL_EVENT_JOYSTICK_HAT_MOTION",
        "SDL_GetGamepadID(assignedGamepad)",
        "const int count = tab_ == 0 ? 7 : tab_ == 1 ? 8 : 16"
    ):
        assert needed in options, f"missing Options hook: {needed}"
    assert "TEST 87.443" in HOME.read_text(encoding="utf-8")
    assert "TEST 87.443" in RENDER.read_text(encoding="utf-8")
    assert "SDL_SCALEMODE_NEAREST" in RENDER.read_text(encoding="utf-8")
    prev = 0
    edge = []
    for current in (4, 4, 4, 0, 4):
        edge.append(current & ~prev)
        prev = current
    assert edge == [4, 0, 0, 0, 4]
    print("XMVSF 87.443: press edges, P1/P2, OS device names, remembered profiles and raw HID: PASS (source)")

if __name__ == "__main__":
    main()
