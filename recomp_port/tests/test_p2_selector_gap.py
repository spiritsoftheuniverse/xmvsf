#!/usr/bin/env python3
"""87.476 F10 CPS-2 native OAM palette-27 selector regression."""
from pathlib import Path
from version_guard import require_matching_versions
import shutil, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[2]
def src(p): return (ROOT / p).read_text(encoding="utf-8")
def main():
    helper, phase = src("recomp/video/selector_oam.h"), src("recomp/games/xmvsf/src/select_phase.h")
    video, native = src("recomp/video/video.cpp"), src("recomp/video/native_render.cpp")
    choosing = src("recomp/games/xmvsf/src/choosing.cpp")
    session, api = src("recomp/fight/session.cpp"), src("recomp/fight/session.h")
    records, vh = src("recomp/video/records.h"), src("recomp/video/video.h")
    front = src("recomp/port/frontend_sdl.cpp")
    home, tex = src("recomp_port/xmvsf_home.h"), src("recomp_port/xmvsf_menu_textures.h")
    assert "IsP2SelectionObject" in helper and "(attributes & 0x001Fu) == 0x001Bu" in helper
    assert "phase == 4u || phase == 6u" in phase
    assert '#include "selector_oam.h"' in video and '#include "selector_oam.h"' in native
    assert "hideP2Selector && native_video::IsP2SelectionObject(attr)" in video
    assert native.count("options.hideP2Selector && IsP2SelectionObject(attr)") == 2
    assert "IsP2SelectorTile" not in video + native
    assert "bool hideP2Selector = false;" in records and "bool hideP2Selector = false;" in vh
    assert "o.hideP2Selector = hideP2Selector;" in vh
    assert "machine.video.hideP2Selector = session::InSession() && modes::Choosing();" in choosing
    assert "d.secondPickCommitted && d.secondPickCommitted()" in session
    assert "g_p1Accepted == 2" not in session
    assert "g_p1AutoFrames > 240" in session
    assert native.count("if (t.hidden || (options.hideObject &&") == 2
    assert "if (session::EnteringTraining()) {" in front
    assert "if (session::CompletingP1Selection()) {" in front
    assert "films_.Draw(renderer, now, filmReveal);" in tex
    assert require_matching_versions(home, tex, minimum=(87,476)) >= (87,476)
    cc = shutil.which("g++")
    assert cc, "MSYS2 UCRT64 g++ is required"
    smoke = ROOT/"recomp_port/tests/test_p2_selector_ink.cpp"
    with tempfile.TemporaryDirectory(prefix="xmvsf_476_") as temp:
        exe = Path(temp)/"native_oam_phase.exe"
        subprocess.run([cc, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(smoke), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("XMVSF 87.476 F10 native OAM selector and P1 phase gates PASS")
if __name__ == "__main__":
    main()
