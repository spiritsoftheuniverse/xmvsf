#include <cstdio> // XM86.34 frame diagnostics
#include <cstring> // XM87.132 replay indicator text length
#include <vector>
#include <string>
// blitter effects via SDL2
#include "burner.h"
#include "vid_support.h"
#include "vid_softfx.h"

#ifdef INCLUDE_SWITCHRES
#include <switchres_wrapper.h>
#endif

#include <stdio.h>
#include <SDL.h>
#include "xmvsf_stage87_74_select_present.h" // XM87.74 native framebuffer matched pen colors
#include "../../../burn/drv/capcom/xmvsf_stage87_262_game_version.h" // original V3 HUD
#include "../../../burner/sdl/xmvsf_stage87_139_probe.h" // XM87.139 F10 probe status
#include "xmvsf_stage87_140_chat_overlay.h" // XM87.140 opt-in DHC history
#include "../../../burner/sdl/xmvsf_stage87_206_crash_report.h" // FIGHT exception breadcrumbs
#include "../../../burner/sdl/xmvsf_stage23_ui.h" /* XMVSF_STAGE23_TRAINING_MENU */
#include "../../../burner/sdl/xmvsf_stage87_217_native_float.h" /* game-pixel SF2 popups */
#include "../../../burner/sdl/xmvsf_stage24_versus.h" /* XMVSF_STAGE24_VERSUS */
#include <SDL_image.h>
#include "../../../burner/sdl/xmvsf_stage86_52_window_state.h" /* XM86.52 */
#include "../../../burner/sdl/xmvsf_stage86_56_final.h" /* XM86.56 global final presentation */
#include "../../../burn/drv/capcom/xmvsf_stage87_33_dizzy_hud.h"
#include "../../../burn/drv/capcom/xmvsf_stage87_28_interior_mask.h" // CPS2 native fighter opacity stencil
#include "xmvsf_stage87_40_hud_color.h" // Separate portrait and HP source-colored overlay
#include "../../../burn/drv/capcom/xmvsf_stage87_35_hud_palette.h" // Exact native HUD chroma overlay // Post-Wily HUD
extern "C" int xm87132ReplayRecordingFrames(); // XM87.132 shared live recorder status
extern "C" int xm87166ReplayVisualStatus(); // v2 F9 recording/saved/error indicator
extern "C" int xm87166ReplayVisualPeek(); // reserved banner space without consuming notice frames
extern UINT8 *CpsRamFF;

extern int vsync;
extern char videofiltering[3];
extern void xm8657EnsureCurrentSprites(); // XM86.57 pre-fight first-frame capture
extern int xm8658GameStatus; // XM86.58 phase observer; 0/1 character-select states
extern int xm14TrainingMode;
extern int xm14InputTarget;
extern "C" void xm8770SelectInput(unsigned,unsigned,int,unsigned,unsigned){
    // XM87.132 CLEAN: retired native selector capture; linkage preserved.
}


static unsigned char* VidMem = NULL;
extern SDL_Window* sdlWindow;
SDL_Renderer* sdlRenderer = NULL;
static SDL_Texture* sdlTexture = NULL;
static SDL_Texture* xm8733Texture = NULL;
static SDL_Texture* xm87217FloatTexture = NULL;
static std::vector<Uint8> xm87217FloatRGBA(384u*224u*4u,0u);
static unsigned char xm8733Pixels[384*224*4]={};
static void xm8733Overlay(SDL_Renderer*,const SDL_Rect&,bool nativeStage=false);
static int  nRotateGame = 0;
static bool bFlipped = false;
static SDL_Rect dstrect;
#include "xmvsf_stage86_49_painter.h" // XM86.49 native-window opt-in stage, forest failback
#include "xmvsf_stage87_274_final_destination.h" // custom stage: sky / platform / native fighter layers
static char Windowtitle[512];
static int display_w = 400, display_h = 300;
Uint32 screenFlags;

extern UINT16 maxLinesMenu;	// sdl2_gui_ingame.cpp: number of lines to show in ingame menus
extern bool didReinitialise;

void RenderMessage()
{
	// First render anything that is key-held based
	if (bAppDoFast)
	{
		inprint_shadowed(sdlRenderer, "FFWD", 10, 10);
	}
	if (bAppShowFPS)
	{
		if (bAppFullscreen)
		{
			inprint_shadowed(sdlRenderer, fpsstring, 10, 50);
		} 
		else 
		{
			sprintf(Windowtitle, "FBNeo - FPS: %s - %s - %s", fpsstring, BurnDrvGetTextA(DRV_NAME), BurnDrvGetTextA(DRV_FULLNAME));
			SDL_SetWindowTitle(sdlWindow, Windowtitle);
		}
	}

	if (messageFrames > 1)
	{
		inprint_shadowed(sdlRenderer, lastMessage, 10, 30);
		messageFrames--;
	}
}

