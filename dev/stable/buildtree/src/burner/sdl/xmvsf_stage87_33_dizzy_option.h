#ifndef XMVSF_STAGE87_33_DIZZY_OPTION_H
#define XMVSF_STAGE87_33_DIZZY_OPTION_H
// Game-wide persisted HUD switch. Applies to all modes; no training RAM writes.
#include <cstdio>
#include <cstdlib>
inline bool& xm8733DizzyEnabled(){static bool enabled=true;return enabled;}
inline void xm8733DizzyApply(){
#ifdef _WIN32
    _putenv_s("XM8733_DIZZY_BAR",xm8733DizzyEnabled()?"1":"0");
#else
    SDL_setenv("XM8733_DIZZY_BAR",xm8733DizzyEnabled()?"1":"0",1);
#endif
    std::fprintf(stderr,"XM87.33 DIZZY_OPTION enabled=%d\n",int(xm8733DizzyEnabled()));
}
inline void xm8733DizzyLoad(){
    const char* prior=std::getenv("XM8733_DIZZY_BAR");
    if(prior && prior[0]=='0' && prior[1]=='\0')xm8733DizzyEnabled()=false;
    FILE* f=std::fopen("config/games/xmvsfu_dizzy_bar.ini","rb");
    if(f){int flag=-1; if(std::fscanf(f,"dizzy_bar=%d",&flag)==1 && (flag==0||flag==1))
        xm8733DizzyEnabled()=(flag==1);std::fclose(f);}
    xm8733DizzyApply();
}
inline void xm8733DizzyToggle(){
    xm8733DizzyEnabled()=!xm8733DizzyEnabled();xm8733DizzyApply();
    FILE* f=std::fopen("config/games/xmvsfu_dizzy_bar.ini","wb");
    if(f){std::fprintf(f,"dizzy_bar=%d\n",int(xm8733DizzyEnabled()));std::fclose(f);}
    else std::fprintf(stderr,"XM87.33 DIZZY_OPTION save failed\n");
}
#endif
