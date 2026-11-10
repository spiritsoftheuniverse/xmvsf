#ifndef XMVSF_STAGE24_3_MENU_POLL_H
#define XMVSF_STAGE24_3_MENU_POLL_H
// XMVSF Stage 24.3: read the very same saved raw devices that Stage 13 uses.
// Result-screen input remains live while the CPS-2 advances naturally behind
// the overlay. Controllers are blocked from gameplay in xm13ReadInput().
#include "xmvsf_stage13_controls.h"
#include "xmvsf_stage24_versus.h"
#include <cstring>
extern bool xm258ArcadeMenuOpen;
extern bool xm258ArcadeReturnRequested; /* XMVSF_STAGE25_8_ARCADE_MENU */

struct XM243Pad {
    SDL_Joystick* joy=nullptr;
    SDL_GameController* controller=nullptr;
    SDL_JoystickID instance=-1;
    XM13_Player mapping{};
    bool previousUp=false, previousDown=false, previousAccept=false, previousCoin=false, previousCancel=false; /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */ /* XMVSF_STAGE25_1_POLISH */
};
static XM243Pad xm243Pads[2];
static bool xm243MenuWasOpen=false;
static Uint32 xm243LastPadProbe=0;
static bool xm243PreviousKeyboardCoin=false; // retain the traditional '5' shortcut