// XM87.132 final presenter overlay. The compositor runs before this draw,
// avoiding readback shifting or grading the red recording indicator.
static bool xm87132CleanStableVideo(){
    return true; // No debug screen files, console, or frame diagnostics.
}
static void xm87132ReplayNotice(SDL_Renderer* renderer){
    if(!renderer)return;
    const int state=xm87166ReplayVisualStatus();
    const int count=xm87132ReplayRecordingFrames();
    if(count<=0 && state==0)return;
    int w=0,h=0;
    SDL_RenderGetLogicalSize(renderer,&w,&h);
    if(w<=0 || h<=0)SDL_GetRendererOutputSize(renderer,&w,&h);
    if(w<=0 || h<=0)return;
    char caption[64];
    if(state<0)
        std::snprintf(caption,sizeof(caption),"F9 RECORD ERROR  SEE LOG");
    else if(state==2)
        std::snprintf(caption,sizeof(caption),"RECORDING SAVED  F9");
    else
        std::snprintf(caption,sizeof(caption),"RECORDING %06d  F9 STOP",count-1);
    // The inline font is approximately 8 logical pixels per glyph.
    int x=w-int(std::strlen(caption))*8-20;
    if(x<8)x=8;
    const int y=h>30?h-23:4;
    Uint8 r=0,g=0,b=0,a=0;
    SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
    SDL_Rect dot={x-12,y+3,7,7};
    if(state<0) SDL_SetRenderDrawColor(renderer,255,172,70,255);
    else if(state==2) SDL_SetRenderDrawColor(renderer,72,218,143,255);
    else SDL_SetRenderDrawColor(renderer,235,45,55,255);
    SDL_RenderFillRect(renderer,&dot);
    SDL_SetRenderDrawColor(renderer,r,g,b,a);
    inprint_shadowed(renderer,caption,x,y);
}

// XM87.139 bottom-right status is drawn AFTER the entire video compositor,
// so it cannot contaminate native character tiles, DF masks, or Wily palette.
static void xm87139ProbeNotice(SDL_Renderer* renderer){
    if(!renderer)return;
    const XM87139Probe::State& st=XM87139Probe::state();
    char caption[80];
    if(st.active)std::snprintf(caption,sizeof(caption),"PROBE ON %06u F10 STOP",st.frames);
    else if(st.error)std::snprintf(caption,sizeof(caption),"PROBE ERROR F10 RETRY");
    else std::snprintf(caption,sizeof(caption),"PROBE OFF F10 START");
    int w=0,h=0;
    SDL_RenderGetLogicalSize(renderer,&w,&h);
    if(w<=0||h<=0)SDL_GetRendererOutputSize(renderer,&w,&h);
    if(w<=0||h<=0)return;
    const int labelWidth=(int)std::strlen(caption)*8;
    const int x=w-labelWidth-15>10?w-labelWidth-15:10;
    const bool f9Banner=xm87132ReplayRecordingFrames()>0 ||
                        xm87166ReplayVisualPeek()!=0;
    const int y=(f9Banner && h>64)?h-46:(h>28?h-24:3);
    Uint8 r=0,g=0,b=0,a=0;SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
    SDL_BlendMode blend=SDL_BLENDMODE_NONE;SDL_GetRenderDrawBlendMode(renderer,&blend);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_Rect backing={x-13,y-2,labelWidth+16,17};
    SDL_SetRenderDrawColor(renderer,0,0,0,184);
    SDL_RenderFillRect(renderer,&backing);
    SDL_Rect light={x-9,y+3,6,6};
    if(st.error)SDL_SetRenderDrawColor(renderer,235,55,55,255);
    else if(st.active)SDL_SetRenderDrawColor(renderer,65,236,138,255);
    else SDL_SetRenderDrawColor(renderer,135,145,155,255);
    SDL_RenderFillRect(renderer,&light);
    SDL_SetRenderDrawColor(renderer,r,g,b,a);
    SDL_SetRenderDrawBlendMode(renderer,blend);
    inprint_shadowed(renderer,caption,x,y);
}

