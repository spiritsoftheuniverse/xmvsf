#!/usr/bin/env python3
"""87.469: guard one-shot Fight! voice timing and the Training reveal handoff."""
from pathlib import Path
from version_guard import require_matching_versions

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "recomp_port"
AUDIO = (SRC / "xmvsf_menu_audio.h").read_text(encoding="utf-8")
FRONT = (ROOT / "recomp/port/frontend_sdl.cpp").read_text(encoding="utf-8")
HOME = (SRC / "xmvsf_home.h").read_text(encoding="utf-8")
TEXTURES = (SRC / "xmvsf_menu_textures.h").read_text(encoding="utf-8")

def main() -> None:
    assert AUDIO == (ROOT / "recomp/port/xmvsf_menu_audio.h").read_text(encoding="utf-8")
    assert (ROOT / "audio/fight!.wav").is_file()
    assert 'fight_.Load("fight!.wav");' in AUDIO
    assert "SDL_AudioStream *fightStream_ = nullptr;" in AUDIO
    assert "SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK" in AUDIO
    cue = AUDIO.split("bool Fight(bool audible = true) {", 1)[1].split("bool FightPlaying()", 1)[0]
    assert "SDL_ClearAudioStream(fightStream_)" in cue
    assert "SDL_PutAudioStreamData(fightStream_, fight_.samples.data(), bytes)" in cue
    assert "fight_.Frames()" in cue and "kDeviceTailMs" in cue
    assert "fightUntil_ = SDL_GetTicks() + durationMs + kDeviceTailMs;" in cue
    assert "void Step(bool visible)" in AUDIO
    assert "SDL_ClearAudioStream(stream_)" in AUDIO
    assert "SDL_ClearAudioStream(fightStream_)" in cue
    assert 'homeAudio.Fight(!port.menu.muted && !audioLog)' in FRONT
    assert FRONT.count('homeAudio.Fight(!port.menu.muted && !audioLog)') == 1
    confirm = FRONT.index("const auto stageAction = stagePicker.Press(")
    voice = FRONT.index("trainingWaitingFight = homeAudio.Fight(")
    launch = FRONT.index("session::StartSession(*port.machine);")
    assert confirm < voice < launch
    assert FRONT.index("if (session::EnteringTraining()) {") < FRONT.index("if (trainingWaitingFight) {")
    assert FRONT.index("if (trainingWaitingFight) {") < FRONT.index("selectReveal.Frame(SDL_GetTicks())")
    waiting = FRONT.split("if (trainingWaitingFight) {", 1)[1].split("if (selectReveal.Active())", 1)[0]
    assert "homeAudio.FightPlaying()" in waiting
    assert "SDL_Delay(8);" in waiting and "continue;" in waiting
    assert "trainingWaitingFight = false;" in waiting
    assert "selectReveal.Capture(" in FRONT
    assert 'port.opt.soundtrack = "../audio/music";' in FRONT
    assert require_matching_versions(HOME, TEXTURES, minimum=(87, 469)) >= (87, 469)
    print("XMVSF 87.469 Fight! on stage confirm, complete before character select PASS")

if __name__ == "__main__":
    main()
