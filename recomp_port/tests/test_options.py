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
    # 87.447: mixing the old home-only audio step with the new combined
    # home/Options step flips visibility false/true every Options frame.
    # The mixer then repeatedly clears and rewinds the music: audible buzz.
    old_audio = module.LEGACY_AUDIO_LINE
    active_audio = module.OPTIONS_AUDIO_LINE
    audio_fixture = ("prefix\n" + old_audio * 6 + active_audio * 3 +
                     "options render\n")
    normalized = module.normalize_menu_audio_steps(audio_fixture)
    assert normalized.count(active_audio) == 1
    assert old_audio not in normalized
    assert module.normalize_menu_audio_steps(normalized) == normalized
    promoted = module.normalize_menu_audio_steps(old_audio * 4 + "tail\n")
    assert promoted.count(active_audio) == 1
    assert old_audio not in promoted
    assert module.normalize_menu_audio_steps(promoted) == promoted
    assert cleaned.startswith("begin\n") and cleaned.endswith("        if (home.Visible()) {\n")
    # Partial legacy rewrites can separate the generated audio line from
    # the generated render block. They must still be normalized exactly.
    separated = fixture.replace(pair, module.OPTIONS_DRAW_BLOCK +
                                module.OPTIONS_AUDIO_LINE, 1)
    separated_fixed = module.recover_repeated_options(separated)
    assert separated_fixed.count(module.OPTIONS_DRAW_BLOCK) == 1
    assert separated_fixed.count(module.OPTIONS_AUDIO_LINE) == 1
    assert module.recover_repeated_options(separated_fixed) == separated_fixed
    # Unknown or hand-edited Options rendering calls must not be deleted.
    malformed = fixture.replace(
        "nextFrame = SDL_GetTicksNS();", "nextFrame = CUSTOM_FRAME_CLOCK();", 1
    )
    try:
        module.recover_repeated_options(malformed)
    except RuntimeError:
        pass
    else:
        raise AssertionError("custom-modified Options blocks must be preserved")
    original = (PORT / "frontend_sdl.cpp").read_text(encoding="utf-8")
    assert original.count(module.OPTIONS_AUDIO_LINE) == 1, (
        "native Options must pump exactly one menu music stream per frontend iteration"
    )
    assert module.LEGACY_AUDIO_LINE not in original, (
        "legacy home-only audio hook conflicts with active Options and rewinds audio"
    )
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
    assert any(version in HOME.read_text(encoding="utf-8") for version in ("TEST 87.442", "TEST 87.443", "TEST 87.444", "TEST 87.445", "TEST 87.446", "TEST 87.447", "TEST 87.448"))
    assert '"GAME VERSION : V2"' in src
    assert '"TOURNAMENT : NOT PORTED"' in src
    assert "SetFeature(\"free_play\"" in src
    assert "SetFeatureValue(\"board_difficulty\"" in src
    assert "SetFeatureValue(\"board_speed\"" in src
    ownersSource = (ROOT / "recomp_port" / "xmvsf_pad_owners.h").read_text(encoding="utf-8")
    assert "recomp::Ini ini(file);" in ownersSource
    assert "ini.save()" in ownersSource
    assert "SaveRaw(" in ownersSource and "AssignFromInstance(" in ownersSource
    assert "SDL_EVENT_GAMEPAD_BUTTON_DOWN" in src
    assert "SDL_EVENT_GAMEPAD_AXIS_MOTION" in src
    assert "SDL_EVENT_KEY_DOWN" in src
    if "XMVSF_RECOMP_PAD_OWNER_87_443" in first:
        owners = (ROOT / "recomp_port" / "xmvsf_pad_owners.h").read_text(encoding="utf-8")
        assert "SDL_GetJoystickName" in owners  # real OS device identity
        assert "owners.DeviceName" in src
    else:
        assert "SDL_GetGamepadName" in src
    assert "BindingSection(false, p)" in ownersSource
    assert "SaveKeyboardMap(" in ownersSource
    assert "SDL_SetWindowSize" in src
    assert "SDL_MaximizeWindow" in src
    assert "SDL_SetWindowFullscreen" in src
    assert "editPad_" not in src, "original input UI must have no keyboard/gamepad toggle"
    assert "owners.AssignNext(" not in src, "original device assignment must be event capture"
    assert "owners.VisibleActions(player_) + 1" in src
    assert "const int deviceY = 156 + (296 - groupHeight) / 2;" in art
    assert "const int inputY = deviceY + 16 + 12;" in art
    assert "options.ControlRow() == i + 1" in art
    assert "bool AssignKeyboard(" in ownersSource and "bool AssignFromInstance(" in ownersSource
    assert "PhysicalForGamepadButton(" in ownersSource
    assert "SDL_GetJoystickName(js)" in ownersSource
    assert "RenderOptions(" in art
    assert "SDL_RenderPresent(renderer);" in art
    assert "SDL_SCALEMODE_NEAREST" in art
    assert "struct BoundTable" in actions and "using recomp::Bindings" in actions
    assert module.SOURCE.read_bytes() == module.OPTIONS.read_bytes()
    print("XMVSF 87.448: original device/keyboard capture, V2 tabs, INI profiles and GPU assets: PASS (source)")

if __name__ == "__main__":
    main()
