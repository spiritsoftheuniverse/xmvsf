#!/usr/bin/env python3
"""Install the XMVSF SDL3 home menu in the pinned recomp checkout.

Idempotent, anchored, fail-closed. Never resets the submodule or touches FBNeo.
Run from RECOMP_BUILD.bat. Revert only these generated changes with Git after
reviewing any additional local edits you made inside recomp/.
"""
from __future__ import annotations
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "recomp" / "port" / "frontend_sdl.cpp"
HEADER = ROOT / "recomp" / "port" / "xmvsf_home.h"
SOURCE = ROOT / "recomp_port" / "xmvsf_home.h"
MARK = "XMVSF_RECOMP_HOME_87_435"

def replace_once(text: str, old: str, new: str, label: str) -> str:
    if new in text:
        return text
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one upstream anchor, found {count}; refusing unsafe patch")
    return text.replace(old, new, 1)

def main() -> int:
    if not TARGET.exists():
        print(f"ERROR: missing upstream source: {TARGET}", file=sys.stderr)
        return 1
    before = TARGET.read_text(encoding="utf-8")
    if MARK not in before:
        original = before
        try:
            before = replace_once(before,
                '#include "bindings.h"\n',
                '#include "bindings.h"\n#include "xmvsf_home.h" // ' + MARK + '\n#include "../fight/session.h"\n',
                "include")
            before = replace_once(before,
                '    std::vector<SDL_Gamepad *> gamepads;\n',
                '''    // ''' + MARK + ''': our native front end is off for headless/automation runs.
    const bool homeEnabled = !std::strcmp(GAME_ID, "xmvsf") && port.opt.frames == 0 && !port.net &&
                             MultiGame().empty() && std::getenv("XMVSF_RECOMP_HOME_OFF") == nullptr;
    xmvsf_home::Menu home(homeEnabled);
    bool homeReturnFromOptions = false;
    std::vector<uint32_t> homePicture;
    std::vector<SDL_Gamepad *> gamepads;
''', "startup")
            before = replace_once(before,
                '''        if (port.menu.IsOpen()) {
            port.menu.Close();
        } else {
            port.MenuPicture(menuBase, menuWidth, menuHeight);
            port.menu.Open(true);
            menuDirty = true;
        }
    };
''',
                '''        if (port.menu.IsOpen()) {
            port.menu.Close();
        } else if (home.Enabled()) {
            home.Toggle(); // ''' + MARK + ''': Esc / gamepad returns to the XMVSF menu.
        } else {
            port.MenuPicture(menuBase, menuWidth, menuHeight);
            port.menu.Open(true);
            menuDirty = true;
        }
    };
''', "toggle")
            before = replace_once(before,
                '''                } else if (ev.key.scancode == SDL_SCANCODE_ESCAPE || ev.key.scancode == SDL_SCANCODE_F1 || ev.key.scancode == SDL_SCANCODE_AC_BACK ||
                           (ev.key.scancode == SDL_SCANCODE_TAB && !KeyBound(SDL_SCANCODE_TAB))) {
                    // (AC_BACK: a phone's Back)
                    toggleMenu();
''',
                '''                } else if (home.Enabled() && ev.key.scancode == SDL_SCANCODE_F1 && home.Visible()) {
                    // ''' + MARK + ''': F1 keeps upstream settings/diagnostics available.
                    home.Hide();
                    homeReturnFromOptions = true;
                    port.MenuPicture(menuBase, menuWidth, menuHeight);
                    port.menu.Open(true);
                    menuDirty = true;
                } else if (ev.key.scancode == SDL_SCANCODE_ESCAPE || ev.key.scancode == SDL_SCANCODE_F1 || ev.key.scancode == SDL_SCANCODE_AC_BACK ||
                           (ev.key.scancode == SDL_SCANCODE_TAB && !KeyBound(SDL_SCANCODE_TAB))) {
                    // (AC_BACK: a phone's Back)
                    toggleMenu();
''', "keyboard")
            before = replace_once(before,
                '''            if (port.menu.IsOpen()) {
                if (menuPressed) {
''',
                '''            if (home.Visible()) { // ''' + MARK + ''': main choices in the native SDL3 loop
                const xmvsf_home::Action choice = home.Press(menuPressed);
                switch (choice) {
                case xmvsf_home::Action::Arcade:
                    EndSessionFeatures();
                    port.machine->Reset();
                    home.Hide();
                    break;
                case xmvsf_home::Action::Training:
                    session::StartSession(*port.machine);
                    home.Hide();
                    break;
                case xmvsf_home::Action::Versus:
                    // First slice: original two-player local arcade with free play;
                    // both players press Start. Full selection lifecycle comes next.
                    EndSessionFeatures();
                    SetFeature("free_play", true);
                    port.machine->Reset();
                    home.Hide();
                    break;
                case xmvsf_home::Action::Options:
                    home.Hide();
                    homeReturnFromOptions = true;
                    port.MenuPicture(menuBase, menuWidth, menuHeight);
                    port.menu.Open(true);
                    menuDirty = true;
                    break;
                case xmvsf_home::Action::None: break;
                }
                heldAtClose = previousPad;
            } else if (port.menu.IsOpen()) {
                if (menuPressed) {
''', "buttons")
            before = replace_once(before,
                '''            heldAtClose &= previousPad;
''',
                '''            if (homeReturnFromOptions && !port.menu.IsOpen()) {
                homeReturnFromOptions = false;
                home.Open();
            }
            heldAtClose &= previousPad;
''', "return-from-options")
            before = replace_once(before,
                '''        if (port.menu.IsOpen()) {
            // The game's picture as it was, with the menu over it
''',
                '''        if (home.Visible()) { // ''' + MARK + ''': present our menu, no game Tick.
            home.Draw(homePicture);
            presenter.Show(homePicture.data(), 384, 224, menuWhere, nullptr, false);
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }
        if (port.menu.IsOpen()) {
            // The game's picture as it was, with the menu over it
''', "render")
            if MARK not in before:
                raise RuntimeError("patch marker was not installed")
            TARGET.write_text(before, encoding="utf-8")
        except Exception:
            # No writes until every anchor succeeds.
            raise
        print(f"XMVSF native menu installed: {TARGET}")
    else:
        print("XMVSF native menu already installed (no source rewrite)")
    body = SOURCE.read_text(encoding="utf-8")
    if not HEADER.exists() or HEADER.read_text(encoding="utf-8") != body:
        HEADER.write_text(body, encoding="utf-8")
        print(f"XMVSF menu drawing header updated: {HEADER}")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("XMVSF menu integration FAILED:", exc, file=sys.stderr)
        sys.exit(1)
