// CPS - Run
#include "cps.h"
#include "xmvsf_stage87_262_game_version.h"
#include "../../../burner/sdl/xmvsf_stage87_98_latency_trace.h" // XM87.98 native state probe
#include "xmvsf_stage87_79_flight_trace.h" // XM87.79 read-only combat RAM capture
#include "xmvsf_stage87_80_flying_velocity.h" // XM87.80 Tournament launch normalization
#include "xmvsf_stage87_81_severe_flight.h" // XM87.81 experimental severe knockback pair
#include "xmvsf_stage87_82_cyclops_slam.h" // XM87.82 experimental Cyclops aerial slam velocity
#include "xmvsf_stage87_83_rom_flight_guard.h" // XM87.83 ROM-qualified flying-screen hit gate
#include "xmvsf_stage87_84_air_heavy_finisher.h" // XM87.84 second-or-later airborne heavy finisher
#include "xmvsf_stage87_85_connected_air_heavy.h" // XM87.85 connected aerial heavy qualifier
#include "xmvsf_stage87_127_storm_voice_mute.h" // 87.223 all-mode QSound native attack source state
#include "xmvsf_stage87_224_charlie_pc.h" // 87.224 bounded original 68000 Charlie knockdown/velocity PC observer
#include "xmvsf_stage87_223_storm_native_flight.h" // 87.223 read-only 68000 Storm HP velocity + native flight index
#include "xmvsf_stage87_86_storm_button_guard.h" // XM87.86 CPS2 physical MK/HK evidence
#include "xmvsf_stage87_95_native_dizzy_air_collision.h" // XM87.95 dizzy init and per-frame gate/f6 source-backed native opcode override
#include "xmvsf_stage87_96_tournament_dizzy_handoff.h" // XM87.96 native dizzy stand handoff
#include "xmvsf_stage87_99_tournament_ground_defense.h" // XM87.99 native Tournament dizzy landing combo/guard
#include "xmvsf_stage87_149_tournament_rules_audit.h" // 87.149 live rule installation audit
#include "xmvsf_stage87_125_stun_rearm_lock.h" // XM87.125 Tournament dizzy->landing stun rearm
#include "xmvsf_stage87_100_training_ko.h"
#include "xmvsf_stage87_137_dhc_native_capture.h" // XM87.137 opt-in read-only tag/super evidence
#include "../../../burner/sdl/xmvsf_stage87_139_probe.h" // XM87.139 read-only F10 input/native diagnostic
#include "../../../burner/sdl/xmvsf_stage87_140_super_chat.h" // XM87.140 native super-input chat (no writes)
#include "xmvsf_stage87_142_rom_dispatch.h" // 87.142 pure native script table+tag observer
#include "xmvsf_stage87_175_native_accept_observer.h" // 87.175 read-only native 68000 vs host DHC trace
#include "xmvsf_stage87_180_subframe_input_observer.h"
#include "xmvsf_stage87_203_stage_capture.h" // read-only CPS2 stage capture // 87.180 intra-frame native input clocks
#include "xmvsf_stage87_203_m68k_program_snapshot.h" // opt-in private MVC1 68000 source, no guest writes
#include "xmvsf_stage87_181_observer_health.h" // 87.181 startup and combat observer diagnostics
#include "xmvsf_stage87_182_native_pc_trace.h" // 87.182 exact native PPC audit, opt-in
#include "xmvsf_stage87_186_opening_observer.h" // 87.186 original entry, first attack, and input boundary audit
#include "xmvsf_stage87_144_native_handoff.h" // legacy Ryu/Ken native test
#include "xmvsf_stage87_274_unified_dhc.h" // ROM-owned two-sided DHC test
#include "xmvsf_stage87_335_native_dhc_bridge.h" // original-ROM native M68K guest bridge
#include "xmvsf_stage87_340_morphus_native_2v2.h" // Morphus dhc.s motions/chord/2v2 chain
#include "xmvsf_stage87_352_cyclops_sob_cleanup.h" // native Hyper-only orphan helper cleanup
#include "xmvsf_stage87_329_morphus_hook_probe.h" // read-only EURO opcode signature audit
#include "xmvsf_stage87_330_morphus_locator.h" // read-only cross-revision M68K signature locator
#include "xmvsf_stage87_331_morphus_endian_locator.h" // correct word-swapped CPS2 opcode mapping
#include "xmvsf_stage87_332_morphus_dhc_context.h" // read-only immediate DHC opcode context
#include "xmvsf_stage87_160_super_discovery.h" // read-only roster-wide native 2E ROM/script and input discovery
#include "../../../burner/sdl/xmvsf_stage87_162_replay_sync.h" // exact F9 movie-frame index for native action logs
#include "xmvsf_stage87_119_cyclops_recovery_probe.h" // XM87.119 read-only recovery state trace
#include "xmvsf_stage87_121_cyclops_release.h" // XM87.121 restore recorded airborne state after throw
#include "xmvsf_stage87_220_zero_dash_startup.h" // TEST 87.220 native dash startup countdown
extern int xm14TrainingMode; // native SDL Training mode selection
extern int xm87102DamageMode; // XM87.102 Training damage option
#include <cstdio> // XM86.34 first native CPS2 frame tracing

// CPS2
INT32 Cps2Volume = 39;
INT32 Cps2DisableDigitalVolume = 0;
UINT8 Cps2VolUp;
UINT8 Cps2VolDwn;
UINT8 AspectDIP; // only for Cps2Turbo == 1

UINT16 Cps2VolumeStates[40] = {
	0xf010, 0xf008, 0xf004, 0xf002, 0xf001, 0xe810, 0xe808, 0xe804, 0xe802, 0xe801,
	0xe410, 0xe408, 0xe404, 0xe402, 0xe401, 0xe210, 0xe208, 0xe204, 0xe202, 0xe201,
	0xe110, 0xe108, 0xe104, 0xe102, 0xe101, 0xe090, 0xe088, 0xe084, 0xe082, 0xe081,
	0xe050, 0xe048, 0xe044, 0xe042, 0xe041, 0xe030, 0xe028, 0xe024, 0xe022, 0xe021
};

// Inputs:
UINT8 CpsReset = 0;
UINT8 Cpi01A = 0, Cpi01C = 0, Cpi01E = 0;

static INT32 nInterrupt;
static INT32 nIrqLine, nIrqCycles;
static bool bEnableAutoIrq50, bEnableAutoIrq52;				// Trigger an interrupt every 32 scanlines

static const INT32 nFirstLine = 0x10;							// The first scanline of the display

INT32 nCpsCyclesExtra;

INT32 CpsDrawSpritesInReverse = 0;

INT32 nIrqLine50, nIrqLine52;

INT32 nCpsNumScanlines = 262;
INT32 Cps1VBlankIRQLine = 2;

INT32 Cps1DrawAtVblank = 0;

static UINT8 AspectDIPLast = 0;

CpsRunInitCallback CpsRunInitCallbackFunction = NULL;
CpsRunInitCallback CpsRunExitCallbackFunction = NULL;
CpsRunResetCallback CpsRunResetCallbackFunction = NULL;
CpsRunFrameStartCallback CpsRunFrameStartCallbackFunction = NULL;
CpsRunFrameMiddleCallback CpsRunFrameMiddleCallbackFunction = NULL;
CpsRunFrameEndCallback CpsRunFrameEndCallbackFunction = NULL;

static void CpsQSoundCheatSearchCallback()
{
	// Q-Sound Shared RAM ranges - not useful for cheat searching, and runs the Z80
	// in the handler, exclude it from cheat searching
	if (Cps == 2) {
		CheatSearchExcludeAddressRange(0x618000, 0x619FFF);
	}
	
	if (Cps1Qs == 1) {	
		CheatSearchExcludeAddressRange(0xF18000, 0xF19FFF);
		CheatSearchExcludeAddressRange(0xF1E000, 0xF1FFFF);
	}
}

static void check_aspect()
{
	if (Cps2Turbo && (AspectDIP & 0x03) != AspectDIPLast) {
		INT32 aspects[3][2] = { { 16, 9 }, { 4, 3 }, { 112, 81 } };
		AspectDIPLast = AspectDIP & 0x03;

		INT32 nAspectX, nAspectY;
		BurnDrvGetAspect(&nAspectX, &nAspectY);

		if (nAspectX != aspects[AspectDIPLast][0] || nAspectY != aspects[AspectDIPLast][1]) {
			bprintf(0, _T("*  CPS-2: Changing to %d:%d aspect\n"), aspects[AspectDIPLast][0], aspects[AspectDIPLast][1]);
			BurnDrvSetAspect(aspects[AspectDIPLast][0], aspects[AspectDIPLast][1]);
			Reinitialise();
		}
	}
}

static INT32 DrvReset()
{
	// Reset machine
	if (Cps == 2 || PangEEP || Cps1Qs == 1 || CpsBootlegEEPROM) EEPROMReset();

	SekOpen(0);
	SekReset();
	SekClose();

	if (((Cps & 1) && !Cps1DisablePSnd) || ((Cps == 2) && !Cps2DisableQSnd)) {
		ZetOpen(0);
		ZetReset();
		ZetClose();
	}

	if (Cps == 2) {
		// Disable beam-synchronized interrupts
		*((UINT16*)(CpsReg + 0x4E)) = BURN_ENDIAN_SWAP_INT16(0x0200);
		*((UINT16*)(CpsReg + 0x50)) = BURN_ENDIAN_SWAP_INT16(nCpsNumScanlines);
		*((UINT16*)(CpsReg + 0x52)) = BURN_ENDIAN_SWAP_INT16(nCpsNumScanlines);

		// CPS-2 uses Object Banks
		SekOpen(0);
		CpsMapObjectBanks(0);
		SekClose();
	}

	nCpsCyclesExtra = 0;

	clear_opposite.reset();

	if (((Cps == 2) && !Cps2DisableQSnd) || Cps1Qs == 1) {			// Sound init (QSound)
		QsndReset();
	}
	
	if (CpsRunResetCallbackFunction) {
		CpsRunResetCallbackFunction();
	}

	if (Cps2Turbo) {
		AspectDIPLast = 0xff;
		check_aspect();
	}

	HiscoreReset();

	return 0;
}

static const eeprom_interface qsound_eeprom_interface =
{
	7,		/* address bits */
	8,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111",	/* erase command */
	0,
	0,
	0,
	0
};

static const eeprom_interface cps2_eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111",	/* erase command */
	0,
	0,
	0,
	0
};

INT32 CpsRunInit()
{
	SekInit(0, 0x68000);					// Allocate 68000
	
	if (CpsMemInit()) {						// Memory init
		return 1;
	}
	
	if (Cps == 2 || PangEEP) {
		EEPROMInit(&cps2_eeprom_interface);
	} else {
		if (Cps1Qs == 1 || CpsBootlegEEPROM) {
			EEPROMInit(&qsound_eeprom_interface);
		}
	}

	CpsRwInit();							// Registers setup

	if (CpsPalInit()) {						// Palette init
		return 1;
	}
	if (CpsObjInit()) {						// Sprite init
		return 1;
	}

	if ((Cps & 1) && Cps1Qs == 0 && Cps1DisablePSnd == 0) {			// Sound init (MSM6295 + YM2151)
		if (PsndInit()) {
			return 1;
		}
	}

	if (((Cps == 2) && !Cps2DisableQSnd) || Cps1Qs == 1) {			// Sound init (QSound)
		if (QsndInit()) {
			return 1;
		}
		QsndSetRoute(BURN_SND_QSND_OUTPUT_1, 2.00, BURN_SND_ROUTE_LEFT);
		QsndSetRoute(BURN_SND_QSND_OUTPUT_2, 2.00, BURN_SND_ROUTE_RIGHT);
	}

	if (Cps == 2 || PangEEP || Cps1Qs == 1 || CpsBootlegEEPROM) EEPROMReset();
	
	if (CpsRunInitCallbackFunction) {
		CpsRunInitCallbackFunction();
	}
	
	DrvReset();

	//Init Draw Function
	DrawFnInit();
	
	pBurnDrvPalette = CpsPal;
	
	if (Cps == 2 || Cps1Qs == 1) {
		CheatSearchInitCallbackFunction = CpsQSoundCheatSearchCallback;
	}

	return 0;
}

