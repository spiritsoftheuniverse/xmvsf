#ifndef XMVSF_STAGE77_SELECTION_H
#define XMVSF_STAGE77_SELECTION_H
#include <SDL.h>
#include <cstdlib>
extern int xm77StageChoice;    // 0 = Random, 1 = Wily Factory
extern int xm77StageResolved;  // 0 = native CPS2 stage, 1 = Wily Factory
// Shared across the host menu and emulation translation units.
inline unsigned& xm77StageRollGeneration(){static unsigned value=0;return value;}
static inline void xm77RollStage() {
    static bool seeded=false;
    if(!seeded){std::srand((unsigned)SDL_GetTicks());seeded=true;}
    // Host stage choice; XM86.30 separately requests the native stage index 5
    // for Wily during the normal pre-fight transition.
    xm77StageResolved=(xm77StageChoice==1 || (std::rand()%13)==0)?1:0;
    ++xm77StageRollGeneration();
}
#endif
