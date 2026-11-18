#!/usr/bin/env python3
"""87.442: replace the upstream Options entry with our 3-tab XMVSF UI.

Installs after apply_menu.py, apply_classic_renderer.py and
apply_textured_menu.py. Uses the runtime's own Bindings/Ini/Feature APIs.
Never rewrites ROM code, translates M68k, touches Stable or resets a submodule.
All anchors must match before any file is written; safe to rerun.
"""
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "recomp" / "port" / "frontend_sdl.cpp"
OPTIONS = ROOT / "recomp" / "port" / "xmvsf_options.h"
SOURCE = ROOT / "recomp_port" / "xmvsf_options.h"
MARK = "XMVSF_RECOMP_OPTIONS_87_442"


def replace_once(source: str, old: str, new: str, name: str) -> str:
    if new in source:
        return source
    n = source.count(old)
    if n != 1:
        raise RuntimeError(f"87.442 {name}: expected one native source anchor, found {n}; no changes written")
    return source.replace(old, new, 1)


# Exact code emitted by 87.442. The 87.437 installer previously injected
# a fresh audio hook before "if (home.Visible())" on every unsuccessful
# build. The 87.442 installer then wrapped that new suffix in another
# identical Options branch. Recover ONLY those exact generated blocks.
OPTIONS_DRAW_BLOCK = '''        if (options.Active()) { // XMVSF_RECOMP_OPTIONS_87_442: three native tabs
            if (presenter.Renderer()) {
                presenter.UseClassicMenuCanvas(true);
                homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),
                                           options, g_bindings, Table(), gamepads);
            }
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }
'''
OPTIONS_AUDIO_LINE = (
    '        homeAudio.Step(home.Visible() || options.Active()); // '
    'XMVSF_RECOMP_OPTIONS_87_442\n'
)


def recover_repeated_options(text: str) -> str:
    """Collapse known duplicate generated code; preserve unknown source.

    Keep the last complete audio+render pair, closest to the home menu.
    Removing a pair also removes its duplicate audio Step, preserving the
    original music timing rather than leaving stale calls on every frame.
    """
    draw_count = text.count(OPTIONS_DRAW_BLOCK)
    if draw_count <= 1:
        return text
    # Verify these are exactly the blocks emitted by our own 87.442
    # installer. An unknown hand-edited block is never deleted.
    paired = OPTIONS_AUDIO_LINE + OPTIONS_DRAW_BLOCK
    pair_count = text.count(paired)
    if pair_count != draw_count:
        raise RuntimeError(
            f"87.446 found {draw_count} generated Options blocks but only "
            f"{pair_count} matching audio/render pairs. Refusing unsafe cleanup."
        )
    fixed = text.replace(paired, "", pair_count - 1)
    assert fixed.count(OPTIONS_DRAW_BLOCK) == 1
    assert fixed.count(paired) == 1
    print(f"XMVSF 87.446: removed {pair_count - 1} repeated Options/audio branches")
    return fixed