static inline void xm243ClosePads() {
    for (int i=0; i<2; ++i) {
        xm248ReconcileMenuPad(-1,xm243Pads[i].joy,
                              xm243Pads[i].controller,xm243Pads[i].instance);
        xm243Pads[i]=XM243Pad{};
    }
    xm243LastPadProbe=0;
    xm243MenuWasOpen=false;
}
static inline bool xm243Button(SDL_Joystick* joy, XM13_Binding binding) {
    // Shared backend also supports a keyboard scancode without a joystick.
    return xm13BindingPressed(binding,joy)!=0;
}
static inline void xm243ReadPad(XM243Pad& p, bool& up, bool& down, bool& accept, bool& coin, bool& cancel) {
    up=down=accept=coin=cancel=false;
    SDL_Joystick* j=p.joy;
    // Coin moved to slot 8 when 3P and 3K were inserted. Slot 6 is 3P!
    coin=xm243Button(j,p.mapping.binding[XM13_COIN]);
    if (!j || !SDL_JoystickGetAttached(j))return;
    if (SDL_JoystickNumAxes(j)>1) {
        const int y=SDL_JoystickGetAxis(j,1);
        up=y<-XM13_DEADZONE;
        down=y>XM13_DEADZONE;
    }
    if (SDL_JoystickNumHats(j)>0) {
        const Uint8 hat=SDL_JoystickGetHat(j,0);
        up=up || (hat&SDL_HAT_UP)!=0;
        down=down || (hat&SDL_HAT_DOWN)!=0;
    }
    if(p.controller) {
        up=up || SDL_GameControllerGetButton(p.controller,SDL_CONTROLLER_BUTTON_DPAD_UP)!=0;
        down=down || SDL_GameControllerGetButton(p.controller,SDL_CONTROLLER_BUTTON_DPAD_DOWN)!=0;
    }
    // Confirm with LP or Start AS ASSIGNED BY THIS PLAYER, not any joystick button.
    accept=SDL_JoystickGetButton(j,0)!=0; /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING: XInput A only */
    cancel=SDL_JoystickGetButton(j,1)!=0; /* XInput B */
}
// Shared recovery for BOTH Versus results and Arcade Coin navigation.
// Reacquire saved device identities after a late/failed open, USB reconnect,
// Bluetooth wake or SDL index shift, without overwriting controller maps.
// Newly acquired held buttons seed the previous state (no ghost actions).
static inline void xm243RefreshPads(XM243Pad pads[2]) {
    XM13_Settings settings{};xm13Load(&settings);
    SDL_GameControllerUpdate();
    for(int i=0;i<2;++i){
        XM243Pad& p=pads[i];
        p.mapping=settings.player[i];
        const int device=xm248FindAssignedDevice(&settings,i);
        const bool changed=xm248ReconcileMenuPad(
            device,p.joy,p.controller,p.instance);
        if(changed){
            xm243ReadPad(p,p.previousUp,p.previousDown,p.previousAccept,
                        p.previousCoin,p.previousCancel);
        }
    }
}
static inline void xm243OpenPads() {
    for(int p=0;p<2;++p)xm243Pads[p]=XM243Pad{};
    xm243RefreshPads(xm243Pads);
    xm243LastPadProbe=SDL_GetTicks();
    const Uint8* keys=SDL_GetKeyboardState(nullptr);
    xm243PreviousKeyboardCoin=keys && keys[SDL_SCANCODE_5]!=0;
    xm243MenuWasOpen=true;
    xm24Log("RESULT_MENU_PAD_POLL_INITIALIZED");
}
static inline void xm243PollLiveMenu() {
    if (!xm24VersusMode) {
        if(xm243MenuWasOpen)xm243ClosePads();
        return;
    }
    if(!xm243MenuWasOpen) {
        xm243OpenPads();
        return;
    }
    SDL_GameControllerUpdate();
    const Uint32 now=SDL_GetTicks();
    if(!xm243LastPadProbe || (Uint32)(now-xm243LastPadProbe)>=150u){
        xm243LastPadProbe=now ? now : 1;
        xm243RefreshPads(xm243Pads);
    }
    int navigate=0;
    bool confirmed=false;
    bool coinPressed=false, cancelPressed=false;
    for(int i=0;i<2;++i) {
        XM243Pad& p=xm243Pads[i];
        bool up=false,down=false,accept=false,coin=false,cancel=false;
        xm243ReadPad(p,up,down,accept,coin,cancel);
        if(coin && !p.previousCoin)coinPressed=true;
        p.previousCoin=coin;
        if(cancel && !p.previousCancel)cancelPressed=true;
        p.previousCancel=cancel;
        if(xm24Session.resultMenu && !navigate) {
            if(up && !p.previousUp)navigate=-1;
            else if(down && !p.previousDown)navigate=1;
        }
        if(xm24Session.resultMenu && accept && !p.previousAccept)confirmed=true;
        p.previousUp=up;
        p.previousDown=down;
        p.previousAccept=accept;
    }
    // Keyboard '5' remains a native Coin shortcut even before assigning
    // a keyboard profile. Combine with mapped Coin into ONE edge.
    const Uint8* keys=SDL_GetKeyboardState(nullptr);
    const bool keyFive=keys && keys[SDL_SCANCODE_5]!=0;
    if(keyFive && !xm243PreviousKeyboardCoin)coinPressed=true;
    xm243PreviousKeyboardCoin=keyFive;
    if(coinPressed){
        xm25MenuSound(true); /* XMVSF_STAGE25_2_LEDGER_RESULTS */
        if(xm25State.quickMenu){
            xm25State.quickMenu=false;xm24Session.resultMenu=false;xm24ResultInputCapture=0;
            xm24Log("VERSUS_COIN_CLOSE");return;
        }
        if(!xm24Session.resultMenu){
            xm25State.quickMenu=true;xm24Session.resultMenu=true;
            xm24Session.selected=0;xm24ResultInputCapture=1;
            xm24Log("VERSUS_COIN_OPEN");return;
        }
    }
    if(!xm24Session.resultMenu)return;
    if(cancelPressed){
        xm25MenuSound(false);
        if(xm25State.quickMenu){
            xm25State.quickMenu=false;
            xm24Session.resultMenu=false;xm24ResultInputCapture=0;
        }else if(xm25State.lobby){
            if(xm25State.ledgerView)xm25State.ledgerView=false;
            else xm24Session.command=XM24_REBOOT_MAIN;
        }else{
            xm24Session.command=XM24_BACK_MENU;
        }
        return;
    } /* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
    if(navigate){
        xm25MenuSound(false);
        if(xm25State.quickMenu)xm24Session.selected=(xm24Session.selected+2+(navigate>0?1:-1))%2;
        else if(xm25State.mainMenu){
            if(!xm25State.controlsView)xm25State.mainRow=(xm25State.mainRow+4+(navigate>0?1:-1))%4;
        }
        else if(xm25State.lobby){
            if(xm25State.ledgerView)xm25State.page=std::max(0,std::min(std::max(0,((int)xm25State.entries.size()-1)/7),xm25State.page+(navigate>0?1:-1))); /* XMVSF_STAGE25_3_MENU_LEDGER_REPAIR */
            else xm25State.setupRow=(xm25State.setupRow+7+(navigate>0?1:-1))%7;
        }else xm24Session.selected=(xm24Session.selected+3+(navigate>0?1:-1))%3;
    }
    if(confirmed && !xm24Session.command){
        xm25MenuSound(true);
        if(xm25State.quickMenu){
            xm24Session.command=xm24Session.selected==0?XM24_BACK_MENU:XM24_RESELECT;
        }else if(xm25State.mainMenu){
            xm24Session.command=XM24_REBOOT_MAIN; /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
            if(xm25State.controlsView)xm25State.controlsView=false;
            else if(xm25State.mainRow==0)xm24Session.command=XM24_TO_ARCADE;
            else if(xm25State.mainRow==1)xm24Session.command=XM24_TO_TRAINING;
            else if(xm25State.mainRow==2){xm25State.mainMenu=false;xm25State.lobby=true;}
            else xm25State.controlsView=true;
        }else if(xm25State.lobby){
            if(xm25State.ledgerView)xm25State.ledgerView=false;
            else if(xm25State.setupRow<5){xm25State.modeIndex=xm25State.setupRow;xm24Session.command=XM24_RESELECT;}
            else if(xm25State.setupRow==5){xm25LoadLedger();xm25State.page=0;xm25State.ledgerView=true;}
            else {xm24Session.command=XM24_REBOOT_MAIN;} /* XMVSF_STAGE25_5_SCOREBOARD_RESET */
        }else xm24Session.command=xm24Session.selected==0?XM24_REMATCH:
             (xm24Session.selected==1?XM24_RESELECT:XM24_BACK_MENU);
    }

}

