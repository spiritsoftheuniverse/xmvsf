#!/usr/bin/env python3
"""87.481 source and C++ syntax preflight for bounded F10 native capture."""
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
    machine_cpp = source("recomp/machine/machine.cpp")
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
        "selection_ram_writes.tsv", "void SelectionWritten(",
        "selection_handoff.tsv", "void CustomGridBeforeCpu(",
        "void WriteSelectionHandoff(", "SoundQueueNext(",
        "SoundRequests(", "sound_requests_hex",
        "0x2642u", "0x2648u", "kMaxHandoffRows = 360",
        "before_.accepted != post.accepted",
        "if (accepted) std::fflush(handoff_)",
        "at >= 0xFF5180u && at < 0xFF51C0u", "selectionRamWriteProbe",
        "both_records_before_hex", "both_records_after_hex",
        "0x5180 + n * 0x20", "0x50B0", "0x5416", "0x5104",
        "machine.ObjectsToCome()", "frameTag_ = machine.frame",
        "kMaxFrames = 900", "kMaxWrites = 750",
        "kMaxBytes = 512u * 1024u", "kMaxRamWrites = 750",
        "kMaxOamRows = 1800", "kMaxOamPerSample = 32",
        'root / "latest"', "XMVSF_F10_RAW_OAM",
        "logRawWrites_ ? &Capture::WriteHook : nullptr",
        "haveOldOam_[bank]", "const bool hadSnapshot = haveOldOam_[bank]",
        "if ((framesSeen_ % 6u == 0u) || stateChanged)",
        "if ((framesSeen_ % 8u == 0u) || stateChanged)",
        'Stop("size limit")', "BytesWritten()",
        "ramWritesSeen_ >= kMaxRamWrites",
        "if (!active_ || RunningAhead()) return;",
        "cpu.pc & 0xFFFFFFu", "machine.cpu.instruction",
        "std::fflush(hooks_)", "F10 M68K CAPTURE START",
    ):
        assert token in header, f"missing native F10 observation: {token}"
    assert 'ev.key.scancode == SDL_SCANCODE_F10' in front
    assert "xmvsf_f10_" not in header, "timestamped F10 folders must not be created"
    assert "650000" not in header, "unbounded raw OAM legacy cap must not return"
    assert "const bool panel =" not in header, "per-frame panel write spam must not return"
    assert "oldBank_" not in header, "cross-bank comparison must not return"
    bank_declaration = header.index("const int bank =")
    frame_sample = header.index("if ((framesSeen_ % 8u == 0u) || stateChanged)")
    oam_sample = header.index("WriteOamDelta(machine, post, bank);")
    assert bank_declaration < frame_sample < oam_sample, (
        "OAM bank must be declared outside sampled frame scope"
    )
    assert 'auto &f10 = xmvsf_f10::Capture::Get();' in front
    assert 'f10.Toggle(*port.machine,' in front
    assert 'xmvsf_f10::Capture::Get().Stop("frontend shutdown")' in front
    assert front.index("ev.key.scancode == SDL_SCANCODE_F10") < front.index("if (touch.Event(ev))")
    assert 'void (*objectWriteProbe)(const Machine &, int bank, uint32_t busAddress)' in machine
    assert 'if (objectWriteProbe) objectWriteProbe(*this, bank, address);' in machine
    assert 'selectionRamWriteProbe' in machine
    assert 'selectionRamWriteProbe(*this, address & ~1u, value, 2);' in machine_cpp
    assert 'selectionRamWriteProbe(*this, address, value, 1);' in machine_cpp
    assert "xmvsf_f10::Capture::Get().BeforeRouting(machine);" in game
    assert "xmvsf_f10::Capture::Get().AfterRouting(machine);" in game
    assert "xmvsf_f10::Capture::Get().BeforeCpu(machine);" in game
    assert "xmvsf_f10::Capture::Get().AfterFrame(machine);" in game
    assert game.index("BeforeRouting(machine)") < game.index("session::BeforeFrame(machine);") < game.index("AfterRouting(machine)")
    assert 'xmvsf_f10::Capture::Get().AcceptHook(' in choosing
    assert "xmvsf_f10::Capture::Get().CustomGridBeforeCpu(" in choosing
    assert "machine, g_layout.index[0], g_layout.index[1]);" in choosing
    # No game-side input mask, sound insertion or change to 87.500 movement.
    assert "const bool active = Ram8(record) <= 3u && Ram8(record + 3u) < 2u;" in choosing
    assert "machine.players[n] &= static_cast<uint16_t>(~fight::kSticks);" not in choosing
    assert "void (*selectionRamWriteOverride)" in machine
    assert "if (BytesWritten() >= kMaxBytes)" in header
    assert "handoffRowsSeen_ >= kMaxHandoffRows" in header
    assert "before_.gameStep != 4u && post.gameStep != 4u" in header
    assert choosing.index("AcceptHook(") < choosing.index("session::CharacterAccepted(n);")
    assert "SelectionDebug SelectionDiagnostics();" in api
    assert "SelectionDebug SelectionDiagnostics() {" in sess
    assert "if (g_fastP1Tail &&" in sess and "g_p1AutoFrames > 240" in sess
    assert "homeAudio.Fight(!port.menu.muted && !audioLog)" in front
    assert "films_.Draw(renderer, now, filmReveal);" in tex
    assert require_matching_versions(home, tex, minimum=(87, 481)) >= (87, 481)
    assert "python3 ../recomp_port/tests/test_f10_m68k_capture.py" in bat
    compiler = shutil.which("g++")
    assert compiler, "UCRT64 g++ needed to compile F10 observer preflight"
    smoke = ROOT / "recomp_port/tests/test_f10_m68k_capture_smoke.cpp"
    include_dirs = [
        ROOT / "recomp/runtime",
        ROOT / "recomp/machine",
        ROOT / "recomp/fight",
        ROOT / "recomp/video",
        ROOT / "recomp/sound",
        ROOT / "recomp/port",
    ]
    # The upstream M68K/Z80 virtual bus headers intentionally leave stub
    # parameters unnamed-in-body. Keep strict errors for new diagnostics,
    # but do not fail this smoke test on those pre-existing warnings.
    command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror",
               "-Wno-error=unused-parameter", "-fsyntax-only"]
    for directory in include_dirs:
        command.extend(["-I", str(directory)])
    command.append(str(smoke))
    subprocess.run(command, check=True)
    print("XMVSF 87.504 F10 compact native handoff, queued sound requests and C++ smoke PASS")

if __name__ == "__main__":
    main()
