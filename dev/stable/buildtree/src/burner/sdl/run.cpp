// XM86.41: compile-time endianness consistency assertion for the native host.
#ifndef LSB_FIRST
#error "XM86.41 native host requires global LSB_FIRST=1 across every object"
#endif
// Run module
#include "burner.h"
#include "xmvsf_stage87_164_host_replay.h" // must precede native reload and Training menu handlers
#include "xmvsf_stage87_55_rule_mode.h" // restore recorded Tournament/Classic host selection
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif
#include "xmvsf_stage14_training.h"
#ifdef BUILD_SDL2
#include "xmvsf_stage86_56_final.h" /* XM86.56 all presentation paths */
#endif
#include "xmvsf_stage15_machine.h"
#include "xmvsf_stage17_auto.h"
#include "xmvsf_stage18_position.h" /* XMVSF_STAGE18_POSITION */
#include "xmvsf_stage20_api.h" /* XMVSF_STAGE20_WASAPI */
#include "xmvsf_native_host.h" /* XMVSF_STAGE21_BOUNDARY */
#include "xmvsf_stage22_select_audio.h" /* XMVSF_STAGE22_SELECTION_SFX */
#include "xmvsf_stage23_engine.h" /* XMVSF_STAGE23_TRAINING_MENU */
#include "xmvsf_menu_glyph_atlas.h" /* release sprite UI before renderer shutdown */
#include "xmvsf_stage87_118_meter_audio.h" /* XM87.118 read-only stock edge (restored in 87.132 build repair) */
#include "xmvsf_stage24_engine.h" /* XMVSF_STAGE24_VERSUS */
#include "xmvsf_stage86_49_camera.h" // XM86.49 opt-in read-only native camera
 #include "xmvsf_stage87_61_replay.h" // XM87.61 opt-in state-anchored input movie
#include "xmvsf_stage87_162_replay_sync.h" // F9 movie index read-only native ROM trace correlation
#include "xmvsf_stage87_139_probe.h" // XM87.139 F10 native/input diagnostic toggle
#include "xmvsf_stage87_140_super_chat.h" // XM87.140 Shift+G chat history toggle
#include "xmvsf_stage87_206_crash_report.h" // uncaught FIGHT crash phase breadcrumbs
#include "xmvsf_stage77_native_stage5.h" // XM86.31 postframe guarded native stage request
#include "xmvsf_stage29_display.h" /* XMVSF_STAGE29_SCALABLE_DISPLAY */
#include "xmvsf_stage24_3_menu_poll.h" /* XMVSF_STAGE24_3_LIVE_RESULTS */
#include "xm20_vs_transition.h" /* XMVSF_STAGE20_1_VERSUS */ // XMVSF_STAGE17_AUTO
#include "xmvsf_stage16_snapshot.h" // XMVSF_STAGE16_SNAPSHOT
#include "xmvsf_stage87_105_training_reset.h" // XM87.105 KO landing snapshot reset
#include "xmvsf_stage87_110_restart.h" // XM87.110 training triple-tap restart
#include "xmvsf_stage87_110_reload.h" // XM87.110 snapshot load + placement
// XM87.105 removed supplemental flying-screen WAV; native ROM sound retained // XMVSF_STAGE15_MACHINE // XMVSF_STAGE14_TRAINING_CHEATS
#ifdef BUILD_SDL2
#include "sdl2_gui_common.h"
#endif
#include <sys/time.h>

static bool xm87132CleanStable(){
    return true; // No runtime debug instrumentation in either build mode.
}
XM8649VisualState xm8649Visual{}; // XM86.49 shared read-only video data
// XM86.58: phase observer for game screen output, never an emulation mutation.
int xm8658GameStatus=-1;
static void xm8658ObserveGamePhase(bool ready){
    xm8658GameStatus=-1;
    if(!ready || bBurnRunAheadFrame)return;
    const char* name=BurnDrvGetTextA(DRV_NAME);
    if(!name || std::strncmp(name,"xmvsf",5)!=0)return;
    cpu_core_config* c=GetCpuCoreConfig(0);
    if(!c || !c->open || !c->close || !c->read)return;
    c->open(0); xm8658GameStatus=int(c->read(0xFF5000u)&255u); c->close();
    static int previous=-2;
    if(xm8658GameStatus!=previous){
        if(xm8658GameStatus==8){
            std::fprintf(stderr,"XM87.206 COMBAT_STATUS_08 frame=%d prior=%d\n",
                         nFramesEmulated,previous);
            std::fflush(stderr);
        }
        if(!xm87132CleanStable()) std::fprintf(stderr,"XM86.58 GAME_PHASE old=%02X new=%02X selectionGray=%d\n",
            previous<0?255u:(unsigned)previous,(unsigned)xm8658GameStatus,
            (int)(xm8658GameStatus<2));
        previous=xm8658GameStatus;
    }
}

static unsigned int nDoFPS = 0;
bool bAltPause = 0;

int bAlwaysDrawFrames = 0;

int counter;                                // General purpose variable used when debugging

static unsigned int nNormalLast = 0;        // Last value of GetTime()
static int          nNormalFrac = 0;        // Extra fraction we did

static bool bAppDoStep = 0;
bool        bAppDoFast = 0;
bool        bAppShowFPS = 0;
static int  nFastSpeed = 6;

// SlowMo T.A. feature
int nSlowMo = 0;
static int flippy = 0; // free running RunFrame() counter

UINT32 messageFrames = 0;
char lastMessage[MESSAGE_MAX_LENGTH];

