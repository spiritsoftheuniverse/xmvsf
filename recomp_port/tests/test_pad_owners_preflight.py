#!/usr/bin/env python3
"""87.450 executable preflight. No writes, no Windows compiler, no ROM work.

Runs BEFORE the controller installer, so Python regex/anchor mistakes cannot
cost another unsuccessful build attempt or mutate the local recomp submodule.
"""
from __future__ import annotations

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PATCHER = ROOT / "recomp_port" / "apply_pad_owners.py"
FRONTEND = ROOT / "recomp" / "port" / "frontend_sdl.cpp"


def run() -> None:
    spec = spec_from_file_location("xmvsf_controller_installer_preflight", PATCHER)
    assert spec and spec.loader
    patcher = module_from_spec(spec)
    spec.loader.exec_module(patcher)

    old_call = (
        "homeTextures.RenderOptions(presenter.Renderer(), SDL_GetTicks(),\n"
        "    options, g_bindings, Table(), gamepads);"
    )
    new_call = old_call.replace("gamepads);", "owners);")

    # This invokes the actual production regex, including its capture groups.
    # The previous release failed on this exact operation:
    # re.error: missing ), unterminated subpattern at position 177
    fixed = patcher._reconcile_render_call(old_call)
    assert fixed == new_call, "Options argument conversion is incorrect"
    assert patcher._reconcile_render_call(fixed) == fixed, (
        "Options argument conversion is not idempotent"
    )

    # The old broad replacement matched several unrelated lines. Explicitly
    # confirm a unique call is converted without changing those extra tails.
    decoy = "otherCall(options, g_bindings, Table(), gamepads);"
    fixture = decoy + "\n" + old_call + "\n" + decoy
    corrected = patcher._reconcile_render_call(fixture)
    assert corrected == decoy + "\n" + new_call + "\n" + decoy

    if not FRONTEND.is_file():
        raise RuntimeError("pinned recomp/port/frontend_sdl.cpp is missing")

    # Dry-run the complete installer, then run it a second time on the
    # transformed source. Nothing is written to disk by these calls.
    current = FRONTEND.read_text(encoding="utf-8")
    proposed = patcher.patch(current)
    assert patcher.patch(proposed) == proposed, (
        "controller installer fails on a second application"
    )
    print("XMVSF 87.450 preflight PASS: real regex, call matching, "
          "full source patch and second-pass idempotence (no file writes)")


if __name__ == "__main__":
    run()
