#ifndef XMVSF_STAGE87_55_RULE_MODE_H
#define XMVSF_STAGE87_55_RULE_MODE_H
// Game Version is independent of Tournament; both persist across modes,
// stages, game restarts, and rematches in the existing shell.
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace XM8755Mode {
inline int& version(){static int selected=4;return selected;}
inline bool& tournament(){static bool enabled=false;return enabled;}
inline bool& loaded(){static bool done=false;return done;}
inline void Apply(){
#ifdef _WIN32
    const int a=_putenv_s("XM8755_GAME_VERSION",version()==4?"4":"3");
    const int b=_putenv_s("XM8755_TOURNAMENT_MODE",tournament()?"1":"0");
#else
    const int a=SDL_setenv("XM8755_GAME_VERSION",version()==4?"4":"3",1);
    const int b=SDL_setenv("XM8755_TOURNAMENT_MODE",tournament()?"1":"0",1);
#endif
    if(a||b)std::fprintf(stderr,"XM87.262 SETTINGS_ENV_FAILED version=%d tournament=%d\n",a,b);
    std::fprintf(stderr,"XM87.262 SETTINGS version=%d tournament=%u\n",
                 version(),unsigned(tournament()));
}
inline void Save(){
    FILE* f=std::fopen("config/games/xmvsfu_rule_mode.ini","wb");
    if(f){
        std::fprintf(f,"game_version=%d\ntournament=%d\n",version(),int(tournament()));
        std::fclose(f);
    }else std::fprintf(stderr,"XM87.262 SETTINGS_SAVE_FAILED\n");
}
inline void Load(){
    // On rematch/return the environment may have been replaced by an
    // external launcher: always reapply the persisted in-process choice.
    if(loaded()){Apply();return;}
    loaded()=true;version()=4;tournament()=false;
    FILE* f=std::fopen("config/games/xmvsfu_rule_mode.ini","rb");
    if(f){
        char line[80];
        while(std::fgets(line,sizeof(line),f)){
            int value=-1;
            if(std::sscanf(line,"game_version=%d",&value)==1 &&
               (value==3||value==4))version()=value;
            else if(std::sscanf(line,"tournament=%d",&value)==1 &&
                    (value==0||value==1))tournament()=(value==1);
        }
        std::fclose(f);
    }
    Apply();
}
inline void ToggleVersion(){version()=version()==4?3:4;Apply();Save();}
inline void ToggleTournament(){tournament()=!tournament();Apply();Save();}
inline void Toggle(){ToggleTournament();} // legacy call compatibility
inline const char* Label(){return tournament()?"ON":"OFF";}
}
#endif
