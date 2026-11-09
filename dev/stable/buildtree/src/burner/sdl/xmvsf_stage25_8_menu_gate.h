#ifndef XMVSF_STAGE25_8_MENU_GATE_H
#define XMVSF_STAGE25_8_MENU_GATE_H
#include <cstdint>
// Boot UI input handoff: every page change consumes the old button press.
// Physical input state is sampled once per main-loop iteration; both SDL
// GameController and Joystick events are ignored until release + settle.
struct XM258MenuGate {
    bool active=false;
    uint32_t neutralSince=0;
    void begin(){active=true;neutralSince=0;}
    bool update(bool anyHeld,uint32_t now){
        if(!active)return false;
        if(anyHeld)neutralSince=0;
        else if(!neutralSince)neutralSince=now?now:1;
        else if((uint32_t)(now-neutralSince)>=20){active=false;} // ~one frame after release, no artificial menu pause
        return active;
    }
};
#endif
