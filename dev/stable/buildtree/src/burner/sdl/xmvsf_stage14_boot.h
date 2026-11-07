// XMVSF Stage 11: seamless single SDL window from the boot menu into CPS-2 gameplay.
// The emulation engine is compiled into the same executable. No libretro DLL or fbneo.exe launch.
// Stage 11 reuses the menu SDL_Window; TRAINING remains disabled until its
// health/time implementation is bound and verified against the USA 961023 game-state.
#ifndef XMVSF_STAGE14_BOOT_INCLUDED
#define XMVSF_STAGE14_BOOT_INCLUDED
#include <cstdio>

#include "xmvsf_stage86_55_options.h" /* shared state */
#include "xmvsf_stage86_56_final.h" /* XM86.56 complete frontend presentation */
#include "xmvsf_menu_glyph_atlas.h" /* Native SF2 large/small sprite glyphs + title logo */
struct XMVSFGlyph { char c; unsigned char row[7]; };
static const XMVSFGlyph xmvsfFont[] = {
    {'A', {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'B', {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}},
    {'C', {0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f}},
    {'D', {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}},
    {'E', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}},
    {'F', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}},
    {'G', {0x0f, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0f}},
    {'H', {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'I', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0c}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}},
    {'M', {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'P', {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}},
    {'Q', {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}},
    {'R', {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}},
    {'S', {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}},
    {'T', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1b, 0x11}},
    {'X', {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}},
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1e, 0x01, 0x01, 0x06, 0x01, 0x01, 0x1e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e}},
    {'6', {0x0f, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x1e}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}},
    {'/', {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10}},
};

static const unsigned char* xmvsfGlyph(char c) {
    for (unsigned int i=0; i<sizeof(xmvsfFont)/sizeof(xmvsfFont[0]); ++i) {
        if (xmvsfFont[i].c == c) return xmvsfFont[i].row;
    }
    return 0;
}
static void xmvsfPixelText(SDL_Renderer* render, const char* text, int cx, int y, int pixel,
                            unsigned char r, unsigned char g, unsigned char b) {
    if (!text) return;
    if(XMGlyph::center(render,text,cx,y,pixel,r,g,b))return;
    int count=0;
    while (text[count]) ++count;
    int x=cx-(count*6*pixel)/2;
    xm8655RenderColor(render,r,g,b);
    for (int i=0; i<count; ++i) {
        const unsigned char* glyph=xmvsfGlyph(text[i]);
        if (glyph) for (int row=0; row<7; ++row) for (int bit=0; bit<5; ++bit) {
            if (glyph[row] & (1 << (4-bit))) {
                SDL_Rect pix = {x+bit*pixel,y+row*pixel,pixel,pixel};
                SDL_RenderFillRect(render, &pix);
            }
        }
        x+=6*pixel;
    }
}


