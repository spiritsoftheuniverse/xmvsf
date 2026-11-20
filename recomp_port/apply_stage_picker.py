#!/usr/bin/env python3
"""XMVSF 87.455: install Training stage picker after the existing SDL3 menu patches.

Anchored and idempotent. Validates every replacement in memory and applies twice
before writing anything. No FBNeo, ROM, generated M68K or Stable modifications.
"""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "recomp" / "port" / "frontend_sdl.cpp"
HEADER_SRC = ROOT / "recomp_port" / "xmvsf_stage_picker.h"
HEADER_DST = ROOT / "recomp" / "port" / "xmvsf_stage_picker.h"
MARK = "XMVSF_RECOMP_STAGES_87_455"


def one(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"87.455 {label}: expected one exact anchor, found {count}")
    return text.replace(old, new, 1)


def validated(text: str) -> bool:
    required = (
        '#include "xmvsf_stage_picker.h" // ' + MARK,
        "xmvsf_home::StagePicker stagePicker;",
        "stagePicker.Open();",
        "stagePicker.Press(menuPressed, stageLeftEdge, stageRightEdge)",
        "SetFeatureForNow(session::kStageFeature, stagePicker.Selected() + 1);",
        "session::StartSession(*port.machine);",
        "homeTextures.RenderStagePicker(presenter.Renderer(), SDL_GetTicks(), stagePicker);",
        "stagePicker.Release();",
    )
    for token in required:
        if text.count(token) != 1:
            raise RuntimeError(f"87.455 partially installed or duplicate token: {token}")
    if "case xmvsf_home::Action::Training:\n                    session::StartSession" in text:
        raise RuntimeError("87.455 stale Training launch still bypasses selector")
    return True


