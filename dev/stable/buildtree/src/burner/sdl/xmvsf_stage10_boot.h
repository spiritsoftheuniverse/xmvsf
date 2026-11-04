// XMVSF Stage 10: self-contained SDL2 boot selector injected into native FBNeo SDL2 build.
// The emulation engine is compiled into the same executable. No libretro DLL or fbneo.exe launch.
// Stage 10 ONLY verifies native game boot. TRAINING is intentionally disabled until its
// health/time implementation is bound and verified against the USA 961023 game-state.
#ifndef XMVSF_STAGE10_BOOT_INCLUDED
#define XMVSF_STAGE10_BOOT_INCLUDED

struct XMVSFGlyph { char c; unsigned char row[7]; };
static const XMVSFGlyph xmvsfFont[] = {
    {'A', {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'B', {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}},
    {'C', {0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f}},
    {'D', {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}},
    {'E', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}},
    {'F', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}},
    {'G', {0x0f, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0f}},
    {'H', {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'I', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0c}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}},
    {'M', {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'P', {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}},
    {'Q', {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}},
    {'R', {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}},
    {'S', {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}},
    {'T', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1b, 0x11}},
    {'X', {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}},
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1e, 0x01, 0x01, 0x06, 0x01, 0x01, 0x1e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e}},
    {'6', {0x0f, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x1e}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}},
    {'/', {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10}},
};

static const unsigned char* xmvsfGlyph(char c) {
    for (unsigned int i=0; i<sizeof(xmvsfFont)/sizeof(xmvsfFont[0]); ++i) {
        if (xmvsfFont[i].c == c) return xmvsfFont[i].row;
    }
    return 0;
}
static void xmvsfPixelText(SDL_Renderer* render, const char* text, int cx, int y, int pixel,
                            unsigned char r, unsigned char g, unsigned char b) {
    if (!text) return;
    int count=0;
    while (text[count]) ++count;
    int x=cx-(count*6*pixel)/2;
    SDL_SetRenderDrawColor(render,r,g,b,255);
    for (int i=0; i<count; ++i) {
        const unsigned char* glyph=xmvsfGlyph(text[i]);
        if (glyph) for (int row=0; row<7; ++row) for (int bit=0; bit<5; ++bit) {
            if (glyph[row] & (1 << (4-bit))) {
                SDL_Rect pix = {x+bit*pixel,y+row*pixel,pixel,pixel};
                SDL_RenderFillRect(render, &pix);
            }
        }
        x+=6*pixel;
    }
}

// Return 0=Arcade, -1=Quit. Training is visible but cannot silently start
// an ordinary match with cheats missing.
static int xmvsfBootSelector() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr,"XMVSF boot menu: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    SDL_Window* window=SDL_CreateWindow("X-MEN VS. STREET FIGHTER | USA 961023",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,540,SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr,"XMVSF boot menu: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit(); return -1;
    }
    SDL_Renderer* render=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!render) render=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if (!render) {
        fprintf(stderr,"XMVSF boot menu: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window); SDL_Quit(); return -1;
    }
    SDL_RenderSetLogicalSize(render,960,540);
    SDL_GameController* pad=0;
    for (int i=0; i<SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {pad=SDL_GameControllerOpen(i); if (pad) break;}
    }
    int selection=0, outcome=-1, running=1;
    Uint32 messageUntil=0;
    while(running) {
        SDL_Event ev;
        while(SDL_PollEvent(&ev)) {
            if(ev.type==SDL_QUIT) {running=0; outcome=-1;}
            if(ev.type==SDL_KEYDOWN) {
                if (ev.key.keysym.sym==SDLK_ESCAPE) {running=0; outcome=-1;}
                if (ev.key.keysym.sym==SDLK_UP || ev.key.keysym.sym==SDLK_DOWN) selection=1-selection;
                if (ev.key.keysym.sym==SDLK_RETURN || ev.key.keysym.sym==SDLK_SPACE) {
                    if(selection==0) {outcome=0;running=0;}
                    else messageUntil=SDL_GetTicks()+4000;
                }
            }
            if (ev.type==SDL_CONTROLLERBUTTONDOWN) {
                if (ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP || ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                    selection=1-selection;
                if (ev.cbutton.button==SDL_CONTROLLER_BUTTON_A || ev.cbutton.button==SDL_CONTROLLER_BUTTON_START) {
                    if(selection==0) {outcome=0;running=0;}
                    else messageUntil=SDL_GetTicks()+4000;
                }
                if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_B) {outcome=-1;running=0;}
            }
        }
        SDL_SetRenderDrawColor(render,0,0,0,255); SDL_RenderClear(render);
        xmvsfPixelText(render,"X-MEN VS STREET FIGHTER",480,92,4,77,178,255);
        xmvsfPixelText(render,"USA 961023",480,145,3,155,150,255);
        SDL_Rect bar={215,218+selection*84,530,68};
        SDL_SetRenderDrawColor(render,25,19,68,255); SDL_RenderFillRect(render,&bar);
        SDL_SetRenderDrawColor(render,76,93,175,255); SDL_RenderDrawRect(render,&bar);
        xmvsfPixelText(render,"ARCADE",480,237,4,selection==0?255:125,selection==0?255:125,255);
        xmvsfPixelText(render,"TRAINING",480,321,4,selection==1?255:125,selection==1?255:125,255);
        if (SDL_GetTicks()<messageUntil) {
            xmvsfPixelText(render,"TRAINING NOT ACTIVE YET",480,408,2,255,118,118);
        } else {
            xmvsfPixelText(render,"SELECT - ENTER / A",480,408,2,139,145,191);
        }
        xmvsfPixelText(render,"ESC TO QUIT",480,455,2,105,105,129);
        SDL_RenderPresent(render);
        SDL_Delay(8);
    }
    if (pad) SDL_GameControllerClose(pad);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return outcome;
}
#endif