// Stage 25.8: Arcade uses its own independent, non-pausing Coin overlay.
// Direct device polling makes it immune to SDL controller/joystick event
// duplication and does not depend on FBNeo's remappable attack buttons.
static XM243Pad xm258ArcadePads[2];
static bool xm258ArcadePadsReady=false;
static Uint32 xm258LastPadProbe=0;
static bool xm258PreviousKeyboardCoin=false;
static inline void xm258CloseArcadePads(){
    for(int i=0;i<2;++i){
        xm248ReconcileMenuPad(-1,xm258ArcadePads[i].joy,
                              xm258ArcadePads[i].controller,xm258ArcadePads[i].instance);
        xm258ArcadePads[i]=XM243Pad{};
    }
    xm258LastPadProbe=0;
    xm258ArcadePadsReady=false;
    xm258PreviousKeyboardCoin=false;
}
static inline void xm258PollArcadeMenu(){
    extern bool xm258ArcadeMenuOpen;
    extern bool xm258ArcadeReturnRequested;
    if(xm14TrainingMode || xm24VersusMode){
        if(xm258ArcadePadsReady)xm258CloseArcadePads();
        xm258ArcadeMenuOpen=false;
        return;
    }
    SDL_GameControllerUpdate();
    if(!xm258ArcadePadsReady){
        for(int i=0;i<2;++i)xm258ArcadePads[i]=XM243Pad{};
        xm243RefreshPads(xm258ArcadePads);
        xm258LastPadProbe=SDL_GetTicks();
        const Uint8* keys=SDL_GetKeyboardState(nullptr);
        xm258PreviousKeyboardCoin=keys && keys[SDL_SCANCODE_5]!=0;
        xm258ArcadePadsReady=true;
        return;
    }
    const Uint32 now=SDL_GetTicks();
    if(!xm258LastPadProbe || (Uint32)(now-xm258LastPadProbe)>=150u){
        xm258LastPadProbe=now ? now : 1;
        xm243RefreshPads(xm258ArcadePads);
    }
    bool coinEdge=false, acceptEdge=false, backEdge=false;
    for(int i=0;i<2;++i){
        XM243Pad& pad=xm258ArcadePads[i];
        bool up=false,down=false,a=false,coin=false,b=false;
        xm243ReadPad(pad,up,down,a,coin,b);
        if(coin && !pad.previousCoin)coinEdge=true;
        if(a && !pad.previousAccept)acceptEdge=true;
        if(b && !pad.previousCancel)backEdge=true;
        pad.previousUp=up;pad.previousDown=down;
        pad.previousAccept=a;pad.previousCoin=coin;pad.previousCancel=b;
    }
    // Preserve keyboard 5 as a legacy Coin fallback. A mapped Coin at the
    // same scancode is OR'ed into the same edge, never toggled twice.
    const Uint8* keys=SDL_GetKeyboardState(nullptr);
    const bool keyFive=keys && keys[SDL_SCANCODE_5]!=0;
    if(keyFive && !xm258PreviousKeyboardCoin)coinEdge=true;
    xm258PreviousKeyboardCoin=keyFive;
    if(coinEdge){
        xm258ArcadeMenuOpen=!xm258ArcadeMenuOpen;
        xm25MenuSound(xm258ArcadeMenuOpen);
        if(!xm258ArcadeMenuOpen)xm25BeginInputHandoff();
        return; // Coin is never also treated as an A or B command.
    }
    if(!xm258ArcadeMenuOpen)return;
    if(backEdge){
        xm258ArcadeMenuOpen=false;
        xm25MenuSound(false);
        xm25BeginInputHandoff();
    } else if(acceptEdge){
        xm25MenuSound(true);
        xm258ArcadeReturnRequested=true;
    }
}
#endif

/* XMVSF_STAGE25_6_MENU_INPUT_TRAINING */