#ifdef BUILD_SDL2
static Uint32 starting_stick;
/// Ingame gui
extern SDL_Renderer* sdlRenderer;
extern void ingame_gui_start(SDL_Renderer* renderer);
/// Save States
static char Windowtitle[512];

extern void AdjustImageSize();		// vid_sdl2.cpp
#endif

int bDrvSaveAll = 0;

// The automatic save
int StatedAuto(int bSave)
{
	static TCHAR szName[MAX_PATH] = _T("");
	int nRet;

#if defined(BUILD_SDL2) && !defined(SDL_WINDOWS)
	_stprintf(szName, _T("%s%s.fs"), szAppEEPROMPath, BurnDrvGetText(DRV_NAME));
#else
	_stprintf(szName, _T("config/games/%s.fs"), BurnDrvGetText(DRV_NAME));
#endif

	if (bSave == 0)
	{
		printf("loading state %i %s\n", bDrvSaveAll, szName);
		nRet = BurnStateLoad(szName, bDrvSaveAll, NULL);		// Load ram
		if (nRet && bDrvSaveAll)
		{
			nRet = BurnStateLoad(szName, 0, NULL);				// Couldn't get all - okay just try the nvram
		}
	}
	else
	{
		printf("saving state %i %s\n", bDrvSaveAll, szName);
		nRet = BurnStateSave(szName, bDrvSaveAll);				// Save ram
	}

	return nRet;
}


/// End Save States

char fpsstring[20];

static time_t fpstimer;
static unsigned int nPreviousFrames;

static void DisplayFPSInit()
{
	nDoFPS = 0;
	fpstimer = 0;
	nPreviousFrames = nFramesRendered;
}

static void DisplayFPS()
{
	time_t temptime = clock();
	double fps = (double)(nFramesRendered - nPreviousFrames) * CLOCKS_PER_SEC / (temptime - fpstimer);
	if (bAppDoFast) {
		fps *= nFastSpeed + 1;
	}
	if (fpstimer && temptime - fpstimer > 0) { // avoid strange fps values
		sprintf(fpsstring, "%2.2lf", fps);
	}

	fpstimer = temptime;
	nPreviousFrames = nFramesRendered;
}


// Video presenter reads status from the ONE recorder instance in run.cpp.
// Preserve the original display ABI used by vid_sdl2.cpp. 87.164 moved
// the F9 hotkey to v2 but left this old-v1-only getter unchanged, so the
// red RECORDING indicator silently disappeared even while v2 wrote data.
extern "C" int xm87132ReplayRecordingFrames(){
    const int v2=XM8764::recordingFrames();
    return v2>0?v2:XM8761::recordingFrames();
}
extern "C" int xm87166ReplayVisualStatus(){
    return XM8764::visualStatus();
}
extern "C" int xm87166ReplayVisualPeek(){
    return XM8764::visualStatus(false);
}

//crappy message system
void UpdateMessage(char* message)
{
	snprintf(lastMessage, MESSAGE_MAX_LENGTH, "%s", message);
	messageFrames = MESSAGE_MAX_FRAMES;
}

// define this function somewhere above RunMessageLoop()
void ToggleLayer(unsigned char thisLayer)
{
	nBurnLayer ^= thisLayer;                         // xor with thisLayer
	VidRedraw();
	VidPaint(0);
}


struct timeval start;

unsigned int GetTime(void)
{
	unsigned int ticks;
	struct timeval now;
	gettimeofday(&now, NULL);
	ticks = (now.tv_sec - start.tv_sec) * 1000000 + now.tv_usec - start.tv_usec;
	return ticks;
}