static int Exit()
{
    extern bool xm26PersistentShell;
    extern bool xm26ReturnToBoot;
    // TEST 87.227: on MAIN MENU the native single SDL_Window persists.
    // Retain its renderer as well: destroying/recreating just the renderer
    // caused an intermittent black boot screen despite working menu input.
    // All gameplay-owned textures are still explicitly destroyed below.
    const bool preserveMenuRenderer=xm26PersistentShell &&
        xm26ReturnToBoot && sdlWindow && sdlRenderer;

#ifdef INCLUDE_SWITCHRES
	sr_deinit();
#endif
	kill_inline_font(); //TODO: This is not supposed to be here
    xm8652TrackWindowGeometry(sdlWindow,true,"game-exit");
    XM8773Present::State().Close(); // XM87.73 release exact-pen overlay texture
    XMFD::cleanup(); // Release procedural Final Destination and original platform textures.
    xm8649VideoCleanup(); // XM86.49 release textures before renderer destruction
    if(xm8733Texture){SDL_DestroyTexture(xm8733Texture);xm8733Texture=NULL;}
    if(xm87217FloatTexture){
        SDL_DestroyTexture(xm87217FloatTexture);
        xm87217FloatTexture=NULL;
    }
    xm8656Cleanup(sdlRenderer); // XM86.56 release final compositor on exit
    if(sdlTexture)SDL_DestroyTexture(sdlTexture);
    sdlTexture=nullptr;
    if(preserveMenuRenderer){
        // Return the same live renderer to the boot selector; reset all
        // game-specific viewport/target settings after disposing textures.
        // 87.234: restore SDL's centered logical letterbox before
        // passing the live renderer back to the boot selector.
        // Keep the original SDL_Renderer/GPU context intact.
        if(!xm8652CenteredBootCanvas(sdlRenderer,"game-to-menu"))
            std::fprintf(stderr,"XM87.234 MENU_VIEWPORT_NOTE unable_to_recenter\n");
        SDL_SetRenderDrawBlendMode(sdlRenderer,SDL_BLENDMODE_NONE);
        std::fprintf(stderr,
            "XM87.227 MENU_RENDERER_HANDOFF retained window=%p renderer=%p\n",
            (void*)sdlWindow,(void*)sdlRenderer);
        std::fflush(stderr);
    }else{
        if(sdlRenderer)SDL_DestroyRenderer(sdlRenderer);
        std::fprintf(stderr,
            "XM87.227 VIDEO_RENDERER_DESTROY exit_not_menu=1\n");
    }
    // Ownership ends with VideoExit. The boot selector will get the
    // retained renderer with SDL_GetRenderer(sdlWindow) if applicable.
    sdlRenderer=nullptr;
	if (!xm26PersistentShell) SDL_DestroyWindow(sdlWindow);
	if (!xm26PersistentShell) sdlWindow = NULL;
	
	if (VidMem)
	{
		free(VidMem);
	}
	return 0;
}

#include "xmvsf_stage31_external_pane.h" /* XMVSF_STAGE31_EXTERNAL_PANE */
void AdjustImageSize()
{
    // XMVSF_STAGE26_PERSISTENT_SHELL: maintain 960x540 SDL window across modes.
    extern bool xm26PersistentShell;
    if (xm26PersistentShell && sdlWindow && sdlRenderer && !nRotateGame) {
        SDL_RenderSetIntegerScale(sdlRenderer,SDL_FALSE);
        SDL_RenderSetLogicalSize(sdlRenderer,xm31LogicalWidth(),XM31_BASE_H); /* XMVSF_STAGE31_EXTERNAL_PANE */
        const int contentW=display_w>0?display_w:384;
        const int contentH=display_h>0?display_h:288;
        const int viewW=(contentW*540/contentH>960)?960:contentW*540/contentH;
        const int viewH=viewW*contentH/contentW;
        dstrect.x=(960-viewW)/2;
        dstrect.y=(540-viewH)/2;
        dstrect.w=viewW;
        dstrect.h=viewH;
        maxLinesMenu=20;
        return;
    }

	screenFlags = SDL_GetWindowFlags(sdlWindow);

	// Scale governed by -windowscale command line option.
	// Screen center fix thanks to Woises
	if (nRotateGame) {
		if (screenFlags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			int w;
			int h;
			SDL_GetWindowSize(sdlWindow, &w, &h);
			SDL_SetWindowSize(sdlWindow, (display_w * w / h) * nWindowScale, display_w * nWindowScale);
			SDL_RenderSetLogicalSize(sdlRenderer, (display_w * w / h), display_w);
			dstrect.x = ((display_w * w / h) - display_w) / 2;
		} else {
			SDL_RestoreWindow(sdlWindow);		// If started fullscreen, switching to window can get maximized
			SDL_SetWindowSize(sdlWindow, display_h * nWindowScale, display_w * nWindowScale);
			SDL_RenderSetLogicalSize(sdlRenderer, display_h, display_w);
			dstrect.x = (display_h - display_w) / 2;
		}
		dstrect.y = (display_w - display_h) / 2;
		maxLinesMenu = display_w / 10 - 6;		// Get number of lines to show in ingame menus
	} else {
		if (screenFlags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			int w;
			int h;
			SDL_GetWindowSize(sdlWindow, &w, &h);
			SDL_SetWindowSize(sdlWindow, (display_h * w / h) * nWindowScale, display_h * nWindowScale);

			if (display_w < (display_h * w / h)) {
				SDL_RenderSetLogicalSize(sdlRenderer, (display_h * w / h), display_h);
				dstrect.x = ((display_h * w / h) - display_w) / 2;
				dstrect.y = 0;
			} else {
				SDL_RenderSetLogicalSize(sdlRenderer, display_w, (display_w * h / w));
				dstrect.x = 0;
				dstrect.y = ((display_w * h / w) - display_h) / 2;
			}
		} else {
			SDL_RestoreWindow(sdlWindow);
			SDL_SetWindowSize(sdlWindow, display_w * nWindowScale, display_h * nWindowScale);
			SDL_RenderSetLogicalSize(sdlRenderer, display_w, display_h);
			dstrect.x = 0;
			dstrect.y = 0;
		}
		maxLinesMenu = display_h / 10 - 6;		// Get number of lines to show in ingame menus
	}
	SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	dstrect.w = display_w;
	dstrect.h = display_h;
}