INT32 CpsRunExit()
{
	if (Cps == 2 || PangEEP || Cps1Qs == 1 || CpsBootlegEEPROM) EEPROMExit();

	// Sound exit
	if (((Cps == 2) && !Cps2DisableQSnd) || Cps1Qs == 1) QsndExit();
	if (Cps != 2 && Cps1Qs == 0 && !Cps1DisablePSnd) PsndExit();

	// Graphics exit
	CpsObjExit();
	CpsPalExit();

	// Sprite Masking exit
	ZBuf = NULL;

	// Memory exit
	CpsRwExit();
	CpsMemExit();

	SekExit();
	
	if (CpsRunExitCallbackFunction) {
		CpsRunExitCallbackFunction();
		CpsRunExitCallbackFunction = NULL;
	}
	CpsRunInitCallbackFunction = NULL;
	CpsRunResetCallbackFunction = NULL;
	CpsRunFrameStartCallbackFunction = NULL;
	CpsRunFrameMiddleCallbackFunction = NULL;
	CpsRunFrameEndCallbackFunction = NULL;
	
	Cps1VBlankIRQLine = 2;
	
	Cps2DisableQSnd = 0;
	CpsBootlegEEPROM = 0;

	Cps1DrawAtVblank = 0;

	return 0;
}

inline static void CopyCpsReg(INT32 i)
{
	memcpy(CpsSaveReg[i], CpsReg, 0x0100);
}

// At VBL, latch the F-regs.  Don't make them active yet
// (we don't want re-draws / etc using them yet)
static void LatchCpsFrg()
{
	memcpy(CpsSaveFrg[1], CpsFrg, 0x0010);
}

// Begining of frame, make them active
static void UnLatchCpsFrg()
{
	memcpy(CpsSaveFrg[0], CpsSaveFrg[1], 0x0010);
}

// Schedule a beam-synchronized interrupt
static void ScheduleIRQ()
{
	INT32 nLine = nCpsNumScanlines;

	if (nIrqLine50 <= nLine) {
		nLine = nIrqLine50;
	}
	if (nIrqLine52 < nLine) {
		nLine = nIrqLine52;
	}

	if (nLine < nCpsNumScanlines) {
		nIrqLine = nLine;
		nIrqCycles = (nLine * nCpsCycles / nCpsNumScanlines) + 1;
	} else {
		nIrqCycles = nCpsCycles + 1;
	}

	return;
}

// Execute a beam-synchronised interrupt and schedule the next one
static void DoIRQ()
{
	// 0x4E - bit 9 = 1: Beam Synchronized interrupts disabled
	// 0x50 - Beam synchronized interrupt #1 occurs at raster line.
	// 0x52 - Beam synchronized interrupt #2 occurs at raster line.

	// Trigger IRQ and copy registers.
	if (nIrqLine >= nFirstLine) {

		nInterrupt++;
		nRasterline[nInterrupt] = nIrqLine - nFirstLine;
	}

	SekSetIRQLine(4, CPU_IRQSTATUS_AUTO);
	SekRun(nCpsCycles * 0x01 / nCpsNumScanlines);
	if (nRasterline[nInterrupt] < 224) {
		CopyCpsReg(nInterrupt);
	} else {
		nRasterline[nInterrupt] = 0;
	}

	// Schedule next interrupt
	if (!bEnableAutoIrq50) {
		if (nIrqLine >= nIrqLine50) {
			nIrqLine50 = nCpsNumScanlines;
		}
	} else {
		if (bEnableAutoIrq50 && nIrqLine == nIrqLine50) {
			nIrqLine50 += 32;
		}
	}
	if (!bEnableAutoIrq52 && nIrqLine >= nIrqLine52) {
		nIrqLine52 = nCpsNumScanlines;
	} else {
		if (bEnableAutoIrq52 && nIrqLine == nIrqLine52) {
			nIrqLine52 += 32;
		}
	}
	ScheduleIRQ();
	if (nIrqCycles < SekTotalCycles()) {
		nIrqCycles = SekTotalCycles() + 1;
	}

	return;
}

INT32 Cps1Frame()
{
	INT32 nDisplayEnd, nNext, i;

	if (CpsReset) {
		DrvReset();
	}

	SekNewFrame();
	if (Cps1Qs == 1) {
		QsndNewFrame();
	} else {
		if (!Cps1DisablePSnd) {
			ZetOpen(0);
			PsndNewFrame();
		}
	}
	
	nCpsCycles = (INT32)((INT64)nCPS68KClockspeed * nBurnCPUSpeedAdjust >> 8);

	CpsRwGetInp();												// Update the input port values

	if (CpsRunFrameStartCallbackFunction) {
		CpsRunFrameStartCallbackFunction();
	}

	nDisplayEnd = (nCpsCycles * (nFirstLine + 224)) / nCpsNumScanlines;	// Account for VBlank

	SekOpen(0);

	SekIdle(nCpsCyclesExtra);
	nCpsCyclesExtra = 0;

	SekRun((nCpsCycles * nFirstLine / nCpsNumScanlines));					// run 68K for the first few lines

	CpsObjGet();											// Get objects

	for (i = 0; i < 4; i++) {
		nNext = ((i + 1) * nCpsCycles) >> 2;					// find out next cycle count to run to
		
		if (i == 2 && CpsRunFrameMiddleCallbackFunction) {
			CpsRunFrameMiddleCallbackFunction();
		}

		if (SekTotalCycles() < nDisplayEnd && nNext > nDisplayEnd) {

			SekRun(nNext - nDisplayEnd);						// run 68K

			memcpy(CpsSaveReg[0], CpsReg, 0x100);				// Registers correct now

			SekSetIRQLine(Cps1VBlankIRQLine, CPU_IRQSTATUS_AUTO);				// Trigger VBlank interrupt

			if (Cps1DrawAtVblank && pBurnDraw) {
				CpsDraw();										// Draw frame
			}
		}

		SekRun(nNext - SekTotalCycles());						// run 68K
	}

	if (pBurnDraw && !Cps1DrawAtVblank) {
		CpsDraw();												// Draw frame
	}

	if (Cps1Qs == 1) {
		QsndEndFrame();
	} else {
		if (!Cps1DisablePSnd) {
			PsndSyncZ80(nCpsZ80Cycles); // sync z80
			PsndEndFrame();             // end frame (BurnTimer: z80)
			PsmUpdateEnd();             // render msm6295, ym2151
			ZetClose();
		}
	}
	
	if (CpsRunFrameEndCallbackFunction) {
		CpsRunFrameEndCallbackFunction();
	}

	nCpsCyclesExtra = SekTotalCycles() - nCpsCycles;

	SekClose();

	return 0;
}

