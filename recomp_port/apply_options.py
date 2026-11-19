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
CAPTURE_SOURCE = ROOT / "recomp_port" / "xmvsf_capture_lifecycle.h"
CAPTURE_TARGET = ROOT / "recomp" / "port" / "xmvsf_capture_lifecycle.h"
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

# Legacy hook inserted repeatedly by the 87.437 upgrade before the
# 87.442 Options page existed. Both calls in one iteration disagree on
# visibility ONLY on Options, which clears and rewinds the stream every
# frame: a short looping buzz rather than menu music.
LEGACY_AUDIO_LINE = (
    '        homeAudio.Step(home.Visible()); // XMVSF_RECOMP_HOME_87_437\n'
)


def normalize_menu_audio_steps(text: str) -> str:
    """Keep one canonical audio step, never alternate false/true on Options.

    Modify only our exact stamped generated statements; leave arbitrary user
    SDL/audio code untouched. Also handles a preinstalled 87.443 owner patch.
    """
    old = text.count(LEGACY_AUDIO_LINE)
    current = text.count(OPTIONS_AUDIO_LINE)
    if not old and current == 1:
        return text

    # A generated Options view must have at least one known audio step.
    if current == 0 and old == 0:
        raise RuntimeError("87.447 Options has no recognized menu audio step; source untouched")
    if current == 0:
        # Promote the last existing 87.437 hook to the 87.442 semantics.
        # Only one shared stream pump may run in a frontend iteration.
        text = text.replace(LEGACY_AUDIO_LINE, "", old - 1)
        text = text.replace(LEGACY_AUDIO_LINE, OPTIONS_AUDIO_LINE, 1)
    else:
        # Retain the last 87.442 step in its existing position.
        text = text.replace(LEGACY_AUDIO_LINE, "")
        text = text.replace(OPTIONS_AUDIO_LINE, "", current - 1)

    if text.count(LEGACY_AUDIO_LINE) != 0 or text.count(OPTIONS_AUDIO_LINE) != 1:
        raise RuntimeError("87.447 menu audio normalization incomplete; source untouched")
    print(f"XMVSF 87.447: removed {old} conflicting legacy audio steps "
          f"and {max(0, current - 1)} repeated active steps")
    return text



def is_complete_f6_options_pair(text: str) -> bool:
    """Recognize the two *intentional* 87.458+ Options draws.

    A native F6 screenshot renders into an offscreen texture without
    presenting; the second invocation renders to the visible window.
    Reject extra calls or changed/partial branches; never delete either.
    """
    return (
        text.count("homeTextures.RenderOptions(") == 2
        and text.count('nativeCapture.SaveMenu(presenter.Renderer(), "options"') == 1
        and text.count("const uint64_t menuTick = SDL_GetTicks();") >= 1
        and text.count("return homeTextures.RenderOptions(") == 1
        and text.count("options, g_bindings, Table(), owners, false);") == 1
        and text.count("options, g_bindings, Table(), owners);") == 1
        and "if (nativeCapture.Pending()) {" in text
        and "presenter.UseClassicMenuCanvas(true);" in text
        and OPTIONS_DRAW_BLOCK not in text
    )


def recover_repeated_options(text: str) -> str:
    """Collapse known duplicate generated code; preserve unknown source.

    Keep the last complete audio+render pair, closest to the home menu.
    Removing a pair also removes its duplicate audio Step, preserving the
    original music timing rather than leaving stale calls on every frame.
    """
    draw_count = text.count(OPTIONS_DRAW_BLOCK)
    total_calls = text.count("homeTextures.RenderOptions(")
    if is_complete_f6_options_pair(text):
        # The source is newer than the legacy 87.442 generated block.
        # Two calls are required; no deduplication should run here.
        return text
    if total_calls > 1 and total_calls != draw_count:
        raise RuntimeError(
            f"87.446 saw {total_calls} Options rendering calls but only "
            f"{draw_count} unmodified 87.442-generated branches; "
            f"refusing to discard user-modified code"
        )
    if draw_count <= 1:
        return text
    # Verify these are exactly the blocks emitted by our own 87.442
    # installer. An unknown hand-edited block is never deleted.
    paired = OPTIONS_AUDIO_LINE + OPTIONS_DRAW_BLOCK
    pair_count = text.count(paired)
    if pair_count == draw_count:
        fixed = text.replace(paired, "", pair_count - 1)
    else:
        # A previous reinstallation may have separated otherwise identical
        # generated audio and draw snippets. Remove only these verified 87.442
        # generated statements, then place one original audio step directly
        # ahead of the remaining exact draw block. Never touch any modified
        # or additional RenderOptions invocation.
        total_calls = text.count("homeTextures.RenderOptions(")
        audio_count = text.count(OPTIONS_AUDIO_LINE)
        if total_calls != draw_count or audio_count < 1:
            raise RuntimeError(
                f"87.446 cannot safely recover Options duplicates: "
                f"known_blocks={draw_count}, render_calls={total_calls}, "
                f"known_audio_lines={audio_count}; source untouched"
            )
        fixed = text.replace(OPTIONS_DRAW_BLOCK, "", draw_count - 1)
        fixed = fixed.replace(OPTIONS_AUDIO_LINE, "")
        fixed = fixed.replace(OPTIONS_DRAW_BLOCK, paired, 1)
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
    if not CAPTURE_SOURCE.is_file():
        raise RuntimeError(f"missing capture-state implementation {CAPTURE_SOURCE}")
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
        updated = normalize_menu_audio_steps(recover_repeated_options(before))
    else:
        updated = normalize_menu_audio_steps(patch(before))
    content = SOURCE.read_bytes()
    capture_content = CAPTURE_SOURCE.read_bytes()
    if b'#include "xmvsf_capture_lifecycle.h"' not in content:
        raise RuntimeError("87.451 Options must include its capture lifecycle header")
    # Install only after the complete source patch has been validated.
    if updated != before:
        FRONTEND.write_text(updated, encoding="utf-8")
        print("XMVSF 87.442 native Options UI installed")
    if not OPTIONS.exists() or OPTIONS.read_bytes() != content:
        OPTIONS.write_bytes(content)
        print("XMVSF 87.451 Options capture lifecycle synchronized")
    if not CAPTURE_TARGET.exists() or CAPTURE_TARGET.read_bytes() != capture_content:
        CAPTURE_TARGET.write_bytes(capture_content)
        print("XMVSF 87.451 capture release gate installed")
    if before == updated and OPTIONS.read_bytes() == content:
        print("XMVSF 87.442 Options patch ready/idempotent")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print("XMVSF 87.442 Options installation FAILED:", error, file=sys.stderr)
        sys.exit(1)