// Stage 12 first-frame diagnostics.
static void xmvsfStage12VideoLog(const char* message)
{
    if(xm87132CleanStableVideo())return;
    FILE* file = fopen("xmvsf_stage12_video.log", "a");
    if (file) { fprintf(file, "%s\n", message); fclose(file); }
}

static int Init()
{
    if(!xm87132CleanStableVideo()) { FILE* log = fopen("xmvsf_stage12_video.log", "w");
      if (log) { fputs("STAGE12_VIDEO_INIT\n", log); fclose(log); } }

	int nMemLen = 0;
	int GameAspectX = 4, GameAspectY = 3;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("vid init error\n");
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return 3;
	}

	nRotateGame = 0;

	if (bDrvOkay)
	{
		// Get the game screen size
		if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL)
		{
#ifdef FBNEO_DEBUG
			printf("Vertical\n");
#endif
			BurnDrvGetVisibleSize(&nVidImageHeight, &nVidImageWidth);
			BurnDrvGetAspect(&GameAspectY, &GameAspectX);
			nRotateGame = 1;
		}
		else
		{
			BurnDrvGetVisibleSize(&nVidImageWidth, &nVidImageHeight);
			BurnDrvGetAspect(&GameAspectX, &GameAspectY);
		}

#ifdef INCLUDE_SWITCHRES
		// Don't force 4:3 aspect-ratio, until there is a command-line switch
		display_w = nVidImageWidth;
		display_h = nVidImageHeight;
		sr_init();
		if (nRotateGame) sr_set_rotation(1);
#else
		if (nRotateGame) {
			display_w = nVidImageHeight * GameAspectX / GameAspectY;
			display_h = nVidImageHeight;
		} else {
			display_w = nVidImageWidth;
			display_h = nVidImageWidth * GameAspectY / GameAspectX;
		}
#endif

		if (BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED)
		{
#ifdef FBNEO_DEBUG
			printf("Flipped\n");
#endif
			bFlipped = 1;
		} else {
			bFlipped = 0;
		}
	}

	sprintf(Windowtitle, "FBNeo - %s - %s", BurnDrvGetTextA(DRV_NAME), BurnDrvGetTextA(DRV_FULLNAME));

	if (bAppFullscreen) screenFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP;
	else screenFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	//Test refresh rate availability
#ifdef FBNEO_DEBUG
	printf("Game resolution: %dx%d@%f\n", nVidImageWidth, nVidImageHeight, nBurnFPS/100.0);
#endif

#ifdef INCLUDE_SWITCHRES
	sr_mode srm;
	unsigned char interlace = 0; // FBN doesn't handle interlace yet, force it to disabled
	double rr = nBurnFPS / 100.0;
	sr_init_disp();
	sr_add_mode(display_w, display_h, rr, interlace, &srm);
	sr_switch_to_mode(display_w, display_h, rr, interlace, &srm);
#endif

	// XMVSF_STAGE11_SINGLE_WINDOW
	if (sdlWindow == NULL) {
	if (nRotateGame)
	{
		sdlWindow = SDL_CreateWindow(
			Windowtitle,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			display_h,
			display_w,
			screenFlags
		);
	}
	else
	{
		sdlWindow = SDL_CreateWindow(
			Windowtitle,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			display_w,
			display_h,
			screenFlags
		);
	}

	} else {
		// Preserve the same SDL_Window created by our native menu.
		SDL_SetWindowTitle(sdlWindow, Windowtitle);
		SDL_SetWindowResizable(sdlWindow, SDL_TRUE);
		if (bAppFullscreen) {
			if (SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
				fprintf(stderr, "Stage11 fullscreen transition: %s\n", SDL_GetError());
        } else {
            // The persistent startup menu owns fullscreen/windowed mode.
            // Do not cancel its selection on entry to character select.
            extern bool xm26PersistentShell;
            if (!xm26PersistentShell) SDL_SetWindowFullscreen(sdlWindow,0);
        }
        // AdjustImageSize() computes the logical viewport for the persistent shell.

		SDL_ShowWindow(sdlWindow);
	}

	// Check that the window was successfully created
	if (sdlWindow == NULL)
	{
		// In the case that the window could not be made...
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	Uint32 renderflags = SDL_RENDERER_ACCELERATED;

	if (vsync)
	{
		renderflags = renderflags | SDL_RENDERER_PRESENTVSYNC;
	}

	// XMVSF_STAGE12_PERSISTENT_RENDERER
	// Reuse the native menu's SDL_Renderer and graphics context.
	sdlRenderer = SDL_GetRenderer(sdlWindow);
	if (sdlRenderer != NULL) {
		xmvsfStage12VideoLog("VIDEO_RENDERER_ADOPTED_FROM_MENU");
	} else {
		xmvsfStage12VideoLog("VIDEO_RENDERER_CREATING_NEW_FALLBACK");
		sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, renderflags);
		if (sdlRenderer == NULL) {
			sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_SOFTWARE);
		}
	}
	if (sdlRenderer == NULL) {
		xmvsfStage12VideoLog("ERROR_CREATE_RENDERER");
		fprintf(stderr, "XMVSF Stage 12: Could not create renderer: %s\n", SDL_GetError());
		return 1;
	}
	SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_NONE);

	nVidImageDepth = 32;

	if (BurnDrvGetFlags() & BDF_16BIT_ONLY)
	{
		nVidImageDepth = 16;
#ifdef FBNEO_DEBUG
		printf("Forcing 16bit color\n");
#endif
	}
