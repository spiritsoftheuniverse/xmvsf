#!/usr/bin/env python3
"""87.472 source and C++ syntax preflight for F10 native M68K capture."""
from pathlib import Path
from version_guard import require_matching_versions
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def source(name):
    return (ROOT / name).read_text(encoding="utf-8")

def main():
    header = source("recomp/port/xmvsf_m68k_capture.h")
    assert header == source("recomp_port/xmvsf_m68k_capture.h")
    front = source("recomp/port/frontend_sdl.cpp")
    machine = source("recomp/machine/machine.h")
    game = source("recomp/games/xmvsf/src/game.cpp")
    choosing = source("recomp/games/xmvsf/src/choosing.cpp")
    sess = source("recomp/fight/session.cpp")
    api = source("recomp/fight/session.h")
    home = source("recomp_port/xmvsf_home.h")
    tex = source("recomp_port/xmvsf_menu_textures.h")
    bat = source("RECOMP_BUILD.bat")
    for token in (
        "class Capture", "bool Toggle(Machine &machine", "void BeforeRouting(",
        "void AfterRouting(", "void BeforeCpu(", "void AfterFrame(",
        "void AcceptHook(", "static void WriteHook(",
        "oam_writes.tsv", "oam_delta.tsv", "frames.tsv", "hooks.tsv",
        "both_records_before_hex", "both_records_after_hex",
        "0x5180 + n * 0x20", "0x50B0", "0x5416", "0x5104",
        "machine.ObjectsToCome()", "frameTag_ = machine.frame",
        "kMaxFrames = 3600", "kMaxWrites = 650000",
        "if (!active_ || RunningAhead()) return;",
        "cpu.pc & 0xFFFFFFu", "machine.cpu.instruction",
        "std::fflush(hooks_)", "F10 M68K CAPTURE START",
    ):
        assert token in header, f"missing native F10 observation: {token}"
    assert 'ev.key.scancode == SDL_SCANCODE_F10' in front
    assert 'xmvsf_f10::Capture::Get().Toggle(*port.machine,' in front
    assert 'xmvsf_f10::Capture::Get().Stop("frontend shutdown")' in front
    assert front.index("ev.key.scancode == SDL_SCANCODE_F10") < front.index("if (touch.Event(ev))")
    assert 'void (*objectWriteProbe)(const Machine &, int bank, uint32_t busAddress)' in machine
    assert 'if (objectWriteProbe) objectWriteProbe(*this, bank, address);' in machine
    assert "xmvsf_f10::Capture::Get().BeforeRouting(machine);" in game
    assert "xmvsf_f10::Capture::Get().AfterRouting(machine);" in game
    assert "xmvsf_f10::Capture::Get().BeforeCpu(machine);" in game
    assert "xmvsf_f10::Capture::Get().AfterFrame(machine);" in game
    assert game.index("BeforeRouting(machine)") < game.index("session::BeforeFrame(machine);") < game.index("AfterRouting(machine)")
    assert 'xmvsf_f10::Capture::Get().AcceptHook(' in choosing
    assert choosing.index("AcceptHook(") < choosing.index("session::CharacterAccepted(n);")
    assert "SelectionDebug SelectionDiagnostics();" in api
    assert "SelectionDebug SelectionDiagnostics() {" in sess
    assert "if (g_fastP1Tail &&" in sess and "g_p1AutoFrames > 240" in sess
    assert "homeAudio.Fight(!port.menu.muted && !audioLog)" in front
    assert "films_.Draw(renderer, now, filmReveal);" in tex
    assert require_matching_versions(home, tex, minimum=(87, 472)) >= (87, 472)
    assert "python3 ../recomp_port/tests/test_f10_m68k_capture.py" in bat
    compiler = shutil.which("g++")
    assert compiler, "UCRT64 g++ needed to compile F10 observer preflight"
    smoke = ROOT / "recomp_port/tests/test_f10_m68k_capture_smoke.cpp"
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra",
                    "-fsyntax-only", str(smoke)], check=True)
    print("XMVSF 87.472 F10 native M68K + original select/OAM observations PASS")

if __name__ == "__main__":
    main()
