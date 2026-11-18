#!/usr/bin/env python3
"""Offline sanity test for the versioned menu installer.

Runs against a TEMP COPY of the user's local upstream frontend, never writes
to the real submodule, build cache, or Stable/TEST binaries.
"""
from pathlib import Path
import importlib.util
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "recomp" / "port" / "frontend_sdl.cpp"
SCRIPT = ROOT / "recomp_port" / "apply_menu.py"
OWN_HEADER = ROOT / "recomp_port" / "xmvsf_home.h"
DECODER = ROOT / "recomp_port" / "vendor" / "stb_image.h"

def main():
    if not SOURCE.is_file():
        raise RuntimeError("initialize recomp submodule first")
    for path in (SCRIPT, OWN_HEADER, DECODER):
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
        module.ROOT = sandbox
        module.TARGET = clone
        module.HEADER = sandbox / "recomp" / "port" / "xmvsf_home.h"
        module.SOURCE = sandbox / "recomp_port" / "xmvsf_home.h"
        assert module.main() == 0
        first = clone.read_bytes()
        assert b"home.Draw(homePicture, SDL_GetTicks())" in first
        assert b"presenter.Show(homePicture.data(), 960, 540" in first
        assert b"session::StartSession(*port.machine)" in first
        assert module.HEADER.read_bytes() == OWN_HEADER.read_bytes()
        assert (sandbox / "recomp" / "port" / "xmvsf_stb_image.h").read_bytes() == DECODER.read_bytes()
        assert module.main() == 0
        assert clone.read_bytes() == first, "installer is not idempotent"
    print("XMVSF 87.436 menu installer: PASS (isolated, idempotent, pixel layout + assets)")

if __name__ == "__main__":
    main()
