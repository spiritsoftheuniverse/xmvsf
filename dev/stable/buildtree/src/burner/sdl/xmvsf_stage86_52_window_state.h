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
// TEST 87.234: SDL_RenderSetLogicalSize computes the 16:9 letterbox
// viewport. The former boot path set the logical size FIRST and then
// SDL_RenderSetViewport(nullptr), accidentally discarding that centered
// viewport at startup. Reset a stale game/menu viewport FIRST, and apply
// the logical size LAST so SDL can center the first visible boot frame.
// This changes renderer coordinates only, never the user's OS window.
static inline bool xm8652CenteredBootCanvas(SDL_Renderer* render,
                                              const char* phase) {
    if(!render)return false;
    if(SDL_SetRenderTarget(render,nullptr)!=0){
        std::fprintf(stderr,"XM87.234 BOOT_VIEWPORT_TARGET_ERROR phase=%s err=%s\n",
            phase?phase:"?",SDL_GetError());
        return false;
    }
    // Do not inherit the game's 960x540/720x540 logical canvas, integer
    // scaling, clip or stale renderer viewport on in-process menu return.
    SDL_RenderSetLogicalSize(render,0,0);
    SDL_RenderSetIntegerScale(render,SDL_FALSE);
    SDL_RenderSetViewport(render,nullptr);
    SDL_RenderSetClipRect(render,nullptr);
    // MUST BE LAST: SDL computes the centered viewport for current output.
    const int result=SDL_RenderSetLogicalSize(render,960,540);
    SDL_Rect vp={0,0,0,0};
    SDL_RenderGetViewport(render,&vp);
    int outW=0,outH=0,lw=0,lh=0;
    SDL_GetRendererOutputSize(render,&outW,&outH);
    SDL_RenderGetLogicalSize(render,&lw,&lh);
    std::fprintf(stderr,
        "XM87.234 BOOT_VIEWPORT_CENTER phase=%s rc=%d output=%dx%d logical=%dx%d viewport=%d,%d,%d,%d\n",
        phase?phase:"?",result,outW,outH,lw,lh,vp.x,vp.y,vp.w,vp.h);
    return result==0 && lw==960 && lh==540;
}
// TEST 87.226: SDL_WINDOWPOS_CENTERED at creation is not enough on some
// Windows DPI/multi-monitor configurations: the first visible boot window
// was left-aligned until maximize or game video reinitialized its geometry.
// Center the ACTUAL outer OS window rectangle on its nearest monitor's
// working area, ONCE when the persistent window is first created.
// Never move a user's already-positioned window on menu return or game start.
static inline void xm8652CenterInitialWindow(SDL_Window* w) {
    if(!w)return;
#if defined(_WIN32)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if(SDL_GetWindowWMInfo(w,&info)==SDL_TRUE &&
       info.subsystem==SDL_SYSWM_WINDOWS) {
        HWND hwnd=info.info.win.window;
        RECT bounds={};
        MONITORINFO monitor={};
        monitor.cbSize=sizeof(monitor);
        HMONITOR which=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
        if(which && GetWindowRect(hwnd,&bounds) &&
           GetMonitorInfo(which,&monitor)) {
            const int width=bounds.right-bounds.left;
            const int height=bounds.bottom-bounds.top;
            const RECT& work=monitor.rcWork;
            const int centerX=work.left+((work.right-work.left)-width)/2;
            const int centerY=work.top+((work.bottom-work.top)-height)/2;
            const int x=centerX<work.left?work.left:centerX;
            const int y=centerY<work.top?work.top:centerY;
            const BOOL moved=SetWindowPos(hwnd,NULL,x,y,0,0,
                SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
            std::fprintf(stderr,
                "XM87.226 BOOT_WINDOW_CENTER Win32 result=%d outer=%dx%d x=%d y=%d\n",
                int(moved),width,height,x,y);
            SDL_PumpEvents();
            return;
        }
    }
#endif
    const int display=SDL_GetWindowDisplayIndex(w);
    SDL_Rect area={};
    const int index=display>=0?display:0;
#if SDL_VERSION_ATLEAST(2,0,5)
    int result=SDL_GetDisplayUsableBounds(index,&area);
    if(result!=0)result=SDL_GetDisplayBounds(index,&area);
#else
    const int result=SDL_GetDisplayBounds(index,&area);
#endif
    if(result!=0)return;
    int width=0,height=0;
    SDL_GetWindowSize(w,&width,&height);
    const int x=area.x+((area.w-width)>0?(area.w-width)/2:0);
    const int y=area.y+((area.h-height)>0?(area.h-height)/2:0);
    SDL_SetWindowPosition(w,x,y);
    SDL_PumpEvents();
    std::fprintf(stderr,"XM87.226 BOOT_WINDOW_CENTER SDL display=%d size=%dx%d x=%d y=%d\n",
        index,width,height,x,y);
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
