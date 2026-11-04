#include "cps.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
// XM86.35 QSound first-frame A/B isolation; normal branch is unmodified.

// QSound

static INT32 nQsndCyclesExtra;

static INT32 qsndTimerOver(INT32, INT32)
{
//	bprintf(PRINT_NORMAL, _T("  - IRQ -> 1.\n"));
	ZetSetIRQLine(0xFF, CPU_IRQSTATUS_HOLD);

	return 0;
}

INT32 QsndInit()
{
	INT32 nRate;

	// Init QSound z80
	if (QsndZInit()) {
		return 1;
	}
	BurnTimerInit(qsndTimerOver, NULL);

	if (Cps1Qs == 1) {
		nCpsZ80Cycles = 8000000 * 100 / nBurnFPS;
		BurnTimerAttachZet(8000000);
	} else {
		if (Cps2Turbo) {
			nCpsZ80Cycles = 16000000 * 100 / nBurnFPS;
			BurnTimerAttachZet(16000000);
		} else {
			nCpsZ80Cycles = 8000000 * 100 / nBurnFPS;
			BurnTimerAttachZet(8000000);
		}
	}

	if (nBurnSoundRate >= 0) {
		nRate = nBurnSoundRate;
	} else {
		nRate = 11025;
	}

	QscInit(nRate);		// Init QSound chip

	return 0;
}

void QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir)
{
	QscSetRoute(nIndex, nVolume, nRouteDir);
}

void QsndReset()
{
	ZetOpen(0);
	BurnTimerReset();
	BurnTimerSetRetrig(0, 1.0 / 252.0);
	ZetClose();

	nQsndCyclesExtra = 0;
}

void QsndExit()
{
	QscExit();							// Exit QSound chip
	QsndZExit();
}

INT32 QsndScan(INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		QsndZScan(nAction);				// Scan Z80
		QscScan(nAction);				// Scan QSound Chip

		BurnTimerScan(nAction, NULL);
		SCAN_VAR(nQsndCyclesExtra);
	}

	return 0;
}

void QsndNewFrame()
{
	ZetNewFrame();

	ZetOpen(0);
	ZetIdle(nQsndCyclesExtra);

	QscNewFrame();
}

// Diagnostic modes change emulation only when explicitly selected at launch.
// timer_only: run Z80/timers, but globally silence all QSound mixing,
//   including QscSyncQsnd calls made from inside the Z80 timer runner.
// mixer_only: avoid Z80 instruction execution, but advance emulated Z80
//   cycles and run the QSound mixer into the ordinary audio output buffer.
// none: isolate all QSound work while preserving the CPU's cycle accounting.
// This is a DIAGNOSTIC and is never a gameplay change to Stage74.
static int xm8635SoundMode() {
    static int mode=[](){
      const char* value=std::getenv("XM8635_QSOUND_MODE");
      if(value && std::strcmp(value,"timer_only")==0)return 1;
      if(value && std::strcmp(value,"mixer_only")==0)return 2;
      if(value && std::strcmp(value,"none")==0)return 3;
      return 0; // normal
    }();
    return mode;
}
void QsndEndFrame()
{
    static unsigned count=0;
    const bool trace=(++count<=2);
    const int mode=xm8635SoundMode();
    if(trace){std::fprintf(stderr,"XM86.35 QSOUND ENTER mode=%d cycles=%d soundlen=%d soundptr=%p z80total=%d\n",
                mode,nCpsZ80Cycles,nBurnSoundLen,(void*)pBurnSoundOut,ZetTotalCycles());std::fflush(stderr);}
    // Diagnostic-only split: in timer_only, preventing *all* QscUpdate
    // invocations during Z80 execution requires clearing pBurnSoundOut.
    INT16* originalSound=pBurnSoundOut;
    if(mode==1 || mode==3)pBurnSoundOut=NULL;
    if(mode==0 || mode==1){
      if(trace){std::fprintf(stderr,"XM86.35 QSOUND BEFORE_TIMER\n");std::fflush(stderr);}
      BurnTimerEndFrame(nCpsZ80Cycles);
      if(trace){std::fprintf(stderr,"XM86.35 QSOUND AFTER_TIMER z80total=%d\n",ZetTotalCycles());std::fflush(stderr);}
    }else{
      // Do not execute the Z80 CPU. Consume its remaining cycle budget in
      // the emulator's existing ZetIdle bookkeeping instead of resetting it.
      int remaining=nCpsZ80Cycles-ZetTotalCycles();
      if(remaining>0)ZetIdle(remaining);
      if(trace){std::fprintf(stderr,"XM86.35 QSOUND Z80_SKIPPED z80total=%d\n",ZetTotalCycles());std::fflush(stderr);}
    }
    pBurnSoundOut=originalSound;
    if(mode==0 || mode==2){
      if(trace){std::fprintf(stderr,"XM86.35 QSOUND BEFORE_MIXER output=%p len=%d\n",(void*)pBurnSoundOut,nBurnSoundLen);std::fflush(stderr);}
      if(pBurnSoundOut)QscUpdate(nBurnSoundLen);
      if(trace){std::fprintf(stderr,"XM86.35 QSOUND AFTER_MIXER\n");std::fflush(stderr);}
      if(Cps2Turbo && pBurnSoundOut)BurnSampleRender(pBurnSoundOut,nBurnSoundLen);
    }else if(originalSound && nBurnSoundLen>0){
      // Ensure that no stale audio content enters the host WASAPI queue.
      // nBurnSoundLen is the established stereo sample count for this frame.
      std::memset(originalSound,0,std::size_t(nBurnSoundLen)*2u*sizeof(INT16));
    }
    if(trace){std::fprintf(stderr,"XM86.35 QSOUND BEFORE_CLOSE\n");std::fflush(stderr);}
    nQsndCyclesExtra=ZetTotalCycles()-nCpsZ80Cycles;
    ZetClose();
    if(trace){std::fprintf(stderr,"XM86.35 QSOUND EXIT\n");std::fflush(stderr);}
}

void QsndSyncZ80()
{
	int nCycles = (INT64)SekTotalCycles() * nCpsZ80Cycles / nCpsCycles;

	if (nCycles <= ZetTotalCycles()) {
		return;
	}

	BurnTimerUpdate(nCycles);
}
