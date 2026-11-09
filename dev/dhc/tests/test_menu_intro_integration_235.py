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
strikes=(SDL/"xmvsf_stage87_235_ii_strikes.h").read_text(encoding="utf-8")
timeline=(SDL/"xmvsf_stage87_235_menu_intro_timeline.h").read_text(encoding="utf-8")
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
assert "backgroundHue(originalPixels_,huePixels_,now,introStrength,width_)" in gif
# Revised 87.235: both first frames are PURE WHITE, and background/title
# restore through source-space Bayer thresholded hue strobing.
assert 'introEnabled?255:8' in boot
assert 'introEnabled?255:16' in boot
assert 'introEnabled?255:36' in boot
assert 'const Uint8 brightness=static_cast<Uint8>(77+int(178.0f*s))' in gif
assert 'output[n+ch]=byte(float(flash[ch])*(1.0f-white)+255.0f*white)' in fx
assert 'bayer[(row&3u)*4u+(col&3u)]' in fx
assert 'result[p]=byte(rr*(1.0f-whiten)+255.0f*whiten)' in fx
assert 'remaining<threshold' in fx
assert "XM87235Intro::MenuStart" in boot
assert "xm20MenuIntroMusic();musicFired=true;" in boot
assert "xm20MenuIntroSilence();xm20MenuCrash();" in boot
assert "m.musicEnabled=false" in mixer
assert "if(m.musicEnabled)advanceClip(m.music,musicStep,true);" in mixer
assert "xm20::m.musicGain=0.f;xm20::m.musicTarget=1.f" in mixer
assert "xm20::m.fadeSeconds=0.5f;" in mixer
# Each of four user-exported source canvases is painted in its own
# ten-step projected mask, using the source-pixel advancing tip for sparks.
for name in ("logo_II.png","logo_II_A.png","logo_II_B.png","logo_II_C.png","logo_II_D.png"):
    assert (ROOT/"_art"/name).is_file(), name
    assert name in strikes, name
assert "XMGlyph::menuLogoFullCanvas(render,canvas)" in strikes
assert 'loadStroke(0,"logo_II_A.png",1,-1)' in strikes
assert 'loadStroke(1,"logo_II_B.png",0,-1)' in strikes
assert 'loadStroke(2,"logo_II_C.png",0,1)' in strikes
assert 'loadStroke(3,"logo_II_D.png",1,-1)' in strikes
assert "p.masked[n]=0" in strikes
assert "tip[FramesPerStroke]" in strikes
assert "void particles(" in strikes
assert "SDL_RenderFillRect(render,&spark)" in strikes
assert "const std::uint32_t StrokeCount=4u" not in timeline # constexpr uses namespace syntax
assert "static constexpr std::uint32_t StrikeCount=4u;" in timeline
assert "static constexpr std::uint32_t FramesPerStrike=10u;" in timeline
assert "static constexpr std::uint32_t StrikeFps=60u;" in timeline
assert "static constexpr std::uint32_t StrikeStart=LogoStart+LogoMs;" in timeline
assert "static constexpr std::uint32_t MenuStart=StrikeStart+StrikeDurationMs;" in timeline
assert "XM87235II::Painter iiPainter;" in boot
assert "iiPainter.draw(render,introElapsed,introEnabled);" in boot
assert "iiPainter.release();" in boot
assert "XM87235Intro::MenuStart" in boot
# Restore nominal menu volume: the menu music, not the voice/Fight one-shots.
assert "float musicGain = 1.f, musicTarget = 1.f" in mixer
assert "xm20::m.musicGain=1.f;" in mixer
assert "xm20::m.musicTarget=1.f;" in mixer
assert "musicDuck=m.fightVoiceOn?0.64f:1.0f" in mixer
assert "TEST 87.235" in boot
print("XMVSF 87.235 cold boot/menu/Fight audio and SDL integration: PASS")
