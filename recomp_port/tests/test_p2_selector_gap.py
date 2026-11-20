#!/usr/bin/env python3
"""87.471: CPU/native/GPU P2 selector and ready-state fast handoff regression."""
from pathlib import Path
from version_guard import require_matching_versions
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
def source(path):
    return (ROOT/path).read_text(encoding="utf-8")
SESSION = source("recomp/fight/session.cpp")
API = source("recomp/fight/session.h")
CHOOSE = source("recomp/games/xmvsf/src/choosing.cpp")
VIDEO = source("recomp/video/video.cpp")
VIDEO_H = source("recomp/video/video.h")
RECORDS = source("recomp/video/records.h")
NATIVE = source("recomp/video/native_render.cpp")
FRONT = source("recomp/port/frontend_sdl.cpp")
HOME = source("recomp_port/xmvsf_home.h")
TEXTURES = source("recomp_port/xmvsf_menu_textures.h")

def main():
    helper = source("recomp/video/selector_pen.h")
    assert "four bitplanes" in helper or "Four" in helper or "four-planar" in helper
    assert "tile >= bytes / 128u" in helper
    assert "const unsigned pen =" in helper
    assert "pen != 15u" in helper
    assert "constexpr unsigned kP2" in helper
    assert "mask & ~kP2" in helper
    assert "#include \"selector_pen.h\"" in VIDEO
    assert "#include \"selector_pen.h\"" in NATIVE
    assert "hideP2Selector = false;" in VIDEO_H
    assert "o.hideP2Selector = hideP2Selector;" in VIDEO_H
    assert "bool hideP2Selector = false;" in RECORDS
    assert "machine.video.hideP2Selector = session::InSession() && modes::Choosing();" in CHOOSE
    assert "native_video::IsP2SelectorTile(graphics, graphicsSize, tile, attr)" in VIDEO
    assert "options.hideP2Selector && IsP2SelectorTile(m_graphics, m_graphicsSize, t.tile, attr)" in NATIVE
    assert NATIVE.count("if (t.hidden || (options.hideObject &&") == 2
    assert "void CharacterAccepted(int player)" in SESSION
    assert "g_fastP1Tail = true;" in SESSION
    assert "SetFastForward(true); // native P1 mode/speed steps" in SESSION
    assert "bool CompletingP1Selection();" in API
    assert "if (session::EnteringTraining()) {" in FRONT
    assert "if (session::CompletingP1Selection()) {" in FRONT
    assert (FRONT.index("if (session::EnteringTraining()) {") <
            FRONT.index("if (session::CompletingP1Selection()) {") <
            FRONT.index("selectReveal.Frame(SDL_GetTicks())"))
    assert "g_fastP1Tail = false;" in SESSION
    assert "g_p1AutoFrames > 240" in SESSION
    assert "ar.Pod(g_p1Accepted);" in SESSION and "ar.Pod(g_fastP1Tail);" in SESSION
    assert "homeAudio.Fight(!port.menu.muted && !audioLog)" in FRONT
    assert "films_.Draw(renderer, now, filmReveal);" in TEXTURES
    assert require_matching_versions(HOME, TEXTURES, minimum=(87,471)) >= (87,471)
    compiler = shutil.which("g++")
    assert compiler, "UCRT64 g++ required for source-native selector smoke"
    smoke = ROOT/"recomp_port/tests/test_p2_selector_ink.cpp"
    with tempfile.TemporaryDirectory(prefix="xmvsf_471_") as folder:
        binary = Path(folder)/"selector_ink_test.exe"
        subprocess.run([compiler,"-std=c++20","-Wall","-Wextra","-Werror",
                        str(smoke),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    print("XMVSF 87.471 planar native P2 cursor hidden, P1-to-dummy zero-gap PASS")

if __name__ == "__main__":
    main()