// With or without sound, run one frame.
// If bDraw is true, it's the last frame before we are up to date, and so we should draw the screen
static unsigned long long xm8640FrameCount=0;
static int RunFrame(int bDraw, int bPause)
{
    static bool firstRunFrame=true;
    const bool traceFrame=false; // Clean release: retired first-frame diagnostic logging
    if(firstRunFrame)firstRunFrame=false;
    if(traceFrame){std::fprintf(stderr,"XM86.34 FIRST RunFrame ENTER draw=%d pause=%d ready=%d\n",bDraw,bPause,(int)bDrvOkay);std::fflush(stderr);}
	if (!bDrvOkay)
	{
		return 1;
	}
    ++xm8640FrameCount;
    XM87206Crash::mark(300,(int)nCurrentFrame);
    if(!xm87132CleanStable() && (xm8640FrameCount==1 || (xm8640FrameCount%300u)==0u)) {
        std::fprintf(stderr,"XM86.40 PROGRESS frames=%llu emulated=%d paused=%d\n",
            xm8640FrameCount,nFramesEmulated,(int)bRunPause);
        std::fflush(stderr);
    }

    if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 BEFORE_SLOWMO\n");std::fflush(stderr);}
	// SlowMo stuff
	flippy++;
	nSlowMo = macroSystemSlowMo[0] + macroSystemSlowMo[1] * 2 + macroSystemSlowMo[2] * 3 + macroSystemSlowMo[3] * 4 + macroSystemSlowMo[4] * 5;
	if ((nSlowMo == 1) && ((flippy % 4) == 0)) return 0;		// 75% speed
	else if ((nSlowMo > 1) && (nSlowMo < 6) && (flippy % ((nSlowMo - 1) * 2)) < (((nSlowMo - 1) * 2) - 1)) return 0;		// 50% and less

    if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 BEFORE_INPUT pause=%d\n",bPause);std::fflush(stderr);}
	if (bPause)
	{
		InputMake(false);
		VidPaint(0);
	}
	else
	{
		nFramesEmulated++;
		nCurrentFrame++;
		InputMake(true);
        // 87.164: host-aware v2, snapshotting actual successful host
        // transitions at a safe pre-emulation-frame boundary. Original
        // 87.61 movies remain supported when v2 is not active.
        XM8764::Host host;
        host.training=xm14TrainingMode;host.versus=xm24VersusMode;
        host.owner=xm14InputTarget;host.damage=xm87102DamageMode;
        host.menu=xm23State.menuOpen?1:0;
        host.selection=xm23State.selected;
        host.hitboxes=xm23State.hitboxes?1:0;
        host.display=xm23State.dataDisplay?1:0;
        host.meter=xm23State.infiniteMeter?1:0;
        host.dizzy=xm23State.dizzyAllowed?1:0;
        host.frameData=xm23State.frameData.enabled?1:0;
        host.stage=xm77StageChoice;
        host.stageResolved=xm77StageResolved;
        host.tournament=XM8755Mode::tournament()?1:0;
        host.selectPhase=xm17.phase;host.selectFrames=xm17.frames;
        host.selectPicks=xm17.picksP1;
        host.selectCandidates=xm17.combatCandidateFrames;
        host.selectCarry=xm17.p2CarryMask;
        const XM8764::FrameResult hostReplay=XM8764::beforeFrame(host);
        if(hostReplay==XM8764::DIVERGED ||
           hostReplay==XM8764::END_GAME)return 0;
        if(hostReplay==XM8764::FRAME_READY){
            XM8764::Context& rec=XM8764::context();
            if(rec.hostChanged){
                const XM8764::Host& h=rec.restored;
                xm14TrainingMode=h.training;xm24VersusMode=h.versus;
                xm14InputTarget=h.owner;xm87102DamageMode=h.damage;
                xm23State.menuOpen=h.menu!=0;xm23State.selected=h.selection;
                xm23State.hitboxes=h.hitboxes!=0;
                xm23State.dataDisplay=h.display!=0;
                xm23State.infiniteMeter=h.meter!=0;
                xm23State.dizzyAllowed=h.dizzy!=0;
                xm23State.frameData.enabled=h.frameData!=0;
                xm77StageChoice=h.stage;
                xm77StageResolved=h.stageResolved;
                if(XM8755Mode::tournament()!=(h.tournament!=0)){
                    XM8755Mode::tournament()=h.tournament!=0;
                    XM8755Mode::Apply();
                }
                xm17.phase=(unsigned char)h.selectPhase;
                xm17.frames=(unsigned)h.selectFrames;
                xm17.picksP1=(unsigned)h.selectPicks;
                xm17.combatCandidateFrames=(unsigned)h.selectCandidates;
                xm17.p2CarryMask=(unsigned char)h.selectCarry;
                rec.hostChanged=false;
            }
            XM87162ReplaySync::publish(rec.allFrames-1u); // monotonic across saved-state resets
        }else{
            XM8761::afterInput(); // original F9 native v1 path
        }
        // The movie index is monotonic even when loading an emulator save
        // state resets GetCurrentFrame(). This identifies the EXACT input
        // frame being presented to BurnDrvFrame() without any input edits.
        XM8761::Context& movie=XM8761::ctx();
        if(hostReplay!=XM8764::FRAME_READY)
            XM87162ReplaySync::publish(
                ((movie.mode==1 || movie.mode==2) && movie.index>0u)?
                movie.index-1u : ~0u);
        // 87.163: visible playback status. The old runner reached a boot
        // menu and gave no feedback because no CPS2 frame ever executed.
        // Title updates are presentation-only, never alter replay inputs.
        if(sdlWindow && XM8764::context().mode==XM8764::PLAY){
            const XM8764::Context& v2=XM8764::context();
            if(v2.allFrames==1u || (v2.allFrames%300u)==0u){
                char progress[160];
                std::snprintf(progress,sizeof(progress),
                    "XMVSF HOST REPLAY | %u FRAMES | SESSION %u | %ux FAST",
                    v2.allFrames,v2.session,XM8764::fastFrames());
                SDL_SetWindowTitle(sdlWindow,progress);
            }
        }
        const char* playback=std::getenv("XM8761_REPLAY_MODE");
        if(sdlWindow && playback && std::strcmp(playback,"play")==0){
            if(movie.mode==2 && movie.index &&
               (movie.index==1u || (movie.index%300u)==0u)){
                char progress[128];
                std::snprintf(progress,sizeof(progress),
                    "XMVSF REPLAY | %u / %u INPUT FRAMES",
                    movie.index,movie.recordedCount);
                SDL_SetWindowTitle(sdlWindow,progress);
            } else if(movie.mode==4 && !movie.errorReported){
                SDL_SetWindowTitle(sdlWindow,
                    "XMVSF REPLAY COMPLETE | ALL RECORDED INPUTS PLAYED");
                movie.errorReported=true;
            } else if(movie.mode==3 && !movie.errorReported){
                SDL_SetWindowTitle(sdlWindow,
                    "XMVSF REPLAY FAILED | SEE replay_status.log");
                movie.errorReported=true;
            }
        }
	}
    if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 AFTER_INPUT\n");std::fflush(stderr);}

	// Test/replay deterministic input stream must not have speculative frames.
	if(std::getenv("XM8761_DISABLE_RUNAHEAD") || XM8761::recordingFrames()>0) bRunAhead=false;
	if (bDraw)
	{
		nFramesRendered++;

        if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 BEFORE_VIDEO runahead=%d disabled=%d fast=%d\n",
           bRunAhead,(int)((BurnDrvGetFlags()&BDF_RUNAHEAD_DISABLED)!=0),bAppDoFast);std::fflush(stderr);}
		if (!bRunAhead || (BurnDrvGetFlags() & BDF_RUNAHEAD_DISABLED) || bAppDoFast) {     // Run-Ahead feature 				-dink aug 02, 2021
            if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 BEFORE_VIDFRAME\n");std::fflush(stderr);}
            XM87206Crash::mark(310,(int)nCurrentFrame);
			const int xm8634VidResult=VidFrame();
            XM87206Crash::mark(311,(int)nCurrentFrame);
            if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 AFTER_VIDFRAME result=%d\n",xm8634VidResult);std::fflush(stderr);}
			if (xm8634VidResult) {				// Do one frame w/o RunAhead or if FFWD is pressed.
				// VidFrame() failed, but we must run a driver frame because we have
            	// a clocked input.  Possibly from recording or netplay(!)
            	// Note: VidFrame() calls BurnDrvFrame() on success.
				pBurnDraw = NULL;			// Make sure no image is drawn
            	BurnDrvFrame();

				AudBlankSound();
			}
		} else {
                if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 BEFORE_RUNAHEAD_DRVRFRAME\n");std::fflush(stderr);}
				pBurnDraw = (BurnDrvGetFlags() & BDF_RUNAHEAD_DRAWSYNC) ? pVidImage : NULL;
				BurnDrvFrame();
                if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 AFTER_RUNAHEAD_DRVRFRAME\n");std::fflush(stderr);}
				StateRunAheadSave();
				INT16 *pBurnSoundOut_temp = pBurnSoundOut;
				pBurnSoundOut = NULL;
				nCurrentFrame++;
				bBurnRunAheadFrame = 1;

				if (VidFrame()) {
					// VidFrame() failed, but we must run a driver frame because we have
					// an input.  Possibly from recording or netplay(!)
					pBurnDraw = NULL;			// Make sure no image is drawn, since video failed this time 'round.
					BurnDrvFrame();
				}

				bBurnRunAheadFrame = 0;
				nCurrentFrame--;
				StateRunAheadLoad();
				pBurnSoundOut = pBurnSoundOut_temp; // restore pointer, for wav & avi writer
		}

        XM87206Crash::mark(319,(int)nCurrentFrame);
        // XM86.57 stage registration and scene gate BEFORE the first stage-frame
        // presentation, not after the fight marquee or after VidPaint.
        if(!bPause){
            xm77ApplyNativeStage5(bDrvOkay,false);
            xm8658ObserveGamePhase(bDrvOkay);
            xm8649ObserveCamera(bDrvOkay);
            xm14ApplyTrainingCheats(xm14TrainingMode && !xm23State.dizzyAllowed); // XM87.44: before first presentation
        }
	    if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 BEFORE_VIDPAINT\n");std::fflush(stderr);}
        XM87206Crash::mark(325,(int)nCurrentFrame);
	VidPaint(0);
        if(traceFrame){std::fprintf(stderr,"XM86.34 FRAME1 AFTER_VIDPAINT\n");std::fflush(stderr);}                                              // paint the screen (no need to validate)
	} else {                                       // frame skipping
		pBurnDraw = NULL;                    // Make sure no image is drawn
		BurnDrvFrame();
		if (!bPause) xm14ApplyTrainingCheats(xm14TrainingMode && !xm23State.dizzyAllowed); // XM87.100 non-rendered frame
	}

	if (!bPause) { /* XM87.44: Training applied before VidPaint, not after */ xm15OnRealFrame(bDrvOkay); xm17OnRealFrame(bDrvOkay); xm18ApplyPosition(bDrvOkay); /* XMVSF_STAGE18_POSITION */
    XM87206Crash::mark(340,(int)nCurrentFrame);
    xm20VsCheck(bDrvOkay,bRunPause); /* XMVSF_STAGE20_1_VERSUS */
    XM87206Crash::mark(341,(int)nCurrentFrame);
    xm22::muteSelectionBgm(bDrvOkay, bRunPause, xm20Vs.triggered); /* XMVSF_STAGE22_SELECTION_SFX */
    xm23OnRealFrame(bDrvOkay); /* XMVSF_STAGE23_TRAINING_MENU */
    xm87118OnRealFrame(bDrvOkay); /* XM87.118 completed super-stock observer */
    xm24OnRealFrame(bDrvOkay); /* XMVSF_STAGE24_VERSUS */
    // Triple-Start restart takes priority over the KO-landing autoload.
    if(xm87110RestartAfterFrame(bDrvOkay,(unsigned)nCurrentFrame))
        XM8764::checkpoint("TRAINING_TRIPLE_START_RELOAD");
    else
        xm87105TrainingResetOnLanding(bDrvOkay,(unsigned)nCurrentFrame); // KO -> landed -> match snapshot
    if(traceFrame){std::fprintf(stderr,"XM86.34 FIRST RunFrame reached stage5 hook\n");std::fflush(stderr);}
    // Stage86.57: stage resolution and camera were sampled before VidPaint.
    // Do not run the stage hook a second time after presenting the frame.
    if(traceFrame){std::fprintf(stderr,"XM86.34 FIRST RunFrame stage5 hook returned\n");std::fflush(stderr);}
    XM87206Crash::mark(350,(int)nCurrentFrame);
    if (bDrvOkay && !bBurnRunAheadFrame && !bRunPause) {
        if (xm18Position.combat) xmvsfHost::enterVs();
        else if (!xm14TrainingMode) {
            static unsigned xm20ScanFrames=0;
            if ((++xm20ScanFrames & 7)==0) {
                cpu_core_config* cpu=GetCpuCoreConfig(0);
                if(cpu && cpu->open && cpu->close && cpu->read) {
                    cpu->open(0);
                    const bool live=(cpu->read(0xFF5000)==0x08);
                    cpu->close();
                    if(live)xmvsfHost::enterVs();
                }
            }
        }
    } /* XMVSF_STAGE20_WASAPI */ /* XMVSF_STAGE17_AUTO */ } // XMVSF_STAGE15_MACHINE // XMVSF_STAGE14_TRAINING_CHEATS

	if (bAppShowFPS) {
		if (nDoFPS < nFramesRendered) {
			DisplayFPS();
			nDoFPS = nFramesRendered + 30;
		}
	}

	return 0;
}

