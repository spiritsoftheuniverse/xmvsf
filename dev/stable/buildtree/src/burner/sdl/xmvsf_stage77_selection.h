#ifndef XMVSF_STAGE77_SELECTION_H
#define XMVSF_STAGE77_SELECTION_H
#include <SDL.h>
#include <cstdlib>
// Implemented in the SDL video TU; no new link target or render-thread decoder.
void xmFDPreloadSetActive(bool enabled);
void xmFDPreloadRelease();
extern int xm77StageChoice;    // 0 Random, 1 Wily, 2 FD, 3 Guile, 4 Sigma
extern int xm77StageResolved;  // 0 native, 1 Wily, 2 FD, 3 Guile, 4 Sigma
// Shared across the host menu and emulation translation units.
inline unsigned& xm77StageRollGeneration(){static unsigned value=0;return value;}
static inline void xm77RollStage() {
    static bool seeded=false;
    if(!seeded){std::srand((unsigned)SDL_GetTicks());seeded=true;}
    // Preserve the 12 native stages and select among all three custom stages.
    // Both custom renderers use native stage 05 for fighter/FX ownership,
    // without changing the ROM's collision or combat geometry.
    if(xm77StageChoice==1)xm77StageResolved=1;
    else if(xm77StageChoice==2)xm77StageResolved=2;
    else if(xm77StageChoice==3)xm77StageResolved=3;
    else if(xm77StageChoice==4)xm77StageResolved=4;
    else {
        const int roll=std::rand()%16; // 12 original stages, plus four custom
        xm77StageResolved=roll==0?1:(roll==1?2:(roll==2?3:(roll==3?4:0)));
    }
    ++xm77StageRollGeneration();
}
#endif
