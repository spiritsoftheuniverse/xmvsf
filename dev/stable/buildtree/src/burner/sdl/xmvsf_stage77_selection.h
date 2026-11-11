#ifndef XMVSF_STAGE77_SELECTION_H
#define XMVSF_STAGE77_SELECTION_H
#include <SDL.h>
#include <cstdlib>
// Implemented in the SDL video TU; no new link target or render-thread decoder.
void xmFDPreloadSetActive(bool enabled);
void xmFDPreloadRelease();
extern int xm77StageChoice;    // 0 = Random, 1 = Wily Factory, 2 = Final Destination
extern int xm77StageResolved;  // 0 = native CPS2 stage, 1 = Wily Factory, 2 = Final Destination
// Shared across the host menu and emulation translation units.
inline unsigned& xm77StageRollGeneration(){static unsigned value=0;return value;}
static inline void xm77RollStage() {
    static bool seeded=false;
    if(!seeded){std::srand((unsigned)SDL_GetTicks());seeded=true;}
    // Preserve native Random and Wily; add one FD slot to the Random pool.
    // Both custom renderers use native stage 05 for fighter/FX ownership,
    // without changing the ROM's collision or combat geometry.
    if(xm77StageChoice==1)xm77StageResolved=1;
    else if(xm77StageChoice==2)xm77StageResolved=2;
    else {
        const int roll=std::rand()%14; // 12 native, one Wily, one FD
        xm77StageResolved=roll==0?1:(roll==1?2:0);
    }
    ++xm77StageRollGeneration();
}
#endif