// Callback used when DSound needs more sound
static int RunGetNextSound(int bDraw)
{
	if (nAudNextSound == NULL)
	{
		return 1;
	}

	if (bRunPause)
	{
		if (bAppDoStep)
		{
			RunFrame(bDraw, 0);
			memset(nAudNextSound, 0, nAudSegLen << 2);                                        // Write silence into the buffer
		}
		else
		{
			RunFrame(bDraw, 1);
		}

		bAppDoStep = 0;                                                   // done one step
		return 0;
	}

	if (bAppDoFast)
	{                                            // do more frames
		for (int i = 0; i < nFastSpeed; i++)
		{
			RunFrame(0, 0);
		}
	}

	// Render frame with sound
	pBurnSoundOut = nAudNextSound;
	RunFrame(bDraw, 0);
    if(!bAppDoStep){
    }
	if (bAppDoStep)
	{
		memset(nAudNextSound, 0, nAudSegLen << 2);                // Write silence into the buffer
	}
	bAppDoStep = 0;                                              // done one step

	return 0;
}

int delay_ticks(int ticks)
{
//sdl_delay can take up to 10 - 15 ticks it doesnt guarentee below this
   int startTicks = 0;
   int endTicks = 0;
   int checkTicks = 0;

   startTicks=SDL_GetTicks();

   while (checkTicks <= ticks)
   {
      endTicks=SDL_GetTicks();
      checkTicks = endTicks - startTicks;
   }

   return ticks;
}
int RunIdle()
{
    static bool firstIdle=true;
    if(firstIdle){firstIdle=false;std::fprintf(stderr,"XM86.34 FIRST RunIdle enter\n");std::fflush(stderr);}
	int nTime, nCount;

	// Independent QPC deadline determines game updates, not the audio play cursor.
    // Even if WASAPI is unavailable, the machine runs at its original frame rate.
    // 87.164 high-speed playback: preserve ordered CPS2 CPU/input steps,
    // render once per batch, and pump real SDL events between batches.
    const unsigned replayBatch=XM8764::fastFrames();
    if(replayBatch){
        if(bAudPlaying)AudSoundStop();
        for(unsigned i=0u;i<replayBatch;++i){
            if(XM8764::context().exitSession ||
               XM8764::context().mode==XM8764::FAILED)break;
            RunFrame(i+1u==replayBatch?1:0,0);
        }
        return 0;
    }
    if(!xmvsfHost::frameDue(nAppVirtualFps)) return 0;
    if(bAudPlaying) AudSoundCheck();
    else RunFrame(1,0);
    return 0; /* XMVSF_STAGE20_WASAPI */


	// Run without sound
	nTime = GetTime() - nNormalLast;
	nCount = (nTime * nAppVirtualFps - nNormalFrac) / 100000;
	if (nCount <= 0) {						// No need to do anything for a bit
		//delay_ticks(2);
		return 0;
	}

	nNormalFrac += nCount * 100000;
	nNormalLast += nNormalFrac / nAppVirtualFps;
	nNormalFrac %= nAppVirtualFps;

	if (nCount > 100) {						// Limit frame skipping
		nCount = 100;
	}
	if (bRunPause) {
		if (bAppDoStep) {					// Step one frame
			nCount = 10;
		}
		else {
			RunFrame(1, 1);					// Paused
			return 0;
		}
	}
	bAppDoStep = 0;


	if (bAppDoFast)
	{									// do more frames
		for (int i = 0; i < nFastSpeed; i++)
		{
			RunFrame(0, 0);
		}
	}

	if (!bAlwaysDrawFrames)
	{
		for (int i = nCount / 10; i > 0; i--)
		{              // Mid-frames
			RunFrame(0, 0);
		}
	}
	RunFrame(1, 0);                                  // End-frame
	// temp added for SDLFBA
	//VidPaint(0);
	return 0;
}

