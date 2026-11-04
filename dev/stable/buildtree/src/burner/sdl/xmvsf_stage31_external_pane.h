#ifndef XMVSF_STAGE31_EXTERNAL_PANE_H
#define XMVSF_STAGE31_EXTERNAL_PANE_H
// Exactly one owner: main.cpp. Other translation units see the same state.
// Geometry is GPU presentation only; game clocks, input and audio unchanged.
extern bool xm31ExternalPane;
static constexpr int XM31_BASE_W=960;
static constexpr int XM31_BASE_H=540;
static constexpr int XM31_EXTRA_W=240;
static inline int xm31PhysicalGutter(int height) {
    return height>0?(height*XM31_EXTRA_W+XM31_BASE_H/2)/XM31_BASE_H:0;
}
static inline int xm31LogicalWidth() {
    return XM31_BASE_W+(xm31ExternalPane?XM31_EXTRA_W:0);
}
static inline void xm31SetPaneWindow(SDL_Window* window,bool enabled) {
    if(xm31ExternalPane==enabled)return;
    if(window) {
        const Uint32 flags=SDL_GetWindowFlags(window);
        const bool fixed=(flags & (SDL_WINDOW_FULLSCREEN|SDL_WINDOW_MAXIMIZED))!=0;
        if(!fixed) {
            int w=0,h=0;
            SDL_GetWindowSize(window,&w,&h);
            const int shift=xm31PhysicalGutter(h);
            const int next=enabled?w+shift:w-shift;
            SDL_SetWindowSize(window,next>0?next:1,h);
        }
    }
    xm31ExternalPane=enabled;
}
#endif
