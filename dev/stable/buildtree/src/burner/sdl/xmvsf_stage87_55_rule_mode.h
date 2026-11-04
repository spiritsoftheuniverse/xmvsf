#ifndef XMVSF_STAGE87_55_RULE_MODE_H
#define XMVSF_STAGE87_55_RULE_MODE_H
// XM87.55 CLASSIC / TOURNAMENT: persisted user selection, no gameplay hooks.
// Stored separately from the Double Feature renderer and dizzy HUD option.
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
namespace XM8755Mode {
inline bool& tournament(){static bool enabled=false;return enabled;}
inline bool& loaded(){static bool done=false;return done;}
inline void Apply(){
#ifdef _WIN32
    const int rc=_putenv_s("XM8755_TOURNAMENT_MODE",tournament()?"1":"0");
#else
    const int rc=SDL_setenv("XM8755_TOURNAMENT_MODE",tournament()?"1":"0",1);
#endif
    if(rc!=0)std::fprintf(stderr,"XM87.55 MODE_ENV_FAILED rc=%d\n",rc);
    std::fprintf(stderr,"XM87.55 RULE_MODE selection=%s gameplay_changes=0\n",
                 tournament()?"TOURNAMENT":"CLASSIC");
}
inline void Load(){
    if(loaded())return;
    loaded()=true;
    tournament()=false; // Safe default: current rules in both modes.
    FILE* f=std::fopen("config/games/xmvsfu_rule_mode.ini","rb");
    if(f){int value=-1;
        if(std::fscanf(f,"tournament=%d",&value)==1 && (value==0 || value==1))
            tournament()=(value==1);
        std::fclose(f);
    }
    Apply();
}
inline void Toggle(){
    tournament()=!tournament();
    Apply();
    FILE* f=std::fopen("config/games/xmvsfu_rule_mode.ini","wb");
    if(f){std::fprintf(f,"tournament=%d\n",int(tournament()));std::fclose(f);}
    else std::fprintf(stderr,"XM87.55 MODE_SAVE_FAILED\n");
}
inline const char* Label(){return tournament()?"TOURNAMENT":"CLASSIC";}
}
#endif