INT32 Cps2Frame()
{
    static bool xm8634First=true;
    const bool xm8634Trace=xm8634First;
    xm8634First=false;
    auto xm8634Mark=[&](const char* msg){
      if(xm8634Trace){std::fprintf(stderr,"XM86.34 CPS2 %s\n",msg);std::fflush(stderr);}
    };
    xm8634Mark("ENTER");
	INT32 nDisplayEnd, nNext;									// variables to keep track of executed 68K cyles
	INT32 i;

	if (CpsReset) {
        xm8634Mark("BEFORE RESET");
		DrvReset();
	}
    xm8634Mark("AFTER RESET");

	if (Cps2Turbo) {
        xm8634Mark("BEFORE ASPECT");
		check_aspect();
	}
    xm8634Mark("AFTER ASPECT");

//	extern INT32 prevline;
//	prevline = -1;

    xm8634Mark("BEFORE CPU/QSOUND NEW FRAME");
	SekNewFrame();
	if (!Cps2DisableQSnd) QsndNewFrame();
    xm8634Mark("AFTER CPU/QSOUND NEW FRAME");

	nCpsCycles = (INT32)(((INT64)nCPS68KClockspeed * nBurnCPUSpeedAdjust) / 0x0100);
    xm8634Mark("BEFORE CPU OPEN");
	SekOpen(0);
	SekSetCyclesScanline(nCpsCycles / nCpsNumScanlines);
    xm8634Mark("AFTER CPU OPEN");

    xm8634Mark("BEFORE CPS INPUT READ");
    // XM87.152 DHC native command replay: submit exactly ONE 17-frame
    // motion/chord to the ORIGINAL CPS-2 input mapper, not to any guessed
    // 68000 action/script or permanently modified host SDL controls.
    // This runs only during a queued Ryu->Ken DHC *after* Ken's own ROM
    // tag exit returns to controllable neutral. Normal tags are inert.
    const char* dhcGame=BurnDrvGetTextA(DRV_NAME);
    const bool dhcGameMatches=dhcGame &&
         std::strncmp(dhcGame,"xmvsf",5)==0;
    // Once per process, compare the ACTUAL decrypted 68000 opcode view
    // against Morphus' guarded EURO hook bytes. No ROM/RAM writes.
    if(dhcGameMatches && !bBurnRunAheadFrame)
        XM87329HookProbe::state().frame(
            CpsCode,(unsigned)nCpsCodeLen,dhcGame);
    // 87.330 optional one-pass even-aligned signature search across CpsCode.
    // Never redirects 68000 execution or assumes a EURO address is valid.
    if(dhcGameMatches && !bBurnRunAheadFrame)
        XM87330Locator::state().frame(
            CpsCode,(unsigned)nCpsCodeLen,dhcGame);
    // 87.331: CpsCode's opcode words have opposite byte order from
    // big-endian M68K source guard strings. Scan normalized words ONLY.
    if(dhcGameMatches && !bBurnRunAheadFrame)
        XM87331Endian::state().frame(
            CpsCode,(unsigned)nCpsCodeLen,dhcGame);
    // 87.332: dump only small Motorola-endian opcode windows around
    // the DHC chain and incoming Hyper hypotheses; do not write code.
    if(dhcGameMatches && !bBurnRunAheadFrame)
        XM87332Context::state().frame(
            CpsCode,(unsigned)nCpsCodeLen,dhcGame);
    const bool xm262V4=dhcGameMatches && XM87262Game::version4();
    const bool xm274Unified=xm262V4 && XM87274Unified::state().enabled();
    const bool xm340Native=xm274Unified && XM87340Donor::enabled();
    static bool xm340WasActive=false;
    if(xm340WasActive!=xm340Native){
        XM87340Donor::state().reset();
        XM87352CyclopsFx::state().reset();
    }
    xm340WasActive=xm340Native;
    if(!xm340Native || !CpsRamFF ||
       XM87142::ramB(CpsRamFF,0x5000u)!=8u)
        XM87352CyclopsFx::state().reset();
    // 87.342: CREATE the existing DHC TSV on startup, not only after a
    // successful motion/Hyper claim. Silence used to mean both "no
    // command" and "an unexecuted/disabled native port".
    static bool xm342DhcBootLogged=false;
    static unsigned xm342DhcLastFrame=~0u;
    const unsigned xm342Now=(unsigned)GetCurrentFrame();
    if(xm342DhcLastFrame!=~0u && xm342Now<xm342DhcLastFrame)
        xm342DhcBootLogged=false;
    xm342DhcLastFrame=xm342Now;
    if(!bBurnRunAheadFrame && dhcGameMatches &&
       (std::getenv("XM87274_UNIFIED_DHC") ||
        std::getenv("XM87340_MORPHUS_NATIVE")) &&
       !xm342DhcBootLogged){
        const bool hook=XM87333NativeDHC::verify(
            CpsCode,(unsigned)nCpsCodeLen).all();
        const bool exit=XM87340Donor::validateExit(
            CpsCode,(unsigned)nCpsCodeLen);
        char boot[210];
        std::snprintf(boot,sizeof(boot),
            "NATIVE_342_BOOT version4=%u unified=%u donor=%u driver=%s US_hooks=%u exit_site=%u ROM=%u RAM=%u",
            xm262V4?1u:0u,xm274Unified?1u:0u,
            xm340Native?1u:0u,dhcGame?dhcGame:"unknown",
            hook?1u:0u,exit?1u:0u,
            CpsRom?1u:0u,CpsRamFF?1u:0u);
        XM87274Unified::state().event(xm342Now,0u,boot);
        xm342DhcBootLogged=true;
    }
    static bool xm274WasUnified=false;
    if(xm274WasUnified!=xm274Unified){
        XM87144::state().reset();
        XM87274Unified::state().reset();
    }
    xm274WasUnified=xm274Unified;
    static bool xm262WasV4=false;
    if(xm262WasV4 && !xm262V4){
        XM87144::state().reset();
        XM87274Unified::state().reset();
        XM87121::state().reset();
        XM87220Dash::state().reset();
    }
    xm262WasV4=xm262V4;
    // 87.341: leave Morphus' original M68K tag handoff intact.
    // Only selection changes: the incoming fighter's actual motion+PP/KK
    // is resolved to its ORIGINAL +D6 Hyper byte before the CPU input.
    // No donor-assigned MP+MK assist chord, no synthetic controller 8.
    if(xm340Native && !bBurnRunAheadFrame && CpsRamFF &&
       dhcGame && std::strcmp(dhcGame,"xmvsfu")==0 &&
       CpsCode && XM87333NativeDHC::verify(
           CpsCode,(unsigned)nCpsCodeLen).all() &&
       XM87340Donor::validateExit(
           CpsCode,(unsigned)nCpsCodeLen) &&
       XM87142::ramB(CpsRamFF,0x5000u)==8u){
        const unsigned now=(unsigned)GetCurrentFrame();
        const unsigned dirs[2]={
            (CpsInp001[0]&1u)|((CpsInp001[1]&1u)<<1u)|
            ((CpsInp001[2]&1u)<<2u)|((CpsInp001[3]&1u)<<3u),
            (CpsInp000[0]&1u)|((CpsInp000[1]&1u)<<1u)|
            ((CpsInp000[2]&1u)<<2u)|((CpsInp000[3]&1u)<<3u)
        };
        const unsigned buttons[2]={
            (CpsInp001[4]&1u)|((CpsInp001[5]&1u)<<1u)|
            ((CpsInp001[6]&1u)<<2u)|((CpsInp011[0]&1u)<<3u)|
            ((CpsInp011[1]&1u)<<4u)|((CpsInp011[2]&1u)<<5u),
            (CpsInp000[4]&1u)|((CpsInp000[5]&1u)<<1u)|
            ((CpsInp000[6]&1u)<<2u)|((CpsInp011[4]&1u)<<3u)|
            ((CpsInp011[5]&1u)<<4u)|((CpsInp020[6]&1u)<<5u)
        };
        for(unsigned side=0u;side<2u;++side){
            const unsigned p=XM87193Exit::actorBase(side);
            const unsigned r=XM87193Exit::reserveBase(side);
            const unsigned who=XM87142::ramW(CpsRamFF,p+0x52u);
            const unsigned bench=XM87142::ramW(CpsRamFF,r+0x52u);
            const XM87184Life::Sample sample={
                who,XM87142::ramW(CpsRamFF,p+0x60u),
                XM87142::ramW(CpsRamFF,p+0xA0u),
                XM87142::ramW(CpsRamFF,p+0x08u),
                XM87142::ramL(CpsRamFF,p+0x34u),
                XM87142::ramB(CpsRamFF,p+0xD6u)};
            const auto& super=XM87189Family::state().side[side];
            const bool exactOldWindow=super.lastFrame!=~0u &&
                now>0u && super.lastFrame==now-1u &&
                super.unifiedDhcWindowAt(super.lastFrame,sample);
            // 87.355: OPEN Charlie QCB+KKK / Sabretooth HCB+KKK
            // during the verified original native Hyper startup and
            // continuation, not just the narrower attack sub-window.
            // preCpuActive demands the exact actor, current action,
            // D6 and observed original 68000 script from last frame.
            // It NEVER accepts an ordinary neutral/Flash Kick input.
            const bool requestedReverseHyper=
                bench==0x20u || bench==0x0Au;
            const bool currentWindow=exactOldWindow ||
                (requestedReverseHyper &&
                 super.preCpuActive(now,sample));
            // 87.349: original M68K Hyper activity is NOT only 2E.
            // Storm/Sabretooth/Bison/Charlie execute real 2C actions
            // under the same 2A controller+D6. The native ROM lifecycle
            // tracker separately verifies the outgoing 2C script window.
            const unsigned superAction=sample.action&0xFF00u;
            const bool hyper=(sample.mode&0xFF00u)==0x2A00u &&
                sample.d6 &&
                (superAction==0x2C00u || superAction==0x2E00u);
            const bool eligible=currentWindow && hyper &&
                XM87142::ramB(CpsRamFF,p+0x03u) &&
                !XM87142::ramB(CpsRamFF,p+0x221u) &&
                !XM87142::ramB(CpsRamFF,p+0x257u) &&
                !XM87142::ramB(CpsRamFF,0x500Au);
            auto& st=XM87340Donor::state().side[side];
            auto& oldCyclopsFx=XM87352CyclopsFx::state().side[side];
            const bool swappedCyclops=
                st.phase==XM87340Donor::COMMITTED &&
                st.point==0x02u && who==st.reserve && bench==0x02u;
            const unsigned retired=oldCyclopsFx.update(
                CpsRamFF,now,swappedCyclops,
                st.phase==XM87340Donor::IDLE && !oldCyclopsFx.committed,
                oldCyclopsFx.committed && now>oldCyclopsFx.at && who==0x02u);
            if(retired){
                char detail[128];
                std::snprintf(detail,sizeof(detail),
                    "NATIVE_352_CYCLOPS_SOB_ORPHAN_HELPERS_RETIRED side=%u count=%u",
                    side+1u,retired);
                XM87274Unified::state().event(now,side,detail);
            }
            const auto before=st.phase;
            const unsigned freshButtons=buttons[side]&~st.prev;
            const unsigned chord=buttons[side]&0x3Fu;
            const bool nativeChord=
                XM87334Native::qualifiedChord(chord);
            const bool newChord=nativeChord && (freshButtons&chord);
            const unsigned oldHidden=st.hidden;
            // +$210 is the ORIGINAL actor life. A one-character team
            // or a defeated sole reserve has no living partner. Zero
            // as a character ID means Wolverine, NOT an empty slot.
            // Never intercept the genuine P1/P2 Hyper inputs in 1v1.
            const unsigned reserveLife=XM87142::ramW(CpsRamFF,r+0x210u);
            const bool livingReserve=
                XM87340Donor::genuineReserve(who,bench,reserveLife);
            // 87.357: no Charlie-only native tracker/previous-frame
            // veto while READING his QCB+KKK. In a genuine native
            // 2A/2C or 2A/2E Hyper, track the physical input regardless
            // of the final DHC cancel flags and stock. The actual tag
            // STILL needs eligible, living reserve and meter.
            const bool charlieNativeSuper=bench==0x20u && hyper;
            if(st.phase==XM87340Donor::IDLE){
                if((!eligible && !charlieNativeSuper) ||
                   !livingReserve ||
                   (!XM87142::ramB(CpsRamFF,p+0x214u) &&
                    !charlieNativeSuper)){
                    st.quiet(now,who,buttons[side]);
                    continue;
                }
                if(!st.windowReady){
                    st.openWindow(now,who,buttons[side],bench,
                        dirs[side],
                        (XM87142::ramB(CpsRamFF,p+0x4Bu)&1u)!=0u);
                    continue;
                }
            }
            if(!livingReserve &&
               st.phase!=XM87340Donor::CHAIN_ENTERED &&
               st.phase!=XM87340Donor::COMMITTED){
                if(st.phase!=XM87340Donor::IDLE || st.hidden)
                    st.reset();
                st.hidden=0u;
                if(newChord){
                    char solo[184];
                    std::snprintf(solo,sizeof(solo),
                        "NATIVE_346_SOLO_NATIVE_INPUT_UNTOUCHED side=%u point=%02X reserveId=%02X reserveLife=%u buttons=%02X dir=%X",
                        side+1u,who,bench,reserveLife,chord,dirs[side]);
                    XM87274Unified::state().event(now,side,solo);
                }
                st.prev=chord;
                st.lastFrame=now;
                continue;
            }
            if(st.phase==XM87340Donor::IDLE){
                const bool cueBefore=st.charlieQueued;
                st.sample(now,who,bench,dirs[side],buttons[side],
                    (XM87142::ramB(CpsRamFF,p+0x4Bu)&1u)!=0u,
                    sample.d6,hyper,eligible,
                    XM87142::ramW(CpsRamFF,p+0x0Cu),
                    XM87142::ramB(CpsRamFF,p+0x214u));
                if(!cueBefore && st.charlieQueued){
                    char event[140];
                    std::snprintf(event,sizeof(event),
                        "NATIVE_357_CHARLIE_QCB_KKK_CUE_HELD actor=%02X reserve=%02X outgoingD6=%02X",
                        who,bench,sample.d6);
                    XM87274Unified::state().event(now,side,event);
                }
            }else{
                st.pendingInput(now,buttons[side]);
            }
            if(before!=XM87340Donor::CLAIMED &&
               st.phase==XM87340Donor::CLAIMED &&
               who==0x02u && sample.d6==0x02u &&
               superAction==0x2C00u){
                oldCyclopsFx.begin(CpsRamFF,now,p,r);
                char fx[128];
                std::snprintf(fx,sizeof(fx),
                    "NATIVE_352_CYCLOPS_SOB_HELPERS_TRACKED side=%u count=%u",
                    side+1u,oldCyclopsFx.n);
                XM87274Unified::state().event(now,side,fx);
            }
            if(before!=XM87340Donor::CLAIMED &&
               st.phase==XM87340Donor::CLAIMED &&
               st.claimedQueuedCharlie){
                char event[144];
                std::snprintf(event,sizeof(event),
                    "NATIVE_357_CHARLIE_QCB_KKK_DEFERRED_CLAIM incoming=%02X d6=%02X side=%u originalInput=38",
                    st.reserve,st.d6,side+1u);
                XM87274Unified::state().event(now,side,event);
            }
            if(before!=XM87340Donor::CLAIMED &&
               st.phase==XM87340Donor::CLAIMED &&
               st.claimedBufferedKKK){
                char event[144];
                std::snprintf(event,sizeof(event),
                    "NATIVE_353_BUFFERED_KKK_CLAIM incoming=%02X d6=%02X side=%u motion=%u",
                    st.reserve,st.d6,side+1u,st.selected+1u);
                XM87274Unified::state().event(now,side,event);
            }
            if(before!=XM87340Donor::CLAIMED &&
               st.phase==XM87340Donor::CLAIMED &&
               (st.selected==7u || st.selected==8u ||
                st.selected==9u)){
                char input[176];
                const char* tag=st.selected==7u?
                    "NATIVE_346_ORIGINAL_F_DF_D_PUNCH":
                    st.selected==8u?
                    "NATIVE_350_FINAL_ATOMIC_BUSTER_360":
                    "NATIVE_350_RAGING_DEMON_LP_LP_F_LK_HP";
                std::snprintf(input,sizeof(input),
                    "%s incoming=%02X d6=%02X side=%u physical=%02X",
                    tag,st.reserve,st.d6,side+1u,st.hidden);
                XM87274Unified::state().event(now,side,input);
            }
            // 87.348: The physical input and its actual incoming D6
            // ARE the authorization. The old 87.344/345 read-only proof
            // checker denied eleven genuine commands in the last match
            // before ANY M68K tag could run. Never wipe a valid claim
            // merely because the fighter uses different native phase
            // opcodes. The bridge itself will try the real ROM route.
            if(before!=XM87340Donor::CLAIMED &&
               st.phase==XM87340Donor::CLAIMED){
                const auto trial=XM87336Dispatch::resolveTrial(
                    CpsCode,(unsigned)nCpsCodeLen,
                    CpsRom,(unsigned)nCpsRomLen,st.reserve,st.d6);
                char route[192];
                std::snprintf(route,sizeof(route),
                    "NATIVE_348_INPUT_ALLOWED fighter=%02X d6=%02X side=%u kind=%u pc=%06X handler=%06X selector=%02X script=%06X",
                    st.reserve,st.d6,side+1u,unsigned(trial.kind),
                    trial.dispatcher,trial.handler,trial.selector,
                    trial.script);
                XM87274Unified::state().event(now,side,route);
                // NO pre-tag cancellation: keep D6, button ownership,
                // and the REAL original 68000 outgoing state intact.
            }
            if(newChord && before==XM87340Donor::IDLE &&
               st.phase!=XM87340Donor::CLAIMED){
                // 87.349: 18 inputs in the 87.348 capture had a
                // genuine active Super, valid DHC window and meter,
                // but the exact incoming motion was not completed.
                // Report the eight ORIGINAL motion trackers instead
                // of guessing new buttons or blaming native 2C code.
                unsigned complete=0u,partial=0u;
                const auto* motions=XM87340Donor::motions();
                for(unsigned i=0u;i<8u;++i){
                    if(!st.timer[i] || !st.step[i])continue;
                    if(st.step[i]>=motions[i].len)complete|=1u<<i;
                    else if(st.step[i]>=2u)partial|=1u<<i;
                }
                char reject[352];
                const bool faceRight=
                    (XM87142::ramB(CpsRamFF,p+0x4Bu)&1u)!=0u;
                const unsigned relativeDir=
                    XM87340Donor::facingDir(dirs[side],faceRight);
                std::snprintf(reject,sizeof(reject),
                    "NATIVE_352_COMMAND_NOT_CLAIMED actor=%02X reserve=%02X buttons=%02X fresh=%02X dir=%X rel=%X right=%u activeHyper=%u window=%u eligible=%u alive=%u gate221=%u gate257=%u gate500A=%u stock=%u phase=%u hidden=%02X complete=%02X partial=%02X",
                    who,bench,chord,freshButtons,dirs[side],relativeDir,
                    faceRight?1u:0u,hyper?1u:0u,currentWindow?1u:0u,
                    eligible?1u:0u,
                    XM87142::ramB(CpsRamFF,p+0x03u)?1u:0u,
                    XM87142::ramB(CpsRamFF,p+0x221u)?1u:0u,
                    XM87142::ramB(CpsRamFF,p+0x257u)?1u:0u,
                    XM87142::ramB(CpsRamFF,0x500Au)?1u:0u,
                    XM87142::ramB(CpsRamFF,p+0x214u),
                    unsigned(super.phase),oldHidden,complete,partial);
                XM87274Unified::state().event(now,side,reject);
            }
            if(before!=st.phase && st.phase==XM87340Donor::CLAIMED){
                char detail[144];
                std::snprintf(detail,sizeof(detail),
                    "NATIVE_341_REAL_SUPER_INPUT point=%02X reserve=%02X motion=%u chord=%02X d6=%02X",
                    st.point,st.reserve,st.selected+1u,st.hidden,st.d6);
                XM87274Unified::state().event(now,side,detail);
            }
            if(st.phase==XM87340Donor::CLAIMED &&
               now>st.claimedAt+12u){
                XM87274Unified::state().event(now,side,
                    "MORPHUS_340_ORIGINAL_TAG_HOOK_NOT_REACHED");
                st.phase=XM87340Donor::IDLE;++st.refusals;
            }
            if(st.phase==XM87340Donor::CHAIN_ENTERED &&
               now>st.enteredAt+90u){
                XM87274Unified::state().event(now,side,
                    "MORPHUS_340_ORIGINAL_NATIVE_SWAP_NOT_CONFIRMED");
                st.phase=XM87340Donor::IDLE;++st.refusals;
            }
            if(st.phase==XM87340Donor::COMMITTED){
                // Independent observation of the ACTUAL next original
                // fighter update; redirecting the PC is not success.
                if(!st.confirmed && who==st.reserve &&
                   bench==st.point && hyper && sample.d6==st.d6){
                    st.confirmed=true;
                    char detail[194];
                    if(superAction==0x2C00u)
                        std::snprintf(detail,sizeof(detail),
                            "MORPHUS_349_ACTUAL_US_2C_HYPER d6=%02X action=%04X script=%06X stock_before=%u stock_now=%u",
                            st.d6,sample.action,sample.script,st.stock,
                            XM87142::ramB(CpsRamFF,p+0x214u));
                    else
                        std::snprintf(detail,sizeof(detail),
                            "MORPHUS_340_ACTUAL_US_2E_HYPER d6=%02X stock_before=%u stock_now=%u",
                            st.d6,st.stock,
                            XM87142::ramB(CpsRamFF,p+0x214u));
                    XM87274Unified::state().event(now,side,detail);
                }
                if(!st.confirmed && now>st.committedAt+16u){
                    XM87274Unified::state().event(now,side,
                        "MORPHUS_349_INCOMING_GUEST_NO_ORIGINAL_2C_OR_2E");
                    ++st.refusals;
                    st.phase=XM87340Donor::IDLE;
                }
                // Donor single-use participation lasts through the
                // incoming Hyper. Only the real neutral state re-arms it.
                if(st.confirmed && !hyper &&
                   !XM87142::ramB(CpsRamFF,p+0x256u) &&
                   !XM87142::ramB(CpsRamFF,p+0x259u))
                    st.reset();
            }
        }
    }
    // 87.181: observer log files must be initialized when the XMVSF
    // CPS2 driver starts, NOT only from the late post-CPU DHC hook.
    // Otherwise an early game/menu exit silently creates no TSV at all.
    // Health also records driver/ROM/RAM/combat gate decisions, without
    // changing the original game or input processing.
    if(!bBurnRunAheadFrame){
        if(dhcGameMatches){
            XM87175Observer::state().init();
            XM87180Subframe::state().init();
        }
        XM87181Health::state().frame((unsigned)GetCurrentFrame(),
            dhcGame,CpsRamFF,CpsRom,
            XM87175Observer::state().enabled,
            XM87180Subframe::state().enabled);
    }
    // 87.182 opt-in: the original 68000 exposes executed PPC after each
    // instruction. Hook only the already armed Ryu->Ken native tag frame.
    // The observer wraps/restores any existing Musashi callback; it never
    // changes the ROM, input port, actor RAM, or CPU cycle budget.
    // 87.186 records genuine super entry BEFORE the original CPU runs.
    // On this first sampled full frame, the bounded original PPC
    // observer may now begin even when controller 8 is NOT armed.
    // It traces initial startup frames, actual startup punch edges and
    // the first 25 original attack frames. No command is submitted.
    if(dhcGameMatches && !bBurnRunAheadFrame){
        // Physical punch state is available before CpsRwGetInp.
        // Do not infer it from native C0/CA one frame too late.
        const unsigned physicalOpeningPunches=
            (CpsInp001[4]&1u)|((CpsInp001[5]&1u)<<1u)|
            ((CpsInp001[6]&1u)<<2u);
        XM87186Opening::state().sample(CpsRamFF,
            (unsigned)GetCurrentFrame(),"PRE_CPU",
            (unsigned)XM87144::state().phase[0],
            0u,physicalOpeningPunches);
    }
    if(dhcGameMatches && !bBurnRunAheadFrame)
        XM87182PC::state().begin(CpsRamFF,(unsigned)GetCurrentFrame(),
             (unsigned)XM87144::state().phase[0],
             XM87144::state().earlyCancel,false,
             XM87186Opening::state().wantsPcTrace(
                (unsigned)GetCurrentFrame()));
    XM87144::State::InputMask dhcInput=
        (!bBurnRunAheadFrame && xm262V4 && !xm274Unified)?
        XM87144::state().inputForFrame(CpsRamFF,(unsigned)GetCurrentFrame()):
        XM87144::State::InputMask();
    // 87.178: the 87.177 complete 3P shield removed the punch edge that
    // the original 68000 controller-8 tag path requires. In the 87.177
    // capture, native LP+MP / LP+HP / MP+HP all entered 2A00/3200;
    // native zero punches on QCF+3P instead entered 0200/0002.
    // Normalize the prequeue 3P port to canonical LP+MP, NOT neutral.
    // This keeps the already-proven native edge without exposing 3P dash.
    // The original physical button array is restored after CpsRwGetInp,
    // allowing the host to recognize QCF+3P normally.
    bool xm87177PrequeueThreePunch=false;
    bool xm87193PrequeueKickToTagPunch=false;
    if(!dhcInput.active && !xm274Unified && !bBurnRunAheadFrame && xm262V4 && CpsRamFF){
        const unsigned physicalPunches=
            (CpsInp001[4]&1u)|((CpsInp001[5]&1u)<<1u)|
            ((CpsInp001[6]&1u)<<2u);
        const unsigned physicalKicks=
            ((CpsInp011[0]&1u)<<3u)|
            ((CpsInp011[1]&1u)<<4u)|
            ((CpsInp011[2]&1u)<<5u);
        const unsigned point=XM87142::ramW(CpsRamFF,0x4052u);
        const unsigned bench=XM87142::ramW(CpsRamFF,0x4852u);
        const unsigned action=XM87142::ramW(CpsRamFF,0x4060u);
        const unsigned mode=XM87142::ramW(CpsRamFF,0x40A0u);
        const XM87184Life::Sample stageBeforeCpu={
            point,action,mode,XM87142::ramW(CpsRamFF,0x4008u),
            XM87142::ramL(CpsRamFF,0x4034u),
            XM87142::ramB(CpsRamFF,0x40D6u)};
        // On a verified *nonterminal* Shinkuu recovery script,
        // 3P must also normalize to native 2P. Never normalize it
        // after the actual action has ended or at terminal 0D3194.
        const bool genuineRecoveryWindow=
            XM87184Life::identify(stageBeforeCpu)==XM87184Life::RECOVERY &&
            XM87185Gate::canRecognize(stageBeforeCpu);
        // Preserve the original 87.177 3P-as-2P input normalization
        // during the ENTIRE actual native Shinkuu action, not only
        // the short controller-8 handoff window. 87.184 accidentally
        // leaked physical 3P (dash) during startup. Still reject
        // terminal 0D3194 and action exit. This is input shielding;
        // whether a tag is safe is separately decided in XM87144.
        // Original-ROM 2E super on either ground or in air: the
        // pre-CPU controller port should see canonical native 2P,
        // never the 3P dash. Match the same actor, root, script and
        // current/previous host frame; do not shield after super exit.
        const bool nativeFamilyPreCpu=XM87144::state().familyTest &&
            XM87189Family::state().side[0].preCpuActive(
                (unsigned)GetCurrentFrame(),stageBeforeCpu);
        xm87177PrequeueThreePunch=
            (XM87185Gate::canRecognize(stageBeforeCpu) ||
             nativeFamilyPreCpu) &&
            XM87177Chords::shieldPrequeue3P(
                XM87144::state().enabled,XM87144::state().earlyCancel,
                unsigned(XM87144::state().phase[0]),
                XM87140Chat::state().candidate[0],
                point,bench,action,mode,physicalPunches|physicalKicks,
                genuineRecoveryWindow,nativeFamilyPreCpu);
        // 87.193: 87.192.zip shows 6/6 PP DHC handoffs but both
        // reserve-owned KK requests fail before Ryu exits. Native C0
        // becomes 0301 (LK+MK) and selects 0214 / 0002, even though
        // the FOLLOWING frame's host port is neutralized. Thus the
        // outgoing controller's input MUST be normalized BEFORE the
        // current native CpsRwGetInp; the host/reserve still sees the
        // untouched original QCF+KK after temporary port restoration.
        // One reusable protocol maps 2K/3K -> original native 2P tag
        // edge. Existing punch-pair/3P behavior is unchanged.
        const auto outgoingPlan=XM87193Exit::beforeCpu(
            XM87144::state().deterministicExitTest &&
            XM87144::state().enabled && XM87144::state().earlyCancel &&
            point==0x12u && bench==0x14u &&
            XM87140Chat::state().candidate[0],
            XM87144::state().phase[0]==XM87144::IDLE,
            nativeFamilyPreCpu,physicalPunches|physicalKicks);
        xm87193PrequeueKickToTagPunch=outgoingPlan.active &&
            (outgoingPlan.originalButtons&0x38u)!=0u &&
            (outgoingPlan.originalButtons&0x07u)==0u;
        if(xm87177PrequeueThreePunch || xm87193PrequeueKickToTagPunch){
            dhcInput.active=true;
            dhcInput.isolated=false; // canonical native 2P, not neutral
            dhcInput.attacks=xm87193PrequeueKickToTagPunch?
                outgoingPlan.nativeButtons:
                XM87177Chords::nativeKenTwoPunchMask();
            dhcInput.directions=
                (CpsInp001[0]&1u)|((CpsInp001[1]&1u)<<1u)|
                ((CpsInp001[2]&1u)<<2u)|((CpsInp001[3]&1u)<<3u);
        }
    }
    // 87.274: replay through either ORIGINAL CPS2 input port. All host
    // arrays are restored immediately after CpsRwGetInp.
    XM87274Unified::Input unifiedInput[2];
    if(xm274Unified && !bBurnRunAheadFrame && CpsRamFF){
        const unsigned dirs[2]={
            (CpsInp001[0]&1u)|((CpsInp001[1]&1u)<<1u)|
            ((CpsInp001[2]&1u)<<2u)|((CpsInp001[3]&1u)<<3u),
            (CpsInp000[0]&1u)|((CpsInp000[1]&1u)<<1u)|
            ((CpsInp000[2]&1u)<<2u)|((CpsInp000[3]&1u)<<3u)
        };
        const unsigned buttons[2]={
            (CpsInp001[4]&1u)|((CpsInp001[5]&1u)<<1u)|
            ((CpsInp001[6]&1u)<<2u)|((CpsInp011[0]&1u)<<3u)|
            ((CpsInp011[1]&1u)<<4u)|((CpsInp011[2]&1u)<<5u),
            (CpsInp000[4]&1u)|((CpsInp000[5]&1u)<<1u)|
            ((CpsInp000[6]&1u)<<2u)|((CpsInp011[4]&1u)<<3u)|
            ((CpsInp011[5]&1u)<<4u)|((CpsInp020[6]&1u)<<5u)
        };
        for(unsigned side=0u;side<2u;++side){
            if(xm340Native){
                // Hide ONLY the actually selected original PP/KK
                // Hyper command once claimed; preserve every other input.
                const unsigned h=XM87340Donor::state().side[side].hidden;
                unifiedInput[side].active=(h!=0u);
                unifiedInput[side].directions=dirs[side];
                unifiedInput[side].attacks=buttons[side]&~h;
            }else{
                unifiedInput[side]=XM87274Unified::state().inputForFrame(
                    CpsRamFF,side,(unsigned)GetCurrentFrame(),
                    dirs[side],buttons[side],
                    XM87140Chat::state().candidate[side]);
            }
        }
    }
    UINT8 uSavedP1Dir[4]={},uSavedP1P[3]={},uSavedP1K[3]={};
    UINT8 uSavedP2Dir[4]={},uSavedP2P[3]={},uSavedP2K[3]={};
    if(unifiedInput[0].active){
        for(unsigned k=0;k<4u;++k){
            uSavedP1Dir[k]=CpsInp001[k];
            CpsInp001[k]=(unifiedInput[0].directions>>k)&1u;
        }
        for(unsigned k=0;k<3u;++k){
            uSavedP1P[k]=CpsInp001[4u+k];uSavedP1K[k]=CpsInp011[k];
            CpsInp001[4u+k]=(unifiedInput[0].attacks>>k)&1u;
            CpsInp011[k]=(unifiedInput[0].attacks>>(3u+k))&1u;
        }
    }
    if(unifiedInput[1].active){
        for(unsigned k=0;k<4u;++k){
            uSavedP2Dir[k]=CpsInp000[k];
            CpsInp000[k]=(unifiedInput[1].directions>>k)&1u;
        }
        for(unsigned k=0;k<3u;++k){
            uSavedP2P[k]=CpsInp000[4u+k];
            CpsInp000[4u+k]=(unifiedInput[1].attacks>>k)&1u;
        }
        uSavedP2K[0]=CpsInp011[4];uSavedP2K[1]=CpsInp011[5];
        uSavedP2K[2]=CpsInp020[6];
        CpsInp011[4]=(unifiedInput[1].attacks>>3u)&1u;
        CpsInp011[5]=(unifiedInput[1].attacks>>4u)&1u;
        CpsInp020[6]=(unifiedInput[1].attacks>>5u)&1u;
    }
    UINT8 dhcSavedDir[4]={},dhcSavedPunch[3]={},dhcSavedKick[3]={};
    if(dhcInput.active){
        for(unsigned k=0;k<4u;++k){
            dhcSavedDir[k]=CpsInp001[k];
            CpsInp001[k]=(dhcInput.directions>>k)&1u;
        }
        for(unsigned k=0;k<3u;++k){
            dhcSavedPunch[k]=CpsInp001[4u+k];
            dhcSavedKick[k]=CpsInp011[k];
            CpsInp001[4u+k]=(dhcInput.attacks>>k)&1u;
            CpsInp011[k]=(dhcInput.attacks>>(k+3u))&1u;
        }
    }
	CpsRwGetInp();											// Update the input port values
    if(unifiedInput[0].active){
        for(unsigned k=0;k<4u;++k)CpsInp001[k]=uSavedP1Dir[k];
        for(unsigned k=0;k<3u;++k){
            CpsInp001[4u+k]=uSavedP1P[k];CpsInp011[k]=uSavedP1K[k];
        }
    }
    if(unifiedInput[1].active){
        for(unsigned k=0;k<4u;++k)CpsInp000[k]=uSavedP2Dir[k];
        for(unsigned k=0;k<3u;++k)CpsInp000[4u+k]=uSavedP2P[k];
        CpsInp011[4]=uSavedP2K[0];CpsInp011[5]=uSavedP2K[1];
        CpsInp020[6]=uSavedP2K[2];
    }
    if(dhcInput.active){
        // Native input bytes have already been sampled. Restore physical
        // controller state before the remaining emulator frame runs.
        for(unsigned k=0;k<4u;++k)CpsInp001[k]=dhcSavedDir[k];
        for(unsigned k=0;k<3u;++k){
            CpsInp001[4u+k]=dhcSavedPunch[k];
            CpsInp011[k]=dhcSavedKick[k];
        }
    }
    // XM87.159: clear offensive collision only on OUTGOING Ryu's linked
    // projectile objects before native 68000 collision evaluation.
    // Normal P1 projectiles, P2 objects and Ken's new projectiles are
    // never touched. The image/animation objects remain alive.
    if(!bBurnRunAheadFrame && xm262V4 && !xm274Unified && CpsRamFF)
        XM87144::state().oldRyuProjectiles.beforeCPU(
            CpsRamFF,(unsigned)GetCurrentFrame());
    // 87.197: freeze only the non-incoming fighter while Ken's native
    // ROM 0212 fall and stored 2E input keep running at full CPS speed.
    // No global CPU pause, guessed CPS-wide freeze flag or audio reset.
    if(!bBurnRunAheadFrame && xm262V4 && !xm274Unified && CpsRamFF &&
       XM87144::state().cinematicFreezeTest)
        XM87144::state().cinematicFreeze.beforeCPU(
            CpsRamFF,unsigned(XM87144::state().phase[0]));
    // XM87.262: Version-4-only dizzy initializer and native state handler; Tournament does not affect this patch.
    XM8795Native::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),(unsigned)GetCurrentFrame(),
        xm262V4,
        xm262V4);
    // XM87.96: native dizzy stand initializer -> ROM normal recovery.
    XM8796Native::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),(unsigned)GetCurrentFrame(),
        xm262V4,
        xm262V4,
        XM8795Native::state().installed);
    // XM87.99: native ROM opcode hook. Parent 95 and 96 must be installed.
    XM8799Ground::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),(unsigned)GetCurrentFrame(),
        xm262V4,
        xm262V4,
        XM8795Native::state().installed,XM8796Native::state().installed);
    // Version 4's native installation is audited independently of the
    // Tournament knockdown-only selection.
    // Read-only; do not bypass any original-ROM signature verification.
    if(!bBurnRunAheadFrame)
        XM87149Rules::state().frame((unsigned)GetCurrentFrame(),
            BurnDrvGetTextA(DRV_NAME),
            xm262V4,
            XM8795Native::state().installed,
            XM8796Native::state().installed,
            XM8799Ground::state().installed);
    // XM87.125: Version 4 only; requires verified native dizzy patch ownership.
    XM87125Stun::state().before(CpsRamFF,(unsigned)GetCurrentFrame(),
        xm262V4 &&
        XM8795Native::state().installed && XM8796Native::state().installed);
    // 87.264: Regenerate is a Training safety rule in BOTH game versions.
    // It clamps original lethal ROM damage to one HP so the existing
    // 120-idle-frame refill can occur. "KO Enabled" restores native KO.
    // Unlike the V4 combat patches, this follows Training's damage option.
    XM87100TrainingKO::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),
        xm14TrainingMode!=0 && xm87102DamageMode==0);
    xm8634Mark("AFTER CPS INPUT READ");

	if (CpsRunFrameStartCallbackFunction) {
		CpsRunFrameStartCallbackFunction();
	}
	
	// Check the volumes every 5 frames or so
    xm8634Mark("BEFORE VOLUME/ROUTE");
	if (GetCurrentFrame() % 5 == 0) {
		if (Cps2VolUp) Cps2Volume++;
		if (Cps2VolDwn) Cps2Volume--;
		
		if (Cps2Volume > 39) Cps2Volume = 39;
		if (Cps2Volume < 0) Cps2Volume = 0;

		double Cps2Amp = 1.00;

		if (strstr(BurnDrvGetTextA(DRV_NAME), "ddsom")) {
			Cps2Amp = 2.0;
		}

		QscSetRoute(BURN_SND_QSND_OUTPUT_1, (Cps2Volume / 39.0) * Cps2Amp, BURN_SND_ROUTE_LEFT);
		QscSetRoute(BURN_SND_QSND_OUTPUT_2, (Cps2Volume / 39.0) * Cps2Amp, BURN_SND_ROUTE_RIGHT);
	}
	
    xm8634Mark("AFTER VOLUME/ROUTE");
	nDisplayEnd = nCpsCycles * (nFirstLine + 224) / nCpsNumScanlines;	// Account for VBlank

	nInterrupt = 0;
	for (i = 0; i < MAX_RASTER + 2; i++) {
		nRasterline[i] = 0;
	}

	// Determine which (if any) of the line counters generates the first IRQ
	bEnableAutoIrq50 = bEnableAutoIrq52 = false;
	nIrqLine50 = nIrqLine52 = nCpsNumScanlines;
	if (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x50))) & 0x8000) {
		bEnableAutoIrq50 = true;
	}
	if (bEnableAutoIrq50 || (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x4E))) & 0x0200) == 0) {
		nIrqLine50 = (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x50))) & 0x01FF);
	}
	if (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x52))) & 0x8000) {
		bEnableAutoIrq52 = true;
	}
	if (bEnableAutoIrq52 || (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x4E))) & 0x0200) == 0) {
		nIrqLine52 = (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x52))) & 0x01FF);
	}
	ScheduleIRQ();
    xm8634Mark("AFTER IRQ SCHEDULE");
    // 87.180 native intra-frame observation; the previous 87.179
    // prime occasionally loses its fresh CA between CPU scheduler slices.
    // Never modify actor RAM, original 68000 opcodes, or controller ports.
    auto xm87180Sample=[&](const char* stage){
        if(!bBurnRunAheadFrame && dhcGameMatches && CpsRamFF)
            XM87180Subframe::state().sample(
                CpsRamFF,(unsigned)GetCurrentFrame(),
                (unsigned)SekTotalCycles(),stage,
                unsigned(XM87144::state().phase[0]),
                XM87140Chat::state().candidate[0],
                dhcInput.active,dhcInput.directions,dhcInput.attacks,
                (unsigned)Cpi01A,(unsigned)Cpi01C,(unsigned)Cpi01E);
    };
    xm87180Sample("BEFORE_CPU");
    // 87.224: executed 68000 PC / victim reaction+velocity deltas while
    // Charlie's actual 1804/1A04 is running. Read-only and bounded to
    // 32 CPU frames per move; wrap (never replace) the older DHC callback.
    // 87.347: this Charlie M68K PC observer is diagnostic only.
    // Do not install its nested instruction callback during native DHC.
    if(!bBurnRunAheadFrame && !xm340Native)
        XM87224CharliePC::state().begin(CpsRamFF,
            (unsigned)GetCurrentFrame(),dhcGameMatches,
            XM87262Game::tournamentKnockdowns(),false,
            CpsCode,(unsigned)nCpsCodeLen);
    // 87.328: only while F10 is ON and native XMVSF is PRE-combat,
    // capture the exact executed 68000 PPC that changes the native
    // chooser/versus transition RAM. Install AFTER existing CPU hooks
    // to preserve strict LIFO restoration and other observers.
    if(!bBurnRunAheadFrame)
        XM87139Probe::state().nativeBegin(CpsRamFF,
            (unsigned)GetCurrentFrame(),dhcGameMatches,false);
    // 87.335: installed LAST, before the CPU advances. A real 68000
    // swap and the exact incoming US PC are required to redirect fetch.
    // No original ROM patch, host-created tag or guessed Hyper action.
    if(!bBurnRunAheadFrame){
        XM87335Native::state().prepare(
            CpsCode,(unsigned)nCpsCodeLen,
            CpsRom,(unsigned)nCpsRomLen,CpsRamFF,dhcGame,
            (unsigned)GetCurrentFrame(),false);
        XM87335Native::state().begin(false);
    }

	SekIdle(nCpsCyclesExtra);
	nCpsCyclesExtra = 0;

    xm8634Mark("BEFORE PRE-DISPLAY CPU RUN");
	if (nIrqCycles < nCpsCycles * nFirstLine / nCpsNumScanlines) {
		SekRun(nIrqCycles);
		DoIRQ();
	}
	nNext = nCpsCycles * nFirstLine / nCpsNumScanlines;
	if (SekTotalCycles() < nNext) {
		SekRun(nNext - SekTotalCycles());
	}

    xm8634Mark("AFTER PRE-DISPLAY CPU RUN");
    xm87180Sample("AFTER_PRE_DISPLAY_CPU");
	CopyCpsReg(0);										// Get inititial copy of registers
	UnLatchCpsFrg();

	if (nIrqLine >= nCpsNumScanlines && (BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x4E))) & 0x0200) == 0) {
		nIrqLine50 = BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x50))) & 0x01FF;
		nIrqLine52 = BURN_ENDIAN_SWAP_INT16(*((UINT16*)(CpsReg + 0x52))) & 0x01FF;
		ScheduleIRQ();
	}

    xm8634Mark("BEFORE MAIN CPU SCANLINES");
	for (i = 0; i < 3; i++) {
		nNext = ((i + 1) * nDisplayEnd) / 3;			// find out next cycle count to run to

		while (nNext > nIrqCycles && nInterrupt < MAX_RASTER) {
			SekRun(nIrqCycles - SekTotalCycles());
			DoIRQ();
		}
		SekRun(nNext - SekTotalCycles());				// run cpu
        xm87180Sample(i==0?"AFTER_SCAN_0":(i==1?"AFTER_SCAN_1":"AFTER_SCAN_2"));
	}
	
    xm8634Mark("AFTER MAIN CPU SCANLINES");
	CpsObjGet();
    // XM87.139 raw native OAM snapshot, AFTER CPS2 fetched objects and BEFORE
    // DF recoloring. The recorder is read-only and runs only after F10 starts.
    if(!bBurnRunAheadFrame && XM87139Probe::state().active && CpsRam708 &&
       XM87139Probe::state().tagUntil >= (unsigned)GetCurrentFrame()) {
        const std::uint16_t* raw=reinterpret_cast<const std::uint16_t*>(
            CpsRam708+((nCpsObjectBank^1)<<15));
        unsigned activeCount=0u;
        for(;activeCount<0x400u;++activeCount){
            const unsigned yw=BURN_ENDIAN_SWAP_INT16(raw[activeCount*4u+1u]);
            const unsigned attr=BURN_ENDIAN_SWAP_INT16(raw[activeCount*4u+3u]);
            if((yw&0x8000u)||attr>=0xFF00u)break;
        }
        XM87139Probe::state().oamFrame((unsigned)GetCurrentFrame(),
            raw,activeCount,6u,6u,false);
    }
    xm8634Mark("AFTER OBJECT CAPTURE");										// Get objects

