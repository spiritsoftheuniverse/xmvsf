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
    # Regression for 87.443's "found 3" failure: render-call suffixes
    # elsewhere are not a valid global replacement anchor. Emulate the
    # pre-ownership Options render with extra matching suffixes outside
    # that one rendering invocation. The patch must change only that call.
    old_call = (
        "                homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),\n"
        "                                           options, g_bindings, Table(), gamepads);"
    )
    new_call = (
        "                homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),\n"
        "                                           options, g_bindings, Table(), owners);"
    )
    # 87.449: first installation (87.443) is followed by the physical
    # gameplay edits (87.448). The next build must recognize the *finished*
    # source; replaying its intermediate loop anchor made the second-pass
    # regression test crash even though the installer had succeeded.
    installed_fixture = (
        patcher.OWNER_INCLUDE + "\n" +
        "\n".join(patcher.FINAL_HOOKS) + "\n" + new_call + "\n"
    )
    assert patcher._finished_frontend(installed_fixture) == installed_fixture
    assert patcher.patch(installed_fixture) == installed_fixture
    downgraded_render = installed_fixture.replace(new_call, old_call, 1)
    assert patcher.patch(downgraded_render) == installed_fixture
    partial_fixture = installed_fixture.replace(
        "owners.Refresh(gamepads, g_bindings, Table());", "", 1
    )
    assert patcher._finished_frontend(partial_fixture) is None
    assert text.count(new_call) == 1, "expected exactly one native Options renderer"
    duplicate_suffix = "                                           options, g_bindings, Table(), gamepads);"
    already_elsewhere = text.count(duplicate_suffix)
    synthetic = text.replace(new_call, old_call, 1)
    # The 87.444 release failed because it assumed the exact surrounding
    # "if (options.Active())" comment and nearby home-menu block. Deliberately
    # change that layout without changing the unique function call.
    old_layout = (
        "        if (options.Active()) { // XMVSF_RECOMP_OPTIONS_87_442: three native tabs"
    )
    if old_layout in synthetic:
        synthetic = synthetic.replace(
            old_layout, "        if (options.Active()) { /* custom local layout */", 1
        )
    # Preserve the pre-existing call suffixes in the user's local
    # checkout. Add only enough decoys to reproduce at least 3 matches.
    required_decoys = max(0, 2 - already_elsewhere)
    synthetic += "\n// 87.444 fixture\n" + (duplicate_suffix + "\n") * required_decoys
    total_suffixes = synthetic.count(duplicate_suffix)
    assert total_suffixes >= 3
    repaired = patcher.patch(synthetic)
    assert repaired.count(new_call) == 1
    assert repaired.count(duplicate_suffix) == total_suffixes - 1
    assert patcher.patch(repaired) == repaired
    assert (PORT / "xmvsf_pad_owners.h").read_bytes() == SOURCE.read_bytes()
    for needed in (
        "previousMenuButtons = menuNow", "menuNow & ~previousMenuButtons",
        "owners.Refresh(gamepads, g_bindings, Table())", "owners.ReadRaw(p, Table())",
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
        "SDL_GetJoysticks(&count)",
        "SDL_EVENT_JOYSTICK_ADDED", "SDL_EVENT_JOYSTICK_REMOVED",
        "SDL_GetJoystickHat(js", "SDL_GetJoystickButton(js",
        "SDL_GetJoystickAxis(js", "SameIdentity",
        "Reserve both explicitly configured LIVE", "recomp::Ini ini(",
        "RawSetting(", "PopulateProfile(",
        "LoadSavedProfiles(", "AdditionalGamepad("
    ):
        assert needed in cpp, f"missing ownership invariant: {needed}"

    options = OPTIONS.read_text(encoding="utf-8")
    for needed in (
        "owners.AssignFromInstance(", "owners.AssignKeyboard(", "owners.SaveRaw(",
        "owners.AssignedInstance(", "SDL_EVENT_JOYSTICK_BUTTON_DOWN",
        "SDL_EVENT_JOYSTICK_AXIS_MOTION", "SDL_EVENT_JOYSTICK_HAT_MOTION",
        "SDL_EVENT_GAMEPAD_BUTTON_DOWN",
        "owners.VisibleActions(player_) + 1"
    ):
        assert needed in options, f"missing Options hook: {needed}"
    assert "owners.AssignNext(" not in options, "old device-cycle UI must be removed"
    assert "editPad_" not in options, "old keyboard/gamepad toggle must be removed"
    for required in (
        "controlRow_ == 0", "StoreDevice(", "StoreInput(",
        "owners.Keyboard(player_)", "owners.VisibleActions(player_) + 1",
        "SDL_EVENT_JOYSTICK_BUTTON_DOWN", "SDL_EVENT_JOYSTICK_AXIS_MOTION",
        "ev.jbutton.which", "ev.jaxis.which", "SDL_GetTicks() < captureAt_ + 180",
    ):
        assert required in options, f"missing Stage13 capture behavior: {required}"
    assert "AssignFromInstance(" in cpp and "AssignKeyboard(" in cpp
    assert "SaveKeyboardAction(" in cpp and "PhysicalForGamepadButton(" in cpp
    assert "SDL_GetGamepadBindings(" in cpp, "semantic mappings must resolve physical indices"
    assert "SDL_SCANCODE_J" in cpp and "SDL_SCANCODE_KP_1" in cpp
    assert "if (player >= 2 || !owners.Physical(player))" in text
    assert "if (p >= 2 || !owners.Physical(p))" in text
    assert "TEST 87.450" in HOME.read_text(encoding="utf-8")
    assert "TEST 87.450" in RENDER.read_text(encoding="utf-8")
    assert "SDL_SCALEMODE_NEAREST" in RENDER.read_text(encoding="utf-8")
    prev = 0
    edge = []
    for current in (4, 4, 4, 0, 4):
        edge.append(current & ~prev)
        prev = current
    assert edge == [4, 0, 0, 0, 4]
    print("XMVSF 87.450: anchored renderer fix + press edges, P1/P2, OS device names, remembered profiles and raw HID: PASS (source)")

if __name__ == "__main__":
    main()
