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
    # 87.511: after native acceptance, the selector stays outlined
    # steadily rather than disappearing; before acceptance it blinks.
    assert "if (showP1 && a >= 0 && (!cursorP1 || ink))" in screen
    assert "if (showP2 && b >= 0 && (!cursorP2 || ink))" in screen
    assert "if (ink && showP1 && cursorP1" not in screen
    assert "if (ink && showP2 && cursorP2" not in screen
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
    assert "g_soundAccepted[side] >= 2u || Ram8(record + 3u) >= 2u" in chooser
    assert "const uint16_t committed = g_committed[side][1];" in chooser
    assert "if (port::xmvsf_select::Find(committed) >= 0) return committed;" in chooser
    assert "return Ram16(kPicks + static_cast<uint32_t>(side) * 4u + 2u);" not in chooser
    assert "uint16_t ChoosingVisualCommittedId(unsigned side, unsigned pick)" in chooser
    assert "return g_committed[side][pick];" in chooser
    assert "cps2recomp::ChoosingVisualCommittedId(0u, 0u)" in frontend
    assert "cps2recomp::ChoosingVisualCommittedId(1u, 0u)" in frontend
    assert "const int fixedP1 = xmvsf_select::Find(firstP1);" in screen
    assert "const int fixedP2 = xmvsf_select::Find(firstP2);" in screen
    assert "Cursor(p1Cursor_, 9, fixedP1);" in screen
    assert "Cursor(p2Cursor_, 8, fixedP2);" in screen
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
    # 87.519: the A-family codes were verified in the ROM's FF2648
    # ring, NOT the sound-CPU shared-memory latch. Correct at ring
    # WRITE BEFORE the ROM converts the packet for QSound; no injection.
    assert 'AddHook(HookAt(0x0787E0)' not in chooser
    assert "CorrectNativeQueuedVoice(" not in chooser
    assert "g_soundAccepted[2]" in chooser
    assert "if (!RunningAhead() && count > g_soundAccepted[n] && count <= 2u)" in chooser
    assert "const std::uint16_t selected = IntendedNativeId(n);" in chooser
    assert "g_committed[n][pick] = selected;" in chooser
    assert "port::xmvsf_select::RewriteQueuedSelectionNames(" in chooser
    assert "CorrectNativeQueue(machine, updated);" in chooser
    assert "CorrectNativeQueue(machine, Ram16(kSoundQueueEnd));" in chooser
    assert "g_nameQueueHead = g_cursorSoundBegin;" in chooser
    assert "value = port::xmvsf_select::RewriteSelectionNameRingWrite(" not in chooser
    assert "machine.ram.size(), selected);" in chooser
    assert "machine.selectionVoiceQueueBeforeSound = nullptr;" in chooser
    assert "machine.selectionVoiceQueueBeforeSound = &RewriteNativeQueuedNames;" in chooser
    assert "machine.selectionSoundRequestFilter = &AuthoritativeSelectionSound;" in chooser
    assert "machine.selectionSoundRequestFilter = g_soundTailFrames > 0u" in chooser
    assert "g_soundTailFrames = 30u;" in chooser
    assert "const uint16_t fixed = g_committed[n][pick] != 0xFFFFu" in chooser
    assert "g_committed[n][pick] = fixed;" in chooser
    assert "if (native != fixed) SetRam16(pickAt, fixed);" in chooser
    assert "machine.LeaveSoundRequest(request)" not in chooser
    assert "DeliverSelectedAudio(" not in chooser
    assert "QueueSelectedAudio(" not in chooser
    assert "g_pendingAudio" not in chooser
    assert "kSelectTagA" not in chooser
    assert "g_injectingAudio" not in chooser
    assert "g_nameTickets" not in chooser
    assert "g_expectedNames" not in chooser
    # 87.521: character select owns an explicit A/B scheme.
    # Default gamepad South=A=kKick1; East=B=kKick2.
    assert "const bool a = (physical & kKick1) != 0u;" in chooser
    assert "const bool b = (physical & kKick2) != 0u;" in chooser
    assert "const bool aEdge = a && !g_aHeld[n];" in chooser
    assert "const bool bEdge = b && !g_bHeld[n];" in chooser
    assert "machine.players[n] |= kPunch1;" in chooser
    assert "CancelTeamPick(static_cast<unsigned>(n));" in chooser
    assert "SetRam8(record + 3u, static_cast<uint8_t>(undone));" in chooser
    assert "g_committed[n][undone] = 0xFFFFu;" in chooser
    assert "g_soundAccepted[n] = static_cast<uint8_t>(undone);" in chooser
    assert "g_confirmQueued[n] = false;" in chooser
    assert "NativePickAllowed(" in chooser
    assert "g_confirmQueued[n]))" in chooser
    assert "NextTeamIndex(g_layout.index[n])" in chooser
    assert "SkipTeamIndex(" in chooser
    assert "SameTeamCharacter(" in chooser
    assert "inline constexpr bool NativePickAllowed(" in layout
    assert "inline constexpr int NextTeamIndex(" in layout
    assert "inline constexpr int SkipTeamIndex(" in layout
    assert "TeamIdentity(std::uint16_t rom)" in layout
    # No native character can auto-confirm merely because the clock elapsed.
    assert "if (count > g_accepted[n] && count <= 2u)" in chooser
    assert "if (accepted == 0u && Ram8(record) >= 2u)" in chooser
    assert "if (accepted == 1u && Ram8(record) >= 4u)" in chooser
    # Existing native cursor-movement tick fallback remains intact.
    select_rewrite = (SRC / "xmvsf_character_select_rewrite.h").read_text()
    layout = (SRC / "xmvsf_character_select_layout.h").read_text()
    assert "std::uint8_t nameVoice;" in layout
    assert "0x08, 0xA4" in layout # Gambit, all player/pick slots
    assert "0x06, 0xA3" in layout # Rogue, all player/pick slots
    assert "0x04, 0xA2" in layout # Storm, all player/pick slots
    assert "0x1E, 0xAF" in layout # Akuma, all player/pick slots
    assert "NativeCharacterVoice(std::uint16_t id)" in select_rewrite
    assert "kSlots[static_cast<unsigned>(slot)].nameVoice;" in select_rewrite
    assert "RewriteSelectionNameRingWrite(" in select_rewrite
    assert "RewriteQueuedSelectionNames(" in select_rewrite
    assert "address < 0xFF2648u || address >= 0xFF2A48u" in select_rewrite
    assert "(offset & 15u) != 3u" in select_rewrite
    assert "(offset & 15u) != 2u" in select_rewrite
    assert "native >= 0xA0u && native <= 0xB3u" in select_rewrite
    assert "native >= 0xC0u && native <= 0xD3u" in select_rewrite
    assert "return static_cast<std::uint16_t>((value & 0xFF00u) | name);" in select_rewrite
    assert "AllowNativeSelectionSound(const std::uint8_t *request)" in select_rewrite
    assert "SelectionSoundEnvelope(request, 0x0Au)" in select_rewrite
    assert "QueueExplicitSelectionSound(" not in select_rewrite
    assert "MakeSelectionSoundRequest(" not in select_rewrite
    assert "kSelectTagA" not in select_rewrite
    assert "side * 0x20u" not in select_rewrite
    assert "QueueMissingCursorMovementSound(std::uint8_t *ram" in select_rewrite
    assert "p[3] != soundCode" in select_rewrite
    assert "entry[3] = static_cast<std::uint8_t>(soundCode);" in select_rewrite
    assert "soundCode != 0x84u && soundCode != 0x85u" in select_rewrite
    assert "entry[6] = 0x10u;" in select_rewrite
    assert "distance > 128u" in select_rewrite
    assert "if (nativeMoveTick(ram + 0x2648u + at)) return false;" in select_rewrite
    assert "SuppressTrailingSelectionSpriteVoice(" not in select_rewrite
    assert "SuppressTrailingSelectionSpriteVoice(" not in chooser
    assert "g_cursorSoundBegin = Ram16(kSoundQueueEnd);" in chooser
    assert "g_gridBeforeFrame[0] = g_layout.index[0];" in chooser
    assert "g_gridBeforeFrame[1] = g_layout.index[1];" in chooser
    assert "g_gridBeforeFrame[0] != g_layout.index[0]" in chooser
    assert "g_gridBeforeFrame[1] != g_layout.index[1]" in chooser
    assert "QueueMissingCursorMovementSound(" in chooser
    assert "const bool p1Moved =" in chooser
    assert "const bool p2Moved =" in chooser
    assert "const unsigned soundCode = side == 0u ? 0x84u : 0x85u;" in chooser
    assert "g_cursorSoundBegin, after, soundCode" in chooser
    assert "if (!RunningAhead()" not in select_rewrite
    assert "AddHook(HookAt(0x0787E0)" not in chooser

    assert "const bool choosing = machine.video.hudShock &&" in chooser
    assert "session::BeforeFrame(machine);\n        ChoosingLayoutBeforeCpu(machine);" in game
    assert "SetRam16(record + kCursor" in chooser
    assert "void ChoosingLayoutAfterFrame(Machine &machine)" in chooser
    assert "ChoosingLayoutAfterFrame(machine);" in game
    assert "g_accepted[n] = accepted;" in chooser
    assert "g_committed[n][pick] = fixed;" in chooser
    assert "SetRam16(pickAt, fixed)" in chooser
    assert "ApplyCustomTeamPicksBeforeMatch();" in (
        ROOT / "recomp/games/xmvsf/src/modes.cpp").read_text()
    machine_header = (ROOT / "recomp/machine/machine.h").read_text()
    machine_src = (ROOT / "recomp/machine/machine.cpp").read_text()
    assert "void (*selectionVoiceQueueBeforeSound)(Machine &) = nullptr;" in machine_header
    assert "bool (*selectionSoundRequestFilter)(Machine &, uint8_t *) = nullptr;" in machine_header
    assert "selectionSoundRequestFilter(*this, soundRam)" in machine_src
    assert "soundRam[0x00F] = 0xFF;" in machine_src
    assert "selectionSoundRequestFilter(*this, delivered)" in machine_src
    assert "memcpy(delivered, request, sizeof(delivered));" in machine_src
    assert "selectionSoundRequestFilter = nullptr;" in machine_src
    assert "if (selectionVoiceQueueBeforeSound)" in machine_src
    assert "selectionVoiceQueueBeforeSound(*this);" in machine_src
    assert machine_src.index("selectionVoiceQueueBeforeSound(*this);") < machine_src.index("RunSound(kSoundCyclesPerLine);")
    assert "selectionRamWriteOverride" in machine_header
    assert "selectionRamWriteOverride(*this, address & ~1u, value, 2)" in machine_src
    assert "selectionRamWriteOverride(*this, address, next, 1)" in machine_src
    assert "RewriteNativeSelectionWrite(" in chooser
    assert "const uint32_t counter = 0xFF5183u + n * 0x20u;" in chooser
    assert "g_committed[n][pick] = IntendedNativeId(n);" in chooser
    assert "g_committed[n][pick] = selected;" in chooser
    assert "if (count == 2u) g_commitPending = true;" in chooser
    assert "const bool wroteCounter" in chooser
    assert "const bool active = Ram8(record) <= 3u && Ram8(record + 3u) < 2u;" in chooser
    assert "const bool p1CursorActive = ram[0x5183u] < 2u;" in frontend
    # Selection stops flickering but its outlined cell remains visible.
    assert "(!cursorP1 || ink)" in screen
    assert "(!cursorP2 || ink)" in screen
    assert "if (ink && showP1 && cursorP1" not in screen
    assert "if (ink && showP2 && cursorP2" not in screen
    assert "const bool p2CursorActive = ram[0x51A3u] < 2u;" in frontend
    assert "p1CursorActive, p2CursorActive," in frontend
    assert "cps2recomp::ChoosingVisualCommittedId(1u, 0u)))" in frontend
    gpp = shutil.which("g++")
    assert gpp, "UCRT64 g++ required for selection grid smoke"
    with tempfile.TemporaryDirectory(prefix="xmvsf_cs_layout_") as d:
        binary = pathlib.Path(d) / "select_layout.exe"
        subprocess.run([gpp, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(ROOT / "recomp_port/tests/test_character_select_grid.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("XMVSF 87.521 A accept / B undo, native timer auto-pick blocked, duplicate team skip, voices unchanged PASS")

if __name__ == "__main__":
    main()