#ifdef FBNEO_DEBUG
	printf("bbp: %d\n", nVidImageDepth);
#endif
	if (bIntegerScale)
	{
		SDL_RenderSetIntegerScale(sdlRenderer, SDL_TRUE);
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, videofiltering);

	AdjustImageSize();

    // XM86.52: do not force 960x540 on game entry. Preserve the shell's
    // physical window dimensions; the native 384x224 canvas is unchanged.
    xm8652TrackWindowGeometry(sdlWindow,true,"game-enter");
    xm8652FocusWindowOnce(sdlWindow,"game-enter");
    {
        int ww=0,hh=0,lw=0,lh=0;
        SDL_GetWindowSize(sdlWindow,&ww,&hh);
        SDL_RenderGetLogicalSize(sdlRenderer,&lw,&lh);
        std::fprintf(stderr,"XM86.52 GAME_WINDOW_INHERITED size=%dx%d logical=%dx%d nativeCanvas=%dx%d viewport=%d,%d,%d,%d focus=%d\n",
            ww,hh,lw,lh,nVidImageWidth,nVidImageHeight,dstrect.x,dstrect.y,dstrect.w,dstrect.h,
            int((SDL_GetWindowFlags(sdlWindow)&SDL_WINDOW_INPUT_FOCUS)!=0));
    }

#ifdef FBNEO_DEBUG
	printf("setting logical size w: %d h: %d\n", display_w, display_h);
#endif
	
	inrenderer(sdlRenderer); //TODO: this is not supposed to be here
	prepare_inline_font();   // TODO: BAD
	incolor(0xFFF000, 0);

	if (nVidImageDepth == 32)
	{
		sdlTexture = SDL_CreateTexture(sdlRenderer,
			SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_STREAMING,
			nVidImageWidth, nVidImageHeight);
	}
	else
	{
		sdlTexture = SDL_CreateTexture(sdlRenderer,
			SDL_PIXELFORMAT_RGB565,
			SDL_TEXTUREACCESS_STREAMING,
			nVidImageWidth, nVidImageHeight);
	}
	if (!sdlTexture)
	{
		xmvsfStage12VideoLog("ERROR_CREATE_TEXTURE");
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create sdlTexture from surface: %s", SDL_GetError());
		return 3;
	}

	nVidImageBPP = (nVidImageDepth + 7) >> 3;
	nBurnBpp = nVidImageBPP;

	SetBurnHighCol(nVidImageDepth);

	nVidImagePitch = nVidImageWidth * nVidImageBPP;
	nBurnPitch = nVidImagePitch;

	nMemLen = nVidImageWidth * nVidImageHeight * nVidImageBPP;
#ifdef FBNEO_DEBUG
	printf("nVidImageWidth=%d nVidImageHeight=%d nVidImagePitch=%d\n", nVidImageWidth, nVidImageHeight, nVidImagePitch);
#endif
	VidMem = (unsigned char*)malloc(nMemLen);
	if (VidMem)
	{
		memset(VidMem, 0, nMemLen);
		pVidImage = VidMem;
#ifdef FBNEO_DEBUG
		printf("Malloc for video Ok %d\n", nMemLen);
#endif
		return 0;
	}
	else
	{
		pVidImage = NULL;
		return 1;
	}

#ifdef FBNEO_DEBUG
	printf("done vid init");
#endif
	return 0;
}

static int vidScale(RECT*, int, int)
{
	return 0;
}

// Run one frame and render the screen
static int Frame(bool bRedraw)                                          // bRedraw = 0
{
	if ((didReinitialise) && (pVidImage == NULL)) {
		didReinitialise = false;
		Exit();
		Init();
	}
	if (pVidImage == NULL)
	{
		return 1;
	}

	static bool xm8634First=true;
    if(xm8634First){std::fprintf(stderr,"XM86.34 SDL2 BEFORE FRAME CALLBACK\n");std::fflush(stderr);}
    VidFrameCallback(bRedraw);
    if(xm8634First){std::fprintf(stderr,"XM86.34 SDL2 AFTER FRAME CALLBACK\n");std::fflush(stderr);xm8634First=false;}

	return 0;
}