int RunReset()
{
    xmvsfHost::resetClock(); /* XMVSF_STAGE20_WASAPI */
	// Reset the speed throttling code
	nNormalLast = 0; nNormalFrac = 0;
	if (!bAudPlaying)
	{
		// run without sound
		nNormalLast = GetTime();
	}
	return 0;
}

int RunInit()
{
    std::fprintf(stderr,"XM86.34 RunInit enter\n");std::fflush(stderr);
	gettimeofday(&start, NULL);
	DisplayFPSInit();
    // V2 playback already owns the exact ROM + host state for the first
    // recorded frame. Do not launch the WASAPI/audio callback early,
    // nor reload a different Training selection / StatedAuto snapshot
    // over the movie's canonical native checkpoint. Fast-forward uses
    // the normal synchronous RunFrame CPU path with no audio throttle.
    if(XM8764::isPlayback()){
        std::fprintf(stderr,
            "XM87.164 V2_PLAYBACK_INIT skip boot snapshots/audio; native checkpoint owns state\n");
        std::fflush(stderr);
        RunReset();
        XM8764::gameBegin(xm24VersusMode?2:xm14TrainingMode?1:0);
        return 0;
    }
	// Try to run with sound
	AudSetCallback(RunGetNextSound);
    std::fprintf(stderr,"XM86.34 RunInit audio play begin\n");std::fflush(stderr);
	AudSoundPlay();
    std::fprintf(stderr,"XM86.34 RunInit audio play complete\n");std::fflush(stderr);
	RunReset();
    std::fprintf(stderr,"XM86.34 RunInit StatedAuto begin\n");std::fflush(stderr);
	StatedAuto(0);
    std::fprintf(stderr,"XM86.34 RunInit StatedAuto complete; training snapshot begin\n");std::fflush(stderr);
    const bool skipSnapshot=std::getenv("XM8634_SKIP_SNAPSHOT")!=nullptr;
    if(skipSnapshot){
     std::fprintf(stderr,"XM86.34 SNAPSHOT SKIPPED: diagnostic fresh-boot run only\n");std::fflush(stderr);
    }else{
     xm16LoadTrainingSnapshot(); // XMVSF_STAGE16_SNAPSHOT
    }
    std::fprintf(stderr,"XM86.34 RunInit training snapshot complete skipped=%d\n",(int)skipSnapshot);std::fflush(stderr);
    XM8764::gameBegin(xm24VersusMode?2:xm14TrainingMode?1:0);
	return 0;
}

