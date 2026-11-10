/*----------------
 * Stuff to finish:
 *
 * It wouldn't be a stretch of the imagination to think the whole of the sdl 'port' needs a redo but here are the main things wrong with this version:
 *
 *
 * The audio output code code could maybe do with some work, but it's getting there...
 * There are lots of problems with the opengl renderer, but you should just use the SDL2 build anyway
 *
 * TODO for SDL2:
 * Add menu for options e.g., saves, IPS patches
 * Maybe a better font output setup with some sort of scaling, maybe add sdl1 support?
 * figure out what is going on with the sdl sound output, something breaks after a few frames
 * ------------------*/

#include "burner.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xmvsf_stage77_selection.h" // XM86.31 safe-host menu-only stage selection
int xm77StageChoice=0, xm77StageResolved=0;
#include "xmvsf_stage87_164_host_replay.h" // shared app/menu/session replay journal
#include "xmvsf_stage14_boot.h" // XMVSF_STAGE14_TRAINING_BOOT
#include "xmvsf_stage87_206_crash_report.h" // TEST 87.206 SEH minidump
int xm14TrainingMode = 0;
int xm87102DamageMode = 0; // XM87.102 Training: 0 regenerate, 1 native KO
int xm24VersusMode = 0; /* XMVSF_STAGE24_VERSUS */
int xm24ResultInputCapture = 0; /* XMVSF_STAGE24_3_LIVE_RESULTS */
bool xm258ArcadeMenuOpen=false;
bool xm258ArcadeReturnRequested=false; /* XMVSF_STAGE25_8_ARCADE_MENU */
// Stage 26: persistent SDL application shell; not a second Windows process.
bool xm26PersistentShell=false;
bool xm26ReturnToBoot=false;
bool xm31ExternalPane=false; /* XMVSF_STAGE31_EXTERNAL_PANE */
int xm14InputTarget = 0;
#include "xmvsf_stage15_machine.h" // XMVSF_STAGE15_MACHINE
XM15Machine xm15={};
#include "xmvsf_stage17_auto.h" // XMVSF_STAGE17_AUTO
XM17Auto xm17={};
#include "xmvsf_stage18_position.h" /* XMVSF_STAGE18_POSITION */
XM18Position xm18Position={};
#include "xmvsf_stage23_ui.h" /* XMVSF_STAGE23_TRAINING_MENU */
XM23State xm23State;
#include "xmvsf_stage24_versus.h" /* XMVSF_STAGE24_VERSUS */
XM24Session xm24Session;
XM25State xm25State; /* XMVSF_STAGE25_SETS */
XM248RuntimeOwner xm248RuntimeOwner[2]; // TEST 87.248: persistent P1/P2 live SDL identities
XM25InputHandoff xm25InputHandoff; /* XMVSF_STAGE25_4_INPUT_HANDOFF */ // XMVSF_STAGE13_CONTROLLERS // XMVSF_STAGE12_PERSISTENT_RENDERER // XMVSF_STAGE11_SINGLE_WINDOW // XMVSF_STAGE10_BOOT_PATCHED

INT32 display_set_controls();

int  nAppVirtualFps = 0;         // App fps * 100
bool bRunPause = 0;
bool bAppFullscreen = 0;
bool bAlwaysProcessKeyboardInput = 0;
int usemenu = 0, usejoy = 0, vsync = 1, dat = 0;
bool bSaveconfig = 1;
bool bIntegerScale = false;
int nWindowScale = 2;			// Default to 2 for compatibility with previous hard coded value.
bool bAlwaysMenu = false;
int nGameSelect = 0;
int nFilterSelect = HARDWARE_PUBLIC_MASK;
bool bShowAvailableOnly = true;
bool bShowClones = true;
int gameSelectedFromFilter = -1;
TCHAR szAppBurnVer[16];
char videofiltering[3];
bool gamefound = 0;
const char* romname = NULL;

extern void InitSupportPaths();

#ifdef BUILD_SDL2
SDL_Window* sdlWindow = NULL;
#endif

#define set_commandline_option_not_config(i,v) i = v;
#define set_commandline_option(i,v) i = v; bSaveconfig = 0;
#define set_commandline_option_string(i, v, length) snprintf(i, length, v); bSaveconfig = 0;

