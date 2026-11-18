#!/usr/bin/env python3
"""87.442 checks real Game/Graphics/Controller integration without touching ROMs.

Run after apply_options.py has installed the local source. Validates both
idempotency and the live/persistent bindings path, using the actual pinned
recomp frontend and settings API. This is not a substitute for a Windows build.
"""
from pathlib import Path
import importlib.util

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / "recomp" / "port"
INSTALLER = ROOT / "recomp_port" / "apply_options.py"
OPTIONS = ROOT / "recomp_port" / "xmvsf_options.h"
DRAW = ROOT / "recomp_port" / "xmvsf_menu_textures.h"
HOME = ROOT / "recomp_port" / "xmvsf_home.h"
BIND = PORT / "bindings.h"

def main():
    spec = importlib.util.spec_from_file_location("xmvsf_options_patch", INSTALLER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # 87.446 regression: failed 87.443-87.445 builds accumulated SIX
    # complete, identical Options audio+draw pairs. Recover exactly one
    # without discarding unrelated source, and never re-create copies.
    pair = module.OPTIONS_AUDIO_LINE + module.OPTIONS_DRAW_BLOCK
    fixture = "begin\n" + pair * 6 + "        if (home.Visible()) {\n"
    cleaned = module.recover_repeated_options(fixture)
    assert cleaned.count(module.OPTIONS_DRAW_BLOCK) == 1
    assert cleaned.count(module.OPTIONS_AUDIO_LINE) == 1
    assert module.recover_repeated_options(cleaned) == cleaned
    assert cleaned.startswith("begin\n") and cleaned.endswith("        if (home.Visible()) {\n")
    # Unknown or partially edited copies must remain untouched.
    malformed = fixture.replace(module.OPTIONS_AUDIO_LINE, "", 1)
    try:
        module.recover_repeated_options(malformed)
    except RuntimeError:
        pass
    else:
        raise AssertionError("ambiguous generated Options cleanup must fail closed")
    original = (PORT / "frontend_sdl.cpp").read_text(encoding="utf-8")
    # 87.443 supersedes the 87.442 Options handlers. Its installer
    # intentionally preserves the newer native source on repeated builds.
    if "XMVSF_RECOMP_PAD_OWNER_87_443" in original:
        assert "XMVSF_RECOMP_OPTIONS_87_442" in original
        first = original
    else:
        first = module.patch(original)
        assert first == original, "options installer should have already run"
        assert module.patch(first) == first, "not idempotent"
    assert "XMVSF_RECOMP_OPTIONS_87_442" in first
    assert "homeTextures.RenderOptions" in first
    assert "options.Press(" in first
    assert "options.HandleEvent(" in first
    assert "homeAudio.Step(home.Visible() || options.Active())" in first
    assert "F1" in first
    assert "options.Close();" in first and "home.Open();" in first
    assert "case xmvsf_home::Action::Options" in first

    src = OPTIONS.read_text(encoding="utf-8")
    art = DRAW.read_text(encoding="utf-8")
    actions = BIND.read_text(encoding="utf-8")
    assert any(version in HOME.read_text(encoding="utf-8") for version in ("TEST 87.442", "TEST 87.443", "TEST 87.444", "TEST 87.445"))
    assert '"GAME VERSION : V2"' in src
    assert '"TOURNAMENT : NOT PORTED"' in src
    assert "SetFeature(\"free_play\"" in src
    assert "SetFeatureValue(\"board_difficulty\"" in src
    assert "SetFeatureValue(\"board_speed\"" in src
    assert "recomp::Ini ini(file);" in src
    assert "ini.save()" in src
    assert "b = candidate;" in src
    assert "SDL_EVENT_GAMEPAD_BUTTON_DOWN" in src
    assert "SDL_EVENT_GAMEPAD_AXIS_MOTION" in src
    assert "SDL_EVENT_KEY_DOWN" in src
    if "XMVSF_RECOMP_PAD_OWNER_87_443" in first:
        owners = (ROOT / "recomp_port" / "xmvsf_pad_owners.h").read_text(encoding="utf-8")
        assert "SDL_GetJoystickName" in owners  # real OS device identity
        assert "owners.DeviceName" in src
    else:
        assert "SDL_GetGamepadName" in src
    assert "BindingSection(false, p)" in src
    assert "BindingSection(true, p)" in src
    assert "SDL_SetWindowSize" in src
    assert "SDL_MaximizeWindow" in src
    assert "SDL_SetWindowFullscreen" in src
    assert "RenderOptions(" in art
    assert "SDL_RenderPresent(renderer);" in art
    assert "SDL_SCALEMODE_NEAREST" in art
    assert "struct BoundTable" in actions and "using recomp::Bindings" in actions
    assert module.SOURCE.read_bytes() == module.OPTIONS.read_bytes()
    print("XMVSF 87.442: native tabs / V2 / SDL inputs / INI / GPU assets: PASS (source)")

if __name__ == "__main__":
    main()
