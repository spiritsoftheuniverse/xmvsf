#ifndef XMVSF_STAGE77_SELECTION_H
#define XMVSF_STAGE77_SELECTION_H
#include <SDL.h>
#include <cstdlib>
extern int xm77StageChoice;    // 0 Random, 1 Wily Factory, 2 Rock Castle
extern int xm77StageResolved;  // 0 native CPS2, 1 Wily, 2 Rock Castle
enum { XM77_RANDOM=0, XM77_WILY=1, XM77_ROCK_CASTLE=2, XM77_STAGE_COUNT=3 };
// Shared host stage contract: one resolved backdrop per game session;
// a second confirmation must never roll the stage mid-combat.
inline unsigned& xm77StageRollGeneration(){static unsigned value=0;return value;}
static inline void xm77RollStage() {
    static bool seeded=false;
    if(!seeded){std::srand((unsigned)SDL_GetTicks());seeded=true;}
    if(xm77StageChoice==XM77_WILY) xm77StageResolved=XM77_WILY;
    else if(xm77StageChoice==XM77_ROCK_CASTLE)
        xm77StageResolved=XM77_ROCK_CASTLE;
    else{
        // Original ROM stages still dominate RANDOM. Custom Wily and
        // Rock each receive one host slot; never replace the native
        // stage bytes in the random-native case.
        const int roll=std::rand()%14;
        xm77StageResolved=roll==0?XM77_WILY:
                          roll==1?XM77_ROCK_CASTLE:XM77_RANDOM;
    }
    ++xm77StageRollGeneration();
}
#endif
