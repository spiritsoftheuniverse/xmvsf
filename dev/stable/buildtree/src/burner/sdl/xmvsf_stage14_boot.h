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
#include "xmvsf_stage87_235_menu_intro_timeline.h"
#include "xmvsf_stage87_235_ii_strikes.h" /* four ten-frame masks and tip particles */
#include "xmvsf_stage25_sets.h"
#include "xmvsf_stage77_selection.h" /* XMVSF_STAGE25_SETS */
#include "xmvsf_stage20_api.h" /* XMVSF_STAGE20_WASAPI */
#include "xmvsf_native_host.h" /* XMVSF_STAGE21_BOUNDARY */
// 0=Arcade, -1=Quit. Training is still disabled, never starts an ordinary match.
extern SDL_Window* sdlWindow;
extern bool xm26PersistentShell; // Persistent window is owned by application lifecycle.
// Dynamic controller-page layout. Whole device+input group is centered in the
// space between the page caption and the P1/P2 cycler. Its first input is
// exactly 12 physical canvas pixels below the device's 16px sprite-glyph line.
static inline int xm14VisibleInputs(const XM13_Settings& settings,int page){
    return xm13IsKeyboard(&settings.player[page])?XM13_ACTIONS:XM13_START+1;
}
static inline int xm14DeviceLineY(int actions){
    const int groupHeight=16+12+16+(actions-1)*18;
    return 156+(296-groupHeight)/2;
}
static int xmvsfBootSelector(bool returningFromGame=false) {
    xm8655Load();
    xm8733DizzyLoad();
    XM8755Mode::Load();
    xm8655ApplyMode(); // Apply saved DF only when the selected game is V4.
    std::fprintf(stderr,"XMVSF NATIVE TEST 87.264 | Game Version 3/4 + independent Tournament selection\n");
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
    const bool xm87226FirstWindow=(sdlWindow==nullptr && !returningFromGame);
    SDL_Window* window=sdlWindow;
    if(!window){
        window=SDL_CreateWindow("X-MEN VS. STREET FIGHTER II | TEST 87.250",
            SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,xm8652StartupWidth,xm8652StartupHeight,
            SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    }
    if(!window) {fprintf(stderr,"XMVSF menu window: %s\n",SDL_GetError());return -1;}
    sdlWindow=window;
    SDL_SetWindowTitle(window,"X-MEN VS. STREET FIGHTER II | TEST 87.250");
    SDL_Renderer* render=SDL_GetRenderer(window);
    const bool existingRenderer=render!=nullptr;
    if(!render)render=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!render)render=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(!render){fprintf(stderr,"XMVSF menu renderer: %s\n",SDL_GetError());return -1;}
    // 87.230: visible proof that game→menu adopted the live renderer,
    // rather than presenting into a destroyed/recreated GPU context.
    SDL_RendererInfo info={};
    int outputW=0,outputH=0;
    const int rendererInfoRc=SDL_GetRendererInfo(render,&info);
    const int outputRc=SDL_GetRendererOutputSize(render,&outputW,&outputH);
    std::fprintf(stderr,
        "XM87.230 MENU_RENDERER_LIVE returned=%d adopted=%d info_rc=%d backend=%s output_rc=%d output=%dx%d\n",
        int(returningFromGame),int(existingRenderer),rendererInfoRc,
        rendererInfoRc==0 && info.name?info.name:"unknown",
        outputRc,outputW,outputH);
    std::fflush(stderr);
    // Center the physical window once BEFORE asking SDL to compute the
    // output-dependent logical letterbox. On multi-DPI desktops a window
    // move can change the renderer's physical output dimensions.
    if (!(SDL_GetWindowFlags(window)&SDL_WINDOW_SHOWN)) SDL_ShowWindow(window);
    if(xm87226FirstWindow)xm8652CenterInitialWindow(window);
    XM87195Menu::Background menuBackground;
    XM87235II::Painter iiPainter; // own source art and masked textures per menu
    // Stage 27: the video backend can leave a non-default renderer target,
    // viewport, clip, or scale after game teardown. Explicitly restore the
    // presentation canvas and present a clean first frame on the SAME window.
    // 87.235: center the *logical renderer* before the FIRST frame.
    // A viewport reset after SDL_RenderSetLogicalSize discarded SDL's
    // calculated letterbox and made the restored-size boot appear top-left.
    if(!xm8652CenteredBootCanvas(render,
            returningFromGame?"menu-return":"menu-first-frame"))
        std::fprintf(stderr,"XM87.235 BOOT_VIEWPORT_NOTE unable_to_recenter\n");
    SDL_SetRenderDrawBlendMode(render,SDL_BLENDMODE_NONE);
    menuBackground.load(render);
    XMGlyph::preload(render);
    iiPainter.load(render);
    // Cold boot only: replay and return-to-menu retain instantaneous access.
    // Open the ALREADY EXISTING WASAPI sink before presenting the first frame.
    const bool introEnabled=xm87226FirstWindow && !XM8764::isPlayback();
    static_assert(XM87235Intro::AnnouncerStart==0u,"announce before logo, not after entry");
    if(introEnabled){xm20MenuIntroSilence();xm20MenuCrash();xm20MenuWelcome();} // play voice before first logo frame
    const Uint32 introStart=SDL_GetTicks();
    bool musicFired=false,introHandoffFinished=false;
    std::fprintf(stderr,"XM87.235 INTRO_BEGIN enabled=%d first_window=%d\n",
                 int(introEnabled),int(xm87226FirstWindow));
    // The OS window was centered before configuring the logical output.
    // Returning from game never moves an already-positioned window.
    xm8655RenderColor(render,introEnabled?255:8,introEnabled?255:16,
                      introEnabled?255:36,255); /* white first-frame strobe */
    SDL_RenderClear(render);
    menuBackground.draw(render,SDL_GetTicks(),introEnabled?1.0f:0.0f);
    if(!introEnabled){
        if(!XMGlyph::menuLogo(render))
            xmvsfPixelText(render,"X-MEN VS STREET FIGHTER",480,130,3,255,255,255);
        iiPainter.draw(render,XM87235Intro::TotalMs,false);
    }
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
            iiPainter.release();
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
        SDL_SetWindowTitle(window,"X-MEN VS. STREET FIGHTER II | TEST 87.250");
        return recordedChoice;
    }
    if(!introEnabled)xmvsfHost::menuStart(); /* XMVSF_STAGE20_WASAPI */
    SDL_Joystick* menuSticks[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    SDL_GameController* menuPads[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    SDL_JoystickID menuInstances[8]={-1,-1,-1,-1,-1,-1,-1,-1};
    Uint32 menuLastPadProbe=0;
    // Do not lock in the first enumeration. A pad can become valid while
    // the menu is already visible (USB, Bluetooth, SDL delayed startup).
    // Each return to the persistent boot menu starts a new input owner
    // selection; no stale Xbox/DS4Windows instance may carry from a match.
    xm249SessionPad[0]=XM249SessionPad{};
    xm249SessionPad[1]=XM249SessionPad{};
    XM13_Settings config;xm13Load(&config);
    xm25OpenSetup(); /* XMVSF_STAGE25_SETS */
    int selection=0,controlsRow=0,controlsPage=0,optionsTab=0,gameRow=0,graphicsRow=xm29NearestPreset(window),screen=0,capture=-1,outcome=-1,running=1;
    int stageRow=xm77StageChoice,pendingMode=1,returnScreen=0;
    std::fprintf(stderr,"XM86.32 BOOT SELECTOR: stage submenu available\n");std::fflush(stderr);
    char message[96]="";Uint32 messageUntil=0,inputCooldown=0;
    XM258MenuGate menuGate{}; // Each menu screen must wait for a fresh confirm.
    if(returningFromGame)menuGate.begin(); // Ignore prior gameplay's held inputs.
    const char* actionNames[XM13_ACTIONS]={
        "LP","MP","HP","LK","MK","HK","3P","3K","COIN","START",
        "LEFT","RIGHT","UP","DOWN"
    };
    while(running) {
        const Uint32 now=SDL_GetTicks();
        if(!menuLastPadProbe || (Uint32)(now-menuLastPadProbe)>=100u){
            menuLastPadProbe=now;
            SDL_PumpEvents();
            for(int device=0;device<8;++device)
                xm248ReconcileMenuPad(device,menuSticks[device],
                                      menuPads[device],menuInstances[device]);
            // Seed both owners while the same SDL process still owns the
            // boot menu. An identical pad cannot silently switch P1/P2.
            for(int player=0;player<2;++player)
                xm248FindAssignedDevice(&config,player);
        }
        const Uint32 introElapsed=introEnabled?
            (Uint32)(now-introStart):XM87235Intro::TotalMs;
        if(introEnabled && !musicFired &&
           introElapsed>=XM87235Intro::MenuStart){
            xm20MenuIntroMusic();musicFired=true;
            std::fprintf(stderr,"XM87.235 INTRO_MENU_MUSIC elapsed=%u\n",introElapsed);
        }
        if(introEnabled && !introHandoffFinished &&
           XM87235Intro::inputAllowed(introElapsed)){
            // Ignore inputs physically held throughout the intro.
            if(xm25HandoffAnyInputHeld())menuGate.begin();
            introHandoffFinished=true;
            std::fprintf(stderr,"XM87.235 INTRO_INPUTS_UNLOCK elapsed=%u\n",introElapsed);
        }
        if(menuGate.active)menuGate.update(xm25HandoffAnyInputHeld(),now);
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            // SDL_QUIT always works, but no menu/keyboard/controller action
            // can skip, select or quit while the cold-boot reveal is running.
            if(introEnabled && !XM87235Intro::inputAllowed(introElapsed) &&
               ev.type!=SDL_QUIT)continue;
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
            if(screen==2 || screen==3 || screen==4) {
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
                    if(screen==4){
                        if(my>=59 && my<=99 && mx>=194 && mx<=768){
                            optionsTab=mx<390?0:mx<582?1:2;
                            capture=-1;xm25MenuSound(false);menuGate.begin();continue;
                        }
                        if(optionsTab==2){
                            if(my>=446 && my<=503){
                                if(mx>=264 && mx<=468){controlsPage=0;controlsRow=0;xm25MenuSound(false);menuGate.begin();continue;}
                                if(mx>=492 && mx<=696){controlsPage=1;controlsRow=0;xm25MenuSound(false);menuGate.begin();continue;}
                            }
                            const int count=xm14VisibleInputs(config,controlsPage);
                            const int deviceY=xm14DeviceLineY(count);
                            const int inputY=deviceY+16+12;
                            if(my>=deviceY-3 && my<deviceY+17 && mx>=160 && mx<=800){
                                controlsRow=0;continue;
                            }
                            if(my>=inputY-3 && my<inputY+count*18 && mx>=160 && mx<=800){
                                controlsRow=std::min(count,1+(my-inputY+3)/18);continue;
                            }
                        }
                        continue;
                    }
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
            if(screen==4 && optionsTab==2 && capture>=0) {
                if(ev.type==SDL_KEYDOWN && !ev.key.repeat){
                    const SDL_Scancode sc=ev.key.keysym.scancode;
                    if(sc==SDL_SCANCODE_ESCAPE){
                        capture=-1;
                        snprintf(message,sizeof(message),"CAPTURE CANCELLED");
                        messageUntil=SDL_GetTicks()+1500;
                        continue;
                    }
                    // Keep system/debug keys available regardless of mapping.
                    if(sc==SDL_SCANCODE_UNKNOWN || sc==SDL_SCANCODE_F9 ||
                       sc==SDL_SCANCODE_F10 || sc==SDL_SCANCODE_F12)continue;
                    if(capture<2){
                        if(xm13AssignKeyboard(&config,capture))
                            snprintf(message,sizeof(message),"PLAYER %d KEYBOARD SAVED",capture+1);
                        else snprintf(message,sizeof(message),"KEYBOARD SAVE FAILED");
                    } else {
                        const int p=(capture-2)/XM13_ACTIONS;
                        const int a=(capture-2)%XM13_ACTIONS;
                        XM13_Settings updated=config;
                        updated.player[p].binding[a].type='K';
                        updated.player[p].binding[a].index=(int)sc;
                        if(xm13Save(&updated)){
                            config=updated;
                            snprintf(message,sizeof(message),"PLAYER %d %s KEY SAVED",p+1,actionNames[a]);
                        }else snprintf(message,sizeof(message),"KEY SAVE FAILED");
                    }
                    capture=-1;
                    messageUntil=SDL_GetTicks()+1800;
                    inputCooldown=SDL_GetTicks()+150;
                    menuGate.begin();
                    continue;
                }
                if(ev.type==SDL_JOYBUTTONDOWN || ev.type==SDL_JOYAXISMOTION){
                    const int instance=ev.type==SDL_JOYBUTTONDOWN?ev.jbutton.which:ev.jaxis.which;
                    int device=-1;
                    for(int i=0;i<SDL_NumJoysticks();i++){
                        if(SDL_JoystickGetDeviceInstanceID(i)==instance){device=i;break;}
                    }
                    if(device<0)continue;
                    if(ev.type==SDL_JOYAXISMOTION && ev.jaxis.value<=20000)continue;
                    if(capture<2){
                        if(ev.type!=SDL_JOYBUTTONDOWN)continue;
                        const int other=1-capture;
                        char guid[33];xm13GuidForDevice(device,guid);
                        const bool swapped=config.player[other].active &&
                            !strcmp(config.player[other].guid,guid) &&
                            config.player[other].occurrence==xm13DeviceOrdinal(device);
                        if(xm13Assign(&config,capture,device))
                            snprintf(message,sizeof(message),
                                swapped?"PLAYER %d SLOT SWAPPED":"PLAYER %d CONTROLLER SAVED",capture+1);
                        else snprintf(message,sizeof(message),"CONTROLLER SAVE FAILED");
                    } else {
                        const int p=(capture-2)/XM13_ACTIONS;
                        const int a=(capture-2)%XM13_ACTIONS;
                        // Do not accidentally bind an unrelated controller.
                        if(device!=xm248FindAssignedDevice(&config,p))continue;
                        XM13_Settings updated=config;
                        updated.player[p].binding[a].type=
                            ev.type==SDL_JOYBUTTONDOWN?'B':'A';
                        updated.player[p].binding[a].index=
                            ev.type==SDL_JOYBUTTONDOWN?ev.jbutton.button:ev.jaxis.axis;
                        if(xm13Save(&updated)){
                            config=updated;
                            snprintf(message,sizeof(message),"PLAYER %d %s SAVED",p+1,actionNames[a]);
                        } else snprintf(message,sizeof(message),"SAVE FAILED");
                    }
                    capture=-1;
                    messageUntil=SDL_GetTicks()+1800;
                    inputCooldown=SDL_GetTicks()+150;
                    menuGate.begin();
                    continue;
                }
                // Suppress navigation until capture completes or ESC cancels.
                continue;
            }
            if(SDL_GetTicks()<inputCooldown &&
               (ev.type==SDL_CONTROLLERBUTTONDOWN || ev.type==SDL_KEYDOWN)) continue;
            int direction=0,accept=0,back=0,tabDirection=0,pageDirection=0;
            if(ev.type==SDL_KEYDOWN && !ev.key.repeat) {
                SDL_Keycode k=ev.key.keysym.sym;
                if(k==SDLK_UP)direction=-1;
                if(k==SDLK_DOWN)direction=1;
                if(k==SDLK_LEFT)pageDirection=-1;
                if(k==SDLK_RIGHT)pageDirection=1;
                if(k==SDLK_q || k==SDLK_PAGEUP)tabDirection=-1;
                if(k==SDLK_e || k==SDLK_PAGEDOWN)tabDirection=1;
                if(k==SDLK_TAB)tabDirection=(ev.key.keysym.mod&KMOD_SHIFT)?-1:1;
                if(k==SDLK_RETURN || k==SDLK_SPACE)accept=1;
                if(k==SDLK_ESCAPE)back=1;
            } else if(ev.type==SDL_CONTROLLERBUTTONDOWN) {
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)direction=-1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)direction=1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT)pageDirection=-1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT)pageDirection=1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_LEFTSHOULDER)tabDirection=-1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)tabDirection=1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_A)accept=1;
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_B)back=1;
            }
            xm256NormalizeBootFaceButtons(ev,accept,back); /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */ /* XMVSF_STAGE25_6_1_COMPILE_REPAIR */
            // The boot menu consumes SDL_CONTROLLERBUTTONDOWN from ANY
            // XInput/DS4Windows controller, even if its P1 config is absent.
            // Remember the SAME physical SDL instance for actual gameplay.
            // Raw joystick A is used for arcade Fight Pi and non-GC devices.
            if(accept && ev.type==SDL_CONTROLLERBUTTONDOWN)
                xm249SelectSessionPad(0,ev.cbutton.which);
            else if(accept && ev.type==SDL_JOYBUTTONDOWN)
                xm249SelectSessionPad(0,ev.jbutton.which);
            if(screen==5) { // Stage selection, before loading the character select save-state.
                if(back){screen=returnScreen;menuGate.begin();continue;}
                if(direction){stageRow=1-stageRow;xm25MenuSound(false);}
                if(accept){
                    if(pendingMode==1)xm20MenuFight(); // Training: cue at stage confirmation
                    else xm25MenuSound(true);
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
            if(screen==4) { // Game / Graphics / Controller: persistent L/R tabs.
                if(back){screen=0;capture=-1;menuGate.begin();continue;}
                // LB/RB or keyboard Q/E (also Tab/Shift-Tab) cycles the tabs.
                // D-pad left/right cycles tabs except on Controller, where it pages P1/P2.
                if(tabDirection || (optionsTab!=2 && pageDirection)){
                    const int step=tabDirection?tabDirection:pageDirection;
                    optionsTab=(optionsTab+3+step)%3;
                    capture=-1;xm25MenuSound(false);menuGate.begin();continue;
                }
                if(optionsTab==0){
                    if(direction){gameRow=(gameRow+4+direction)%4;xm25MenuSound(false);}
                    if(accept){
                        xm25MenuSound(true);
                        if(gameRow==0){XM8755Mode::ToggleVersion();xm8655ApplyMode();}
                        else if(gameRow==1)XM8755Mode::ToggleTournament();
                        else if(gameRow==2)xm8655Toggle();
                        else xm8733DizzyToggle();
                        menuGate.begin();
                    }
                } else if(optionsTab==1){
                    const int rows=XM29_PRESET_COUNT+2;
                    if(direction){graphicsRow=(graphicsRow+rows+direction)%rows;xm25MenuSound(false);}
                    if(accept){
                        xm25MenuSound(true);
                        if(graphicsRow<XM29_PRESET_COUNT)xm29WindowPreset(window,graphicsRow);
                        else if(graphicsRow==XM29_PRESET_COUNT)xm29DesktopFit(window);
                        else xm29WindowMaximize(window);
                        menuGate.begin();
                    }
                } else {
                    if(pageDirection){
                        controlsPage=(controlsPage+2+pageDirection)%2;
                        controlsRow=0;xm25MenuSound(false);menuGate.begin();
                    }
                    if(direction){
                        const int rowCount=xm14VisibleInputs(config,controlsPage)+1;
                        controlsRow=(controlsRow+rowCount+direction)%rowCount;
                        xm25MenuSound(false);
                    }
                    if(accept){
                        xmvsfHost::confirm();
                        capture=controlsRow==0?controlsPage:2+controlsPage*XM13_ACTIONS+(controlsRow-1);
                        menuGate.begin();
                        snprintf(message,sizeof(message),"PRESS KEY OR DEVICE BUTTON");
                        messageUntil=SDL_GetTicks()+5000;
                        if(controlsRow>0 && !config.player[controlsPage].active){
                            capture=-1;
                            snprintf(message,sizeof(message),"ASSIGN CONTROLLER FIRST");
                            messageUntil=SDL_GetTicks()+2400;
                        }
                    }
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
                // B remains Back in submenus; only ESC or SDL_QUIT quits root.
                if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_ESCAPE)
                    {running=0;outcome=-1;}
                if(direction){xmvsfHost::cursorMove();selection=(selection+4+direction)%4;}
                if(accept){
                    // Training enters a stage picker first: announce Fight
                    // only after that choice, not at the main menu.
                    if(selection==0 || selection==2)xm20MenuFight();
                    else xmvsfHost::confirm();
                    if(selection==0){xm25BeginInputHandoff();outcome=0;running=0;}
                    else if(selection==1){
                        pendingMode=1;returnScreen=0;stageRow=xm77StageChoice;screen=5;menuGate.begin();
                        std::fprintf(stderr,"XM86.32 TRAINING ENTER STAGE MENU selection=%d\n",stageRow);std::fflush(stderr);
                    } else if(selection==2){
                        screen=2;xm25State.setupRow=xm25State.modeIndex;menuGate.begin();
                    } else {
                        screen=4;optionsTab=0;gameRow=0;capture=-1;menuGate.begin();
                    }
                }
            }
        }
        const float backgroundStrength=introEnabled?
            XM87235Intro::backgroundStrength(introElapsed):0.0f;
        const Uint8 clearRed=XMMenuColor::byte(8.f+247.f*backgroundStrength);
        const Uint8 clearGreen=XMMenuColor::byte(16.f+239.f*backgroundStrength);
        const Uint8 clearBlue=XMMenuColor::byte(36.f+219.f*backgroundStrength);
        xm8655RenderColor(render,clearRed,clearGreen,clearBlue,255);
        SDL_RenderClear(render); /* white to original navy canvas */
        // Full 960x540 animated background, beneath ALL interactive menus.
        // Frame delays and 30% RGB brightness are owned by the GIF layer.
        menuBackground.draw(render,SDL_GetTicks(),
            backgroundStrength);
        if(screen==0) {
            // User-authored 4:3 reference, placed on the centered safe frame
            // of the 960x540 native SDL canvas. The PNG is cropped by alpha
            // bounds and fitted WITHOUT stretching its source artwork.
            if(!introEnabled||XM87235Intro::logoVisible(introElapsed)){
                const float remaining=introEnabled?
                    XM87235Intro::logoStrength(introElapsed):0.0f;
                if(!XMGlyph::menuLogo(render,remaining))
                    xmvsfPixelText(render,"X-MEN VS STREET FIGHTER",480,130,3,255,255,255);
                // Draw II separately after the base title. The four source
                // exports reveal in A/B/C/D order at ten frames each;
                // original II PNG remains the final, exact palette layer.
                iiPainter.draw(render,introElapsed,introEnabled);
            }
            if(!introEnabled||XM87235Intro::menuVisible(introElapsed)){
                XMGlyph::menuReveal(render,introEnabled?
                    XM87235Intro::menuReveal(introElapsed):1.0f);
            const char* entries[4]={"ARCADE","TRAINING","VERSUS","OPTIONS"};
            for(int a=0;a<4;a++) {
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
            else if(!XMGlyph::menuHint(render,"A SELECT  B/ESC BACK",235))
                xmvsfPixelText(render,"A SELECT  B/ESC BACK",480,
                    XMGlyph::MainLayout::HintY,2,235,235,235);
            if(!XMGlyph::menuExit(render,"ESC TO QUIT"))
                xmvsfPixelText(render,"ESC TO QUIT",58,
                    XMGlyph::MainLayout::ExitY,1,185,185,185);
            if(!XMGlyph::menuVersion(render,"TEST 87.250"))
                xmvsfPixelText(render,"TEST 87.250",896,
                    XMGlyph::MainLayout::ExitY,1,185,185,185);
            }
        } else if(screen==5) {
            xmvsfPixelText(render,"SELECT STAGE",480,90,5,108,193,255);
            const char* stages[2]={"RANDOM","WILY FACTORY"};
            for(int i=0;i<2;++i){
                const int y=205+i*80;
                const Uint8 bright=(i==stageRow)?255:80;
                xmvsfPixelText(render,stages[i],480,y,4,bright,bright,bright);
            }
            xmvsfPixelText(render,"RANDOM INCLUDES WILY FACTORY",480,405,2,137,177,215);
            xmvsfPixelText(render,"A SELECT  B/ESC BACK",480,465,2,152,192,235);
        } else if(screen==2) { /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
            xm255BootVersus(render);
        } else if(screen==3) {
            xm255BootScoreboard(render);
        } else if(screen==4) {
            xmvsfPixelText(render,"OPTIONS",480,23,4,132,157,186);
            // Compact single-line navigation: LB/Q Game Graphics Controller RB/E
            xmvsfPixelText(render,"LB/Q",112,76,2,149,111,69);  // dark tan
            const char* tabs[3]={"GAME","GRAPHICS","CONTROLLER"};
            const int tabX[3]={292,472,677};
            for(int i=0;i<3;i++){
                const Uint8 bright=i==optionsTab?255:86;
                xmvsfPixelText(render,tabs[i],tabX[i],76,2,bright,bright,bright);
            }
            xmvsfPixelText(render,"RB/E",862,76,2,149,111,69); // dark tan
            if(optionsTab==0){
                for(int i=0;i<4;i++){
                    char label[80]={};
                    if(i==0)std::snprintf(label,sizeof(label),"GAME VERSION : %d",XM8755Mode::version());
                    else if(i==1)std::snprintf(label,sizeof(label),"TOURNAMENT : %s",XM8755Mode::Label());
                    else if(i==2)std::snprintf(label,sizeof(label),"DOUBLE FEATURE : %s",xm8655DoubleFeature()?"ON":"OFF");
                    else std::snprintf(label,sizeof(label),"DIZZY BAR : %s",xm8733DizzyEnabled()?"ON":"OFF");
                    const Uint8 bright=i==gameRow?255:78;
                    xmvsfPixelText(render,label,480,170+i*65,3,bright,bright,bright);
                }
            } else if(optionsTab==1){
                for(int i=0;i<XM29_PRESET_COUNT+2;i++){
                    const char* label=i<XM29_PRESET_COUNT?xm29Presets[i].label:
                        i==XM29_PRESET_COUNT?"FULLSCREEN FIT":"MAXIMIZE WINDOW";
                    const Uint8 bright=i==graphicsRow?255:78;
                    xmvsfPixelText(render,label,480,151+i*38,2,bright,bright,bright);
                }
                xmvsfPixelText(render,"CTRL + / - RESIZE   ALT + ENTER FULLSCREEN",480,480,2,124,151,181);
            } else {
                xmvsfPixelText(render,"LEFT / RIGHT PAGE",480,134,2,129,160,190);
                const int visibleInputs=xm14VisibleInputs(config,controlsPage);
                const int deviceY=xm14DeviceLineY(visibleInputs);
                const int inputY=deviceY+16+12; // exactly 12px below glyph bottom
                char deviceLabel[128]={};
                const XM13_Player& player=config.player[controlsPage];
                const int device=xm248FindAssignedDevice(&config,controlsPage);
                if(!player.active)
                    std::snprintf(deviceLabel,sizeof(deviceLabel),"P%d DEVICE : NOT ASSIGNED",controlsPage+1);
                else if(xm13IsKeyboard(&player))
                    std::snprintf(deviceLabel,sizeof(deviceLabel),"P%d DEVICE : KEYBOARD",controlsPage+1);
                else if(device<0)
                    std::snprintf(deviceLabel,sizeof(deviceLabel),"P%d DEVICE : DISCONNECTED",controlsPage+1);
                else {
                    char systemName[44]={};
                    xm13DeviceDisplayName(device,systemName,sizeof(systemName));
                    std::snprintf(deviceLabel,sizeof(deviceLabel),"P%d DEVICE : %s",controlsPage+1,systemName);
                }
                xmvsfPixelText(render,deviceLabel,480,deviceY,2,
                               controlsRow==0?255:110,controlsRow==0?255:136,controlsRow==0?255:164);

                // Measure every label using the real sf2fontsmall.png advances.
                // Left-align all labels to the same origin, then center that
                // measured column on the native 960px canvas.
                char inputLabels[XM13_ACTIONS][128]={};
                int widest=0;
                for(int a=0;a<visibleInputs;a++){
                    const XM13_Binding binding=player.binding[a];
                    if(binding.type=='N'){
                        std::snprintf(inputLabels[a],sizeof(inputLabels[a]),"%s : NONE",actionNames[a]);
                    }else if(binding.type=='K'){
                        const char* name=SDL_GetScancodeName((SDL_Scancode)binding.index);
                        std::snprintf(inputLabels[a],sizeof(inputLabels[a]),"%s : KEY %s",
                                      actionNames[a],name&&*name?name:"UNKNOWN");
                    }else{
                        std::snprintf(inputLabels[a],sizeof(inputLabels[a]),"%s : %s %d",
                                      actionNames[a],binding.type=='A'?"AXIS":"BUTTON",binding.index);
                    }
                    widest=std::max(widest,XMGlyph::smallTextWidth(render,inputLabels[a],2));
                }
                const int inputLeft=480-widest/2;
                for(int a=0;a<visibleInputs;a++){
                    const Uint8 intensity=controlsRow==a+1?255:92;
                    // Explicitly use the authored SF2 small-glyph sheet,
                    // never the legacy bitmap or per-row centered text.
                    XMGlyph::left(render,inputLabels[a],inputLeft,inputY+a*18,0,2,intensity);
                }
                if(capture>=0)
                    xmvsfPixelText(render,"PRESS KEY OR CONTROLLER BUTTON",480,462,2,255,208,82);
                else if(SDL_GetTicks()<messageUntil)
                    xmvsfPixelText(render,message,480,462,2,227,175,117);
                xmvsfPixelText(render,"<",275,490,2,127,157,187);
                xmvsfPixelText(render,"PLAYER 1",375,490,2,
                    controlsPage==0?255:98,controlsPage==0?255:122,controlsPage==0?255:150);
                xmvsfPixelText(render,"PLAYER 2",585,490,2,
                    controlsPage==1?255:98,controlsPage==1?255:122,controlsPage==1?255:150);
                xmvsfPixelText(render,">",685,490,2,127,157,187);
            }
            xmvsfPixelText(render,"A SELECT   B/ESC BACK",480,518,2,129,157,188);
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
    iiPainter.release();
    XMGlyph::release(); // clean asset textures before game reuses the renderer
    std::fprintf(stderr,"XM27: boot selector EXIT mode=%d\n",outcome);
    std::fflush(stderr);
    for(int i=0;i<8;i++)
        xm248ReconcileMenuPad(-1,menuSticks[i],menuPads[i],menuInstances[i]);
    // Stage12: retain BOTH the SDL_Window and its SDL_Renderer on Arcade entry.
    if(outcome==0 || outcome==1 || outcome==2) { /* XMVSF_STAGE24_VERSUS */
        xmvsfHost::enterGame(); /* XMVSF_STAGE20_WASAPI */
        sdlWindow=window;
        SDL_SetWindowTitle(sdlWindow,"X-MEN VS. STREET FIGHTER II | TEST 87.250");
    } else {
        SDL_DestroyRenderer(render);
        if(window==sdlWindow)sdlWindow=nullptr;
        SDL_DestroyWindow(window); // Actual Quit, never a Main Menu transition.
        // SDL_Quit remains owned by main() for the persistent shell.
    }
    return outcome;
}
#endif