int parseSwitches(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		if (*argv[i] != '-' && !gamefound)
		{
			romname = argv[i];
			gamefound = 1;
		}
		if (strcmp(argv[i], "-joy") == 0)
		{
			set_commandline_option_not_config(usejoy, 1);
		}
		else if (strcmp(argv[i], "-menu") == 0)
		{
			set_commandline_option_not_config(usemenu, 1);
		}
		else if (strcmp(argv[i], "-novsync") == 0)
		{
			set_commandline_option(vsync, 0);
		}
		else if (strcmp(argv[i], "-integerscale") == 0)
		{
			set_commandline_option(bIntegerScale, 1);
		}
		else if (strcmp(argv[i], "-windowscale") == 0)
		{
			int num;

			if (++i >= argc)
			{
				return 1;
			}

			num = atoi(argv[i]);
			if (num < 1 || 20 < num)	// Check it's not too small or ridiculously too big.
			{
				return 1;
			}
			set_commandline_option(nWindowScale, num);
		}
		else if (strcmp(argv[i], "-dat") == 0)
		{
			set_commandline_option_not_config(dat, 1);
		}
		else if (strcmp(argv[i], "-fullscreen") == 0)
		{
			set_commandline_option(bAppFullscreen, 1);
		}
		else if (strcmp(argv[i], "-nearest") == 0)
		{
			set_commandline_option_string(videofiltering, "0", 3);
		}
		else if (strcmp(argv[i], "-linear") == 0)
		{
			set_commandline_option_string(videofiltering, "1", 3);
		}
		else if (strcmp(argv[i], "-best") == 0)
		{
			set_commandline_option_string(videofiltering, "2", 3);
		}
		else if (strcmp(argv[i], "-autosave") == 0)
		{
			bDrvSaveAll = 1;
		}
		else if (strcmp(argv[i], "-cd") == 0)
		{
			i++;
			_tcscpy(CDEmuImage, argv[i]);
		}
	}
	return 0;
}