int RunExit()
{
	nNormalLast = 0;
	StatedAuto(1);
	return 0;
}

#ifdef BUILD_SDL2
void pause_game()
{
	AudSoundStop();

	if(nVidSelect) {
		// no Text in OpenGL...
		SDL_GL_SwapWindow(sdlWindow);
	}else{
		inprint_shadowed(sdlRenderer, "PAUSE", 10, 10);
		SDL_RenderPresent(sdlRenderer); /* XM86.57 preserve graded native HUD/FX on pause */
	}

    int finished = 0;
	while (!finished)
  	{
		starting_stick = SDL_GetTicks();

 		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				finished=1;
			}
			if (e.type == SDL_KEYDOWN)
			{
			  switch (e.key.keysym.sym)
			  {
				  case SDLK_TAB:
				  case SDLK_p:
					finished=1;
					break;
				  default:
					break;
			  }
			}
			if (e.type == SDL_WINDOWEVENT)
			{ // Window Event
				switch (e.window.event)
				{
					//case SDL_WINDOWEVENT_RESTORED: // keep pause when restore window
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						finished=1;
						break;
					case SDL_WINDOWEVENT_CLOSE:
						finished=1;
						RunExit();
						break;
					default:
						break;
				}
			}
		}

		// limit 5 FPS (free CPU usage)
		if ( ( 1000 / 5 ) > SDL_GetTicks() - starting_stick) {
			SDL_Delay( 1000 / 5 - ( SDL_GetTicks() - starting_stick ) );
		}

	}

	AudSoundPlay();
}
#endif

#ifndef BUILD_MACOS
// The main message loop
int RunMessageLoop()
{ /* XMVSF_STAGE24_VERSUS */
	int quit = 0;
    const char* xm8640ExitReason="unspecified";
    const unsigned long long xm8640StartFrames=xm8640FrameCount;
    // Keep the left DHC Super Chat hidden until explicitly toggled with
    // Shift+G in THIS game session. Message logging and F10 stay available.
    XM87140Chat::state().open=false;
    // XM86.55: the VS detector and the selection-music suppressor are session-scoped.
    // An earlier match must never keep the next character select in combat mode.
    if(xm20Vs.log){std::fclose(xm20Vs.log);xm20Vs.log=nullptr;}
    xm20Vs=XM20VsTracker{};
    xm22::diagnosticWritten=false;
    xm22::writes=0;
    std::fprintf(stderr,"XM86.55 SESSION_AUDIO_RESET vs=0 select_mute_rearmed=1\n");
    std::fprintf(stderr,"XM86.34 RunMessageLoop ENTER\n");std::fflush(stderr);
	xm24StartSession(); /* XMVSF_STAGE24_VERSUS */
    std::fprintf(stderr,"XM86.34 xm24StartSession complete\n");std::fflush(stderr);
	RunInit();
    std::fprintf(stderr,"XM86.34 RunInit returned to message loop\n");std::fflush(stderr);
	GameInpCheckMouse();                                                                     // Hide the cursor
    if(xm14TrainingMode || xm24VersusMode)xm25BeginInputHandoff(); // post-snapshot, pre-game input

	while (!quit)
	{

		SDL_Event event;
        while (SDL_PollEvent(&event))
		{
            // XM87.141: F10 must reach the recorder BEFORE display, Training
            // and Versus event handlers have any opportunity to consume it.
            // 87.161: restore the existing 87.61 F9 state-anchored input
            // movie recorder BEFORE the display/Training/Versus handlers
            // can consume its SDL event. Shift+F9 is the legacy quick-save.
            // Keep event.repeat suppressed to prevent repeated toggles.
            if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F9){
                if(event.key.repeat==0){
                    if(event.key.keysym.mod & KMOD_SHIFT)
                        QuickState(0);
                    else
                        if(XM8764::isPlayback())
                            XM8764::status("F9_IGNORED_DURING_PLAYBACK");
                        else if(std::getenv("XM8764_REPLAY_ROOT"))
                            XM8764::toggleF9();
                        else
                            XM8761::toggleF9(bDrvOkay!=0);
                }
                continue;
            }
            if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F10){
                if(event.key.repeat==0){
                    if(event.key.keysym.mod & KMOD_SHIFT){
                        QuickState(1); // original Shift+F10 load
                        XM8764::checkpoint("HOST_SHIFT_F10_QUICKLOAD");
                    }else{
                        const bool wasActive=XM87139Probe::state().active;
                        const bool ok=XM87139Probe::state().toggle(bDrvOkay!=0);
                        char note[124];
                        if(!ok)std::snprintf(note,sizeof(note),
                            "F10 PROBE ERROR: %s",
                            bDrvOkay?"LOG FILE COULD NOT OPEN":"NO GAME ACTIVE");
                        else if(wasActive)std::snprintf(note,sizeof(note),
                            "F10 PROBE STOP frames=%u file=%s",
                            XM87139Probe::state().frames,
                            XM87139Probe::state().tracePath.c_str());
                        else std::snprintf(note,sizeof(note),
                            "F10 PROBE ON writing %s",
                            XM87139Probe::state().tracePath.c_str());
                        XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                            ok?XM87140Chat::SYSTEM:XM87140Chat::ALERT,note);
                    }
                }
                continue;
            }
