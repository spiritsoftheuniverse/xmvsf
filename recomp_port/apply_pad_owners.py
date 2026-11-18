#!/usr/bin/env python3
"""87.450: original SDL2 physical capture, installed safely and repeatably.

Source-only anchored patch after 87.442 Options. Never modifies ROMs,
resets the user's submodule, or invalidates the 87.441 renderer design.
"""
from __future__ import annotations
from pathlib import Path
import re
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
            f"87.449 {what}: expected one anchored native block, found {count}; refusing partial write")
    return code.replace(old, new, 1)

# 87.449: The 87.448 gameplay loop is intentionally a *later edit* of
# the 87.443 replacement. On a second pass, the intermediate "new" block
# is absent and must not be demanded again. Check the finished source
# BEFORE applying any intermediate anchors. Only complete integrations may
# take this fast path; partial/foreign source still fails closed.
OWNER_INCLUDE = '#include "xmvsf_pad_owners.h" // XMVSF_RECOMP_PAD_OWNER_87_443'


def _reconcile_render_call(code: str) -> str:
    old = re.compile(
        r"(\bhomeTextures\s*\.\s*RenderOptions\s*\(\s*"
        r"presenter\s*\.\s*Renderer\s*\(\s*\)\s*,\s*"
        r"SDL_GetTicks\s*\(\s*\)\s*,\s*"
        r"options\s*,\s*g_bindings\s*,\s*"
        r"Table\s*\(\s*\)\s*,\s*)"
        r"gamepads(\s*\)\s*;)"
    )
    new = re.compile(
        r"\bhomeTextures\s*\.\s*RenderOptions\s*\(\s*"
        r"presenter\s*\.\s*Renderer\s*\(\s*\)\s*,\s*"
        r"SDL_GetTicks\s*\(\s*\)\s*,\s*"
        r"options\s*,\s*g_bindings\s*,\s*"
        r"Table\s*\(\s*\)\s*,\s*owners\s*\)\s*;"
    )
    old_count = len(old.findall(code))
    new_count = len(new.findall(code))
    if old_count == 1 and new_count == 0:
        return old.sub(lambda m: m.group(1) + "owners" + m.group(2), code, count=1)
    if old_count == 0 and new_count == 1:
        return code
    raise RuntimeError(
        f"87.449 expected one Options RenderOptions call; "
        f"old={old_count}, new={new_count}; source untouched"
    )


# Each entry belongs to the FINAL 87.448 source, not to an intermediate
# replacement that was subsequently transformed by another upgrade.
FINAL_HOOKS = (
    'xmvsf_home::PadOwners owners; // XMVSF_RECOMP_PAD_OWNER_87_443',
    'owners.Init(port.opt.settingsPath);',
    'owners.LoadSavedProfiles(g_bindings, Table());',
    'unsigned previousMenuButtons = 0;',
    'owners.Event(ev);',
    'ev, g_bindings, Table(), port.opt.settingsPath, owners)) {',
    'owners.Refresh(gamepads, g_bindings, Table());',
    'if (player >= 2 || !owners.Physical(player))',
    'SDL_Gamepad *gp = p < 2 ? owners.Gamepad(p) : owners.AdditionalGamepad(p - 2);',
    'if (p < 2 && (owners.Physical(p) || owners.Raw(p)))',
    'if (p >= 2 || !owners.Physical(p))',
    'menuNow |= owners.RawMenu(Table());',
    'unsigned menuPressed = menuNow & ~previousMenuButtons;',
    'previousMenuButtons = menuNow;',
    'port.opt.settingsPath, owners);',
    'owners.Close(); // 87.443 raw SDL_Joystick resources before SDL_Quit.',
)


def _finished_frontend(code: str) -> str | None:
    """Return a validated installed frontend; None if never installed.

    A synthetic regression may revert only the Options renderer argument.
    Repair that unique invocation without replaying the previous 87.443 and
    87.448 gameplay rewrites or changing the native pixel renderer.
    """
    if OWNER_INCLUDE not in code:
        if "XMVSF_RECOMP_PAD_OWNER_87_443" in code:
            raise RuntimeError("87.449 partial owner integration (include missing); no source rewrite")
        return None

    missing = [marker for marker in FINAL_HOOKS if marker not in code]
    if missing:
        return None  # permit a genuinely older 87.443 frontend to be upgraded
    done = _reconcile_render_call(code)
    if done.count(OWNER_INCLUDE) != 1:
        raise RuntimeError("87.449 duplicate ownership header; no source rewrite")
    if done.count('unsigned menuPressed = menuNow & ~previousMenuButtons;') != 1:
        raise RuntimeError("87.449 duplicate menu edge gate; no source rewrite")
    if done.count('owners.Refresh(gamepads, g_bindings, Table());') != 1:
        raise RuntimeError("87.449 duplicate controller refresh; no source rewrite")
    if 'menuRepeat.Step(menuNow)' in done:
        raise RuntimeError("87.449 obsolete hold-repeat path remains; no source rewrite")
    return done


def patch(code: str) -> str:
    finished = _finished_frontend(code)
    if finished is not None:
        return finished
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
    code = _reconcile_render_call(code)
    # 87.448: restore Stage13's actual physical input path after the
    # 87.443 device ownership hooks are installed. These are incremental
    # upgrades; rerunning this script leaves completed hooks untouched.
    code = once(code,
        '        owners.Refresh(gamepads);',
        '        owners.Refresh(gamepads, g_bindings, Table());',
        "refresh raw physical profiles with owner and device",
    )
    code = once(code,
        '''                    pads[player] |= ReadKeyboard(keys, player);''',
        '''                    // Stage13: a controller-owned player only
                    // receives explicitly bound physical/K actions,
                    // not the recomp's default keyboard at the same time.
                    if (player >= 2 || !owners.Physical(player))
                        pads[player] |= ReadKeyboard(keys, player);''',
        "keyboard obeys player owner",
    )
    code = once(code,
        '''                if (p < 2) pads[p] |= owners.ReadRaw(p, Table());
                if (!gp) continue;
                pads[p] |= ReadGamepad(gp, p);''',
        '''                if (p < 2 && (owners.Physical(p) || owners.Raw(p)))
                    pads[p] |= owners.ReadRaw(p, Table());
                if (!gp) continue;
                // Once a physical device is explicitly assigned, do NOT
                // also apply its generic SDL semantic pad mapping.
                if (p >= 2 || !owners.Physical(p))
                    pads[p] |= ReadGamepad(gp, p);''',
        "physical controller mapping replaces default semantic binding",
    )
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
        print("XMVSF 87.448: persistent P1/P2 routing and press-edge menu installed")
    blob = SRC.read_bytes()
    if not OUT.is_file() or OUT.read_bytes() != blob:
        OUT.write_bytes(blob)
        print("XMVSF 87.449: joystick identity/profile header synchronized")
    return 0

if __name__ == "__main__":
    try: sys.exit(main())
    except Exception as err:
        print("XMVSF 87.449 controller patch FAILED:", err, file=sys.stderr)
        sys.exit(1)
