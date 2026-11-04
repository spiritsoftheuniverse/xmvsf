#ifndef XMVSF_STAGE76_SELECTION_H
#define XMVSF_STAGE76_SELECTION_H
#include <cstdlib>
#include <SDL.h>
// Host-only state, independent from CPS2 save-state RAM.
// 0 = RANDOM (the ROM's normal selection plus Wily), 1 = WILY FACTORY.
extern int xm76StageChoice;
extern int xm76StageResolved; // 0 = ordinary ROM stage; 1 = custom Wily stage.
static inline void xm76RollStage() {
    static bool seeded=false;
    if(!seeded){std::srand((unsigned)SDL_GetTicks());seeded=true;}
    // The ROM continues to randomize its native stages; Wily joins that pool.
    // One Wily slot among twelve native-stage slots (adjust if roster changes).
    xm76StageResolved=(xm76StageChoice==1 || (std::rand()%13)==0)?1:0;
}
#endif