#ifdef BUILD_SDL2
            // Unified suite: F9 replay and F10 diagnostics retain their
            // original early handlers. F11 owns MVC1 native level capture.
            // Never arm capture during menu/replay or in the XMVSF game.
            if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F11){
                if(event.key.repeat==0){
                    if(event.key.keysym.mod & KMOD_SHIFT){
                        // Preserve legacy FPS function under Shift+F11.
                        bAppShowFPS=!bAppShowFPS;
                        if(!bAppShowFPS){
                            std::sprintf(Windowtitle,"FBNeo - %s - %s",
                                BurnDrvGetTextA(DRV_NAME),BurnDrvGetTextA(DRV_FULLNAME));
                            SDL_SetWindowTitle(sdlWindow,Windowtitle);
                        }
                    }else{
                        const char* driver=bDrvOkay?BurnDrvGetTextA(DRV_NAME):nullptr;
                        const char* folder=std::getenv("XM87203_STAGE_CAPTURE_DIR");
                        const bool allowed=driver && std::strncmp(driver,"mvsc",4)==0 &&
                            folder && *folder && !XM8764::isPlayback();
                        if(allowed){
                            const char* flag=std::getenv("XM87203_LEVEL_CAPTURE_ACTIVE");
                            const bool active=flag && std::strcmp(flag,"1")==0;
                            bool ok=true;
                            if(!active){
                                static unsigned captureGeneration=0u;
                                char generation[24];
                                std::snprintf(generation,sizeof(generation),"%u",++captureGeneration);
                                ok=SDL_setenv("XM87203_LEVEL_CAPTURE_GENERATION",generation,1)==0;
                            }
                            if(ok)ok=SDL_setenv("XM87203_LEVEL_CAPTURE_ACTIVE",
                                active?"0":"1",1)==0;
                            const char* message=!ok?"F11 LEVEL CAPTURE ERROR: SDL ENVIRONMENT":
                                active?"F11 LEVEL CAPTURE STOP":
                                       "F11 LEVEL CAPTURE START: NEW SOURCE SESSION";
                            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                                ok?XM87140Chat::SYSTEM:XM87140Chat::ALERT,message);
                            std::fprintf(stderr,"XM87.203 %s driver=%s\n",
                                message,driver);
                            std::fflush(stderr);
                        }else{
                            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                                XM87140Chat::ALERT,
                                "F11 LEVEL CAPTURE: SELECT MVC1 UNDER RUN_TEST OPTION 3");
                        }
                    }
                }
                continue;
            }
#endif
            // V2 restores recorded semantic menu decisions at exact
            // checkpoints. Physical keyboard/gamepad input must not
            // inject NEW Training/Versus commands while the replay runs.
            // Window events and SDL_QUIT still reach the normal handler.
            if(XM8764::isPlayback() &&
               (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP ||
                event.type==SDL_CONTROLLERBUTTONDOWN ||
                event.type==SDL_CONTROLLERBUTTONUP ||
                event.type==SDL_JOYBUTTONDOWN ||
                event.type==SDL_JOYBUTTONUP ||
                event.type==SDL_JOYAXISMOTION ||
                event.type==SDL_JOYHATMOTION))continue;
            if(xm29HandleDisplayHotkey(event,sdlWindow))continue;
            if(!xm14TrainingMode && !xm24VersusMode && event.type==SDL_KEYDOWN && !event.key.repeat){
                // Coin (including keyboard 5) is edge-polled in the
                // shared Arcade menu handler. Do not also toggle on KEYDOWN:
                // that would open and immediately close the overlay.
                if(xm258ArcadeMenuOpen && (event.key.keysym.sym==SDLK_ESCAPE ||
                                            event.key.keysym.sym==SDLK_BACKSPACE)){
                    xm258ArcadeMenuOpen=false;xm25MenuSound(false);
                    xm25BeginInputHandoff();continue;
                }
                if(xm258ArcadeMenuOpen && (event.key.keysym.sym==SDLK_RETURN ||
                                            event.key.keysym.sym==SDLK_SPACE)){
                    xm258ArcadeReturnRequested=true;xm25MenuSound(true);continue;
                }
            }
            if(xm258ArcadeMenuOpen && (event.type==SDL_CONTROLLERBUTTONDOWN ||
                event.type==SDL_CONTROLLERBUTTONUP || event.type==SDL_JOYBUTTONDOWN ||
                event.type==SDL_JOYBUTTONUP))continue;
            if(xm87105TrainingHPHotkey(event)) continue; // Training numpad HP instead of volume
            if(xm257HandleTrainingMenuEvent(event,xm14TrainingMode!=0)) continue;
			if(xm24HandleEvent(event)) continue; /* XMVSF_STAGE24_VERSUS */
			switch (event.type)
			{
			case SDL_QUIT:                                        /* Windows was closed */
                xm8640ExitReason="SDL_QUIT";
				quit = 1;
				break;

#ifdef BUILD_SDL2
			case SDL_WINDOWEVENT:  // Window Event
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_MINIMIZED:
					case SDL_WINDOWEVENT_FOCUS_LOST:
						pause_game();
						break;
				}
				break;
