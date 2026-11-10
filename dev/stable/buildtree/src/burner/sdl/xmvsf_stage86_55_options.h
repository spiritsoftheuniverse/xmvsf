#ifndef XMVSF_STAGE86_55_OPTIONS_H
#define XMVSF_STAGE86_55_OPTIONS_H
// Stage 86.55: persisted native Double Feature switch and shared frontend palette.
// XM86.56: global final-frame grayscale plus Wily-specific independent lighting.
// Changing the mode must affect both main/menu and video translation units.
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../burn/drv/capcom/xmvsf_stage87_262_game_version.h"
inline bool& xm8655DoubleFeature(){static bool enabled=false;return enabled;}
inline bool& xm8655Loaded(){static bool loaded=false;return loaded;}
static inline const char* xm8655SettingsPath(){return "config/games/xmvsfu_double_feature.ini";}
static inline void xm8655ApplyMode(){
#ifdef _WIN32
    // The renderer reads getenv() from the same Windows CRT environment.
    // An empty value removes the variable, so the locked Wily painter stays untouched.
    const int result=_putenv_s("XM8649_WILY_RENDER",
        (XM87262Game::version4() && xm8655DoubleFeature())?"1":"");
#else
    const int result=SDL_setenv("XM8649_WILY_RENDER",
        (XM87262Game::version4() && xm8655DoubleFeature())?"1":"",1);
#endif
    if(result!=0)std::fprintf(stderr,"XM86.55 ERROR: could not apply render mode\n");
    std::fprintf(stderr,"XM86.55 DOUBLE_FEATURE mode=%d (Wily overlay gate)\n",int(xm8655DoubleFeature()));
}
static inline void xm8655Load(){
    if(xm8655Loaded())return;
    xm8655Loaded()=true;
    // Preserve launcher selection until the user explicitly saves an option.
    xm8655DoubleFeature()=std::getenv("XM8649_WILY_RENDER")!=nullptr;
    FILE* f=std::fopen(xm8655SettingsPath(),"rb");
    if(f){int value=-1;
        if(std::fscanf(f,"double_feature=%d",&value)==1 && (value==0 || value==1))
            xm8655DoubleFeature()=(value==1);
        std::fclose(f);
    }
    xm8655ApplyMode();
}
static inline void xm8655Toggle(){
    xm8655DoubleFeature()=!xm8655DoubleFeature();
    xm8655ApplyMode();
    FILE* f=std::fopen(xm8655SettingsPath(),"wb");
    if(f){std::fprintf(f,"double_feature=%d\n",int(xm8655DoubleFeature()));std::fclose(f);}
    else std::fprintf(stderr,"XM86.55 WARNING: could not save Double Feature setting\n");
}
static inline void xm8655RenderColor(SDL_Renderer* r,Uint8 red,Uint8 green,Uint8 blue,Uint8 alpha=255){
    if(XM87262Game::version4() && xm8655DoubleFeature()){
        // Integer Rec.601 luma, deliberately applied to every frontend draw primitive.
        const Uint8 lum=Uint8((77u*red+150u*green+29u*blue+128u)>>8);
        red=green=blue=lum;
    }
    SDL_SetRenderDrawColor(r,red,green,blue,alpha);
}
#endif
