#!/usr/bin/env python3
"""87.485: F10-only diagnosis, native-step full HUD and original FBNeo clock."""
from pathlib import Path
from version_guard import require_matching_versions
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
def get(path):
    return (ROOT / path).read_text(encoding="utf-8")

def main():
    front = get("recomp/port/frontend_sdl.cpp")
    bridge = get("recomp_port/xmvsf_vs_handoff.h")
    player = get("recomp_port/xmvsf_versus_audio.h")
    video = get("recomp/video/video.cpp")
    native = get("recomp/video/native_render.cpp")
    native_h = get("recomp/video/native_render.h")
    records = get("recomp/video/records.h")
    video_h = get("recomp/video/video.h")
    game = get("recomp/games/xmvsf/src/game.cpp")
    sound = get("recomp/machine/soundtrack.cpp")
    sound_h = get("recomp/machine/soundtrack.h")
    home = get("recomp_port/xmvsf_home.h")
    render = get("recomp_port/xmvsf_menu_textures.h")
    original = get("dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_392_hud_shock.h")
    gate = get("dev/stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h")
    for filename in ("xmvsf_vs_handoff.h", "xmvsf_versus_audio.h"):
        assert get("recomp_port/" + filename) == get("recomp/port/" + filename)
    assert (ROOT / "audio/music/versusscreen.ogg").is_file()
    assert (ROOT / "audio/music/characterselect.ogg").is_file()
    assert "DelayFrames=120u" in original and "RevealFrames=30u" in original
    assert "namespace XM20VsEarly" in gate and "VS_SCREEN_EDGE" in gate
    for name in ("xm20_vs_early_phase.h", "xmvsf_stage87_392_hud_shock.h"):
        assert name in bridge, "must include original FBNeo source, not a rewrite"
    assert "gate.observe(step, p1, p2)" in bridge
    assert "hud.prepareForSong(frame);" in bridge
    assert "if (fight < 8u" in bridge
    assert "struct CombatHudClock" in bridge
    assert "if (phase != 8u || firstInkSeen" in bridge
    assert "if (phase != 8u || !firstInkSeen || frame <= firstInkFrame)" in bridge
    assert "return original < elapsed ? original : elapsed;" in bridge
    assert "inline int HudObjectPhase(int phase, unsigned nativeGameStep)" in bridge
    assert "return nativeGameStep == 8u ? 8 : phase;" in bridge
    assert "void PreviewAt(unsigned frame)" not in bridge
    assert "inline void PreviewShockAt(unsigned frame)" not in bridge
    assert "CombatHud().Reset();" in bridge
    assert "XMVSF 87.485 HUD_COMBAT_FIRST_INK" in bridge
    assert "startWithSong(static_cast<unsigned>(sequence_), frame)" in player
    assert "OpenDecoder(path)" in player and "SDL_PutAudioStreamData" in player
    assert "versusscreen.ogg" in player and "if (!frames)" in player
    assert "suppressXmvsfVsMusic = homeEnabled;" in front
    assert "suppressXmvsfVsMusic = false;" in sound_h
    assert "shared[0] == 0x00 && shared[1] == 0x04" in sound
    assert "shared[0x0F] = 0xFF;" in sound
    assert "state.song == 0x02" in sound and "shared[1] == 0x05" in sound
    assert "port.machine->video.hudShock = homeEnabled;" in front
    assert "versusAudio.Start(sequence)" in front
    assert "xmvsf_vs::PreviewShockAt(" not in front
    assert "if (!wasRecording && f10.Active()" not in front
    assert "f10.Toggle(*port.machine" in front
    f10 = get("recomp/port/xmvsf_m68k_capture.h")
    assert "HUD_CALIBRATION f10_press_frame=" in f10
    assert "song_elapsed_at_press=" in f10
    assert "status_FF5000=%02X" in f10
    assert "F10 is never a gameplay trigger." in f10
    assert "if (combat) {" in front and "versusAudio.Stop();" in front
    assert "port.machine->soundtrack.state.song = -1;" in front
    assert "if (music) SDL_ClearAudioStream(music);" in front
    assert "versusAudio.Step(" in front
    assert front.count("ensureSelectMusic();") == 3
    assert front.count("trainingSelectSongOverride = false;") >= 3
    assert "const bool hideSelect = versusAudio.Active()" in front
    assert "port.machine->soundtrack.state.song == 0x02" in front
    assert "XM87392::objectForPhase(" in video
    assert "XM87392::verifiedTransitionHud(" in video
    assert "XM87392::timeline().noteHudSource(" in video
    assert "XM87392::visible(" in video and "XM87392::shineCore(" in video
    assert "XM87392::shineRGB(" in video
    assert "xmvsf_vs::HudStep(" in video
    assert "xmvsf_vs::NoteCombatHudInk(" in video
    assert "const bool liveHp = hudSource && level == 7u" in video
    assert "HudObjectPhase(" in video
    assert "hudShockGameStep" in video
    assert "static_cast<unsigned>(down), false, false);" in video
    assert "(attr & 0x1Fu) >= 0x1Au);" not in video
    assert "if (shown && shockTile &&" in video
    assert "if (hudInk && shockTile) hudInk[px] = 1;" in video
    assert "void DrawObjects(" in video_h and "uint8_t *hudInk = nullptr" in video_h
    assert "hudShockFrame = hudShockFrame;" in video_h
    assert "hudShockPhase = hudShockPhase;" in video_h
    assert "hudShockGameStep = hudShockGameStep;" in video_h
    assert "bool hudShock = false;" in records
    assert "unsigned hudShockGameStep = 0;" in records
    assert "int16_t sourceX = 0, sourceY = 0;" in native_h
    assert "t.sourceX = static_cast<int16_t>(x);" in native
    assert "XM87392::objectForPhase(" in native
    assert "XM87392::verifiedTransitionHud(" in native
    assert "XM87392::shineRGB(" in native
    assert "xmvsf_vs::HudStep(" in native
    assert "xmvsf_vs::NoteCombatHudInk(" in native
    assert "const bool liveHp = hudSource && t.level == 7u" in native
    assert "HudObjectPhase(" in native
    assert "options.hudShockGameStep" in native
    assert "across, down, false, false);" in native
    assert "(t.attributes & 0x1Fu) >= 0x1Au);" not in native
    assert "if (shown && shockTile &&" in native
    assert "uint8_t *hudInk = nullptr" in native_h
    assert "xmvsf_vs::Current().Observe(" in game
    assert "Ram16(xmvsf::kGameStep)" in game and "Ram8(xmvsf::kFight)" in game
    assert game.count("machine.video.hudShockGameStep = Ram16(xmvsf::kGameStep);") == 2
    assert require_matching_versions(home, render, minimum=(87, 485)) >= (87, 485)
    compiler = shutil.which("g++")
    assert compiler, "UCRT64 g++ is required before recomp build"
    include = [
        ROOT / "recomp/runtime", ROOT / "recomp/machine",
        ROOT / "recomp/fight", ROOT / "recomp/video",
        ROOT / "recomp/sound", ROOT / "recomp/port",
    ]
    base = [compiler, "-std=c++20", "-fsyntax-only"]
    for path in include:
        base += ["-I", str(path)]
    for path in (
        ROOT / "recomp/video/video.cpp",
        ROOT / "recomp/video/native_render.cpp",
        ROOT / "recomp/games/xmvsf/src/game.cpp",
        ROOT / "recomp_port/tests/test_vs_hud_shock_smoke.cpp",
    ):
        subprocess.run(base + [str(path)], check=True)
    # The compact smoke also RUNS the original FBNeo transition and clock.
    with tempfile.TemporaryDirectory(prefix="xmvsf_vs_485_") as temp:
        executable = Path(temp) / "vs_shock.exe"
        smoke = ROOT / "recomp_port/tests/test_vs_hud_shock_smoke.cpp"
        subprocess.run([compiler, "-std=c++20", str(smoke), "-o",
                        str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print("XMVSF 87.485 F10 independent, real FF5104 top HUD classifier, both renderers compile PASS")

if __name__ == "__main__":
    main()
