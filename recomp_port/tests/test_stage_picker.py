#!/usr/bin/env python3
"""87.456: local source preflight for Training stage thumbnails and native routing."""
from __future__ import annotations

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "recomp_port"
RECOMP = ROOT / "recomp"
STAGES = RECOMP / "games/xmvsf/src/modes.cpp"
NAMES = RECOMP / "games/xmvsf/src/xmvsf.h"
FRONTEND = RECOMP / "port/frontend_sdl.cpp"

ORIGINAL_FILES = tuple(f"{i}.png" for i in range(11))
EXTRA_FILES = (
    "01_wilystage.png",
    "02_finaldestination.png",
    "03_guilestage.png",
    "04_sigmastage.png",
    "05_akumastage.png",
    "06_ryustage.png",
    "07_boomerstage.png",
)


def main() -> int:
    picker = (SRC / "xmvsf_stage_picker.h").read_text(encoding="utf-8")
    installed_picker = (RECOMP / "port/xmvsf_stage_picker.h").read_text(encoding="utf-8")
    source_menu = (SRC / "xmvsf_menu_textures.h").read_text(encoding="utf-8")
    installed_menu = (RECOMP / "port/xmvsf_menu_textures.h").read_text(encoding="utf-8")
    frontend = FRONTEND.read_text(encoding="utf-8")
    modes = STAGES.read_text(encoding="utf-8")
    names = NAMES.read_text(encoding="utf-8")
    assert picker == installed_picker, "source/installed picker headers differ"
    assert source_menu == installed_menu, "source/installed texture menu differs"
    assert "constexpr int kStages = 11;" in names
    assert "constexpr uint32_t kStage = 0xFF5113;" in names
    assert 'AddHook(HookAt(0x00BAEA), "StageChosen", StageChosen);' in modes
    assert "SetRam8(kStage, static_cast<uint8_t>(stage - 1));" in modes
    assert "kOriginals = 11;" in picker and "kExtras = 7;" in picker
    assert "kCell = 150;" in picker and "kPageSize = 10;" in picker
    assert "SDL_SCALEMODE_NEAREST" in picker
    assert "XMVSF_RECOMP_STAGES_87_455" in picker and frontend
    assert "stagePicker.Open();" in frontend and "stagePicker.Press(" in frontend
    assert "homeTextures.RenderStagePicker(" in frontend
    assert "stagePicker.Release();" in frontend
    assert "stagePicker.Active() && Shoulder(ev.gbutton.button)" in frontend
    assert "stageLeftEdge" in frontend and "stageRightEdge" in frontend
    assert "stageBeforeTraining" in frontend
    assert "SetFeatureForNow(session::kStageFeature, stagePicker.Selected() + 1);" in frontend
    assert frontend.count("session::StartSession(*port.machine);") == 1
    assert "SetFeatureValue(session::kStageFeature, stagePicker.Selected() + 1)" not in frontend
    assert "XMVSF2 STAGE NOT PORTED TO RECOMP" in picker
    assert "RenderStagePicker(SDL_Renderer *renderer" in source_menu
    assert 'std::to_string(i) + ".png"' in picker, "stage index filename mapping absent"
    for i, filename in enumerate(ORIGINAL_FILES):
        assert (ROOT / "_art/stagethumbs" / filename).is_file(), f"missing original thumbnail {i}: {filename}"
    for i, filename in enumerate(EXTRA_FILES):
        assert (ROOT / "_art/stagethumbs/xmvsf2" / filename).is_file(), f"missing XMVSF2 thumbnail {i}: {filename}"
        assert picker.count(f'"{filename}"') == 1, f"XMVSF2 order not explicit at {i}: {filename}"
    spec = importlib.util.spec_from_file_location("apply_stage_picker", SRC / "apply_stage_picker.py")
    assert spec and spec.loader
    import sys
    from types import ModuleType
    installer: ModuleType = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = installer
    spec.loader.exec_module(installer)
    assert installer.patch(frontend) == frontend, "stage installer not idempotent"

    assert 'XMVSF 87.456 stage picker' in picker
    print("XMVSF 87.456: all 18 committed thumbnails, native stage mapping and idempotence PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