def patch(text: str) -> str:
    text = replace_once(
        text,
        '#include "xmvsf_menu_textures.h" // XMVSF_RECOMP_MENU_TEXTURES_87_441\n',
        '#include "xmvsf_menu_textures.h" // XMVSF_RECOMP_MENU_TEXTURES_87_441\n'
        '#include "xmvsf_options.h" // ' + MARK + '\n',
        "Options header include",
    )
    text = replace_once(
        text,
        '    xmvsf_home::TexturedMenu homeTextures(home);\n',
        '    xmvsf_home::TexturedMenu homeTextures(home);\n'
        '    xmvsf_home::OptionsPanel options; // ' + MARK + '\n',
        "Options controller ownership",
    )
    text = replace_once(
        text,
        '''        if (port.menu.IsOpen()) {
            port.menu.Close();
        } else if (home.Enabled()) {''',
        '''        if (options.Active()) {
            options.Close();
            home.Open();
        } else if (port.menu.IsOpen()) {
            port.menu.Close();
        } else if (home.Enabled()) {''',
        "Esc / guide returns from custom Options",
    )
    text = replace_once(
        text,
        '''                } else if (home.Enabled() && ev.key.scancode == SDL_SCANCODE_F1 && home.Visible()) {
                    // XMVSF_RECOMP_HOME_87_435: F1 keeps upstream settings/diagnostics available.
                    home.Hide();
                    homeReturnFromOptions = true;
                    port.MenuPicture(menuBase, menuWidth, menuHeight);
                    port.menu.Open(true);
                    menuDirty = true;''',
        '''                } else if (home.Enabled() && ev.key.scancode == SDL_SCANCODE_F1 &&
                           home.Visible() && !ev.key.repeat) {
                    // 87.442 F1 = our Options; Shift+F1 retains the separate
                    // upstream diagnostics as an explicit engineering escape.
                    home.Hide();
                    if (presenter.Renderer() && !(ev.key.mod & SDL_KMOD_SHIFT)) {
                        options.Open();
                    } else {
                        homeReturnFromOptions = true;
                        port.MenuPicture(menuBase, menuWidth, menuHeight);
                        port.menu.Open(true);
                        menuDirty = true;
                    }''',
        "F1 opens native Options instead of upstream config",
    )
    text = replace_once(
        text,
        '''                case xmvsf_home::Action::Options:
                    home.Hide();
                    homeReturnFromOptions = true;
                    port.MenuPicture(menuBase, menuWidth, menuHeight);
                    port.menu.Open(true);
                    menuDirty = true;
                    break;''',
        '''                case xmvsf_home::Action::Options:
                    home.Hide();
                    if (presenter.Renderer()) {
                        options.Open(); // XMVSF_RECOMP_OPTIONS_87_442
                    } else {
                        // GPU/headless diagnostic fallback only. Our ordinary
                        // SDL3 2D path always uses the real custom Options.
                        homeReturnFromOptions = true;
                        port.MenuPicture(menuBase, menuWidth, menuHeight);
                        port.menu.Open(true);
                        menuDirty = true;
                    }
                    break;''',
        "Options selection replaces the recomp generic menu",
    )
    text = replace_once(
        text,
        '''            switch (ev.type) {
            case SDL_EVENT_QUIT: running = false; break;''',
        '''            // XMVSF_RECOMP_OPTIONS_87_442: capture key / pad remaps
            // before game shortcuts. Captured inputs never reach the game.
            if (options.Active() && options.HandleEvent(
                    ev, g_bindings, Table(), port.opt.settingsPath, gamepads)) {
                if (!options.Active()) home.Open();
                continue;
            }
            switch (ev.type) {
            case SDL_EVENT_QUIT: running = false; break;''',
        "capture and tab events before upstream game inputs",
    )
    text = replace_once(
        text,
        '''                heldAtClose = previousPad;
            } else if (port.menu.IsOpen()) {
                if (menuPressed) {''',
        '''                heldAtClose = previousPad;
            } else if (options.Active()) {
                const int optionAction = options.Press(
                    menuPressed, window, g_bindings, Table(), gamepads);
                if (optionAction == 1) homeAudio.Move();
                if (optionAction == 2 || optionAction == 3) homeAudio.Select();
                if (!options.Active()) home.Open();
                heldAtClose = previousPad;
            } else if (port.menu.IsOpen()) {
                if (menuPressed) {''',
        "Options navigation through native menu-repeat input",
    )
    text = replace_once(
        text,
        '        homeAudio.Step(home.Visible()); // XMVSF_RECOMP_HOME_87_437\n',
        '        homeAudio.Step(home.Visible() || options.Active()); // XMVSF_RECOMP_OPTIONS_87_442\n',
        "keep original menu music alive on Options",
    )
    text = replace_once(
        text,
        '''        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.''',
        '''        if (options.Active()) { // XMVSF_RECOMP_OPTIONS_87_442: three native tabs
            if (presenter.Renderer()) {
                presenter.UseClassicMenuCanvas(true);
                homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),
                                           options, g_bindings, Table(), gamepads);
            }
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }
        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.''',
        "render original-style Game/Graphics/Controller page",
    )
    return text


def main() -> int:
    if not FRONTEND.exists():
        raise RuntimeError(f"missing pinned source {FRONTEND}")
    if not SOURCE.exists():
        raise RuntimeError(f"missing Options implementation {SOURCE}")
    before = FRONTEND.read_text(encoding="utf-8")
    # 87.446: native Options is already installed as soon as its include
    # marker exists, whether or not a later controller upgrade succeeded.
    # The legacy renderer insertion contained its own old anchor; replaying
    # it after the older 87.437 audio hook rewrote the surrounding text
    # accumulated duplicate Options branches in each failed build.
    if "XMVSF_RECOMP_OPTIONS_87_442" in before:
        if ('#include "xmvsf_options.h" // XMVSF_RECOMP_OPTIONS_87_442' not in before
                or 'xmvsf_home::OptionsPanel options;' not in before
                or 'homeTextures.RenderOptions(' not in before):
            raise RuntimeError("87.446 incomplete Options integration; refusing to reinstall over unknown source")
        updated = recover_repeated_options(before)
    else:
        updated = patch(before)
    content = SOURCE.read_bytes()
    # Install only after the complete source patch has been validated.
    if updated != before:
        FRONTEND.write_text(updated, encoding="utf-8")
        print("XMVSF 87.442 native Options UI installed")
    if not OPTIONS.exists() or OPTIONS.read_bytes() != content:
        OPTIONS.write_bytes(content)
        print("XMVSF 87.442 input/feature/graphics bindings installed")
    if before == updated and OPTIONS.read_bytes() == content:
        print("XMVSF 87.442 Options patch ready/idempotent")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print("XMVSF 87.442 Options installation FAILED:", error, file=sys.stderr)
        sys.exit(1)