#include "xmvsf_stage86_55_options.h" /* XM86.55 DOUBLE_FEATURE */
#include "xmvsf_stage87_33_dizzy_option.h" /* Game-wide meter */
#include "xmvsf_stage87_55_rule_mode.h" /* Classic / Tournament selector only */
#include "xmvsf_stage13_controls.h"
#include "xmvsf_stage25_6_training_guard.h" /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
#include "xmvsf_stage25_8_menu_gate.h" /* XMVSF_STAGE25_8_MENU_RELEASE */
#include "xmvsf_stage86_52_window_state.h" /* XM86.52 SAVED_WINDOW_AND_FOCUS */
#include "xmvsf_stage29_display.h" /* XMVSF_STAGE29_SCALABLE_DISPLAY */
#include "xmvsf_stage87_195_menu_gif.h" /* animated local _art menu background at 30% brightness */
#include "xmvsf_stage25_sets.h"
#include "xmvsf_stage77_selection.h" /* XMVSF_STAGE25_SETS */
#include "xmvsf_stage20_api.h" /* XMVSF_STAGE20_WASAPI */
#include "xmvsf_native_host.h" /* XMVSF_STAGE21_BOUNDARY */
// 0=Arcade, -1=Quit. Training is still disabled, never starts an ordinary match.
extern SDL_Window* sdlWindow;
extern bool xm26PersistentShell; // Persistent window is owned by application lifecycle.
static int xmvsfBootSelector(bool returningFromGame=false) {
    xm8655Load();
    xm8733DizzyLoad();
    XM8755Mode::Load();
    std::fprintf(stderr,"XMVSF NATIVE TEST 87.212 | PNG title + SF2 proportional glyphs | original game mode\n");
    std::fprintf(stderr,"XM27: boot selector ENTER (%s)\n",returningFromGame?"return":"startup");
    std::fflush(stderr);
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER)!=0) {
        fprintf(stderr,"XMVSF menu SDL initialization: %s\n",SDL_GetError());return -1;
    }
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    // Stage 26: reuse the exact same OS window/handle every time we return.
    int xm8652StartupWidth=960,xm8652StartupHeight=540;
    if (!sdlWindow) xm8652ReadGeometry(&xm8652StartupWidth,&xm8652StartupHeight);
    SDL_Window* window=sdlWindow;
    if(!window){
        window=SDL_CreateWindow("X-MEN VS. STREET FIGHTER II | TEST 87.212",
            SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,xm8652StartupWidth,xm8652StartupHeight,
            SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    }
    if(!window) {fprintf(stderr,"XMVSF menu window: %s\n",SDL_GetError());return -1;}
    sdlWindow=window;
    SDL_SetWindowTitle(window,"X-MEN VS. STREET FIGHTER II | TEST 87.212");
    SDL_Renderer* render=SDL_GetRenderer(window);
    if(!render)render=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!render)render=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!render){fprintf(stderr,"XMVSF menu renderer: %s\n",SDL_GetError());return -1;}
    XM87195Menu::Background menuBackground;
    // Stage 27: the video backend can leave a non-default renderer target,
    // viewport, clip, or scale after game teardown. Explicitly restore the
    // presentation canvas and present a clean first frame on the SAME window.
    SDL_SetRenderTarget(render,nullptr);
    SDL_RenderSetLogicalSize(render,960,540);
    SDL_RenderSetIntegerScale(render,SDL_FALSE);
    SDL_RenderSetViewport(render,nullptr);
    SDL_RenderSetClipRect(render,nullptr);
    SDL_SetRenderDrawBlendMode(render,SDL_BLENDMODE_NONE);
    menuBackground.load(render);
    XMGlyph::preload(render);
    if (!(SDL_GetWindowFlags(window)&SDL_WINDOW_SHOWN)) SDL_ShowWindow(window);
    xm8655RenderColor(render,8,16,36,255); /* XMVSF_STAGE32_NAVY */
    SDL_RenderClear(render);
    menuBackground.draw(render,SDL_GetTicks());
    if(!XMGlyph::menuLogo(render))
        xmvsfPixelText(render,"X-MEN VS STREET FIGHTER",480,130,3,255,255,255);
    xm8656Present(render); /* XM86.56 global final pass */
    SDL_PumpEvents();
    xm8652FocusWindowOnce(window,returningFromGame?"boot-return":"boot-startup");
    std::fprintf(stderr,"XM27: boot FIRST FRAME presented window=%u renderer=%s\n",
                 (unsigned)SDL_GetWindowID(window),SDL_GetRenderer(window)==render?"ready":"mismatch");
    std::fflush(stderr);
    // V2 replay must enter the REAL boot selector so the shared SDL
    // window, renderer, video geometry and input focus are initialized.
    // Once its first native menu frame has been presented, resolve the
    // recorded semantic choice without polling a physical controller.
    // This is NOT the 87.163 direct-driver bypass.
    if(XM8764::isPlayback()){
        const int recordedChoice=XM8764::nextMode();
        if(recordedChoice<0){
            // No driver should launch if all movie sessions are consumed.
            // Close exactly the resources this selector owns.
            menuBackground.release();
            SDL_DestroyRenderer(render);
            if(window==sdlWindow)sdlWindow=nullptr;
            SDL_DestroyWindow(window);
            return -1;
        }
        // The 87.164 replay branch originally returned before the normal
        // menuStart / Versus setup / enterGame lifecycle ran. This breaks
        // audio and host-state contracts after in-process reset. Replay
        // semantic selection automatically, but run the authentic setup.
        xmvsfHost::menuStart();
        xm25OpenSetup();
        if(recordedChoice>=0 && XM8764::context().playIndex<
                                  XM8764::context().segments.size()){
            const XM8764::Host& h=
                XM8764::context().segments[XM8764::context().playIndex].host;
            xm77StageChoice=h.stage;
            xm77StageResolved=h.stageResolved;
            ++xm77StageRollGeneration(); // same new-session stage latch
        }
        std::fprintf(stderr,
            "XM87.164 REPLAY_BOOT_MENU_READY choice=%d window=%p\\n",
            recordedChoice,(void*)sdlWindow);
        std::fflush(stderr);
        xm8652TrackWindowGeometry(window,true,"boot-exit");
        xmvsfHost::enterGame();
        SDL_SetWindowTitle(window,"X-MEN VS. STREET FIGHTER II | TEST 87.212");
        return recordedChoice;
    }
    xmvsfHost::menuStart(); /* XMVSF_STAGE20_WASAPI */
    SDL_Joystick* menuSticks[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    SDL_GameController* menuPads[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    for(int i=0;i<SDL_NumJoysticks() && i<8;i++) {
        menuSticks[i]=SDL_JoystickOpen(i);
        if(SDL_IsGameController(i)) menuPads[i]=SDL_GameControllerOpen(i);
    }
    XM13_Settings config;xm13Load(&config);
    xm25OpenSetup(); /* XMVSF_STAGE25_SETS */
    int selection=0,controlsRow=0,displayRow=3+xm29NearestPreset(window),screen=0,capture=-1,outcome=-1,running=1;
    int stageRow=xm77StageChoice,pendingMode=1,returnScreen=0;
    std::fprintf(stderr,"XM86.32 BOOT SELECTOR: stage submenu available\n");std::fflush(stderr);
    char message[96]="";Uint32 messageUntil=0,inputCooldown=0;
    XM258MenuGate menuGate{}; // Each menu screen must wait for a fresh confirm.
    if(returningFromGame)menuGate.begin(); // Ignore prior gameplay's held inputs.
    const char* actionNames[8]={"LP","MP","HP","LK","MK","HK","COIN","START"};
    while(running) {
        if(menuGate.active)menuGate.update(xm25HandoffAnyInputHeld(),SDL_GetTicks());
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            // v2 F9 can arm/stop an app-wide movie INSIDE the custom main
            // menu, before any CPS2 game or input slot exists.
            if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F9 &&
               ev.key.repeat==0 && !(ev.key.keysym.mod&KMOD_SHIFT) &&
               std::getenv("XM8764_REPLAY_ROOT")){
                XM8764::toggleF9();
                continue;
            }
            if(ev.type==SDL_QUIT) {outcome=-1;running=0;continue;}
            if(xm29HandleDisplayHotkey(ev,window)){menuGate.begin();continue;}
            if(menuGate.active)continue; // Discard BOTH SDL GameController and raw Joystick events.
            if(screen==2 || screen==3) {
                if (ev.type==SDL_MOUSEBUTTONDOWN && ev.button.button==SDL_BUTTON_LEFT) {
                    // Correct mouse coordinates for maximized/letterboxed windows.
                    float logicalX=0.0f,logicalY=0.0f;
#if SDL_VERSION_ATLEAST(2,0,18)
                    SDL_RenderWindowToLogical(render,ev.button.x,ev.button.y,&logicalX,&logicalY);
#else
                    int ww=0,hh=0;SDL_GetWindowSize(window,&ww,&hh);
                    const float scale=(ww>0&&hh>0)?std::min((float)ww/960.0f,(float)hh/540.0f):1.0f;
                    const float side=((float)ww-960.0f*scale)*0.5f;
                    const float top=((float)hh-540.0f*scale)*0.5f;
                    logicalX=((float)ev.button.x-side)/scale;
                    logicalY=((float)ev.button.y-top)/scale;
#endif
                    int mx=(int)logicalX,my=(int)logicalY;
                    if(screen==3 && my>=456 && my<=491) {
                        if(mx>=78 && mx<=280){xm25State.page=std::max(0,xm25State.page-1);xm25MenuSound(false);continue;}
                        if(mx>=680 && mx<=866){xm25State.page=std::min(xm255MaxPage(),xm25State.page+1);xm25MenuSound(false);continue;}
                    }
                    if(screen==2) {
                        if(xm25NameMouse(mx,my))continue;
                        if(my>=99 && my<334 && mx>=220 && mx<=740) {
                            xm25State.setupRow=std::min(4,(my-99)/47);continue;
                        }
                        if(my>=343 && my<442) {
                            xm25State.setupRow=my<391?5:6;
                        }
                    }
                }
                if(ev.type==SDL_TEXTINPUT && screen==2 && xm25State.focus>=0) {
                    xm25EditName(xm25State.focus,ev.text.text);continue;
                }
                if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_BACKSPACE &&
                   screen==2 && xm25State.focus>=0) {xm25Backspace(xm25State.focus);continue;}
            } /* XMVSF_STAGE25_SETS */
            if(screen==1 && capture>=0) {
                if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_ESCAPE) {
                    capture=-1;snprintf(message,sizeof(message),"CAPTURE CANCELLED");messageUntil=SDL_GetTicks()+2000;continue;
                }
                if(ev.type==SDL_JOYBUTTONDOWN || ev.type==SDL_JOYAXISMOTION) {
                    int joyInstance=ev.type==SDL_JOYBUTTONDOWN?ev.jbutton.which:ev.jaxis.which;
                    int device=-1;
                    for(int i=0;i<SDL_NumJoysticks();i++) {
                        if(SDL_JoystickGetDeviceInstanceID(i)==joyInstance) {device=i;break;}
                    }
                    if(device<0)continue;
                    if(ev.type==SDL_JOYAXISMOTION && ev.jaxis.value<=20000)continue;
                    if(capture<2) {
                        if(ev.type!=SDL_JOYBUTTONDOWN)continue;
                        if(xm13Assign(&config,capture,device))snprintf(message,sizeof(message),"PLAYER %d CONTROLLER SAVED",capture+1);
                        else snprintf(message,sizeof(message),"DEVICE USED OR SAVE FAILED");
                    } else {
                        int p=(capture-2)/8, a=(capture-2)%8;
                        int selectedDevice=xm13FindDevice(&config.player[p]);
                        if(device!=selectedDevice)continue;
                        config.player[p].binding[a].type=ev.type==SDL_JOYBUTTONDOWN?'B':'A';
                        config.player[p].binding[a].index=ev.type==SDL_JOYBUTTONDOWN?ev.jbutton.button:ev.jaxis.axis;
                        if(xm13Save(&config))snprintf(message,sizeof(message),"PLAYER %d %s SAVED",p+1,actionNames[a]);
                        else snprintf(message,sizeof(message),"SAVE FAILED");
                    }
                    capture=-1;messageUntil=SDL_GetTicks()+2200;
                    inputCooldown=SDL_GetTicks()+250;menuGate.begin();continue;
                }
                // Do not handle menu navigation while waiting for a physical input.
                continue;
            }
            if(SDL_GetTicks()<inputCooldown &&
               (ev.type==SDL_CONTROLLERBUTTONDOWN || ev.type==SDL_KEYDOWN)) continue;
            int direction=0,accept=0,back=0;
            if(ev.type==SDL_KEYDOWN && !ev.key.repeat) {
                SDL_Keycode k=ev.key.keysym.sym;
                if(k==SDLK_UP)direction=-1;
                if(k==SDLK_DOWN)direction=1;
                if(k==SDLK_RETURN || k==SDLK_SPACE)accept=1;
                if(k==SDLK_ESCAPE)back=1;
            } else if(ev.type==SDL_CONTROLLERBUTTONDOWN) {
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)direction=-1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)direction=1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_A)accept=1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_B)back=1;
            }
            xm256NormalizeBootFaceButtons(ev,accept,back); /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */ /* XMVSF_STAGE25_6_1_COMPILE_REPAIR */
            if(screen==5) { // Stage selection, before loading the character select save-state.
                if(back){screen=returnScreen;menuGate.begin();continue;}
                if(direction){stageRow=1-stageRow;xm25MenuSound(false);}
                if(accept){
                    xm25MenuSound(true);
                    xm77StageChoice=stageRow;
                    xm77RollStage(); // Resolve once per match; game-side request follows during emulation.
                    std::fprintf(stderr,"XM86.32 STAGE MENU CONFIRM mode=%d selected=%d resolved=%d\n",pendingMode,xm77StageChoice,xm77StageResolved);std::fflush(stderr);
                    xm25BeginInputHandoff();
                    outcome=pendingMode;running=0;
                    // Leave the SDL input queue NOW; a second queued button
                    // must not resolve the stage twice or advance the game.
                    break;
                }
                continue;
            }
            if(screen==4) { // Display options remain inside the persistent SDL window.
                if(back){screen=0;menuGate.begin();continue;}
                if(direction){displayRow=(displayRow+XM29_PRESET_COUNT+5+direction)%(XM29_PRESET_COUNT+5);xm25MenuSound(false);}
                if(accept){
                    xm25MenuSound(true);
                    if(displayRow==0)XM8755Mode::Toggle();
                    else if(displayRow==1)xm8655Toggle();
                    else if(displayRow==2)xm8733DizzyToggle();
                    else if(displayRow<=XM29_PRESET_COUNT+2)xm29WindowPreset(window,displayRow-3);
                    else if(displayRow==XM29_PRESET_COUNT+3)xm29DesktopFit(window);
                    else xm29WindowMaximize(window);
                    menuGate.begin(); // One press cannot activate another entry.
                }
                continue;
            }
            if(screen==2) {
                if(back) {screen=0;xm25State.focus=-1;SDL_StopTextInput();menuGate.begin();}
                if(direction){xm25State.setupRow=(xm25State.setupRow+7+direction)%7;xm25MenuSound(false);}
                if(accept){
                    xm25MenuSound(true);
                    if(xm25State.focus>=0 && ev.type==SDL_KEYDOWN) {
                        xm25State.focus=-1;SDL_StopTextInput();
                    } else if(xm25State.setupRow<=4){
                        xm25State.modeIndex=xm25State.setupRow;
                        for(int i=0;i<2;i++)xm25SanitizeName(xm25State.names[i],i==0?"PLAYER 1":"PLAYER 2");
                        xm25SaveNames();SDL_StopTextInput();xm25State.focus=-1;
                        pendingMode=2;returnScreen=2;stageRow=xm77StageChoice;screen=5;menuGate.begin();
                        std::fprintf(stderr,"XM86.32 VERSUS ENTER STAGE MENU selection=%d\n",stageRow);std::fflush(stderr);
                    } else if(xm25State.setupRow==5){xm25LoadLedger();xm25State.page=0;screen=3;menuGate.begin();}
                    else {screen=0;menuGate.begin();}
                }
                continue;
            }
            if(screen==3) {
                if(back){screen=2;menuGate.begin();}
                if(direction>0 || (ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_RIGHT)){xm25MenuSound(false);
                    xm25State.page=std::min(std::max(0,((int)xm25State.entries.size()-1)/7),xm25State.page+1);}
                if(direction<0 || (ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_LEFT)){xm25MenuSound(false);
                    xm25State.page=std::max(0,xm25State.page-1);}
                if(accept){xm25MenuSound(true);screen=2;menuGate.begin();}
                continue;
            }
            if(screen==0) {
                if(back) {running=0;outcome=-1;}
                if(direction){xmvsfHost::cursorMove();selection=(selection+5+direction)%5; /* XMVSF_STAGE24_VERSUS */} /* XMVSF_STAGE20_WASAPI */
                if(accept) { xmvsfHost::confirm(); /* XMVSF_STAGE20_WASAPI */
                    if(selection==0){xm25BeginInputHandoff();outcome=0;running=0;}
                    else if(selection==1) {
                        pendingMode=1;returnScreen=0;stageRow=xm77StageChoice;screen=5;menuGate.begin();
                        std::fprintf(stderr,"XM86.32 TRAINING ENTER STAGE MENU selection=%d\n",stageRow);std::fflush(stderr);
                    } else if(selection==2){screen=2;xm25State.setupRow=xm25State.modeIndex;menuGate.begin();} /* XMVSF_STAGE25_SETS */
                    else if(selection==3){screen=1;controlsRow=0;menuGate.begin();}
                    else {screen=4;displayRow=0;menuGate.begin();}
                }
            } else {
                if(back){screen=0;capture=-1;menuGate.begin();}
                if(direction){xmvsfHost::cursorMove();controlsRow=(controlsRow+18+direction)%18;} /* XMVSF_STAGE20_WASAPI */
                if(accept) { xmvsfHost::confirm(); /* XMVSF_STAGE20_WASAPI */
                    capture=controlsRow;
                    menuGate.begin(); // Accept used to open capture is not a captured binding.
                    snprintf(message,sizeof(message),"PRESS DEVICE BUTTON TO ASSIGN");
                    messageUntil=SDL_GetTicks()+5000;
                    if(controlsRow>=2 && !config.player[(controlsRow-2)/8].active) {
                        capture=-1;
                        snprintf(message,sizeof(message),"ASSIGN CONTROLLER FIRST");messageUntil=SDL_GetTicks()+2400;
                    }
                }
            }
        }
        xm8655RenderColor(render,8,16,36,255);SDL_RenderClear(render); /* XMVSF_STAGE32_NAVY */
        // Full 960x540 animated background, beneath ALL interactive menus.
        // Frame delays and 30% RGB brightness are owned by the GIF layer.
        menuBackground.draw(render,SDL_GetTicks());
        if(screen==0) {
            // User-authored 4:3 reference, placed on the centered safe frame
            // of the 960x540 native SDL canvas. The PNG is cropped by alpha
            // bounds and fitted WITHOUT stretching its source artwork.
            if(!XMGlyph::menuLogo(render))
                xmvsfPixelText(render,"X-MEN VS STREET FIGHTER",480,130,3,255,255,255);
            const char* entries[5]={"ARCADE","TRAINING","VERSUS","CONTROLS","OPTIONS"};
            for(int a=0;a<5;a++) {
                const int y=XMGlyph::MainLayout::FirstRowY+
                            a*XMGlyph::MainLayout::RowStep;
                const Uint8 bright=(a==selection)?255:80;
                // Proportional sprite glyphs at 1.35x; selected bright,
                // others dark. No surrounding selection box or arrow.
                if(!XMGlyph::menuChoice(render,entries[a],a,a==selection))
                    xmvsfPixelText(render,entries[a],
                        XMGlyph::MainLayout::TextCenter,y,2,bright,bright,bright);
            }
            if(SDL_GetTicks()<messageUntil)
                xmvsfPixelText(render,message,
                    XMGlyph::MainLayout::TextCenter,
                    XMGlyph::MainLayout::HintY,2,255,132,132);
            else if(!XMGlyph::menuHint(render,"A SELECT  B BACK",235))
                xmvsfPixelText(render,"A SELECT  B BACK",480,
                    XMGlyph::MainLayout::HintY,2,235,235,235);
            if(!XMGlyph::menuExit(render,"ESC TO QUIT"))
                xmvsfPixelText(render,"ESC TO QUIT",480,
                    XMGlyph::MainLayout::ExitY,1,105,105,105);
            if(!XMGlyph::menuVersion(render,"TEST 87.212"))
                xmvsfPixelText(render,"TEST 87.212",896,
                    XMGlyph::MainLayout::ExitY,1,185,185,185);
        } else if(screen==5) {
            xmvsfPixelText(render,"SELECT STAGE",480,90,5,108,193,255);
            const char* stages[2]={"RANDOM","WILY FACTORY"};
            for(int i=0;i<2;++i){
                const int y=205+i*80;
                const Uint8 bright=(i==stageRow)?255:80;
                xmvsfPixelText(render,stages[i],480,y,4,bright,bright,bright);
            }
            xmvsfPixelText(render,"RANDOM INCLUDES WILY FACTORY",480,405,2,137,177,215);
            xmvsfPixelText(render,"A SELECT  B BACK",480,465,2,152,192,235);
        } else if(screen==2) { /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
            xm255BootVersus(render);
        } else if(screen==3) {
            xm255BootScoreboard(render);
        } else if(screen==4) {
            xmvsfPixelText(render,"OPTIONS",480,47,4,108,193,255);
            for(int i=0;i<XM29_PRESET_COUNT+5;i++){
                const char* label="";
                char optionName[64]={};
                if(i==0){
                    std::snprintf(optionName,sizeof(optionName),"GAME MODE : %s",XM8755Mode::Label());
                    label=optionName;
                }else if(i==1){
                    std::snprintf(optionName,sizeof(optionName),"DOUBLE FEATURE : %s",xm8655DoubleFeature()?"ON":"OFF");
                    label=optionName;
                }else if(i==2){
                    std::snprintf(optionName,sizeof(optionName),"DIZZY BAR : %s",xm8733DizzyEnabled()?"ON":"OFF");
                    label=optionName;
                }else if(i<=XM29_PRESET_COUNT+2)label=xm29Presets[i-3].label;
                else label=(i==XM29_PRESET_COUNT+3?"FULLSCREEN FIT":"MAXIMIZE WINDOW");
                const int y=96+i*31;
                const Uint8 bright=(i==displayRow)?255:77;
                xmvsfPixelText(render,label,480,y,2,bright,bright,bright);
            }
            xmvsfPixelText(render,"A SELECT  B BACK",480,468,2,148,179,210);
            xmvsfPixelText(render,"CTRL + / - RESIZE   ALT + ENTER FULLSCREEN",480,506,2,120,148,184);
        } else {
            xmvsfPixelText(render,"CONTROLS",480,30,5,105,181,255);
            xmvsfPixelText(render,"PLAYER 1 / PLAYER 2",480,86,2,117,142,203);
            int page=controlsRow/9,first=page*9;
            for(int a=first;a<first+9;a++) {
                char label[128]={0};
                if(a<2) {
                    int device=xm13FindDevice(&config.player[a]);
                    if(!config.player[a].active)snprintf(label,sizeof(label),"P%d DEVICE : NOT ASSIGNED",a+1);
                    else if(device<0)snprintf(label,sizeof(label),"P%d DEVICE : DISCONNECTED",a+1);
                    else snprintf(label,sizeof(label),"P%d DEVICE : PAD %d",a+1,device+1);
                } else {
                    int p=(a-2)/8, action=(a-2)%8;
                    XM13_Binding b=config.player[p].binding[action];
                    if(b.type=='N')snprintf(label,sizeof(label),"P%d %s : NONE",p+1,actionNames[action]);
                    else snprintf(label,sizeof(label),"P%d %s : %s %d",p+1,actionNames[action],b.type=='A'?"AXIS":"BUTTON",b.index);
                }
                int y=132+(a-first)*33;
                const Uint8 bright=(a==controlsRow)?255:78;
                xmvsfPixelText(render,label,480,y,2,bright,bright,bright);
            }
            if(capture>=0) xmvsfPixelText(render,"PRESS A PHYSICAL BUTTON",480,441,2,255,208,82);
            else if(SDL_GetTicks()<messageUntil)xmvsfPixelText(render,message,480,441,2,245,171,109);
            else xmvsfPixelText(render,"A ASSIGN  B BACK",480,441,2,117,160,193);
            xmvsfPixelText(render,page==0?"PAGE 1 / 2":"PAGE 2 / 2",480,466,2,105,105,129);
            SDL_GameControllerUpdate();
            for(int p=0;p<2;p++) for(int a=0;a<6;a++) {
                char testLabel[16];snprintf(testLabel,sizeof(testLabel),"P%d %s",p+1,actionNames[a]);
                int pressed=xm13ReadInput(&config,p,0x80+a,menuSticks,menuPads,8);
                xmvsfPixelText(render,testLabel,118+a*145,487+p*24,2,
                               pressed>0?245:79,pressed>0?246:91,pressed>0?255:130);
            }
        }
        // F9 is also armed at the CUSTOM BOOT MENU, not only in combat.
        // The legacy indicator lived exclusively in the CPS2 presenter.
        // Draw the same red timer here and report success/failure in-place,
        // after menus but BEFORE final presentation.
        const int v2rec=XM8764::visualStatus();
        // The menu's Double Feature final pass is applied FIRST, just
        // as in the gameplay presenter. The red F9 indicator must remain
        // red and legible even when menu grayscale is enabled.
        if(v2rec!=0)xm8656FinalPass(render,false);
        if(v2rec!=0){
            const unsigned recorded=XM8764::context().allFrames;
            char caption[80]={};
            unsigned char red=235,green=45,blue=55;
            if(v2rec==1)
                std::snprintf(caption,sizeof(caption),
                    "RECORDING %06u F9 STOP",recorded);
            else if(v2rec==2){
                std::snprintf(caption,sizeof(caption),
                    "RECORDING SAVED %06u",recorded);
                red=72;green=218;blue=143;
            }else{
                std::snprintf(caption,sizeof(caption),
                    "F9 RECORD ERROR SEE LOG");
                red=255;green=172;blue=70;
            }
            SDL_Rect badge={618,8,334,33};
            SDL_SetRenderDrawColor(render,5,10,22,235);
            SDL_RenderFillRect(render,&badge);
            SDL_SetRenderDrawColor(render,red,green,blue,255);
            SDL_Rect dot={632,19,11,11};
            SDL_RenderFillRect(render,&dot);
            xmvsfPixelText(render,caption,796,17,2,red,green,blue);
        }
        if(v2rec!=0)SDL_RenderPresent(render);
        else xm8656Present(render); /* XM86.56 global final pass */
        xm8652TrackWindowGeometry(window,false,"boot");
        SDL_Delay(8);
    }
    xm8652TrackWindowGeometry(window,true,"boot-exit");
    menuBackground.release(); // lifetime ends before shared SDL renderer changes owner
    XMGlyph::release(); // clean asset textures before game reuses the renderer
    std::fprintf(stderr,"XM27: boot selector EXIT mode=%d\n",outcome);
    std::fflush(stderr);
    for(int i=0;i<8;i++) {
        if(menuPads[i]) SDL_GameControllerClose(menuPads[i]);
        if(menuSticks[i]) SDL_JoystickClose(menuSticks[i]);
    }
    // Stage12: retain BOTH the SDL_Window and its SDL_Renderer on Arcade entry.
    if(outcome==0 || outcome==1 || outcome==2) { /* XMVSF_STAGE24_VERSUS */
        xmvsfHost::enterGame(); /* XMVSF_STAGE20_WASAPI */
        sdlWindow=window;
        SDL_SetWindowTitle(sdlWindow,"X-MEN VS. STREET FIGHTER II | TEST 87.212");
    } else {
        SDL_DestroyRenderer(render);
        if(window==sdlWindow)sdlWindow=nullptr;
        SDL_DestroyWindow(window); // Actual Quit, never a Main Menu transition.
        // SDL_Quit remains owned by main() for the persistent shell.
    }
    return outcome;
}
#endif
