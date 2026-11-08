#!/usr/bin/env python3
"""87.235 source-level regression for user-authored cold boot effects.
Native SDL and WASAPI playback remain local Windows verification tasks.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
SDL=ROOT/"dev/stable/buildtree/src/burner/sdl"
AUDIO=ROOT/"dev/stable/buildtree/src/intf/audio/sdl"
boot=(SDL/"xmvsf_stage14_boot.h").read_text(encoding="utf-8")
fx=(SDL/"xmvsf_menu_color_fx.h").read_text(encoding="utf-8")
atlas=(SDL/"xmvsf_menu_glyph_atlas.h").read_text(encoding="utf-8")
gif=(SDL/"xmvsf_stage87_195_menu_gif.h").read_text(encoding="utf-8")
mixer=(AUDIO/"xm20_wasapi_impl.h").read_text(encoding="utf-8")
api=(SDL/"xmvsf_stage20_api.h").read_text(encoding="utf-8")
for name in ("crash.wav","xmvsf_welcome.wav","fight!.wav"):
    assert (ROOT/"audio"/name).is_file(), f"missing root/audio/{name}"
    assert f'clipLoadRepoAudio(m.' in mixer
    assert f'"{name}"' in mixer, name
for entry in ("xm20MenuCrash","xm20MenuWelcome","xm20MenuFight"):
    assert f"void {entry}();" in api
    assert f"void {entry}()" in mixer
assert 'const char* relatives[]={"../../audio/","../audio/","audio/"}' in mixer
assert "xmvsfPcm::loadEffectStereo16" in mixer
assert "SDL_OpenAudioDevice" not in mixer
assert "m.crashOn" in mixer and "m.welcomeOn" in mixer
assert "m.fightVoiceOn" in mixer
assert "m.fightVoiceOn=false" in mixer  # reset only when returning to main menu
assert "const bool introEnabled=xm87226FirstWindow && !XM8764::isPlayback();" in boot
assert boot.index("xm20MenuCrash();") < boot.index("xm8656Present(render)")
assert boot.index("xm20MenuWelcome();") < boot.index("while(SDL_PollEvent(&ev))")
assert "XM87235Intro::inputAllowed(introElapsed)" in boot
assert "ev.type!=SDL_QUIT)continue;" in boot
assert "XM87235Intro::backgroundStrength(introElapsed)" in boot
assert "XM87235Intro::logoStrength(introElapsed)" in boot
assert "XM87235Intro::menuReveal(introElapsed)" in boot
assert "if(selection<=2)xm20MenuFight();" in boot
assert "XMMenuColor::introLogoDither" in atlas
assert "bayer[(y&3)*4+(x&3)]>=level" in atlas
assert "backgroundHue(originalPixels_,huePixels_,now,introStrength)" in gif
assert "TEST 87.235" in boot
print("XMVSF 87.235 cold boot/menu/Fight audio and SDL integration: PASS")