def patch(code: str) -> str:
    if MARK in code:
        validated(code)
        return code
    code = one(code,
        '#include "xmvsf_menu_textures.h" // XMVSF_RECOMP_MENU_TEXTURES_87_441\n',
        '#include "xmvsf_menu_textures.h" // XMVSF_RECOMP_MENU_TEXTURES_87_441\n'
        '#include "xmvsf_stage_picker.h" // ' + MARK + '\n', "include")
    code = one(code,
        '    xmvsf_home::TexturedMenu homeTextures(home);\n',
        '    xmvsf_home::TexturedMenu homeTextures(home);\n'
        '    xmvsf_home::StagePicker stagePicker; // ' + MARK + '\n'
        '    int stageBeforeTraining = -1; // restore the former stage when Arcade/Versus starts\n'
        '    bool stageLeftHeld = false, stageRightHeld = false;\n', "state")
    code = one(code,
        '''        if (options.Active()) {
            options.Close();
            home.Open();
''',
        '''        if (stagePicker.Active()) {
            stagePicker.Close(); // shortcut/back returns to HOME, never hides the game
            return;
        }
        if (options.Active()) {
            options.Close();
            home.Open();
''', "back")
    code = one(code,
        '''            case SDL_EVENT_KEY_DOWN:
                if (home.Enabled() && !ev.key.repeat &&
''',
        '''            case SDL_EVENT_KEY_DOWN:
                if (stagePicker.Active()) {
                    // Navigation is handled by the edge-filtered menu poll below.
                    // In particular, Backspace must NOT reach port.Key('k') / restart.
                    if (!ev.key.repeat &&
                        (ev.key.scancode == SDL_SCANCODE_ESCAPE ||
                         ev.key.scancode == SDL_SCANCODE_AC_BACK ||
                         ev.key.scancode == SDL_SCANCODE_F1)) {
                        stagePicker.Close(); // Escape/F1 return to HOME
                    }
                    break; // no gameplay/debug key side effects while picking
                }
                if (home.Enabled() && !ev.key.repeat &&
''', "keyboard event isolation")
    code = one(code,
        '''            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                if (!port.menu.IsOpen() && Shoulder(ev.gbutton.button) && ZoomHeld(SDL_GetGamepadFromID(ev.gbutton.which))) {
''',
        '''            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                if (stagePicker.Active() && Shoulder(ev.gbutton.button)) {
                    break; // selector LB/RB; must not trigger in-game restart or zoom
                }
                if (!port.menu.IsOpen() && Shoulder(ev.gbutton.button) && ZoomHeld(SDL_GetGamepadFromID(ev.gbutton.which))) {
''', "shoulder")
    code = one(code,
        '''            if (both) {
                menuPressed = 0;
            }
            if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: main choices in the native SDL3 loop
''',
        '''            if (both) {
                menuPressed = 0;
            }
            bool stageLeftNow = stagePicker.Active() && keys[SDL_SCANCODE_Q];
            bool stageRightNow = stagePicker.Active() && keys[SDL_SCANCODE_E];
            if (stagePicker.Active()) {
                for (SDL_Gamepad *gp : gamepads) {
                    stageLeftNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
                    stageRightNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
                }
            }
            const bool stageLeftEdge = !both && stageLeftNow && !stageLeftHeld;
            const bool stageRightEdge = !both && stageRightNow && !stageRightHeld;
            stageLeftHeld = stageLeftNow;
            stageRightHeld = stageRightNow;
            if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: main choices in the native SDL3 loop
                if (stagePicker.Active()) {
                    const auto stageAction = stagePicker.Press(menuPressed, stageLeftEdge, stageRightEdge);
                    if (stageAction == xmvsf_home::StagePicker::Action::Move) homeAudio.Move();
                    if (stageAction == xmvsf_home::StagePicker::Action::Back) homeAudio.Select();
                    if (stageAction == xmvsf_home::StagePicker::Action::Confirm) {
                        // Stage feature 1..11 maps to native 0..10 at FF5113.
                        // No INI write. Do not let training's stage leak into later modes.
                        if (stageBeforeTraining < 0)
                            stageBeforeTraining = FeatureSetting(session::kStageFeature);
                        SetFeatureForNow(session::kStageFeature, stagePicker.Selected() + 1);
                        homeAudio.Select();
                        session::StartSession(*port.machine);
                        stagePicker.Close();
                        home.Hide();
                    }
                } else {
''', "input")
    code = one(code,
        '''                case xmvsf_home::Action::Arcade:
                    EndSessionFeatures();
''',
        '''                case xmvsf_home::Action::Arcade:
                    if (stageBeforeTraining >= 0) {
                        SetFeatureForNow(session::kStageFeature, stageBeforeTraining);
                        stageBeforeTraining = -1;
                    }
                    EndSessionFeatures();
''', "arcade restore")
    code = one(code,
        '''                case xmvsf_home::Action::Training:
                    session::StartSession(*port.machine);
                    home.Hide();
                    break;
''',
        '''                case xmvsf_home::Action::Training:
                    stagePicker.Open(); // select before training resets the machine
                    break;
''', "training")
    code = one(code,
        '''                case xmvsf_home::Action::Versus:
                    // First slice''',
        '''                case xmvsf_home::Action::Versus:
                    if (stageBeforeTraining >= 0) {
                        SetFeatureForNow(session::kStageFeature, stageBeforeTraining);
                        stageBeforeTraining = -1;
                    }
                    // First slice''', "versus restore")
    code = one(code,
        '''                case xmvsf_home::Action::None: break;
                }
                heldAtClose = previousPad;
''',
        '''                case xmvsf_home::Action::None: break;
                }
                }
                heldAtClose = previousPad;
''', "dispatch close")
    code = one(code,
        '''        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.
''',
        '''        if (stagePicker.Active()) { // ''' + MARK + ''': no game Tick while choosing
            if (presenter.Renderer()) {
                presenter.UseClassicMenuCanvas(true);
                homeTextures.RenderStagePicker(presenter.Renderer(), SDL_GetTicks(), stagePicker);
            }
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }
        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.
''', "stage renderer")
    code = one(code,
        '''    homeTextures.Release();
    homeAudio.Close();
''',
        '''    stagePicker.Release(); // destroy thumb textures before SDL renderer closes
    homeTextures.Release();
    homeAudio.Close();
''', "release")
    validated(code)
    return code


def main() -> int:
    before = FRONTEND.read_text(encoding="utf-8")
    installed = patch(before)
    if patch(installed) != installed:
        raise RuntimeError("87.455 installer is not idempotent; no writes")
    header = HEADER_SRC.read_bytes()
    if MARK.encode() not in header:
        raise RuntimeError("87.455 stage header marker missing; no writes")
    # All anchors and source dependencies checked before any write.
    if not HEADER_DST.is_file() or HEADER_DST.read_bytes() != header:
        HEADER_DST.write_bytes(header)
    if installed != before:
        FRONTEND.write_text(installed, encoding="utf-8", newline="")
    print("XMVSF 87.455 Training stage picker ready/idempotent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