//	nCpsCyclesSegment[0] = (nCpsCycles * nVBlank) / nCpsNumScanlines;
//	nDone += SekRun(nCpsCyclesSegment[0] - nDone);

	SekSetIRQLine(2, CPU_IRQSTATUS_AUTO);				// VBlank
    xm8634Mark("BEFORE CPS DRAW");
	if (pBurnDraw) {
        XM8798Latency::state().beforeDraw();
		CpsDraw();
        XM8798Latency::state().afterDraw();
	}
    xm8634Mark("AFTER CPS DRAW");

	LatchCpsFrg();										// Latch the f-regs

    xm8634Mark("BEFORE CPU VBLANK RUN");
	SekRun(nCpsCycles - SekTotalCycles());
    xm8634Mark("AFTER CPU VBLANK RUN");
    xm87180Sample("AFTER_VBLANK_CPU");
    // Unwind the 87.335 direct native DHC callback FIRST (reverse install
    // order). F10/Charlie/87.182 callbacks remain in the previous order.
    XM87335Native::state().end();
    // F10 native transition logger stops first, without altering CPU cycles.
    // Includes a final summary when the same 68000 frame enters combat.
    XM87139Probe::state().nativeEnd();
    // Charlie's trace was never installed for native DHC; end() is a
    // harmless no-op for that mode. Preserve the original callback LIFO
    // when the observer is used by non-DHC Tournament tests.
    XM87224CharliePC::state().end();

	nCpsCyclesExtra = SekTotalCycles() - nCpsCycles;

    xm8634Mark("BEFORE QSOUND END FRAME");
	if (!Cps2DisableQSnd) QsndEndFrame();
    xm8634Mark("AFTER QSOUND END FRAME");
    // Restore the original instruction callback before closing CPU 0.
    // Only bounded original-68000 observations were buffered.
    XM87182PC::state().end();

	SekClose();
    // Live CPS-2 driver controls, not RAM-inferred move-strength labels.
    // P1: CpsInp001[4:6] punches, CpsInp011[0:2] kicks.
    // P2: CpsInp000[4:6] punches, CpsInp011[4:5] kicks, CpsInp020[6] HK.
    const unsigned xm8786_p1_buttons =
        (CpsInp001[4]&1u) | ((CpsInp001[5]&1u)<<1) | ((CpsInp001[6]&1u)<<2) |
        ((CpsInp011[0]&1u)<<3) | ((CpsInp011[1]&1u)<<4) | ((CpsInp011[2]&1u)<<5);
    const unsigned xm8786_p2_buttons =
        (CpsInp000[4]&1u) | ((CpsInp000[5]&1u)<<1) | ((CpsInp000[6]&1u)<<2) |
        ((CpsInp011[4]&1u)<<3) | ((CpsInp011[5]&1u)<<4) | ((CpsInp020[6]&1u)<<5);
    // TEST 87.223: observe native 68000 Storm HP hit velocity, reaction,
    // +E0 flight index and candidate A6 writers BEFORE the Tournament
    // 87.86 host flight conversion. This probe does not modify RAM/ROM.
    if(!bBurnRunAheadFrame){
        const char* nativeDriver=BurnDrvGetTextA(DRV_NAME);
        XM87223Flight::state().frame((unsigned)GetCurrentFrame(),CpsRamFF,
            CpsCode,(unsigned)nCpsCodeLen,
            nativeDriver && std::strncmp(nativeDriver,"xmvsf",5)==0,
            XM87262Game::tournamentKnockdowns(),
            xm8786_p1_buttons,xm8786_p2_buttons,
            xm340Native); // silence diagnostic capture, keep flight physics
    }
    // TEST 87.223: track Storm's original attack action independently of
    // Classic/Tournament. The Z80/QSound sample can arrive after this pose.
    if(!bBurnRunAheadFrame){
        const char* voiceDriver=BurnDrvGetTextA(DRV_NAME);
        XM87127Voice::state().observe((unsigned)GetCurrentFrame(),CpsRamFF,
            xm262V4 && voiceDriver && std::strncmp(voiceDriver,"xmvsf",5)==0);
    }
    XM8786Flight::state().frame((unsigned)GetCurrentFrame(),CpsRamFF,
        xm8786_p1_buttons,xm8786_p2_buttons);
    // Stage 87.140: inspect actual CPS2 controller bits AFTER native CPU run.
    // d_cps2.cpp: [0]=Right [1]=Left [2]=Down [3]=Up.
    const unsigned xm87140_p1_dir=
        (CpsInp001[0]&1u)|((CpsInp001[1]&1u)<<1u)|
        ((CpsInp001[2]&1u)<<2u)|((CpsInp001[3]&1u)<<3u);
    const unsigned xm87140_p2_dir=
        (CpsInp000[0]&1u)|((CpsInp000[1]&1u)<<1u)|
        ((CpsInp000[2]&1u)<<2u)|((CpsInp000[3]&1u)<<3u);
    // Never interpret other CPS-2 games through XMVSF's fighter directory.
    const char* xm87142Game=BurnDrvGetTextA(DRV_NAME);
    if(!bBurnRunAheadFrame && CpsRamFF && CpsRom && xm87142Game &&
       std::strncmp(xm87142Game,"xmvsf",5)==0){
        // 87.143: populate match-start queues for the two roster fighters.
        // Never run/replace a native super here: this only resolves data
        // tables from CpsRom. Normal tags do not rebuild the queued roster.
        const bool rosterPrepared=XM87143Queue::state().prepare(
            CpsRamFF,CpsRom,(unsigned)nCpsRomLen,(unsigned)GetCurrentFrame());
        if(rosterPrepared){
            for(unsigned side=0;side<2u;++side)
                for(unsigned slot=0;slot<2u;++slot){
                    const unsigned fighter=XM87143Queue::state().roster[side][slot];
                    for(unsigned i=0;i<4u;++i){
                        const XM87143Queue::Known* item=
                            XM87143Queue::definition(fighter,i);
                        if(!item)break;
                        const auto* e=XM87143Queue::state().find(
                            side,fighter,item->command);
                        unsigned resolved=0;
                        if(e)for(unsigned j=0;j<e->count;++j)
                            if(e->scripts[j].resolved)++resolved;
                        char note[XM87140Chat::CHAT_LENGTH];
                        std::snprintf(note,sizeof(note),
                            "P%u MATCH-START QUEUE %02X %s ROM=%u/%u",
                            side+1u,fighter,item->name,resolved,e?e->count:0u);
                        XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                            XM87140Chat::SYSTEM,note);
                    }
                }
        }
        // Native 68000 dispatch (PC 0x01038E) resolves ROM script via
        // fighter id, actor action and selector. Read only; no DHC trigger.
        const XM87142::Report romReport=XM87142::state().frame(
            CpsRamFF,CpsRom,(unsigned)nCpsRomLen,
            (unsigned)GetCurrentFrame(),false);
        if(xm340Native){
            // The older XM87144 gameplay frame is intentionally bypassed
            // in this mode. Keep its ORIGINAL, read-only M68K 2E lifecycle
            // tracker current for next frame's pre-CPU DHC window. Without
            // this update the input selector reads a permanently stale
            // phase and silently refuses all real PP/KK Hyper commands.
            for(unsigned side=0u;side<2u;++side){
                const unsigned p=XM87193Exit::actorBase(side);
                const XM87184Life::Sample observed={
                    XM87142::ramW(CpsRamFF,p+0x52u),
                    XM87142::ramW(CpsRamFF,p+0x60u),
                    XM87142::ramW(CpsRamFF,p+0xA0u),
                    XM87142::ramW(CpsRamFF,p+0x08u),
                    XM87142::ramL(CpsRamFF,p+0x34u),
                    XM87142::ramB(CpsRamFF,p+0xD6u)};
                XM87189Family::state().side[side].update(
                    (unsigned)GetCurrentFrame(),observed,
                    CpsRom,(unsigned)nCpsRomLen,true); // 87.349 2C opt-in
            }
        }
        // Exact post-CPU boundary, before host-side command matching
        // and before a possible DHC controller-8 write.
        XM87186Opening::state().sample(CpsRamFF,
            (unsigned)GetCurrentFrame(),"POST_CPU_PRE_HOST",
            (unsigned)XM87144::state().phase[0],
            xm87140_p1_dir,xm8786_p1_buttons);
        XM87140Chat::state().frame(CpsRamFF,(unsigned)GetCurrentFrame(),
            xm87140_p1_dir,xm87140_p2_dir,
            xm8786_p1_buttons,xm8786_p2_buttons,
            romReport.verifiedSuperMask,CpsRom,(unsigned)nCpsRomLen);
        // 87.309: read-only physical input evidence for experimental universal DHC.
        // The old chat only reported gestures AFTER a ROM super candidate.
        // Capture fresh physical button edges independently so a silent
        // candidate/rejection can be distinguished from missing controller input.
        // Never use this observer to drive a tag, alter CPS2 ports, or spend meter.
        {
            static unsigned lastPhysical[2]={0u,0u};
            static bool watching=false;
            static unsigned lastWatchFrame=~0u;
            // Disable the full-round physical-button DHC monitor when
            // native DHC is enabled. Only the window-gated parser may act.
            const bool active=xm274Unified && !xm340Native &&
                XM87142::ramB(CpsRamFF,0x5000u)==8u;
            const unsigned now=(unsigned)GetCurrentFrame();
            if(!active || (watching && now<lastWatchFrame)){
                watching=false;lastPhysical[0]=lastPhysical[1]=0u;
            }
            if(active){
                if(!watching){
                    watching=true;
                    XM87274Unified::state().event(now,0u,
                        "UNIFIED_DHC_PHYSICAL_INPUT_MONITOR_ACTIVE");
                    XM87140Chat::state().log(now,XM87140Chat::SYSTEM,
                        "UNIFIED M68K DHC ACTIVE: physical input monitor for P1/P2");
                }
                const unsigned buttons[2]={xm8786_p1_buttons,xm8786_p2_buttons};
                const unsigned directions[2]={xm87140_p1_dir,xm87140_p2_dir};
                for(unsigned side=0u;side<2u;++side){
                    const unsigned pressed=buttons[side]&~lastPhysical[side];
                    if(pressed){
                        char message[XM87140Chat::CHAT_LENGTH];
                        const bool nativeWindow=
                            XM87189Family::state().side[side].recognizes();
                        std::snprintf(message,sizeof(message),
                            "P%u DHC PHYSICAL INPUT read=%02X held=%02X dir=%X native_super=%u candidate=%u",
                            side+1u,pressed,buttons[side],directions[side],
                            (unsigned)nativeWindow,
                            (unsigned)XM87140Chat::state().candidate[side]);
                        XM87140Chat::state().log(now,XM87140Chat::INPUT,message);
                        XM87274Unified::state().event(now,side,message);
                    }
                    lastPhysical[side]=buttons[side];
                }
                lastWatchFrame=now;
            }
        }
        if(dhcInput.priming)
            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                XM87140Chat::SYSTEM,
                "DHC_179_NATIVE_2P_PORT_EDGE_PREPARED_BEFORE_CONTROLLER_08");
        if(xm87177PrequeueThreePunch)
            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                XM87140Chat::SYSTEM,
                "XM87.178_PREQUEUE_3P_NORMALIZED_TO_NATIVE_2P");
        if(xm87193PrequeueKickToTagPunch)
            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                XM87140Chat::MATCH,
                "DHC_193_OUTGOING_2K_TO_NATIVE_2P_BEFORE_CPU_RESERVE_KEEPS_ORIGINAL_KK");
        // 87.176: diagnostic only. The queued DHC takes exclusive control
        // of the P1 CPS input port during tag transfer, shielding the
        // physical chord from being consumed AGAIN by outgoing Ryu.
        // The host SDL input arrays are restored after CpsRwGetInp.
        if(dhcInput.isolated && xm8786_p1_buttons)
            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                XM87140Chat::SYSTEM,
                "XM87.176 DHC_PHYSICAL_CHORD_SHIELDED_DURING_NATIVE_HANDOFF");
        // Autonomous ROM inventory. No newly enumerated 2E action is
        // permitted to enter the DHC runtime until its command, owner and
        // exact native script are independently verified.
        // The one-shot TEST input injection is marked as synthetic; it
        // must NOT contaminate physical controller-command discovery.
        XM87160::state().exportPrivateCode(
            CpsCode,(unsigned)nCpsCodeLen,CpsRom,(unsigned)nCpsRomLen);
        XM87160::state().frame(
            CpsRamFF,CpsRom,(unsigned)nCpsRomLen,(unsigned)GetCurrentFrame(),
            dhcInput.active?dhcInput.directions:xm87140_p1_dir,
            xm87140_p2_dir,
            dhcInput.active?dhcInput.attacks:xm8786_p1_buttons,
            xm8786_p2_buttons,dhcInput.active,
            XM87162ReplaySync::movieFrame());
        for(unsigned i=0;i<romReport.count;++i){
            const XM87142::Event& e=romReport.event[i];
            char note[XM87140Chat::CHAT_LENGTH];
            std::snprintf(note,sizeof(note),
                "P%u %s actor=%02X bench=%02X move=%02X%02X ROM=%06X ptr=%06X",
                e.side+1u,XM87142::name(e.kind),e.pointId,e.reserveId,
                e.action,e.selector,e.tableScript,e.partnerPointer);
            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                e.kind==XM87142::NATIVE_SUPER_ENTRY?XM87140Chat::MATCH:XM87140Chat::SYSTEM,
                note);
        }
        // 87.144 gameplay affecting TEST is explicitly opt-in, strictly
        // Ryu->Ken P1 DHC command in active native super, never normal tags.
        const unsigned dhcWindow=(XM87140Chat::state().candidate[0]?1u:0u) |
                                 (XM87140Chat::state().candidate[1]?2u:0u);
        const XM87144::Phase phaseBefore=XM87144::state().phase[0];
        // 87.175 observer: snapshots AFTER native 68000 CPU execution but
        // BEFORE DHC's host frame callback can write state. Opt-in only.
        XM87175Observer::state().postCpu(
            CpsRamFF,(unsigned)GetCurrentFrame(),
            XM87162ReplaySync::movieFrame(),XM87144::phaseName(phaseBefore),
            dhcInput.active,dhcInput.directions,dhcInput.attacks);
        if(xm262V4 && !xm274Unified)
            XM87144::state().frame(CpsRamFF,CpsRom,(unsigned)nCpsRomLen,
                (unsigned)GetCurrentFrame(),dhcWindow);
        if(xm274Unified && !xm340Native)
            XM87274Unified::state().frame(CpsRamFF,CpsRom,(unsigned)nCpsRomLen,
                (unsigned)GetCurrentFrame(),
                CpsCode,(unsigned)nCpsCodeLen);
        XM87186Opening::state().sample(CpsRamFF,
            (unsigned)GetCurrentFrame(),"POST_DHC_HOST",
            (unsigned)XM87144::state().phase[0],
            xm87140_p1_dir,xm8786_p1_buttons);
        xm87180Sample("AFTER_DHC_HOST");
        XM87175Observer::state().postDhc(
            CpsRamFF,(unsigned)GetCurrentFrame(),
            XM87162ReplaySync::movieFrame(),
            XM87144::phaseName(XM87144::state().phase[0]),
            dhcInput.active,dhcInput.directions,dhcInput.attacks);
        // ROM may regenerate the detached projectile attack descriptors
        // each frame. Capture late Ryu-spawned objects until Ken swaps;
        // suppress their offense after CPU and before the next CPU frame.
        const XM87144::Phase dhcPhase=XM87144::state().phase[0];
        const bool outgoingRyu=
            XM87144::rw(CpsRamFF,0x4052u)==0x12u &&
            XM87144::rw(CpsRamFF,0x4852u)==0x14u &&
            (dhcPhase==XM87144::WAIT_TAG_CONTROLLER ||
             dhcPhase==XM87144::WAIT_NATIVE_SWAP);
        if(xm262V4 && !xm274Unified)XM87144::state().oldRyuProjectiles.afterCPU(
            CpsRamFF,(unsigned)GetCurrentFrame(),outgoingRyu);
        const XM87144::Phase phaseAfter=XM87144::state().phase[0];
        if(phaseAfter!=phaseBefore && XM87144::state().enabled){
            char status[XM87140Chat::CHAT_LENGTH];
            std::snprintf(status,sizeof(status),
                "P1 DHC ROM HANDOFF: %s (native test, opponent hold opt-in)",
                XM87144::phaseName(phaseAfter));
            XM87140Chat::state().log((unsigned)GetCurrentFrame(),
                phaseAfter==XM87144::FAILED?XM87140Chat::ALERT:XM87140Chat::MATCH,
                status);
        }
    }
    // Read-only after-CPU capture: do not edit fighter RAM or native action.
    XM8795Native::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        xm262V4);
    // XM87.125: before Stage 87.96 clears the native dizzy marker on pop exit.
    XM87125Stun::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        xm262V4 &&
        XM8795Native::state().installed && XM8796Native::state().installed);
    // Clear ONLY native dizzy marker after a real game-state pop exit.
    XM8796Native::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        xm262V4,
        xm262V4);
    XM8799Ground::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        xm262V4);
    XM8779Flight::state().frame((unsigned)GetCurrentFrame(),CpsRamFF,CpsReg,
                                 (unsigned)Cpi01A,(unsigned)Cpi01C,(unsigned)Cpi01E);
    // F10 probe: actual CPS2 attack bits + port values AND the mapped
    // SDL input history; all four actor records on each real combat frame.
    if(!bBurnRunAheadFrame && XM87139Probe::state().active) {
        unsigned mf[2]={0,0},md[2]={0,0},mh[2]={0,0},mp[2]={0,0};
        for(unsigned side=0;side<2u;++side)
            XM87139Probe::xm87137ReadMappedInput(side,0u,
                &mf[side],&md[side],&mh[side],&mp[side]);
        XM87139Probe::state().frame(CpsRamFF,BurnDrvGetTextA(DRV_NAME),
            (unsigned)GetCurrentFrame(),false,xm8786_p1_buttons,xm8786_p2_buttons,
            (unsigned)Cpi01A,(unsigned)Cpi01C,(unsigned)Cpi01E,
            mf[0],md[0],mh[0],mp[0],mf[1],md[1],mh[1],mp[1]);
    }
    // XM87.139: no Stage87.138 forced tag positioning/facing or other DHC
    // gameplay writes. Record the actual ROM behavior as it happens.
    XM8798Latency::state().nativeFrame(CpsRamFF,(unsigned)GetCurrentFrame());
    XM87119::state().frame((unsigned)GetCurrentFrame(),CpsRamFF); // XM87.119 read-only probe
    // XM87.137: real-frame native evidence only; never tag/spend/suppress here.
    // The four-slot capture is entirely inert unless its TSV path is supplied.
    if(!bBurnRunAheadFrame)
        XM87137Capture::state().frame(CpsRamFF,BurnDrvGetTextA(DRV_NAME),
          (unsigned)GetCurrentFrame(),xm8786_p1_buttons,xm8786_p2_buttons,
          (unsigned)Cpi01A,(unsigned)Cpi01C,(unsigned)Cpi01E);
    // Stage 87.121's proven post-throw recovery belongs to XMVSF only.
    // 87.209 restores it in normal XMVSF TEST, without mutating the MVC1
    // donor or other CPS2 titles even if they share Cyclops sprites/RAM.
    const char* cyclopsGame=BurnDrvGetTextA(DRV_NAME);
    // XM87.213: DO NOT mutate host-side throw event state on speculative
    // runahead frames. Its static Tracker and lastFrame guard are not part
    // of a savestate; a runahead invocation can otherwise consume the real
    // frame's grab-end edge and discard the corresponding CpsRamFF write.
    // Exactly one committed native frame can advance the 100-frame clock.
    if(!bBurnRunAheadFrame && Cps==2 && CpsRamFF && xm262V4 && cyclopsGame &&
       std::strncmp(cyclopsGame,"xmvsf",5)==0)
        XM87121::state().frame((unsigned)GetCurrentFrame(),CpsRamFF);
    // Stage 87.220: after committed native CPU and after proven 87.219
    // recovery, remove only waiting animation ticks at a genuine dash.
    // No dash input synthesis, ROM patch, X velocity or position coercion.
    if(!bBurnRunAheadFrame && Cps==2 && CpsRamFF)
        XM87220Dash::state().frame((unsigned)GetCurrentFrame(),CpsRamFF,
            xm262V4 && cyclopsGame && std::strncmp(cyclopsGame,"xmvsf",5)==0,
            XM87262Game::tournamentKnockdowns());
    // XM87.203: source-accurate, read-only stage telemetry, no framebuffer,
    // ROM, gameplay, controller, palette or F9 replay modifications.
    // This private one-shot 68000 source dump is independent of the heavy
    // frame/tilemap recorder. RUN_TEST.bat mvscur1 trace needs NO stage capture.
    // The helper is inert unless an exclusive trace path was configured.
    if(!bBurnRunAheadFrame && Cps==2 && CpsCode && CpsRom){
        const char* programDriver=BurnDrvGetTextA(DRV_NAME);
        if(XM87203Stage::isMarvelVsCapcom(programDriver))
            XM87203M68K::once(programDriver,(unsigned)GetCurrentFrame(),
                              CpsRom,(unsigned)nCpsRomLen,
                              CpsCode,(unsigned)nCpsCodeLen);
    }
    // F11 gate is host-only: never touch emulated RAM or source graphics.
    // Source snapshots are captured only while the MVC1 donor is armed.
    const char* captureDir=std::getenv("XM87203_STAGE_CAPTURE_DIR");
    const char* captureFlag=std::getenv("XM87203_LEVEL_CAPTURE_ACTIVE");
    const char* captureGeneration=std::getenv("XM87203_LEVEL_CAPTURE_GENERATION");
    const bool captureRequested=captureFlag && std::strcmp(captureFlag,"1")==0 &&
        captureDir && *captureDir && Cps==2 &&
        XM87203Stage::isMarvelVsCapcom(BurnDrvGetTextA(DRV_NAME));
    // Each F11 ON begins a fresh exclusive session. F11 OFF flushes and
    // closes the preceding source streams; next activation never appends to
    // older MAP1/OAM/PALETTE data. No reset on run-ahead frames.
    static bool priorCaptureRequested=false;
    static unsigned priorCaptureGeneration=0u;
    if(!bBurnRunAheadFrame){
        const unsigned newGeneration=captureGeneration?
            (unsigned)std::strtoul(captureGeneration,nullptr,10):0u;
        if(priorCaptureRequested && (!captureRequested ||
                                    newGeneration!=priorCaptureGeneration)){
            XM87203Stage::state().statusLine("F11_CAPTURE_STOP",
                (unsigned)GetCurrentFrame(),"original_session_sealed");
            XM87203Stage::state().resetBatch();
        }else if(captureRequested && newGeneration!=priorCaptureGeneration){
            XM87203Stage::state().resetBatch();
        }
        priorCaptureRequested=captureRequested;
        priorCaptureGeneration=newGeneration;
    }
    if(captureRequested && !bBurnRunAheadFrame &&
       CpsRamFF && CpsSavePal && CpsRam708 &&
       CpsSaveReg[0] && CpsSaveFrg[0]){
        const char* driver=BurnDrvGetTextA(DRV_NAME);
        if(XM87203Stage::isMarvelVsCapcom(driver)){
            XM87203Stage::Frame f;
            f.frame=(unsigned)GetCurrentFrame();
            // These two FF5xxx offsets belong to XMVSF and are NOT
            // source-backed stage/phase addresses for Marvel vs. Capcom.
            // Record raw CPS2 hardware, defer ROM stage-ID discovery.
            f.phase=~0u;f.stage=~0u;
            f.driver=driver;f.captureActive=true;
            f.gfx=CpsGfx;f.gfxSize=(unsigned)nCpsGfxLen;
            f.gfxMask=(unsigned)nCpsGfxMask;
            for(unsigned l=0;l<4u;++l)f.gfxScrollOffset[l]=(unsigned)nCpsGfxScroll[l];
            f.screenW=nCpsScreenWidth;f.screenH=nCpsScreenHeight;
            f.objectBank=((unsigned)nCpsObjectBank^1u)&1u;
            f.oam=CpsRam708+(f.objectBank<<15u);
            f.palette=CpsSavePal;
            unsigned n=1u;
            while(n<XM87203Stage::MAX_SLICES && nRasterline[n])++n;
            f.sliceCount=n;
            const unsigned bankReg[3]={0x02u,0x04u,0x06u};
            for(unsigned slice=0;slice<n;++slice){
                const unsigned char* r=CpsSaveReg[slice];
                f.reg[slice]=r;
                f.frg[slice]=CpsSaveFrg[0]; // hardware priority, not invented per-slice ownership
                f.rasterStart[slice]=slice?(unsigned)nRasterline[slice]:0u;
                if(!r)continue;
                for(unsigned layer=0;layer<3u;++layer){
                    const unsigned bank=XM87203Stage::gfxMapBank(r,bankReg[layer]);
                    f.mapBank[slice][layer]=bank;
                    f.map[slice][layer]=CpsFindGfxRam((INT32)bank,0x4000);
                }
                if((XM87203Stage::le16(r,0x22u)&1u) && !CpsDisableRowScroll){
                    const unsigned bank=XM87203Stage::rowBank(r,0x08u);
                    f.rowBank[slice]=bank;
                    f.row[slice]=CpsFindGfxRam((INT32)bank,0x0800);
                }
            }
            XM87203Stage::state().observe(f);
        }
    }
    xm8634Mark("COMPLETE");

