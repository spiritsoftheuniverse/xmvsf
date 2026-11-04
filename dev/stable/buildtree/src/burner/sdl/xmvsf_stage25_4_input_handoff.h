#ifndef XMVSF_STAGE25_4_INPUT_HANDOFF_H
#define XMVSF_STAGE25_4_INPUT_HANDOFF_H
// XMVSF Stage25.4: do not forward a menu-confirm press into character select.
// Only affects the short UI->game handoff. No emulator pause, key remapping,
// frame advance, controller configuration, or save-state changes.
#include <SDL.h>

struct XM25InputHandoff {
    bool active=false;
    bool sampled=false;
    bool blocked=true;
    Uint32 neutralSince=0;
    Uint32 lastProbeAt=0;
};
extern XM25InputHandoff xm25InputHandoff;

static inline void xm25BeginInputHandoff() {
    xm25InputHandoff.active=true;
    xm25InputHandoff.sampled=false;
    xm25InputHandoff.blocked=true;
    xm25InputHandoff.neutralSince=0;
}

static inline bool xm25HandoffAnyInputHeld() {
    // Update physical state; don't depend on stale KEYDOWN or button events.
    SDL_PumpEvents();
    int keyCount=0;
    const Uint8* keys=SDL_GetKeyboardState(&keyCount);
    if(keys)for(int i=0;i<keyCount;++i)if(keys[i])return true;
    // A mouse selection must also be released before game input is enabled.
    if(SDL_GetMouseState(nullptr,nullptr) & (SDL_BUTTON(SDL_BUTTON_LEFT) |
                                              SDL_BUTTON(SDL_BUTTON_RIGHT)))return true;
    SDL_JoystickUpdate();
    const int devices=SDL_NumJoysticks();
    for(int i=0;i<devices;++i){
        SDL_Joystick* stick=SDL_JoystickOpen(i);
        if(!stick)continue;
        bool held=false;
        for(int j=0;j<SDL_JoystickNumButtons(stick);++j){
            if(SDL_JoystickGetButton(stick,j)){held=true;break;}
        }
        for(int j=0;!held && j<SDL_JoystickNumHats(stick);++j){
            if(SDL_JoystickGetHat(stick,j)!=SDL_HAT_CENTERED){held=true;break;}
        }
        // The first two axes are the usual directional stick; avoid testing
        // trigger axes that report -32768 at rest on some controllers.
        for(int j=0;!held && j<SDL_JoystickNumAxes(stick) && j<2;++j){
            const int axis=(int)SDL_JoystickGetAxis(stick,j);
            if(axis>16000 || axis< -16000){held=true;break;}
        }
        SDL_JoystickClose(stick); // SDL handles are ref-counted; preserve existing maps.
        if(held)return true;
    }
    return false;
}

static inline bool xm25InputHandoffBlock() {
    if(!xm25InputHandoff.active)return false;
    const Uint32 now=SDL_GetTicks();
    // The gameplay input callback may run hundreds of times per frame.
    if(xm25InputHandoff.sampled && (Uint32)(now-xm25InputHandoff.lastProbeAt)<8)
        return xm25InputHandoff.blocked;
    xm25InputHandoff.sampled=true;
    xm25InputHandoff.lastProbeAt=now;
    if(xm25HandoffAnyInputHeld()){
        xm25InputHandoff.neutralSince=0;
    }else if(!xm25InputHandoff.neutralSince){
        xm25InputHandoff.neutralSince=now ? now : 1;
    }else if((Uint32)(now-xm25InputHandoff.neutralSince)>=80){
        // Only a new, deliberate press can now reach XMVSF.
        xm25InputHandoff.active=false;
        xm25InputHandoff.blocked=false;
        return false;
    }
    xm25InputHandoff.blocked=true;
    return true;
}
#endif // XMVSF_STAGE25_4_INPUT_HANDOFF_H
