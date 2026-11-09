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
assert "static constexpr std::uint32_t AnnouncerStart=0u;" in timeline
assert "if(introEnabled){xm20MenuIntroSilence();xm20MenuCrash();xm20MenuWelcome();}" in boot
assert "LogoX=208, LogoY=51, LogoW=544, LogoH=289" in atlas
assert "combinedCenterShiftX(scale)" in atlas
assert "setLogoIIAlphaBounds(render,final_?finalBounds_" in strikes
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
assert 'const char* entries[4]={"ARCADE","TRAINING","VERSUS","OPTIONS"}' in boot
assert 'const char* tabs[3]={"GAME","GRAPHICS","CONTROLLER"}' in boot
assert 'optionsTab=(optionsTab+3+step)%3' in boot
assert 'SDL_CONTROLLER_BUTTON_LEFTSHOULDER' in boot
assert 'SDL_CONTROLLER_BUTTON_RIGHTSHOULDER' in boot
assert 'graphicsRow<XM29_PRESET_COUNT' in boot
assert 'if(gameRow==0)XM8755Mode::Toggle();' in boot
assert 'else if(gameRow==1)xm8655Toggle();' in boot
assert 'else xm8733DizzyToggle();' in boot
assert 'if(screen==4 && optionsTab==2 && capture>=0)' in boot
assert 'capture=controlsRow==0?controlsPage:2+controlsPage*XM13_ACTIONS+(controlsRow-1);' in boot
assert 'for(int row=0;row<=XM13_ACTIONS;row++)' in boot
assert 'for(int p=0;p<2;p++) for(int a=0;a<6;a++)' not in boot
assert 'screen=1;' not in boot
assert 'DEVICE USED OR SAVE FAILED' not in boot
controls=(SDL/"xmvsf_stage13_controls.h").read_text(encoding="utf-8")
assert 'updated.player[player]=updated.player[other];' in controls
assert 'updated.player[other]=prior;' in controls
assert 'if(!xm13Save(&updated))return 0;' in controls
assert '*settings=updated;' in controls
assert '#define XM13_ACTIONS 14' in controls
assert 'XMVSF_CONTROLLERS_2' in controls and 'XMVSF_CONTROLLERS_1' in controls
assert 'const int oldIndices[8]={0,1,2,3,4,5,8,9};' in controls
assert 'SDL_JoystickNameForIndex(device)' in controls
assert 'SDL_GameControllerNameForIndex(device)' in controls
assert 'xm13DeviceDisplayName(device,systemName,sizeof(systemName));' in boot
assert 'char systemName[44]={};' in boot
assert 'XM13_3P' in controls and 'XM13_3K' in controls
assert 'xm13BindingPressed(p->binding[XM13_3P],joy)' in controls
assert 'xm13BindingPressed(p->binding[XM13_3K],joy)' in controls
assert 'xm13AssignKeyboard(&config,capture)' in boot
assert 'SDL_GetScancodeName((SDL_Scancode)binding.index)' in boot
assert 'if(sc==SDL_SCANCODE_UNKNOWN || sc==SDL_SCANCODE_F9' in boot
assert 'LEFT","RIGHT","UP","DOWN"' in boot
assert 'controlsRow=(controlsRow+XM13_ACTIONS+1+direction)%(XM13_ACTIONS+1)' in boot
assert '174+row*19' in boot
assert 'xmvsfPixelText(render,"LB/Q",112,76,2,149,111,69)' in boot
assert 'xmvsfPixelText(render,"RB/E",862,76,2,149,111,69)' in boot
assert '"LB / RB TABS   Q / E TABS"' not in boot
assert "if(controlsRow>0 && !config.player[controlsPage].active)" in boot
assert "xm13BindingPressed(p->binding[idx],joy)" in controls
assert "static int xm13AssignKeyboard(" in controls
assert 'SDL_GetKeyboardState(nullptr)' in controls
assert 'SDL_SCANCODE_A,SDL_SCANCODE_D,SDL_SCANCODE_W,SDL_SCANCODE_S' in controls
mapper=(SDL/"input_sdl2.cpp").read_text(encoding="utf-8")
game_input=(ROOT/"dev/stable/buildtree/src/intf/input/sdl/inp_sdl2.cpp").read_text(encoding="utf-8")
gate=(SDL/"xmvsf_stage25_8_menu_gate.h").read_text(encoding="utf-8")
assert 'if(!xm13Loaded || (Uint32)(now-xm13MappedAt)>100u)' in mapper
assert '(Uint32)(xm13Now-xm13LastReload)>=250u' in game_input
assert '#define JOYSTICK_DETECT_MS 0' in game_input
assert '#define JOYSTICK_DETECT_MS 600' in game_input
assert '(uint32_t)(now-neutralSince)>=20' in gate

assert "TEST 87.236" in boot
print("XMVSF 87.235 cold boot/menu/Fight audio and SDL integration: PASS")
