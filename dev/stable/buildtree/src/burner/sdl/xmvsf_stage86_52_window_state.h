// Stage86.52: one persistent SDL window geometry and startup focus policy.
// Presentation-only: never touches CPS2 game RAM, camera, or timing.
#ifndef XMVSF_STAGE86_52_WINDOW_STATE_H
#define XMVSF_STAGE86_52_WINDOW_STATE_H
#include <SDL.h>
#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#include <SDL_syswm.h>
#endif

static inline bool xm8652WindowFile(char* dst, size_t cap) {
    if (!dst || cap < 32) return false;
    char* folder = SDL_GetBasePath();
    if (!folder) return false;
    const int n=std::snprintf(dst,cap,"%sXMVSF_WindowState.ini",folder);
    SDL_free(folder);
    return n>0 && (size_t)n<cap;
}
static inline bool xm8652ValidGeometry(int w,int h) {
    return w>=320 && h>=224 && w<=10000 && h<=6000;
}
static inline void xm8652ReadGeometry(int* w,int* h) {
    if (!w || !h) return;
    *w=960; *h=540;
    char path[1024];
    if (!xm8652WindowFile(path,sizeof(path))) return;
    FILE* f=std::fopen(path,"rb");
    if (!f) return;
    char line[128]={0};
    if (std::fgets(line,sizeof(line),f)) {
        int rw=0,rh=0;
        if (std::sscanf(line,"XM86.52 %d %d",&rw,&rh)==2 && xm8652ValidGeometry(rw,rh)) {
            *w=rw;*h=rh;
        }
    }
    std::fclose(f);
    std::fprintf(stderr,"XM86.52 WINDOW_RESTORE size=%dx%d file=%s\n",*w,*h,path);
}
// Called on the existing window, never creates another window.
// Track windowed dimensions only: don't record a fullscreen display mode or
// a transient minimized/maximized size as the preferred windowed resolution.
static inline void xm8652TrackWindowGeometry(SDL_Window* w,bool force,const char* source) {
    if (!w) return;
    const Uint32 flags=SDL_GetWindowFlags(w);
    if (flags & (SDL_WINDOW_FULLSCREEN|SDL_WINDOW_MAXIMIZED|SDL_WINDOW_MINIMIZED)) return;
    int width=0,height=0;
    SDL_GetWindowSize(w,&width,&height);
    if (!xm8652ValidGeometry(width,height)) return;
    struct State {int observedW=0,observedH=0,savedW=0,savedH=0;Uint32 changedAt=0;};
    static State state;
    const Uint32 now=SDL_GetTicks();
    if (width!=state.observedW || height!=state.observedH) {
        state.observedW=width;state.observedH=height;state.changedAt=now;
    }
    if (!force && now-state.changedAt<300) return;
    if (width==state.savedW && height==state.savedH) return;
    char path[1024];
    if (!xm8652WindowFile(path,sizeof(path))) return;
    FILE* f=std::fopen(path,"wb");
    if (!f) {std::fprintf(stderr,"XM86.52 WINDOW_SAVE_FAILED path=%s\n",path);return;}
    std::fprintf(f,"XM86.52 %d %d\n",width,height);
    const int error=std::fclose(f);
    if (error==0) {
        state.savedW=width;state.savedH=height;
        std::fprintf(stderr,"XM86.52 WINDOW_SAVED phase=%s size=%dx%d\n",source?source:"?",width,height);
    }
}
// A single request when entering the boot UI / game video, never a timer.
static inline void xm8652FocusWindowOnce(SDL_Window* w,const char* phase) {
    if (!w) return;
    SDL_ShowWindow(w);
    SDL_RaiseWindow(w);
    SDL_PumpEvents();
#if defined(_WIN32)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(w,&info)==SDL_TRUE && info.subsystem==SDL_SYSWM_WINDOWS) {
        HWND hwnd=info.info.win.window;
        if (IsIconic(hwnd)) ShowWindow(hwnd,SW_RESTORE);
        SetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
        SetForegroundWindow(hwnd);
        if (GetForegroundWindow()==hwnd) SetFocus(hwnd);
    }
#endif
#if SDL_VERSION_ATLEAST(2,0,5)
    if (!(SDL_GetWindowFlags(w)&SDL_WINDOW_INPUT_FOCUS)) {
        if (SDL_SetWindowInputFocus(w)!=0)
            std::fprintf(stderr,"XM86.52 WINDOW_INPUT_FOCUS_NOTE phase=%s err=%s\n",phase?phase:"?",SDL_GetError());
    }
#endif
    SDL_PumpEvents();
    int width=0,height=0;
    SDL_GetWindowSize(w,&width,&height);
    std::fprintf(stderr,"XM86.52 WINDOW_FOCUS phase=%s size=%dx%d inputFocus=%d\n",phase?phase:"?",width,height,
        (int)((SDL_GetWindowFlags(w)&SDL_WINDOW_INPUT_FOCUS)!=0));
}
#endif
