#ifndef XMVSF_STAGE25_6_TRAINING_GUARD_H
#define XMVSF_STAGE25_6_TRAINING_GUARD_H
// Isolate a controller-owner handoff from a held character-select button.
// The owner is an existing Stage14 value; do not invent character-selected
// memory flags. Stage17's selection detector remains authoritative.
#include <SDL.h>
// Observe the owner only where Stage14 variables are in scope. The
// release/neutral logic itself remains in the shared Stage25.4 latch.
static inline void xm256ObserveTrainingOwner(bool training, int owner) {
    // XM87.65: Stage17 owns the selection handoff; rearming the global
    // Stage25.4 latch here blocked ALL movement until every physical input
    // had been neutral for 80ms, sometimes indefinitely. The menu-to-game
    // handoff still uses the existing latch; only selection-owner switches
    // stop creating a new all-input lock.
    (void)training;
    (void)owner;
} /* XM87.65 TRAINING_OWNER_NO_GLOBAL_REARM */
template<typename AcceptFlag, typename BackFlag>
static inline void xm256NormalizeBootFaceButtons(const SDL_Event& ev,AcceptFlag& accept,BackFlag& back){ /* XMVSF_STAGE25_6_2_BUILD_REPAIR */
    // This changes menu behavior only, not fight controls or stored mappings.
    if(ev.type==SDL_CONTROLLERBUTTONDOWN){
        accept=ev.cbutton.button==SDL_CONTROLLER_BUTTON_A;
        back=ev.cbutton.button==SDL_CONTROLLER_BUTTON_B;
    } else if(ev.type==SDL_JOYBUTTONDOWN){
        // SDL's XInput joystick layout puts A/B on physical buttons 0/1.
        accept=ev.jbutton.button==0;
        back=ev.jbutton.button==1;
    }
}
#endif
