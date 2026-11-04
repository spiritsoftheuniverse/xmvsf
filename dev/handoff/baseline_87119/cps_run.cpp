// CPS - Run
#include "cps.h"
#include "../../../burner/sdl/xmvsf_stage87_98_latency_trace.h" // XM87.98 native state probe
#include "xmvsf_stage87_79_flight_trace.h" // XM87.79 read-only combat RAM capture
#include "xmvsf_stage87_80_flying_velocity.h" // XM87.80 Tournament launch normalization
#include "xmvsf_stage87_81_severe_flight.h" // XM87.81 experimental severe knockback pair
#include "xmvsf_stage87_82_cyclops_slam.h" // XM87.82 experimental Cyclops aerial slam velocity
#include "xmvsf_stage87_83_rom_flight_guard.h" // XM87.83 ROM-qualified flying-screen hit gate
#include "xmvsf_stage87_84_air_heavy_finisher.h" // XM87.84 second-or-later airborne heavy finisher
#include "xmvsf_stage87_85_connected_air_heavy.h" // XM87.85 connected aerial heavy qualifier
#include "xmvsf_stage87_86_storm_button_guard.h" // XM87.86 CPS2 physical MK/HK evidence
#include "xmvsf_stage87_95_native_dizzy_air_collision.h" // XM87.95 dizzy init and per-frame gate/f6 source-backed native opcode override
#include "xmvsf_stage87_96_tournament_dizzy_handoff.h" // XM87.96 native dizzy stand handoff
#include "xmvsf_stage87_99_tournament_ground_defense.h" // XM87.99 native Tournament dizzy landing combo/guard
#include "xmvsf_stage87_100_training_ko.h"
#include "xmvsf_stage87_119_cyclops_recovery_probe.h" // XM87.119 read-only recovery state trace // XM87.100 native Training-only KO suppression
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
	CpsRwGetInp();											// Update the input port values
    // XM87.95 Tournament-only native dizzy initializer AND per-frame state handler bypass.
    XM8795Native::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),(unsigned)GetCurrentFrame(),
        XM8795Native::env("XM8755_TOURNAMENT_MODE"),
        XM8795Native::env("XM8795_NATIVE_HIT_TEST"));
    // XM87.96: native dizzy stand initializer -> ROM normal recovery.
    XM8796Native::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),(unsigned)GetCurrentFrame(),
        XM8796Native::env("XM8755_TOURNAMENT_MODE"),
        XM8796Native::env("XM8796_DIZZY_HANDOFF_TEST"),
        XM8795Native::state().installed);
    // XM87.99: native ROM opcode hook. Parent 95 and 96 must be installed.
    XM8799Ground::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),(unsigned)GetCurrentFrame(),
        XM8799Ground::env("XM8755_TOURNAMENT_MODE"),
        XM8799Ground::env("XM8799_GROUND_DEFENSE_TEST"),
        XM8795Native::state().installed,XM8796Native::state().installed);
    XM87100TrainingKO::state().before(CpsCode,(unsigned)nCpsCodeLen,
        BurnDrvGetTextA(DRV_NAME),xm14TrainingMode!=0 && xm87102DamageMode==0);
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
	}
	
    xm8634Mark("AFTER MAIN CPU SCANLINES");
	CpsObjGet();
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

	nCpsCyclesExtra = SekTotalCycles() - nCpsCycles;

    xm8634Mark("BEFORE QSOUND END FRAME");
	if (!Cps2DisableQSnd) QsndEndFrame();
    xm8634Mark("AFTER QSOUND END FRAME");

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
    XM8786Flight::state().frame((unsigned)GetCurrentFrame(),CpsRamFF,
        xm8786_p1_buttons,xm8786_p2_buttons);
    // Read-only after-CPU capture: do not edit fighter RAM or native action.
    XM8795Native::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        XM8795Native::env("XM8755_TOURNAMENT_MODE"));
    // Clear ONLY native dizzy marker after a real game-state pop exit.
    XM8796Native::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        XM8796Native::env("XM8755_TOURNAMENT_MODE"),
        XM8796Native::env("XM8796_DIZZY_HANDOFF_TEST"));
    XM8799Ground::state().after(CpsRamFF,(unsigned)GetCurrentFrame(),
        XM8799Ground::env("XM8755_TOURNAMENT_MODE"));
    XM8779Flight::state().frame((unsigned)GetCurrentFrame(),CpsRamFF,CpsReg,
                                 (unsigned)Cpi01A,(unsigned)Cpi01C,(unsigned)Cpi01E);
    XM8798Latency::state().nativeFrame(CpsRamFF,(unsigned)GetCurrentFrame());
    XM87119::state().frame((unsigned)GetCurrentFrame(),CpsRamFF); // XM87.119 read-only probe
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