//	bprintf(PRINT_NORMAL, _T("    -\n"));

#if 0 && defined FBNEO_DEBUG
	if (nInterrupt) {
		bprintf(PRINT_IMPORTANT, _T("Beam synchronized interrupt at line %2X.\r"), nRasterline[nInterrupt]);
	} else {
		bprintf(PRINT_NORMAL, _T("Beam synchronized interrupt disabled.   \r"));
	}

	extern INT32 counter;
	if (counter) {
		bprintf(PRINT_NORMAL, _T("\n\nSlices start at: "));
		for (i = 0; i < MAX_RASTER + 2; i++) {
			bprintf(PRINT_NORMAL, _T("%2X "), nRasterline[i]);
		}
		bprintf(PRINT_NORMAL, _T("\n"));
		for (i = 0; i < 0x80; i++) {
			if (*((UINT16*)(CpsSaveReg[0] + i * 2)) != *((UINT16*)(CpsSaveReg[nInterrupt] + i * 2))) {
				bprintf(PRINT_NORMAL, _T("Register %2X: %4X -> %4X\n"), i * 2, *((UINT16*)(CpsSaveReg[0] + i * 2)), *((UINT16*)(CpsSaveReg[nInterrupt] + i * 2)));
			}
		}
		bprintf(PRINT_NORMAL, _T("\n"));
		for (i = 0; i < 0x010; i++) {
			if (CpsSaveFrg[0][i] != CpsSaveFrg[nInterrupt][i]) {
				bprintf(PRINT_NORMAL, _T("FRG %X: %02X -> %02X\n"), i, CpsSaveFrg[0][i], CpsSaveFrg[nInterrupt][i]);
			}
		}
		bprintf(PRINT_NORMAL, _T("\n"));
		if (((CpsSaveFrg[0][4] << 8) | CpsSaveFrg[0][5]) != ((CpsSaveFrg[nInterrupt][4] << 8) | CpsSaveFrg[nInterrupt][5])) {
			bprintf(PRINT_NORMAL, _T("Layer-sprite priority: %04X -> %04X\n"), ((CpsSaveFrg[0][4] << 8) | CpsSaveFrg[0][5]), ((CpsSaveFrg[nInterrupt][4] << 8) | CpsSaveFrg[nInterrupt][5]));
		}

		bprintf(PRINT_NORMAL, _T("\n"));
		for (INT32 j = 0; j <= nInterrupt; j++) {
			if (j) {
				bprintf(PRINT_NORMAL, _T("IRQ : %i (triggered at line %3i)\n\n"), j, nRasterline[j]);
			} else {
				bprintf(PRINT_NORMAL, _T("Initial register status\n\n"));
			}

			for (i = 0; i < 0x080; i+= 8) {
				bprintf(PRINT_NORMAL, _T("%2X: %4X %4X %4X %4X %4X %4X %4X %4X\n"), i * 2, *((UINT16*)(CpsSaveReg[j] + 0 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 2 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 4 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 6 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 8 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 10 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 12 + i * 2)), *((UINT16*)(CpsSaveReg[j] + 14 + i * 2)));
			}

			bprintf(PRINT_NORMAL, _T("\nFRG: "));
			for (i = 0; i < 0x010; i++) {
				bprintf(PRINT_NORMAL, _T("%02X "), CpsSaveFrg[j][i]);
			}
			bprintf(PRINT_NORMAL, _T("\n\n"));

		}

		extern INT32 bRunPause;
		bRunPause = 1;
		counter = 0;
	}
#endif

	return 0;
}

