#ifndef XMVSF_STAGE87_262_GAME_VERSION_H
#define XMVSF_STAGE87_262_GAME_VERSION_H
// Shared by native combat, sound and HUD. Never inspect selected stage,
// Arcade/Training/Versus, random stage result or opponent character here.
#include <cstdlib>
#include <cstring>
namespace XM87262Game {
inline bool version4(){
    const char* v=std::getenv("XM8755_GAME_VERSION");
    return v && std::strcmp(v,"4")==0;
}
inline bool tournamentKnockdowns(){
    const char* t=std::getenv("XM8755_TOURNAMENT_MODE");
    return version4() && t && std::strcmp(t,"1")==0;
}
}
#endif
