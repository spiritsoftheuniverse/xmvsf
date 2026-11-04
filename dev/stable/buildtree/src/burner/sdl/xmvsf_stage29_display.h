// Stage29: SDL2 GPU presentation/window policy. No emulator timing changes.
#ifndef XMVSF_STAGE29_DISPLAY_H
#define XMVSF_STAGE29_DISPLAY_H
#include "xmvsf_stage31_external_pane.h" /* XMVSF_STAGE31_EXTERNAL_PANE */

// A window-sized native raster choice through 6x. The existing Stage26
// 960x540 logical canvas is retained to keep all menu/game layouts stable.
struct XM29WindowPreset { int w; int h; const char* label; };
static const XM29WindowPreset xm29Presets[] = {
    {384,224,"NATIVE 384 X 224"},
    {768,448,"2X 768 X 448"},
    {1152,672,"3X 1152 X 672"},
    {1536,896,"4X 1536 X 896"},
    {1920,1120,"5X 1920 X 1120"},
    {2304,1344,"6X 2304 X 1344"}
};
static constexpr int XM29_PRESET_COUNT = (int)(sizeof(xm29Presets)/sizeof(xm29Presets[0]));

static inline bool xm29IsFullscreen(SDL_Window* w){
    return w && (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN) != 0;
}
static inline bool xm29WindowPreset(SDL_Window* w,int idx){
    if(!w || idx<0 || idx>=XM29_PRESET_COUNT)return false;
    // Explicit windowed resolution selections are never capped to the monitor.
    // Windows itself may restrict coordinates, but we never crop the renderer.
    if(xm29IsFullscreen(w) && SDL_SetWindowFullscreen(w,0)!=0)return false;
    if(SDL_GetWindowFlags(w)&SDL_WINDOW_MAXIMIZED)SDL_RestoreWindow(w);
    SDL_SetWindowSize(w,xm29Presets[idx].w+
        (xm31ExternalPane?xm31PhysicalGutter(xm29Presets[idx].h):0),
        xm29Presets[idx].h);
    return true;
}
static inline bool xm29DesktopFit(SDL_Window* w){
    if(!w)return false;
    // Borderless desktop resolution: no video mode or emulation refresh change.
    return SDL_SetWindowFullscreen(w,SDL_WINDOW_FULLSCREEN_DESKTOP)==0;
}
static inline bool xm29ToggleDesktopFit(SDL_Window* w){
    if(!w)return false;
    return SDL_SetWindowFullscreen(w,xm29IsFullscreen(w)?0:SDL_WINDOW_FULLSCREEN_DESKTOP)==0;
}
static inline bool xm29WindowMaximize(SDL_Window* w){
    if(!w)return false;
    if(xm29IsFullscreen(w) && SDL_SetWindowFullscreen(w,0)!=0)return false;
    SDL_MaximizeWindow(w);
    return true;
}
static inline int xm29NearestPreset(SDL_Window* w){
    if(!w)return 1;
    int width=960,height=540;
    SDL_GetWindowSize(w,&width,&height);
    int best=0;
    long long bestDifference=-1;
    for(int i=0;i<XM29_PRESET_COUNT;i++){
        long long dw=(long long)width-xm29Presets[i].w-
            (xm31ExternalPane?xm31PhysicalGutter(xm29Presets[i].h):0);
        long long dh=(long long)height-xm29Presets[i].h;
        long long diff=dw*dw+dh*dh;
        if(bestDifference<0||diff<bestDifference){best=i;bestDifference=diff;}
    }
    return best;
}
static inline bool xm29StepWindow(SDL_Window* w,int direction){
    if(!w || !direction)return false;
    // Fullscreen always shows the largest uncropped image. Ctrl+/- changes
    // only a windowed size; it does not zoom/crop fullscreen presentation.
    if(xm29IsFullscreen(w))return true;
    const int curr=xm29NearestPreset(w);
    const int target=curr+(direction>0?1:-1);
    if(target<0||target>=XM29_PRESET_COUNT)return true;
    return xm29WindowPreset(w,target);
}
static inline bool xm29HandleDisplayHotkey(const SDL_Event& e,SDL_Window* w){
    if(!w || e.type!=SDL_KEYDOWN || e.key.repeat)return false;
    const SDL_Keycode k=e.key.keysym.sym;
    const SDL_Keymod mod=(SDL_Keymod)e.key.keysym.mod;
    if((mod & KMOD_ALT) && k==SDLK_RETURN){xm29ToggleDesktopFit(w);return true;}
    if((mod & KMOD_CTRL) && !(mod & KMOD_ALT)){
        if(k==SDLK_PLUS||k==SDLK_EQUALS||k==SDLK_KP_PLUS){xm29StepWindow(w,+1);return true;}
        if(k==SDLK_MINUS||k==SDLK_KP_MINUS){xm29StepWindow(w,-1);return true;}
    }
    return false;
}
#endif
