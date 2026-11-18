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
    # Versioned 87.436 upgrade: applies even when 87.435 already changed the
    # pinned submodule. Preserve user changes everywhere else.
    old_render = '''            home.Draw(homePicture);
            presenter.Show(homePicture.data(), 384, 224, menuWhere, nullptr, false);'''
    new_render = '''            home.Draw(homePicture, SDL_GetTicks());
            // Original Stage14 menu uses its own 960x540 canvas. Preserve the
            // 16:9 composition in the current window without stretching.
            const float fitHome = std::max(0.001f, std::min(float(width) / 960.0f, float(height) / 540.0f));
            const SDL_FRect homeWhere{(float(width) - 960.0f * fitHome) * 0.5f,
                                      (float(height) - 540.0f * fitHome) * 0.5f,
                                      960.0f * fitHome, 540.0f * fitHome};
            presenter.Show(homePicture.data(), 960, 540, homeWhere, nullptr, false);'''
    installed = TARGET.read_text(encoding="utf-8")
    if old_render in installed:
        upgraded = installed.replace(old_render, new_render, 1)
        TARGET.write_text(upgraded, encoding="utf-8")
        print("XMVSF 87.436 native 960x540 presentation upgraded")
    elif new_render not in installed and "XMVSF_RECOMP_HOME_87_438: pixel-perfect menu viewport" not in installed:
        raise RuntimeError("unknown native menu render block; refusing to overwrite")
    # 87.437: native SDL3 window hotkeys + menu PCM mixer. Apply to both
    # already-patched 87.436 worktrees and freshly initialized submodules.
    updates = [
        (
            '#include "xmvsf_home.h" // XMVSF_RECOMP_HOME_87_435\n',
            '#include "xmvsf_home.h" // XMVSF_RECOMP_HOME_87_435\n'
            '#include "xmvsf_menu_audio.h" // XMVSF_RECOMP_HOME_87_437\n'
            '#include "xmvsf_window.h"\n',
            "87.437 includes",
        ),
        (
            '    xmvsf_home::Menu home(homeEnabled);\n',
            '    xmvsf_home::Menu home(homeEnabled);\n'
            '    xmvsf_home::MenuAudio homeAudio(homeEnabled);\n',
            "87.437 menu audio",
        ),
        (
            '            case SDL_EVENT_KEY_DOWN:\n'
            '                if (!port.menu.IsOpen() && !ev.key.repeat &&\n',
            '            case SDL_EVENT_KEY_DOWN:\n'
            '                if (home.Enabled() && !ev.key.repeat &&\n'
            '                    xmvsf_home::WindowShortcut(window, ev.key.scancode, ev.key.mod)) {\n'
            '                    break; // native window shortcut, not an arcade input\n'
            '                }\n'
            '                if (!port.menu.IsOpen() && !ev.key.repeat &&\n',
            "87.437 keyboard scaling",
        ),
        (
            '                const xmvsf_home::Action choice = home.Press(menuPressed);\n'
            '                switch (choice) {\n',
            '                const int oldMenuRow = home.Selection();\n'
            '                const xmvsf_home::Action choice = home.Press(menuPressed);\n'
            '                if (home.Selection() != oldMenuRow) homeAudio.Move();\n'
            '                if (choice != xmvsf_home::Action::None) homeAudio.Select();\n'
            '                switch (choice) {\n',
            "87.437 menu navigation and select audio",
        ),
        (
            '        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.\n',
            '        homeAudio.Step(home.Visible()); // XMVSF_RECOMP_HOME_87_437\n'
            '        if (home.Visible()) { // XMVSF_RECOMP_HOME_87_435: present our menu, no game Tick.\n',
            "87.437 PCM lifecycle",
        ),
    ]
    updated = TARGET.read_text(encoding="utf-8")
    before_update = updated
    for old, new, label in updates:
        updated = replace_once(updated, old, new, label)
    if updated != before_update:
        TARGET.write_text(updated, encoding="utf-8")
        print("XMVSF 87.437 native window and audio hooks upgraded")

    # 87.438: integer pixel geometry and cached/lightweight menu rendering.
    # Game picture gets square CPS-2 pixels, without changing the M68k code.
    perf_updates = [
        (
            '    std::vector<uint32_t> homePicture;\n',
            '    std::vector<uint32_t> homeSourcePixels;\n'
            '    std::vector<uint32_t> homePicture;\n'
            '    uint64_t homeArtStep = ~uint64_t(0);\n'
            '    int homeShownSelection = -1, homeShownDivisor = -1;\n',
            '87.438 menu frame cache',
        ),
        (
            '    SDL_Window *window = makeWindow(!preset.empty(), carried ? &placed : nullptr, 0, windowHeight, port.opt.fullscreen);\n',
            '    SDL_Window *window = makeWindow(!preset.empty(), carried ? &placed : nullptr, 0, windowHeight, port.opt.fullscreen);\n'
            '    // XMVSF_RECOMP_HOME_87_438: an exact 960x540 (2x base) menu by default.\n'
            '    // Leave explicitly restored, fullscreen, and test-run windows unchanged.\n'
            '    if (window && !std::strcmp(GAME_ID, "xmvsf") && port.opt.frames == 0 &&\n'
            '        !port.net && MultiGame().empty() && !std::getenv("XMVSF_RECOMP_HOME_OFF") &&\n'
            '        !carried && !port.opt.fullscreen) SDL_SetWindowSize(window, 960, 540);\n',
            '87.438 menu initial window',
        ),
        (
            '        presenter.SetVsync(FeatureOn("display_vsync")); // (the Display page',
            '        presenter.SetVsync(home.Visible() ? false : FeatureOn("display_vsync")); // (the Display page',
            '87.438 avoid vsync stalls while dragging the menu',
        ),
        (
            '        presenter.SetScaling(DisplayScaling());\n',
            '        // XMVSF_RECOMP_HOME_87_438: no 4:3 horizontal distortion / mixed-size pixels.\n'
            '        // The 384x224 scene remains native; only the output shape changes.\n'
            '        const Shape xmShape = home.Enabled() ? Shape::SquarePixels : DisplayShape();\n'
            '        const Scaling xmScaling = home.Enabled() ? Scaling::WholePixels : DisplayScaling();\n'
            '        presenter.SetScaling(xmScaling);\n',
            '87.438 pixel-sharp scaling',
        ),
        (
            '        const SDL_FRect where = turned ? FitPicture(width, height, Video::kHeight, port.ShownWidth(), DisplayShape(), DisplayScaling())\n'
            '                                       : FitPicture(width, height, shapeColumns, Video::kHeight, DisplayShape(), DisplayScaling());\n'
            '        const SDL_FRect menuWhere = turned ? FitPicture(width, height, port.ShownWidth(), Video::kHeight, DisplayShape(), DisplayScaling()) : where;\n',
            '        const SDL_FRect where = turned ? FitPicture(width, height, Video::kHeight, port.ShownWidth(), xmShape, xmScaling)\n'
            '                                       : FitPicture(width, height, shapeColumns, Video::kHeight, xmShape, xmScaling);\n'
            '        const SDL_FRect menuWhere = turned ? FitPicture(width, height, port.ShownWidth(), Video::kHeight, xmShape, xmScaling) : where;\n',
            '87.438 square-pixel CPS-2 viewport',
        ),
        (
            '''            home.Draw(homePicture, SDL_GetTicks());
            // Original Stage14 menu uses its own 960x540 canvas. Preserve the
            // 16:9 composition in the current window without stretching.
            const float fitHome = std::max(0.001f, std::min(float(width) / 960.0f, float(height) / 540.0f));
            const SDL_FRect homeWhere{(float(width) - 960.0f * fitHome) * 0.5f,
                                      (float(height) - 540.0f * fitHome) * 0.5f,
                                      960.0f * fitHome, 540.0f * fitHome};
            presenter.Show(homePicture.data(), 960, 540, homeWhere, nullptr, false);
            SDL_Delay(8);''',
            '''            // XMVSF_RECOMP_HOME_87_438: pixel-perfect menu viewport.
            // Recompose the expensive 960x540 GIF/logo/font canvas at most
            // every 80ms, or immediately when the selection changes.
            const uint64_t uiTime = SDL_GetTicks();
            const uint64_t artStep = uiTime / 80;
            const int shownRow = home.Selection();
            if (homeSourcePixels.empty() || artStep != homeArtStep || shownRow != homeShownSelection) {
                home.Draw(homeSourcePixels, uiTime);
                homeArtStep = artStep;
                homeShownSelection = shownRow;
                homeShownDivisor = -1;
            }
            const auto pixelView = xmvsf_home::MenuViewport(width, height);
            if (homeShownDivisor != pixelView.divisor || homePicture.empty()) {
                xmvsf_home::DownsampleHome(homeSourcePixels, homePicture, pixelView);
                homeShownDivisor = pixelView.divisor;
            }
            if (!homePicture.empty()) {
                const SDL_FRect homeWhere{float(pixelView.x), float(pixelView.y),
                                          float(pixelView.w), float(pixelView.h)};
                presenter.Show(homePicture.data(), pixelView.sourceW,
                               pixelView.sourceH, homeWhere, nullptr, false);
            }
            // Smaller upload and lower idle refresh pressure during OS window moves.
            SDL_Delay(24);''',
            '87.438 cached native menu render',
        ),
    ]
    installed = TARGET.read_text(encoding="utf-8")
    before_perf = installed
    for old, new, label in perf_updates:
        installed = replace_once(installed, old, new, label)
    if installed != before_perf:
        TARGET.write_text(installed, encoding="utf-8")
        print("XMVSF 87.438 integer pixels and cached/low-overhead menu activated")

    body = SOURCE.read_text(encoding="utf-8")
    if not HEADER.exists() or HEADER.read_text(encoding="utf-8") != body:
        HEADER.write_text(body, encoding="utf-8")
        print(f"XMVSF menu drawing header updated: {HEADER}")
    # Keep upstream linked as a pinned submodule, and copy our own third-party
    # decoder into it only for local compilation (no extra SDL3_image package).
    decoder_src = ROOT / "recomp_port" / "vendor" / "stb_image.h"
    decoder_dst = ROOT / "recomp" / "port" / "xmvsf_stb_image.h"
    decoder = decoder_src.read_bytes()
    if not decoder_dst.exists() or decoder_dst.read_bytes() != decoder:
        decoder_dst.write_bytes(decoder)
        print(f"XMVSF menu decoder installed: {decoder_dst}")
    # Source-owned SDL3 mixer/window controls are local to the recomp worktree.
    for filename in ("xmvsf_menu_audio.h", "xmvsf_window.h"):
        source = ROOT / "recomp_port" / filename
        target = ROOT / "recomp" / "port" / filename
        bytes_ = source.read_bytes()
        if not target.exists() or target.read_bytes() != bytes_:
            target.write_bytes(bytes_)
            print(f"XMVSF 87.438 helper installed: {target}")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("XMVSF menu integration FAILED:", exc, file=sys.stderr)
        sys.exit(1)
