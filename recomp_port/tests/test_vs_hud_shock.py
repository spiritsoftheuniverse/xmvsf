#!/usr/bin/env python3
"""87.482: enforce FBNeo native VS song edge and real HUD ink source, before Ninja."""
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
    assert "if (shown && shockTile &&" in video
    assert "if (hudInk && shockTile) hudInk[px] = 1;" in video
    assert "void DrawObjects(" in video_h and "uint8_t *hudInk = nullptr" in video_h
    assert "hudShockFrame = hudShockFrame;" in video_h
    assert "hudShockPhase = hudShockPhase;" in video_h
    assert "bool hudShock = false;" in records
    assert "int16_t sourceX = 0, sourceY = 0;" in native_h
    assert "t.sourceX = static_cast<int16_t>(x);" in native
    assert "XM87392::objectForPhase(" in native
    assert "XM87392::verifiedTransitionHud(" in native
    assert "XM87392::shineRGB(" in native
    assert "if (shown && shockTile &&" in native
    assert "uint8_t *hudInk = nullptr" in native_h
    assert "xmvsf_vs::Current().Observe(" in game
    assert "Ram16(xmvsf::kGameStep)" in game and "Ram8(xmvsf::kFight)" in game
    assert require_matching_versions(home, render, minimum=(87, 482)) >= (87, 482)
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
    with tempfile.TemporaryDirectory(prefix="xmvsf_vs_482_") as temp:
        executable = Path(temp) / "vs_shock.exe"
        smoke = ROOT / "recomp_port/tests/test_vs_hud_shock_smoke.cpp"
        subprocess.run([compiler, "-std=c++20", str(smoke), "-o",
                        str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print("XMVSF 87.482 FBNeo VS, original 120+30 shock, BOTH video renderers and game compile PASS")

if __name__ == "__main__":
    main()
