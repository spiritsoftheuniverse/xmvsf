#!/usr/bin/env python3
"""87.498: native 18-slot composition assets, FDBs, mirrored code, C++ navigation smoke."""
from __future__ import annotations
import pathlib
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
ART = ROOT / "_art/menu/characterselect"
SRC = ROOT / "recomp/port"
MIRROR = ROOT / "recomp_port"

def fdb(path: pathlib.Path, expected):
    with path.open("rb") as fh:
        magic, width, height, frames, fmt, stride, unused = struct.unpack(
            "<8s6I", fh.read(32))
        assert (magic, width, height, frames, fmt, stride, unused) == (
            b"XMFDBIN2", *expected, 2, expected[0] * 4, 0), path
        last_end = 32 + 16 * frames
        total = path.stat().st_size
        for _ in range(frames):
            offset, size, reserved = struct.unpack("<QII", fh.read(16))
            assert offset == last_end and size >= 8 and reserved == 0
            last_end = offset + size
            assert last_end <= total
        assert last_end == total

def main():
    for name in ["xmvsf_character_select_layout.h",
                 "xmvsf_character_select_rewrite.h",
                 "xmvsf_character_select_backdrop.h",
                 "xmvsf_character_select_screen.h",
                 "xmvsf_menu_textures.h"]:
        assert (SRC / name).read_bytes() == (MIRROR / name).read_bytes(), name
    fdb(ART / "csbackground.fdb", (384, 224, 166))
    fdb(ART / "portraitbackground.fdb", (160, 160, 89))
    layout = (SRC / "xmvsf_character_select_layout.h").read_text()
    portraits = re.findall(r'"(layer_\d+_0x[0-9A-Fa-f]+_[^"]+\.png)"', layout)
    assert len(portraits) == 18
    all_portraits = list((ART / "portraits").glob("*.png"))
    assert len(all_portraits) >= 19
    for name in portraits:
        assert (ART / "portraits" / name).is_file(), name
    layers = ["Characters.png", "character grid.png", "1p Selector.png",
              "2p Selector.png", "Portrait Mask L.png",
              "Portrait Mask R.png", "Stage Brace.png"]
    for name in layers:
        image = ART / "screencomposition" / name
        with image.open("rb") as fh: head = fh.read(24)
        assert head[:8] == b"\x89PNG\r\n\x1a\n"
        assert struct.unpack_from(">II", head, 16) == (384, 224), image
    names = (ROOT / "_art/stagethumbs/levelnames.csv").read_text().splitlines()
    assert len(names) >= 18 and names[0] == "Apocalypse Now!"
    # Guard against mistakenly showing the brace before an actual signal.
    screen = (SRC / "xmvsf_character_select_screen.h").read_text()
    assert "stageVisible = false" in screen
    assert 'Cursor(p1Cursor_, 9, a)' in screen
    assert 'Cursor(p2Cursor_, 8, b)' in screen
    # The second pick ends selector inversion independently for each side.
    assert "if (ink && showP1 && cursorP1 && a >= 0)" in screen
    assert "if (ink && showP2 && cursorP2 && b >= 0)" in screen
    frontend = (SRC / "frontend_sdl.cpp").read_text()
    chooser = (ROOT / "recomp/games/xmvsf/src/choosing.cpp").read_text()
    game = (ROOT / "recomp/games/xmvsf/src/game.cpp").read_text()
    assert "homeTextures.ComposeCharacterSelect(" in frontend
    # F10-confirmed mismatch at frame 884: ROM cursor Gambit (0x08),
    # but the custom grid and post-commit pick are Rogue (0x06).
    # Rendering MUST use the committed/custom ID, not transient cursor.
    assert "cps2recomp::ChoosingVisualCharacterId(0u)" in frontend
    assert "cps2recomp::ChoosingVisualCharacterId(1u)" in frontend
    assert "cursor(0x518Cu), cursor(0x51ACu)" not in frontend
    assert "uint16_t ChoosingVisualCharacterId(unsigned side)" in chooser
    assert "if (!g_layoutRunning) return Ram16(record + kCursor);" in chooser
    assert "if (Ram8(record + 3u) >= 2u)" in chooser
    assert "return Ram16(kPicks + static_cast<uint32_t>(side) * 4u + 2u);" in chooser
    assert "return port::xmvsf_select::kSlots[slot].rom;" in chooser
    # The working 87.500 selector and audio path must not be touched.
    assert "const bool active = Ram8(record) <= 3u && Ram8(record + 3u) < 2u;" in chooser
    assert "machine.players[n] &= static_cast<uint16_t>(~fight::kSticks);" not in chooser
    assert "const unsigned direction = machine.players[n] & (kRight | kLeft | kDown | kUp);" in chooser
    assert "ChoosingLayoutBeforeCpu(machine);" in game
    # Native ROM must still receive real directions to generate move sounds.
    assert "machine.players[n] &= static_cast<uint16_t>(~fight::kSticks);" not in chooser
    assert "machine.selectionRamWriteOverride = &RemapSelectionWrite;" in chooser
    assert "machine.selectionRamWriteOverride = nullptr;" in chooser
    # 87.508 does not install a translated ROM/M68K hook: the prior
    # 0787E0 hook crashed upon the stage/level-select handoff.
    assert "AddHook(HookAt(0x0787E0)" not in chooser
    assert "CorrectNativeQueuedVoice(Machine &machine)" in chooser
    assert "CorrectQueuedCharacterVoice(" in chooser
    assert "NativeCharacterVoice(g_voiceDesired)" in chooser
    assert "g_voiceReadAt = Ram16(kSoundQueueEnd);" in chooser
    assert "const unsigned diff =" in chooser
    assert "if (diff > 128u)" in chooser
    assert "g_voiceHandled = true;" in chooser
    assert "if (g_voiceSide >= 0 && g_voiceReadAt <= kSoundQueueMask" in chooser
    assert "machine.selectionVoiceQueueBeforeSound = &CorrectNativeQueuedVoice;" in chooser
    assert "machine.selectionVoiceQueueBeforeSound = nullptr;" in chooser
    assert "constexpr uint16_t kConfirm = kPunch1 | kPunch2 | kPunch3 |" in chooser
    assert "if (active && (machine.players[n] & kConfirm) != 0u)" in chooser
    assert "g_voiceSide = -2;" in chooser
    assert "g_voiceSide = -1;" in chooser
    assert "CorrectQueuedCharacterVoice(std::uint8_t *ram" in (
        SRC / "xmvsf_character_select_rewrite.h").read_text()

    assert "const bool choosing = machine.video.hudShock &&" in chooser
    assert "session::BeforeFrame(machine);\n        ChoosingLayoutBeforeCpu(machine);" in game
    assert "SetRam16(record + kCursor" in chooser
    assert "void ChoosingLayoutAfterFrame(Machine &machine)" in chooser
    assert "ChoosingLayoutAfterFrame(machine);" in game
    assert "g_accepted[n] = accepted;" in chooser
    assert "g_committed[n][pick] = committed;" in chooser
    assert "SetRam16(pickAt, committed)" in chooser
    assert "ApplyCustomTeamPicksBeforeMatch();" in (
        ROOT / "recomp/games/xmvsf/src/modes.cpp").read_text()
    machine_header = (ROOT / "recomp/machine/machine.h").read_text()
    machine_src = (ROOT / "recomp/machine/machine.cpp").read_text()
    assert "void (*selectionVoiceQueueBeforeSound)(Machine &) = nullptr;" in machine_header
    assert "if (selectionVoiceQueueBeforeSound)" in machine_src
    assert "selectionVoiceQueueBeforeSound(*this);" in machine_src
    assert machine_src.index("selectionVoiceQueueBeforeSound(*this);") < machine_src.index("RunSound(kSoundCyclesPerLine);")
    assert "selectionRamWriteOverride" in machine_header
    assert "selectionRamWriteOverride(*this, address & ~1u, value, 2)" in machine_src
    assert "selectionRamWriteOverride(*this, address, next, 1)" in machine_src
    assert "RewriteNativeSelectionWrite(" in chooser
    assert "const uint32_t counter = 0xFF5183u + n * 0x20u;" in chooser
    assert "g_committed[n][pick] = IntendedNativeId(n);" in chooser
    assert "if (count == 2u) g_commitPending = true;" in chooser
    assert "const bool wroteCounter" in chooser
    assert "const bool active = Ram8(record) <= 3u && Ram8(record + 3u) < 2u;" in chooser
    assert "const bool p1CursorActive = ram[0x5183u] < 2u;" in frontend
    assert "const bool p2CursorActive = ram[0x51A3u] < 2u;" in frontend
    assert "p1CursorActive, p2CursorActive))" in frontend
    gpp = shutil.which("g++")
    assert gpp, "UCRT64 g++ required for selection grid smoke"
    with tempfile.TemporaryDirectory(prefix="xmvsf_cs_layout_") as d:
        binary = pathlib.Path(d) / "select_layout.exe"
        subprocess.run([gpp, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(ROOT / "recomp_port/tests/test_character_select_grid.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("XMVSF 87.508 guarded pre-sound queued voice fix, native navigation, 18-ID mapping PASS")

if __name__ == "__main__":
    main()
