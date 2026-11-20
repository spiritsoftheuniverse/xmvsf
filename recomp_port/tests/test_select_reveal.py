#!/usr/bin/env python3
"""87.468: deterministic source checks for the real-pixel reveal and select song."""
from pathlib import Path
from version_guard import require_matching_versions
import shutil
import subprocess
ROOT = Path(__file__).resolve().parents[2]
REVEAL = (ROOT / "recomp_port/xmvsf_select_reveal.h").read_text(encoding="utf-8")
FRONT = (ROOT / "recomp/port/frontend_sdl.cpp").read_text(encoding="utf-8")
FILM = (ROOT / "recomp_port/xmvsf_menu_textures.h").read_text(encoding="utf-8")
HOME = (ROOT / "recomp_port/xmvsf_home.h").read_text(encoding="utf-8")
MUSIC = (ROOT / "recomp/games/xmvsf/src/music.cpp").read_text(encoding="utf-8")
SOUND = (ROOT / "recomp/port/soundtrack.cpp").read_text(encoding="utf-8")

def main() -> None:
    assert REVEAL == (ROOT / "recomp/port/xmvsf_select_reveal.h").read_text(encoding="utf-8")
    for token in (
        "class SelectReveal", "SDL_TEXTUREACCESS_TARGET",
        "SDL_RenderReadPixels(renderer, nullptr)",
        "SDL_SetRenderTarget(renderer, previous)",
        "SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ABGR8888)",
        "SDL_GetRenderOutputSize(renderer, &w, &h)",
        "FilmEdgeDither::Curl(", "FilmEdgeDither::Lattice(",
        "kDurationMs = 580", "thresholds_[i] < reveal ? 0u : outgoing_[i]",
    ):
        assert token in REVEAL, f"missing real-pixel transition: {token}"
    assert '#include "xmvsf_select_reveal.h"' in FRONT
    assert "selectReveal.Capture(presenter.Renderer(), at, [&]()" in FRONT
    assert FRONT.index("selectReveal.Capture(") < FRONT.index("session::StartSession(*port.machine);")
    assert "if (session::EnteringTraining()) {" in FRONT
    assert FRONT.index("if (session::EnteringTraining()) {") < FRONT.index("selectReveal.Frame(SDL_GetTicks())")
    assert FRONT.index("selectReveal.Frame(SDL_GetTicks())") < FRONT.index("bool drawn = false;")
    assert "presenter.SetOverlay(selectReveal.Pixels()," in FRONT
    assert "selectReveal.Cancel();" in FRONT
    assert '"custom_soundtrack", 1' in FRONT
    assert '"custom_soundtrack", 0' in FRONT
    assert 'port.opt.soundtrack = "../audio/music";' in FRONT
    assert FRONT.count("trainingSelectSongOverride = false;") >= 3
    assert '{0x02, "player_select", "Player select"}' in MUSIC
    assert (ROOT / "audio/music/soundtrack.ini").read_text(encoding="utf-8") == (
        "[player_select]\nfile = characterselect.ogg\n")
    assert (ROOT / "audio/music/characterselect.ogg").is_file()
    assert "soundtrack.ini" in SOUND and "tap.Replace(" in SOUND
    assert require_matching_versions(HOME, FILM, minimum=(87, 468)) >= (87, 468)
    assert "films_.Draw(renderer, now, filmReveal);" in FILM
    # Use the installed SDL3 development headers to catch compile errors
    # before the expensive game recompile. -fsyntax-only writes no binary.
    compiler = shutil.which("g++")
    assert compiler, "UCRT64 g++ is required for 87.468 SDL3 source smoke"
    smoke = ROOT / "recomp_port/tests/test_select_reveal_smoke.cpp"
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-fsyntax-only", str(smoke)], check=True)
    print("XMVSF 87.468 native character-select noise reveal and Ogg song 02 PASS")

if __name__ == "__main__":
    main()
