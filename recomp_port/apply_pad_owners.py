#!/usr/bin/env python3
"""87.443: explicit SDL3 P1/P2 ownership and one-press-one-menu-step.

Source-only anchored patch after 87.442 Options. Never modifies ROMs,
resets the user's submodule, or invalidates the 87.441 renderer design.
"""
from __future__ import annotations
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "recomp" / "port" / "frontend_sdl.cpp"
OUT = ROOT / "recomp" / "port" / "xmvsf_pad_owners.h"
SRC = ROOT / "recomp_port" / "xmvsf_pad_owners.h"
TAG = "XMVSF_RECOMP_PAD_OWNER_87_443"

def once(code: str, old: str, new: str, what: str) -> str:
    if new in code:
        return code
    count = code.count(old)
    if count != 1:
        raise RuntimeError(
            f"87.443 {what}: expected one anchored native block, found {count}; refusing partial write")
    return code.replace(old, new, 1)

def patch(code: str) -> str:
    code = once(code,
        '#include "xmvsf_options.h" // XMVSF_RECOMP_OPTIONS_87_442\n',
        '#include "xmvsf_options.h" // XMVSF_RECOMP_OPTIONS_87_442\n'
        '#include "xmvsf_pad_owners.h" // ' + TAG + '\n',
        "include")
    code = once(code,
        '    xmvsf_home::OptionsPanel options; // XMVSF_RECOMP_OPTIONS_87_442\n',
        '''    xmvsf_home::OptionsPanel options; // XMVSF_RECOMP_OPTIONS_87_442
    xmvsf_home::PadOwners owners; // XMVSF_RECOMP_PAD_OWNER_87_443
    owners.Init(port.opt.settingsPath);
    owners.LoadSavedProfiles(g_bindings, Table());
''',
        "persistent ownership and stored profiles")
    code = once(code,
        '''    recomp::MenuRepeat menuRepeat; // (the arrows held go on through the rows and the choices)''',
        '''    // XMVSF_RECOMP_PAD_OWNER_87_443: one deliberate key/button press
    // per menu action. Releasing all bits rearms the corresponding input.
    unsigned previousMenuButtons = 0;''',
        "remove 60fps hold-repeat state")
    code = once(code,
        '''            // XMVSF_RECOMP_OPTIONS_87_442: capture key / pad remaps
            // before game shortcuts. Captured inputs never reach the game.
            if (options.Active() && options.HandleEvent(
                    ev, g_bindings, Table(), port.opt.settingsPath, gamepads)) {''',
        '''            // XMVSF_RECOMP_PAD_OWNER_87_443: hotplug raw HID sticks;
            // event capture uses assigned SDL instance ID, not vector index.
            owners.Event(ev);
            // XMVSF_RECOMP_OPTIONS_87_442: capture key / pad remaps
            // before game shortcuts. Captured inputs never reach the game.
            if (options.Active() && options.HandleEvent(
                    ev, g_bindings, Table(), port.opt.settingsPath, owners)) {''',
        "physical joystick hotplug and capture")
    code = once(code,
        '''        if (port.opt.moviePlay.empty() && !port.opt.autoPlay) {
            // The keyboard is every player's''',
        '''        // XMVSF_RECOMP_PAD_OWNER_87_443: resolve identities once per
        // frame. Live instance pins P1/P2 against SDL hotplug reordering.
        owners.Refresh(gamepads);
        if (port.opt.moviePlay.empty() && !port.opt.autoPlay) {
            // The keyboard is every player's''',
        "reconnect resolving")
    code = once(code,
        '''            for (size_t i = 0; i < gamepads.size() && static_cast<int>(i) < Players(); i++) {
                pads[i] |= ReadGamepad(gamepads[i], static_cast<int>(i));
                const bool left = stepOnTrigger && SDL_GetGamepadAxis(gamepads[i], SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > kDead;
                trigger = trigger || left;
                hurry = hurry || (multi && left);
                // (while the game runs, the left trigger is its player's Tag: kTag; in a Multi game run, fast forward)
                if (left && !port.paused && !multi) {
                    pads[i] |= kTag;
                }
                both = both || (SDL_GetGamepadButton(gamepads[i], SDL_GAMEPAD_BUTTON_START) &&
                                SDL_GetGamepadButton(gamepads[i], SDL_GAMEPAD_BUTTON_BACK));
            }''',
        '''            for (int p = 0; p < Players(); ++p) {
                // Saved P1/P2 identities are authoritative. P3/P4 retain
                // remaining unclaimed gamepads for the recomp's co-op modes.
                SDL_Gamepad *gp = p < 2 ? owners.Gamepad(p) : owners.AdditionalGamepad(p - 2);
                if (p < 2) pads[p] |= owners.ReadRaw(p, Table());
                if (!gp) continue;
                pads[p] |= ReadGamepad(gp, p);
                const bool left = stepOnTrigger && SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > kDead;
                trigger = trigger || left;
                hurry = hurry || (multi && left);
                if (left && !port.paused && !multi) pads[p] |= kTag;
                both = both || (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_START) &&
                                SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_BACK));
            }''',
        "route native game actions by owned controller, not connection order")
    code = once(code,
        '''            menuNow |= touch.Menu();
            unsigned menuPressed = menuRepeat.Step(menuNow);''',
        '''            menuNow |= owners.RawMenu(Table());
            menuNow |= touch.Menu();
            // 87.443: no auto-repeat in any menu. Key/button must be
            // released before another action, irrespective of frame rate.
            unsigned menuPressed = menuNow & ~previousMenuButtons;
            previousMenuButtons = menuNow;''',
        "one press / one step")
    code = once(code,
        '''                const int optionAction = options.Press(
                    menuPressed, window, g_bindings, Table(), gamepads);''',
        '''                const int optionAction = options.Press(
                    menuPressed, window, g_bindings, Table(),
                    port.opt.settingsPath, owners);''',
        "Controller device selector")
    # 87.444: The generic continuation line occurs multiple times in some
    # locally patched frontends. Anchor the complete rendering invocation
    # to the one Options page, and leave other calls entirely untouched.
    render_begin = (
        "        if (options.Active()) { // XMVSF_RECOMP_OPTIONS_87_442: three native tabs\n"
    )
    render_end = (
        "        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu"
    )
    old_render = (
        "                homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),\n"
        "                                           options, g_bindings, Table(), gamepads);"
    )
    new_render = (
        "                homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),\n"
        "                                           options, g_bindings, Table(), owners);"
    )
    begin = code.find(render_begin)
    finish = code.find(render_end, begin + len(render_begin)) if begin >= 0 else -1
    if begin < 0 or finish < 0 or code.count(render_begin) != 1:
        raise RuntimeError("87.444 cannot uniquely locate the native Options renderer; source untouched")
    section = code[begin:finish]
    if new_render not in section:
        count = section.count(old_render)
        if count != 1:
            raise RuntimeError(
                f"87.444 Options render call: expected one complete invocation in Options block, found {count}; source untouched"
            )
        code = code[:begin] + section.replace(old_render, new_render, 1) + code[finish:]
    code = once(code,
        '''    homeTextures.Release();
    homeAudio.Close();
    presenter.Close();''',
        '''    homeTextures.Release();
    homeAudio.Close();
    owners.Close(); // 87.443 raw SDL_Joystick resources before SDL_Quit.
    presenter.Close();''',
        "clean raw joystick lifetime")
    return code

def main() -> int:
    if not TARGET.is_file() or not SRC.is_file():
        raise RuntimeError("missing frontend or ownership header; initialize pinned recomp source")
    original = TARGET.read_text(encoding="utf-8")
    updated = patch(original)  # all anchors verified before writing
    if updated != original:
        TARGET.write_text(updated, encoding="utf-8")
        print("XMVSF 87.443: persistent P1/P2 routing and press-edge menu installed")
    blob = SRC.read_bytes()
    if not OUT.is_file() or OUT.read_bytes() != blob:
        OUT.write_bytes(blob)
        print("XMVSF 87.443: joystick identity/profile header installed")
    return 0

if __name__ == "__main__":
    try: sys.exit(main())
    except Exception as err:
        print("XMVSF 87.443 controller patch FAILED:", err, file=sys.stderr)
        sys.exit(1)