#endif

			case SDL_KEYDOWN:                                                // need to find a nicer way of doing this...
				switch (event.key.keysym.sym)
				{
				case SDLK_F1:
					bAppDoFast = 1;
					break;
                // F9 recording and Shift+F9 quick-save are handled
                // above all input-consuming menu handlers (87.161).
                                case SDLK_g: // XM87.140 expand/collapse DHC chat history
                    if(event.key.repeat==0 && (event.key.keysym.mod&KMOD_SHIFT))
                        XM87140Chat::state().toggle();
                    break;
                // F10 handled before menu event consumers (XM87.141).

				case SDLK_F11:
					bAppShowFPS = !bAppShowFPS;
#ifdef BUILD_SDL2
					if (!bAppShowFPS)
					{
						sprintf(Windowtitle, "FBNeo - %s - %s", BurnDrvGetTextA(DRV_NAME), BurnDrvGetTextA(DRV_FULLNAME));
						SDL_SetWindowTitle(sdlWindow, Windowtitle);
					}
#endif
					break;
#ifdef BUILD_SDL2
				case SDLK_TAB:
					if(!nVidSelect) {
						ingame_gui_start(sdlRenderer);
					} else {
						// Pause with SDL2 OpenGL mode
						pause_game();
					}
					break;

                case SDLK_RETURN:
                    // Stage29 handles Alt+Enter before gameplay event dispatch.
                    break;
#endif
				case SDLK_KP_MINUS: // volumme -
					nAudVolume -= 500;
					if (nAudVolume < 0) {
						nAudVolume = 0;
					}
					if (AudSoundSetVolume() == 0) {
					}
					break;
				case SDLK_KP_PLUS: // volume -+
					nAudVolume += 500;
					if (nAudVolume > 10000) {
						nAudVolume = 10000;
					}
					if (AudSoundSetVolume() == 0) {
					}
					break;
				default:
					break;
				}
				break;

			case SDL_KEYUP:                                                // need to find a nicer way of doing this...
				switch (event.key.keysym.sym)
				{
				case SDLK_F1:
					bAppDoFast = 0;
					break;
				case SDLK_F12:
                    xm8640ExitReason="F12";
					quit = 1;
					break;

				default:
					break;
				}
				break;
			}
		}

        // Replay menu decisions are restored from the v2 host snapshot,
        // never generated from a newly attached physical controller.
        if(!XM8764::isPlayback())
            xm257PollTrainingMenu(xm14TrainingMode!=0);
        if(xm14TrainingMode && xm23State.returnMainRequested){
            xm23State.returnMainRequested=false;
            xm26ReturnToBoot=true; // Stage26 in-process return, no process spawn
        }
        const int oldVersusCommand=xm24Session.command;
		xm24HandlePendingCommand(); /* XMVSF_STAGE24_VERSUS */
        if(oldVersusCommand!=XM24_NONE)
            XM8764::checkpoint("VERSUS_RESULT_OR_RESELECT_COMMAND");
		xm243PollLiveMenu(); /* XMVSF_STAGE24_3_LIVE_RESULTS */
        xm258PollArcadeMenu(); /* XMVSF_STAGE25_8_ARCADE_MENU */
        if(xm258ArcadeReturnRequested){
            xm258ArcadeReturnRequested=false;
            xm26ReturnToBoot=true; // Stage26 in-process return, no process spawn
        }
        if(XM8764::context().exitSession ||
           XM8764::context().mode==XM8764::FAILED){
            const bool finished=XM8764::context().mode==XM8764::COMPLETE ||
                XM8764::context().mode==XM8764::FAILED;
            xm26ReturnToBoot=!finished;
            xm8640ExitReason=finished?"replay_done":"replay_session_end";
            quit=1;continue;
        }
        if(xm26ReturnToBoot){
            // TEST 87.227: do not submit a full black frame during the
            // persistent game->menu handoff. The menu paints a complete first
            // frame on the very same live SDL renderer after media cleanup.
            // Old code explicitly presented black here, leaving an empty
            // screen if the renderer was re-created late or presentation
            // ownership was disrupted.
            std::fprintf(stderr,
                "XM87.227 MENU_HANDOFF_NO_BLACK_FRAME renderer=%p\n",
                (void*)sdlRenderer);
            std::fflush(stderr);
            if(sdlRenderer)SDL_SetRenderTarget(sdlRenderer,nullptr);
            xm8640ExitReason="return_to_main";
            quit=1;
            continue;
        }
		RunIdle(); // game runs naturally; custom overlay retains input and score
        if(XM8764::context().exitSession ||
           XM8764::context().mode==XM8764::FAILED){
            const bool finished=XM8764::context().mode==XM8764::COMPLETE ||
                XM8764::context().mode==XM8764::FAILED;
            xm26ReturnToBoot=!finished;
            xm8640ExitReason=finished?"replay_done":"replay_session_end";
            quit=1;
        }

	}

    std::fprintf(stderr,"XM86.40 SESSION EXIT reason=%s frames=%llu emulated=%d returnToBoot=%d\n",
         xm8640ExitReason,xm8640FrameCount-xm8640StartFrames,nFramesEmulated,(int)xm26ReturnToBoot);
    std::fflush(stderr);
    XMGlyph::release(); // Prevent textures surviving native SDL renderer teardown.
	RunExit();

	return 0;
}

#endif