void generateDats()
{
	char filename[1024] = { 0 };

#if defined(BUILD_SDL2)
	printf("Creating fbneo dats in %s\n", szAppDatListsPath);
#if defined(ALT_DAT)
	_stprintf(filename, _T("%sFBNeo_-_ALL_SYSTEMS.dat"), szAppDatListsPath);
	create_datfile(filename, -1);
#endif
	_stprintf(filename, _T("%sFBNeo_-_Arcade.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_ARCADE_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_MegaDrive.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_MEGADRIVE_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_PC_ENGINE.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_PCENGINE_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_TurboGrafx-16.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_TG16_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_PC_Engine_SuperGrafx.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_SGX_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Sega_SG-1000.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_SG1000_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_ColecoVision.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_COLECO_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Sega_Master_System.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_MASTERSYSTEM_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Sega_Game_Gear.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_GAMEGEAR_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_MSX.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_MSX_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Sinclair_ZX_Spectrum.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_SPECTRUM_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Neogeo.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_NEOGEO_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Nintendo_Entertainment_System.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_NES_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Nintendo_Famicom_Disk_System.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_FDS_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Super_Nintendo_Entertainment_System.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_SNES_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Neo_Geo_Pocket.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_NGP_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Fairchild_Channel_F.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_CHANNELF_ONLY);

	_stprintf(filename, _T("%sFBNeo_-_Bally_Astrocade.dat"), szAppDatListsPath);
	create_datfile(filename, DAT_ASTROHOME_ONLY);

#else
	printf("Creating fbneo dats\n");

#if defined(ALT_DAT)
	sprintf(filename, "FBNeo_-_ALL_SYSTEMS.dat");
	create_datfile(filename, -1);
#endif
	sprintf(filename, "FBNeo_-_Arcade.dat");
	create_datfile(filename, DAT_ARCADE_ONLY);

	sprintf(filename, "FBNeo_-_MegaDrive.dat");
	create_datfile(filename, DAT_MEGADRIVE_ONLY);

	sprintf(filename, "FBNeo_-_PC_ENGINE.dat");
	create_datfile(filename, DAT_PCENGINE_ONLY);

	sprintf(filename, "FBNeo_-_TurboGrafx-16.dat");
	create_datfile(filename, DAT_TG16_ONLY);

	sprintf(filename, "FBNeo_-_PC_Engine_SuperGrafx.dat");
	create_datfile(filename, DAT_SGX_ONLY);

	sprintf(filename, "FBNeo_-_Sega_SG-1000.dat");
	create_datfile(filename, DAT_SG1000_ONLY);

	sprintf(filename, "FBNeo_-_ColecoVision.dat");
	create_datfile(filename, DAT_COLECO_ONLY);

	sprintf(filename, "FBNeo_-_Sega_Master_System.dat");
	create_datfile(filename, DAT_MASTERSYSTEM_ONLY);

	sprintf(filename, "FBNeo_-_Sega_Game_Gear.dat");
	create_datfile(filename, DAT_GAMEGEAR_ONLY);

	sprintf(filename, "FBNeo_-_MSX.dat");
	create_datfile(filename, DAT_MSX_ONLY);

	sprintf(filename, "FBNeo_-_Sinclair_ZX_Spectrum.dat");
	create_datfile(filename, DAT_SPECTRUM_ONLY);

	sprintf(filename, "FBNeo_-_Neogeo.dat");
	create_datfile(filename, DAT_NEOGEO_ONLY);

	sprintf(filename, "FBNeo_-_Nintendo_Entertainment_System.dat");
	create_datfile(filename, DAT_NES_ONLY);

	sprintf(filename, "FBNeo_-_Nintendo_Famicom_Disk_System.dat");
	create_datfile(filename, DAT_FDS_ONLY);

	sprintf(filename, "FBNeo_-_Super_Nintendo_Entertainment_System.dat");
	create_datfile(filename, DAT_SNES_ONLY);

	sprintf(filename, "FBNeo_-_Neo_Geo_Pocket.dat");
	create_datfile(filename, DAT_NGP_ONLY);

	sprintf(filename, "FBNeo_-_Fairchild_Channel_F.dat");
	create_datfile(filename, DAT_CHANNELF_ONLY);

	sprintf(filename, "FBNeo_-_Bally_Astrocade.dat");
	create_datfile(filename, DAT_ASTROHOME_ONLY);
#endif
}


// Stage87.203: remember a real ROM initialization failure in donor mode.
// Prior host code discarded DrvInit's nonzero result, then main returned 0.
// Only this opt-in donor path changes exit status; legacy XMVSF mode untouched.
static int xm87203DonorRomLoadFailure=0;
void DoGame(int gameToRun)
{
    // Restore the user's Training MENU choices once for each Training
    // session, including a fresh process or a return from the boot menu.
    // This never loads a fight state and never affects Arcade or Versus.
    XM87206Crash::mark(150);
    // The boot selector owns the rule choice. Reassert the selected rule
    // BEFORE any native 68000 opcode hook or ROM/game driver initializes.
    // Playback bypasses this selector and manages its own recorded mode.
    if(xm26PersistentShell)XM8755Mode::Apply();
    XM87206Crash::rearm();
    std::fprintf(stderr,"XM87.222 GAME_LAUNCH mode=%s persistent=%u\n",
        XM8755Mode::Label(),unsigned(xm26PersistentShell));
    std::fflush(stderr);
    if(xm14TrainingMode)xm257LoadTrainingOptions();
    std::fprintf(stderr,"XM86.32 HOST ENTER DoGame id=%d modeTraining=%d modeVersus=%d stageChoice=%d resolved=%d\n",gameToRun,xm14TrainingMode,xm24VersusMode,xm77StageChoice,xm77StageResolved);
    std::fflush(stderr);
    const int driverResult=DrvInit(gameToRun, 0);
    XM87206Crash::mark(151);
    const char* donorCapture=std::getenv("XM87203_STAGE_CAPTURE_DIR");
    const bool donorRequested=romname && std::strncmp(romname,"mvsc",4)==0;
    if(driverResult!=0 && (donorRequested || (donorCapture && *donorCapture))){
        xm87203DonorRomLoadFailure=1;
        std::fprintf(stderr,
            "XM87.203 ERROR: DONOR_ROM_LOAD_FAILED rc=%d. No CPS2 frames, F9 movie or stage capture possible.\n",
            driverResult);
        std::fflush(stderr);
    }
    std::fprintf(stderr,"XM86.32 HOST DrvInit return=%d\n",driverResult);
    std::fflush(stderr);
	if (!driverResult)
	{
        std::fprintf(stderr,"XM86.32 HOST MediaInit begin\n");std::fflush(stderr);
		MediaInit();
        // TEST 87.222: Classic/Tournament returns re-enter the SDL/native
        // driver in the same process; preserve crash reporting on re-entry.
        XM87206Crash::rearm();
        XM87206Crash::mark(160);
        if(xm14TrainingMode){
            // The Frame Data toggle is persisted, but an SDL session's
            // external-pane geometry is not: apply the saved toggle to
            // the newly initialized renderer/window now.
            xm31SetPaneWindow(sdlWindow,xm23State.frameData.enabled);
            AdjustImageSize();
        }
        std::fprintf(stderr,"XM86.32 HOST MediaInit complete; controls begin\n");std::fflush(stderr);
		display_set_controls();
        std::fprintf(stderr,"XM86.32 HOST controls complete; RunMessageLoop begin\n");std::fflush(stderr);
        XM87206Crash::mark(190);
		RunMessageLoop();
        XM87206Crash::mark(191);
        XM8764::gameEnd(); // checkpoint/close recorded CPS2 session before DrvExit
        std::fprintf(stderr,"XM86.32 HOST RunMessageLoop returned\n");std::fflush(stderr);
	}
	else
	{
		printf("There was an error loading your selected game.\n");
	}

	if (bSaveconfig)
	{
		ConfigAppSave();
	}
	DrvExit();
	MediaExit();
}

void bye(void)
{
#ifdef FBNEO_DEBUG
	printf("Doing exit cleanup\n");
#endif

	DrvExit();
	MediaExit();
	BurnLibExit();
	SDL_Quit();
}

TCHAR* AdaptiveEncodingReads(const TCHAR* pszFileName)
{
	return NULL;
}

static int __cdecl AppDebugPrintf(int nStatus, TCHAR* pszFormat, ...)
{

	va_list args;
	va_start(args, pszFormat);
	vfprintf(stderr, pszFormat, args);
	va_end(args);

	return 0;
}

int main(int argc, char* argv[])
{
    XM87206Crash::install();
    XM87206Crash::mark(100);
	UINT32      i = 0;
	int fail = 0;
	atexit(bye);
	// TODO: figure out if we can use hardware Gamma until then, force software gamma
	bVidUseHardwareGamma = 0;
	bHardwareGammaOnly = 0;
	//

	// Make version string
	if (nBurnVer & 0xFF)
	{
		// private version (alpha)
		_stprintf(szAppBurnVer, _T("%x.%x.%x.%02x"), nBurnVer >> 20, (nBurnVer >> 16) & 0x0F, (nBurnVer >> 8) & 0xFF, nBurnVer & 0xFF);
	}
	else
	{
		// public version
		_stprintf(szAppBurnVer, _T("%x.%x.%x"), nBurnVer >> 20, (nBurnVer >> 16) & 0x0F, (nBurnVer >> 8) & 0xFF);
	}
	printf("FBNeo v%s\n", szAppBurnVer);


	// set default videofiltering
	snprintf(videofiltering, 3, "0");

	bool switchesOK = parseSwitches(argc, argv) == 0;
    // Fail closed on a bad host-aware journal; do NOT silently fall back
    // to the interactive boot menu when the requested movie is invalid.
    if(std::getenv("XM8764_REPLAY_SESSION")){
        XM8764::initialize();
        if(XM8764::context().mode==XM8764::FAILED)return 1;
    }
    // XM87.163: the dedicated native F9 replay runner explicitly supplies
    // the xmvsfu driver, bypassing the interactive boot mode selector.
    // The original movie was recorded in Training; reconstruct the same
    // host-mode state BEFORE DrvInit / the first replay BurnStateLoad.
    // Ordinary Arcade/Training/Versus launches are completely unchanged.
    const char* replayMode=std::getenv("XM8761_REPLAY_MODE");
    const char* replayHostMode=std::getenv("XM8761_REPLAY_HOST_MODE");
    if(switchesOK && replayMode && std::strcmp(replayMode,"play")==0 &&
       romname && std::strcmp(romname,"xmvsfu")==0) {
        xm14TrainingMode= !replayHostMode ||
            std::strcmp(replayHostMode,"training")==0 ? 1 : 0;
        xm24VersusMode= replayHostMode &&
            std::strcmp(replayHostMode,"versus")==0 ? 1 : 0;
        xm26PersistentShell=false;
        usejoy=1;
        std::fprintf(stderr,
            "XM87.163 REPLAY_DIRECT_BOOT rom=%s training=%d versus=%d (boot menu bypassed)\n",
            romname,xm14TrainingMode,xm24VersusMode);
        std::fflush(stderr);
    }
    // Stage 26: initialize a persistent single-window application shell.
    if (switchesOK && romname == NULL && !usemenu && !dat) {
        xm26PersistentShell=true;
        // Always enter the REAL boot selector, including v2 playback.
        // It initializes the persistent SDL window, then selects the
        // recorded host mode only AFTER presenting the first menu frame.
        const int selectedMode=xmvsfBootSelector();
        if(XM8764::isRecording())
            XM8764::hostEvent("BOOT_MODE_SELECTED",(unsigned)(selectedMode+1));
        if(selectedMode==0)xm77StageResolved=0;
        xm14TrainingMode = (selectedMode == 1);
        xm24VersusMode = (selectedMode == 2); /* XMVSF_STAGE24_VERSUS */
        xm15Reset(); // XMVSF_STAGE15_MACHINE original mapping / Training state
        if(selectedMode<0){
            // F9 may still be armed when the user closes the custom boot
            // menu. Seal all committed segments before normal SDL teardown.
            if(XM8764::isRecording())
                XM8764::finalizeRecording("APPLICATION_EXIT");
            return XM8764::context().failed?1:0;
        }
        romname = "xmvsfu";
        gamefound = 1;
        usejoy = 1;
    }


	// Do these bits before override via ConfigAppLoad
	bCheatsAllowed = 1;
	nAudDSPModule[0] = 0;
	EnableHiscores = 1;

	if (!switchesOK || ((romname == NULL) && !usemenu && !bAlwaysMenu && !dat))
	{
		printf("Usage: %s [-cd] [-joy] [-menu] [-novsync] [-integerscale] [-windowscale <num>] [-fullscreen] [-dat] [-autosave] [-nearest] [-linear] [-best] <romname>\n", argv[0]);
		printf("Note the -menu switch does not require a romname\n");
		printf("e.g.: %s mslug\n", argv[0]);
		printf("e.g.: %s -windowscale 1 asteroid\n", argv[0]);
		printf("e.g.: %s -menu -joy\n", argv[0]);
		printf("For NeoCD games:\n");
		printf("%s neocdz -cd path/to/ccd/filename.cue (or .ccd)\n", argv[0]);
		printf("Usage is restricted by the license at https://raw.githubusercontent.com/finalburnneo/FBNeo/master/src/license.txt\n");
		return 0;
	}

#ifdef BUILD_SDL
	SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

	SDL_WM_SetCaption("FinalBurn Neo", "FinalBurn Neo");
#endif

#ifdef BUILD_SDL2
#ifdef SDL_WINDOWS
	char *base_path = SDL_GetBasePath();
#define DIRCNT 9
	// Make sure there are roms and cfg subdirectories
	TCHAR szDirs[DIRCNT][MAX_PATH] = {
		{_T("config")},
		{_T("config/games")},
		{_T("config/ips")},
		{_T("config/localisation")},
		{_T("config/presets")},
		{_T("recordings")},
		{_T("roms")},
		{_T("savestates")},
		{_T("screenshots")},
	};
	TCHAR currentPath[MAX_PATH];
	for(int x = 0; x < DIRCNT; x++) {
		snprintf(currentPath, MAX_PATH, "%s%s", base_path, szDirs[x]);
		CreateDirectory(currentPath, NULL);
	}
#undef DIRCNT

	SDL_setenv("SDL_AUDIODRIVER", "directsound", true);        // fix audio for windows
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 0;
	}
#endif

	//SDL_ShowCursor(SDL_DISABLE);

#if defined(BUILD_SDL2) && !defined(SDL_WINDOWS)
    // Load mapping from file if it exists
	char gamecontrollerdbfile[MAX_PATH] = _T("");
	TCHAR *szSDLconfigPath = NULL;
	szSDLconfigPath = SDL_GetPrefPath("fbneo", "config");
	_stprintf(gamecontrollerdbfile, _T("%sgamecontrollerdb.txt"), szSDLconfigPath);
	SDL_free(szSDLconfigPath);
	if (SDL_GameControllerAddMappingsFromFile(gamecontrollerdbfile) > 0) printf("Game controller mappings loaded from: %s\n", gamecontrollerdbfile);
#endif

	// create a default ini if one is not valid
	fail = ConfigAppLoad();
// Stage 12: keep Stage 10 video backend selection behavior.
	if (fail && bSaveconfig)
	{
		#if defined(BUILD_SDL2) && !defined(SDL_WINDOWS)
		InitSupportPaths();
		#endif
		ConfigAppSave();		// for SDL2 this will also create folders in ~/.local/share/fbneo/
	}
	ComputeGammaLUT();
#if defined(BUILD_SDL2) && !defined(SDL_WINDOWS)
	bprintf = AppDebugPrintf;
#endif
	BurnLibInit();

	// Search for a game now, for use in the menu and loading a games
	if (romname != NULL)
	{
		for (i = 0; i < nBurnDrvCount; i++)
		{
			nBurnDrvActive = i;
			if (strcmp(BurnDrvGetTextA(DRV_NAME), romname) == 0)
			{
				break;
			}
		}
	}

	if (dat)
	{
		generateDats();
	}
#ifdef BUILD_SDL2
	else if (usemenu || bAlwaysMenu)
	{
		bool quit = 0;

		while (!quit)
		{
			gui_init();
			int selectedOk = gui_process();
			gui_exit();

			switch (selectedOk)
			{
			case -1:			
				quit = 1;
				return 0;

			default:
				DoGame(selectedOk);
				break;
			}
		}
	}
#endif
	else
	{
		if (i == nBurnDrvCount)
		{
			printf("%s is not supported by FinalBurn Neo.\n", romname);
			return 1;
		}

        if (!xm26PersistentShell) {
            DoGame(i);
        } else {
            // Game sessions return here on MAIN MENU rather than exiting the EXE.
            // BurnLibInit, configuration and the SDL window survive each session.
            for (;;) {
                xm26ReturnToBoot=false;
                DoGame(i);  // DrvExit and MediaExit complete before showing boot UI.
                std::fprintf(stderr,"XM27: DoGame returned; returnToBoot=%d, window=%p\n",
                             (int)xm26ReturnToBoot,(void*)sdlWindow);
                std::fflush(stderr);
                if (!xm26ReturnToBoot) break;
                // Keep the window intact; the boot selector owns the renderer
                // after the game video plugin has released its own renderer.
                xm26ReturnToBoot=false;
                xm258ArcadeMenuOpen=false;
                xm258ArcadeReturnRequested=false;
                xm31SetPaneWindow(sdlWindow,false); // Restore standard window width for boot.
                // Reset only frame measurements when leaving Training.
                // The ON/OFF preference is saved separately and reloaded
                // when the user next chooses Training.
                xm23State.frameData.clear(false);
                xm14TrainingMode=0;
                xm24VersusMode=0;
                xm24ResultInputCapture=0;
                xm14InputTarget=0;
                xm15Reset();
                // The menu uses the SAME SDL_Window, avoiding Windows focus jumps.
                const int selectedMode=xmvsfBootSelector(true);
                if(XM8764::isRecording())
                    XM8764::hostEvent("BOOT_MODE_SELECTED",(unsigned)(selectedMode+1));
                if (selectedMode<0) break;
                if(selectedMode==0)xm77StageResolved=0;
                xm14TrainingMode=(selectedMode==1);
                xm24VersusMode=(selectedMode==2);
                xm15Reset();
                xm25BeginInputHandoff();
                std::fprintf(stderr,"XM27: boot handed mode=%d to game session\n",selectedMode);
                std::fflush(stderr);
            }
        }
    }
    if(XM8764::isRecording())
        XM8764::finalizeRecording("APPLICATION_EXIT");
    return (xm87203DonorRomLoadFailure || XM8764::context().failed)?1:0;
}

/* const */ TCHAR* ANSIToTCHAR(const char* pszInString, TCHAR* pszOutString, int nOutSize)
{
#if defined (UNICODE)
	static TCHAR szStringBuffer[1024];

	TCHAR* pszBuffer = pszOutString ? pszOutString : szStringBuffer;
	int    nBufferSize = pszOutString ? nOutSize * 2 : sizeof(szStringBuffer);

	if (MultiByteToWideChar(CP_ACP, 0, pszInString, -1, pszBuffer, nBufferSize))
	{
		return pszBuffer;
	}

	return NULL;
#else
	if (pszOutString)
	{
		_tcscpy(pszOutString, pszInString);
		return pszOutString;
	}

	return (TCHAR*)pszInString;
#endif
}

/* const */ char* TCHARToANSI(const TCHAR* pszInString, char* pszOutString, int nOutSize)
{
#if defined (UNICODE)
	static char szStringBuffer[1024];
	memset(szStringBuffer, 0, sizeof(szStringBuffer));

	char* pszBuffer = pszOutString ? pszOutString : szStringBuffer;
	int   nBufferSize = pszOutString ? nOutSize * 2 : sizeof(szStringBuffer);

	if (WideCharToMultiByte(CP_ACP, 0, pszInString, -1, pszBuffer, nBufferSize, NULL, NULL))
	{
		return pszBuffer;
	}

	return NULL;
#else
	if (pszOutString)
	{
		strcpy(pszOutString, pszInString);
		return pszOutString;
	}

	return (char*)pszInString;
#endif
}

bool AppProcessKeyboardInput()
{
	return true;
}
