#!/usr/bin/env python3
"""Offline sanity test for the versioned menu installer.

Runs against a TEMP COPY of the user's local upstream frontend, never writes
to the real submodule, build cache, or Stable/TEST binaries.
"""
from pathlib import Path
import ast
import importlib.util
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "recomp" / "port" / "frontend_sdl.cpp"
SCRIPT = ROOT / "recomp_port" / "apply_menu.py"
OWN_HEADER = ROOT / "recomp_port" / "xmvsf_home.h"
DECODER = ROOT / "recomp_port" / "vendor" / "stb_image.h"
AUDIO = ROOT / "recomp_port" / "xmvsf_menu_audio.h"
WINDOW = ROOT / "recomp_port" / "xmvsf_window.h"

def main():
    if not SOURCE.is_file():
        raise RuntimeError("initialize recomp submodule first")
    for path in (SCRIPT, OWN_HEADER, DECODER, AUDIO, WINDOW):
        if not path.is_file():
            raise RuntimeError(f"required source not present: {path}")
    spec = importlib.util.spec_from_file_location("xmvsf_menu_installer", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix="xmvsf_recomp_menu_") as folder:
        sandbox = Path(folder)
        (sandbox / "recomp" / "port").mkdir(parents=True)
        (sandbox / "recomp_port" / "vendor").mkdir(parents=True)
        clone = sandbox / "recomp" / "port" / "frontend_sdl.cpp"
        shutil.copy2(SOURCE, clone)
        shutil.copy2(OWN_HEADER, sandbox / "recomp_port" / "xmvsf_home.h")
        shutil.copy2(DECODER, sandbox / "recomp_port" / "vendor" / "stb_image.h")
        shutil.copy2(AUDIO, sandbox / "recomp_port" / "xmvsf_menu_audio.h")
        shutil.copy2(WINDOW, sandbox / "recomp_port" / "xmvsf_window.h")
        module.ROOT = sandbox
        module.TARGET = clone
        module.HEADER = sandbox / "recomp" / "port" / "xmvsf_home.h"
        module.SOURCE = sandbox / "recomp_port" / "xmvsf_home.h"
        assert module.main() == 0
        first = clone.read_bytes()
        assert b"home.Draw(homePicture, SDL_GetTicks())" in first
        assert b"presenter.Show(homePicture.data(), 960, 540" in first
        assert b"session::StartSession(*port.machine)" in first
        assert b"xmvsf_home::WindowShortcut(window, ev.key.scancode, ev.key.mod)" in first
        assert b"homeAudio.Move()" in first and b"homeAudio.Select()" in first
        assert b"homeAudio.Step(home.Visible())" in first
        # The 87.438 generated source may be present in an existing submodule.
        # The 87.439 installer must remove every generated override.
        assert b"XMVSF_RECOMP_HOME_87_438" not in first
        assert b"presenter.SetScaling(DisplayScaling());" in first
        assert b"home.Draw(homePicture, SDL_GetTicks());" in first
        assert b"SDL_Delay(8);" in first
        assert module.HEADER.read_bytes() == OWN_HEADER.read_bytes()
        assert (sandbox / "recomp" / "port" / "xmvsf_stb_image.h").read_bytes() == DECODER.read_bytes()
        assert (sandbox / "recomp" / "port" / "xmvsf_menu_audio.h").read_bytes() == AUDIO.read_bytes()
        assert (sandbox / "recomp" / "port" / "xmvsf_window.h").read_bytes() == WINDOW.read_bytes()
        assert module.main() == 0
        assert clone.read_bytes() == first, "installer is not idempotent"
        # Simulate the precise, already-installed 87.438 frontend in a
        # sandbox and ensure 87.439 reverses only its six added blocks.
        tree = ast.parse(SCRIPT.read_text(encoding="utf-8"))
        restore = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                       and n.name == "restore_87_438")
        rule_assignment = next(n for n in restore.body
                               if isinstance(n, ast.Assign) and
                               any(isinstance(t, ast.Name) and t.id == "perf_updates"
                                   for t in n.targets))
        rules = ast.literal_eval(rule_assignment.value)
        assert len(rules) == 6
        altered = clone.read_text(encoding="utf-8")
        for old, changed, label in rules:
            assert altered.count(old) == 1, f"87.438 simulation anchor: {label}"
            altered = altered.replace(old, changed, 1)
        assert "XMVSF_RECOMP_HOME_87_438: pixel-perfect menu viewport" in altered
        clone.write_text(altered, encoding="utf-8")
        assert module.main() == 0, "87.438 -> 87.437 rollback failed"
        assert clone.read_bytes() == first, "87.438 rollback did not restore exact 87.437 frontend"
        assert module.main() == 0
        assert clone.read_bytes() == first, "rollback did not stay idempotent"
    print("XMVSF 87.439 installer: PASS (87.438 graphics rollback) (isolated, idempotent, window + audio + assets)")

if __name__ == "__main__":
    main()
