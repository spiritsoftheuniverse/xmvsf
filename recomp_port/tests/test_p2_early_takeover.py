#!/usr/bin/env python3
"""87.476 F10-verified Training P1 second pick uses native phase 03->04."""
from pathlib import Path
from version_guard import require_matching_versions
ROOT = Path(__file__).resolve().parents[2]
def src(path): return (ROOT / path).read_text(encoding="utf-8")
def main():
    s, h = src("recomp/fight/session.cpp"), src("recomp/fight/session.h")
    tr = src("recomp/games/xmvsf/src/training.cpp")
    ch = src("recomp/games/xmvsf/src/choosing.cpp")
    phase = src("recomp/games/xmvsf/src/select_phase.h")
    f = src("recomp/port/frontend_sdl.cpp")
    textures = src("recomp_port/xmvsf_menu_textures.h")
    assert 'AddHook(HookAt(0x00B1E8), "Chosen", Chosen);' in ch
    assert "session::CharacterAccepted(n);" in ch
    assert "++g_p1Accepted;" in s and "if (g_p1Accepted == 2)" not in s
    assert "bool (*secondPickCommitted)() = nullptr;" in h
    assert "d.secondPickCommitted = [] {" in tr
    assert "xmvsf::NativeP1SecondPickCommitted(Ram8(kChoosing))" in tr
    assert "phase == 4u || phase == 6u" in phase
    assert "d.secondPickCommitted && d.secondPickCommitted()" in s
    assert "const bool earlyOwner = nativeSecondPick && !g_p1AutoFallback;" in s
    assert "SetFastForward(true); // accelerate native 04/06 tail" in s
    assert "if (g_fastP1Tail &&" in s and "(d.chosen && d.chosen(0))" in s
    assert "g_p1AutoFrames > 240" in s
    assert "machine.players[1] = (machine.players[1] & (kStart | kCoin)) | (held & ~heldOver);" in s
    assert "g_choosingTwo ? g_heldOver & held : held" in s
    assert "(!d.chosen || !d.chosen(1))" in s
    assert "if (session::EnteringTraining()) {" in f
    assert "if (session::CompletingP1Selection()) {" in f
    assert "films_.Draw(renderer, now, filmReveal);" in textures
    assert require_matching_versions(src("recomp_port/xmvsf_home.h"), textures, minimum=(87,476)) >= (87,476)
    print("XMVSF 87.476 F10 native 03->04 Training early handoff PASS")
if __name__ == "__main__":
    main()