// Native 384x224 alpha overlay AFTER Wily/background/native rendering.
// Bar hue animation uses nFramesEmulated; the fighter-opacity mask uses
// GetCurrentFrame() from public burn.h, matching Cps2ObjDraw's source frame.
// This isolates stun HUD from art layers; gameplay RAM is never written.
extern "C" bool xm8746DizzyHudAllowed(){
    if(!XM87262Game::version4())return false;
    const char* option=std::getenv("XM8733_DIZZY_BAR");
    return !(option && option[0]=='0' && option[1]==0) &&
           !(xm14TrainingMode && !xm23State.dizzyAllowed);
}
// Exact right-side original PNG, decoded one time as native RGBA32 bytes.
// The left side mirrors those source bytes in XM8733::DrawOne. This stays
// in the current SDL HUD/UI pass; no compositor, UI priority or ROM change.
static const XM8733::BarArt* xm87207StunArt(){
    static bool attempted=false;
    static std::vector<unsigned char> pixels;
    static XM8733::BarArt art;
    if(attempted)return art.valid()?&art:nullptr;
    attempted=true;
    std::vector<std::string> candidates={
        "_art/stunbar_base.png","../_art/stunbar_base.png",
        "../../_art/stunbar_base.png"};
    char* base=SDL_GetBasePath();
    if(base){
        const std::string path(base);
        candidates.push_back(path+"../../_art/stunbar_base.png");
        candidates.push_back(path+"../_art/stunbar_base.png");
        candidates.push_back(path+"_art/stunbar_base.png");
        SDL_free(base);
    }
    for(const auto& file:candidates){
        SDL_Surface* original=IMG_Load(file.c_str());
        if(!original)continue;
        if(original->w!=61||original->h!=6){
            std::fprintf(stderr,
                "XM87.207 STUN_PNG_SIZE_MISMATCH %s actual=%dx%d expected=61x6\n",
                file.c_str(),original->w,original->h);
            SDL_FreeSurface(original);
            continue;
        }
        SDL_Surface* rgba=SDL_ConvertSurfaceFormat(original,SDL_PIXELFORMAT_RGBA32,0);
        SDL_FreeSurface(original);
        if(!rgba)continue;
        if(SDL_LockSurface(rgba)!=0){SDL_FreeSurface(rgba);continue;}
        const int w=rgba->w,h=rgba->h;
        pixels.resize((std::size_t)w*(std::size_t)h*4u);
        for(int row=0;row<h;row++)
            std::memcpy(pixels.data()+(std::size_t)row*(std::size_t)w*4u,
                        (const unsigned char*)rgba->pixels+
                        (std::size_t)row*(std::size_t)rgba->pitch,
                        (std::size_t)w*4u);
        SDL_UnlockSurface(rgba);
        SDL_FreeSurface(rgba);
        art.rgba=pixels.data();art.width=w;art.height=h;art.pitch=w*4;
        std::fprintf(stderr,
            "XM87.207 STUN_PNG_READY %s right=source left=mirror baseTopY=45\n",
            file.c_str());
        return &art;
    }
    std::fprintf(stderr,
        "XM87.210 STUN_PNG_MISSING _art/stunbar_base.png; bar omitted (no obsolete 76px fallback)\n");
    return nullptr;
}
// Stun HUD now uses the existing UI stack for both ordinary and Wily stages.
static void xm8733Overlay(SDL_Renderer* render,const SDL_Rect& view,bool nativeStage) {
    if(!XM87262Game::version4())return;
    const char* name=BurnDrvGetTextA(DRV_NAME);
    // Phase 06 is the combat scene's opening/round-intro, ahead of
    // status 08 ("FIGHT!"). Keep the bar hidden on character select
    // and the Versus screen (phases 02/04).
    if(!render || !name || std::strncmp(name,"xmvsf",5)!=0 ||
       (xm8658GameStatus!=6 && xm8658GameStatus!=8) ||
       !CpsRamFF)return;
    const char* option=std::getenv("XM8733_DIZZY_BAR");
    const bool dizzyBarEnabled=!(option && option[0]=='0' && option[1]==0) &&
        !(xm14TrainingMode && !xm23State.dizzyAllowed); // XM87.37 Training DIZZY OFF hides HUD
    if(!xm8733Texture){
        xm8733Texture=SDL_CreateTexture(render,SDL_PIXELFORMAT_RGBA32,
                                        SDL_TEXTUREACCESS_STREAMING,384,224);
        if(!xm8733Texture){std::fprintf(stderr,"XM87.33 HUD_TEXTURE_FAILED %s\n",SDL_GetError());return;}
        SDL_SetTextureBlendMode(xm8733Texture,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
        SDL_SetTextureScaleMode(xm8733Texture,SDL_ScaleModeNearest);
#endif
    }
    std::memset(xm8733Pixels,0,sizeof(xm8733Pixels));
    // XM87.41: source-qualified portrait/HP palettes are now applied to CPS-2
    // priority-7 objects BEFORE drawing. Never flood/scan the finished screen.
    // XM87.40: isolate backdrop blue and HP green from the actual native
    // sprite buffer. NO character-name palette modification or RGB-name test.
    // No unsafe Wily/FBNeo post-composition recoloring. The transparent
    // overlay below contains ONLY the independent dizzy meter; disabling
    // that meter does not disable the object-color stage in cps_obj.cpp.
    static bool xm8741Logged=false;
    if(!xm8741Logged){std::fprintf(stderr,
        "XM87.41 HUD_SOURCE_COLOR native_priority7=1 screen_flood=0 names=original\n");
        xm8741Logged=true;}
    const char* df=std::getenv("XM8649_WILY_RENDER");
    if(dizzyBarEnabled) XM8733::Draw(CpsRamFF,xm8733Pixels,384*4,4,(unsigned)nFramesEmulated,
                 xm8658GameStatus,df && df[0]=='1' && df[1]==0,
                 xm87207StunArt());
    // Wily composites the bar BEFORE the native fighter sprite texture.
    // Native stages have already composited those fighters in sdlTexture;
    // use the SAME current-frame fighter opacity to put the stun bar
    // beneath them, without changing CPS2 sprite priorities or moving
    // the stun overlay in front of the custom-stage fighters.
    if(nativeStage && dizzyBarEnabled){
        const XM8728::Mask& foreground=XM8728::State();
        if(foreground.started && foreground.ready &&
           foreground.frame==(unsigned)GetCurrentFrame() &&
           foreground.width==384 && foreground.height==224 &&
           foreground.body.size()==384u*224u){
            for(std::size_t pixel=0;pixel<foreground.body.size();++pixel)
                if(foreground.body[pixel])
                    xm8733Pixels[pixel*4u+3u]=0u;
        }
    }
    if(SDL_UpdateTexture(xm8733Texture,NULL,xm8733Pixels,384*4)==0)
        SDL_RenderCopy(render,xm8733Texture,NULL,&view);
}

// Stage 87.217: frame-data labels are a 384x224 NATIVE GAME PIXEL layer.
// All movement, font sampling, and six-frame horizontal compression happen
// in source pixels BEFORE the texture is scaled into the gameplay viewport.
// No window/logical-space SDL text is used. This game layer is shared by the
// regular CPS2 canvas and the native Wily stage compositor. It is rebuilt
// from transparent pixels each paint, so repeated paused paints cannot smear.
static void xm87217NativeFloat(SDL_Renderer* render,const SDL_Rect& gameView){
    if(!XM87262Game::version4())return;
    if(!render || !xm14TrainingMode || !xm23State.combat ||
       xm23State.menuOpen || nVidImageWidth!=384 || nVidImageHeight!=224 ||
       !XM87217::any(xm23State.frameData))return;
    const char* driver=BurnDrvGetTextA(DRV_NAME);
    if(!driver || std::strncmp(driver,"xmvsf",5)!=0)return;
    const Uint8* glyphPixels=nullptr;
    int glyphPitch=0;
    if(!XMGlyph::atlas().nativeSmallPixels(render,glyphPixels,glyphPitch))
        return; // no invented font when source asset is absent
    XM87217::Font font;
    if(!font.prepare(glyphPixels,128,48,glyphPitch))return;
    if(!xm87217FloatTexture){
        xm87217FloatTexture=SDL_CreateTexture(
            render,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,384,224);
        if(!xm87217FloatTexture){
            std::fprintf(stderr,"XM87.217 NATIVE_FLOAT_TEXTURE_FAILED %s\n",
                         SDL_GetError());
            return;
        }
        SDL_SetTextureBlendMode(xm87217FloatTexture,SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2,0,12)
        SDL_SetTextureScaleMode(xm87217FloatTexture,SDL_ScaleModeNearest);
#endif
        std::fprintf(stderr,
            "XM87.217 NATIVE_FLOAT_READY 384x224 source_pixels=1 exit_frames=6 font=sf2fontsmall.png\n");
    }
    std::memset(xm87217FloatRGBA.data(),0,xm87217FloatRGBA.size());
    XM87217::Image image;
    image.rgba=xm87217FloatRGBA.data();
    image.w=384;image.h=224;image.pitch=384*4;
    XM87217::draw(image,font,xm23State.frameData);
    if(SDL_UpdateTexture(xm87217FloatTexture,NULL,
                         xm87217FloatRGBA.data(),384*4)==0)
        SDL_RenderCopy(render,xm87217FloatTexture,NULL,&gameView);
}

// Paint the BlitFX surface onto the primary surface
static int Paint(int bValidate)
{
    static unsigned int stage12PaintCount = 0;
    if (stage12PaintCount < 3) xmvsfStage12VideoLog("VIDEO_PAINT_CALLED");
    ++stage12PaintCount;
    XM87206Crash::mark(400,nFramesEmulated);
    if(!sdlRenderer || !sdlTexture || !pVidImage){
        static bool reported=false;
        if(!reported){
            std::fprintf(stderr,"XM87.206 VIDEO_RESOURCE_NOT_READY renderer=%p texture=%p pixels=%p\n",
                (void*)sdlRenderer,(void*)sdlTexture,(void*)pVidImage);
            std::fflush(stderr);reported=true;
        }
        return 1; // no dereference of missing FIGHT frame resources
    }
    // XM87.132 CLEAN: no automatic framebuffer, GPU, or BMP capture in TEST/Stable.
    SDL_SetRenderDrawColor(sdlRenderer,8,16,36,255); /* XMVSF_STAGE32_NAVY */
	SDL_RenderClear(sdlRenderer);
    if (SDL_UpdateTexture(sdlTexture, NULL, pVidImage, nVidImagePitch) < 0) {
        if (stage12PaintCount <= 3) xmvsfStage12VideoLog("ERROR_UPDATE_TEXTURE");
    }
    XM87206Crash::mark(410,nFramesEmulated);
    xm8657EnsureCurrentSprites(); // stage registered immediately before VidPaint
    XM87206Crash::mark(411,nFramesEmulated);
    // XM87.128: 87.127 displaced the ENTIRE 720x540 display rectangle by
    // 60 logical units, clipping the bottom. Restore original draw coordinates.
    // Native 384x224 vs display 384x288 is a pixel-aspect conversion, NOT
    // 64 blank framebuffer rows. Read-only diagnostics identify any DF offset.
    SDL_Rect xm87127View=dstrect;
    XM87206Crash::mark(420,nFramesEmulated);
	const bool xm8649Painted=XMFD::paint(sdlRenderer,xm87127View) ||
	                         xm8649Paint(sdlRenderer,xm87127View);
    XM87206Crash::mark(421,nFramesEmulated);
	if(!xm8649Painted){
	if (nRotateGame)
	{
		SDL_RenderCopyEx(sdlRenderer, sdlTexture, NULL, &xm87127View, (bFlipped ? 90 : 270), NULL, SDL_FLIP_NONE);
	}
	else
	{
		if (bFlipped)
		{
			SDL_RenderCopyEx(sdlRenderer, sdlTexture, NULL, &xm87127View, 180, NULL, SDL_FLIP_NONE);
		}
		else
		{
			SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &xm87127View);
		}
	}

	} // XM86.49 native draw fallback

    // Keep native UI order: scene -> stun bar -> other HUD/notifications.
    // Wily's compositor draws the same stun UI itself, so skip a second pass.
    if(!xm8649Painted)xm8733Overlay(sdlRenderer,xm87127View,true);
    // The label layer belongs to the GAME viewport, not SDL logical UI.
    // Its source pixels are already rasterized at 384x224 and scale with
    // the original stage, including custom Wily and Double Feature.
    xm87217NativeFloat(sdlRenderer,xm87127View);
	RenderMessage();
	xm23Overlay(sdlRenderer,xm87127View,nVidImageWidth,nVidImageHeight); /* XMVSF_STAGE23_TRAINING_MENU */
	XM87206Crash::mark(430,nFramesEmulated);
    xm24Overlay(sdlRenderer,xm87127View); /* XMVSF_STAGE24_VERSUS */
    XM87206Crash::mark(431,nFramesEmulated);
    static bool xm87104Notice=false;
    if(!xm87104Notice){std::fprintf(stderr,"XM87.104 SELECTOR_OVERLAY_DISABLED native_ROM_only=1\n");xm87104Notice=true;}

	// XM86.57: CPS2 palette grading happened before sprite/scroll composition.
    // Applying Stage86.56 full-frame grayscale here would erase the colored
    // HUD / verified FX and Wily's hue drift. Menu routes retain the final pass.
    // Stage86.58: selection is drawn by the game engine rather than the host
    // menu.  Before stage loading, use the old final grayscale pass for its
    // portraits/characters; after commitment, preserve layer-specific HUD/FX.
    // Automatic selector frame recording retired; F9 input replay remains available.
    XM8773Present::State().Select(false); // XM87.104 overlay/calibration off
    if(xm8658GameStatus>=0 && xm8658GameStatus<2 && !xm8649Visual.sceneActive) {
        // XM87.70: retain XM86.65's selection-luma diagnostic while
        // splitting grade and present, so the owner overlays survive DF.
        // This is the same condition and message as xm8662SelectionPresent().
        static bool xm8770SelectionLumaReported=false;
        if(!xm8770SelectionLumaReported && xm8655DoubleFeature()){
            std::fprintf(stderr,"XM86.65 SELECTION_LUMA no_full_screen_ink96=true\n");
            xm8770SelectionLumaReported=true;
        }
        // DF grading FIRST, screen-position evidence SECOND,
        // final-color selection owner indicators LAST.
        xm8656FinalPass(sdlRenderer,false);
        // XM87.71: screen recording moved to native CPU framebuffer path.
        // Do not perform GPU readback or file IO in presentation.
        /* XM87.104 removed colored selector overlay */

xm87132ReplayNotice(sdlRenderer);
        xm87139ProbeNotice(sdlRenderer);
        xm87140ChatOverlay(sdlRenderer);
        SDL_RenderPresent(sdlRenderer);
    } else { /* XM87.104 removed colored selector overlay */

xm87132ReplayNotice(sdlRenderer);
        xm87139ProbeNotice(sdlRenderer);
        xm87140ChatOverlay(sdlRenderer);
        SDL_RenderPresent(sdlRenderer);
    } /* XM86.58 native layer-aware gameplay */
    XM87206Crash::mark(450,nFramesEmulated);
    xm8652TrackWindowGeometry(sdlWindow,false,"game-frame");
    if (stage12PaintCount <= 3) xmvsfStage12VideoLog("VIDEO_PRESENT_CALLED");

	return 0;
}

extern "C" int xm8774SelectorCalibrationConsumesArrows(){
    return XM8773Present::State().ConsumesArrows()?1:0;
}

static int GetSettings(InterfaceInfo* pInfo)
{
	return 0;
}

// The Video Output plugin:
struct VidOut VidOutSDL2 = { Init, Exit, Frame, Paint, vidScale, GetSettings, _T("SDL2 video output") };
