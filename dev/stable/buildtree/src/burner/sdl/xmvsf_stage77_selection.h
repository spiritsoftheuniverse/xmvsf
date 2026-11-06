#ifndef XMVSF_STAGE77_SELECTION_H
#define XMVSF_STAGE77_SELECTION_H
#include <SDL.h>
#include <cstdlib>
// Source type, NOT the ROM stage index. Both custom stages use verified native
// stage 05 as a sprite/camera donor; neither modifies CPS2 stage geometry.
enum XM77StageChoice {
    XM77_NATIVE=0, XM77_WILY_FACTORY=1, XM77_ROCK_CASTLE=2
};
extern int xm77StageChoice;
extern int xm77StageResolved;
// Shared across the host menu and emulation translation units.
inline unsigned& xm77StageRollGeneration(){static unsigned value=0;return value;}
static inline void xm77RollStage() {
    static bool seeded=false;
    if(!seeded){std::srand((unsigned)SDL_GetTicks());seeded=true;}
    if(xm77StageChoice==XM77_ROCK_CASTLE)
        xm77StageResolved=XM77_ROCK_CASTLE;
    else if(xm77StageChoice==XM77_WILY_FACTORY)
        xm77StageResolved=XM77_WILY_FACTORY;
    else
        // Preserve the established one-in-thirteen Wily random slot.
        xm77StageResolved=((std::rand()%13)==0)?XM77_WILY_FACTORY:XM77_NATIVE;
    ++xm77StageRollGeneration();
}
#endif
