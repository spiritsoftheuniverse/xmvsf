// XMVSF Stage20: preserves FBNeo's AudOut interface while replacing SDL output
// with one persistent Windows WASAPI device. A QPC scheduler, not sound-buffer
// consumption, determines when RunGetNextSound advances the machine.
/* XMVSF_STAGE21_BOUNDARY: SDL audio dependency removed */
#include "burner.h"
#include "aud_dsp.h"
#include "xm20_wasapi_impl.h"
#include <cstdlib>
#include <cstring>

static int (*xm20NextSound)(int) = nullptr;

static int xm20Silence(int) {
    if(nAudNextSound && nAudSegLen>0) memset(nAudNextSound,0,nAudSegLen*4);
    return 0;
}
static int XM20SoundBlank() {return xm20Silence(0);}
static int XM20SoundCheck() {
    // Exactly one segment per independently scheduled game frame.
    if(!bAudPlaying || !nAudNextSound || !nAudSegLen)return 0;
    (xm20NextSound ? xm20NextSound : xm20Silence)(1);
    if(nAudDSPModule[0]) DspDo(nAudNextSound,nAudSegLen);
    xm20::queue(nAudNextSound,static_cast<unsigned>(nAudSegLen),static_cast<unsigned>(nAudSampleRate[0]));
    return 0;
}
static int XM20SoundInit() {
    if(nAudSampleRate[0]<=0 || nAppVirtualFps<=0)return 1;
    nAudSegLen=(nAudSampleRate[0]*100+(nAppVirtualFps>>1))/nAppVirtualFps;
    if(nAudSegLen<=0)return 1;
    nAudNextSound=static_cast<short*>(malloc(nAudSegLen*4));
    if(!nAudNextSound)return 1;
    memset(nAudNextSound,0,nAudSegLen*4);
    xm20NextSound=xm20Silence;
    DspInit();
    xm20MenuStart(); // no extra device; same sink already used by menu
    fprintf(stderr,"XM20 sound init: %d Hz, %d samples per emulated frame\n",nAudSampleRate[0],nAudSegLen);
    return 0;
}
static int XM20SoundSetCallback(int (*fn)(int)) { xm20NextSound=fn?fn:xm20Silence;return 0; }
static int XM20SoundPlay(){bAudPlaying=1;return 0;}
static int XM20SoundStop(){bAudPlaying=0;return 0;}
static int XM20SoundExit(){
    DspExit();
    free(nAudNextSound);nAudNextSound=nullptr;
    // Native output deliberately continues for menu/game transitions and is
    // stopped on process exit. Nothing invokes SDL_CloseAudio here.
    return 0;
}
static int XM20SetVolume(){
    if (xm20::m.csReady) {
        EnterCriticalSection(&xm20::m.lock);
        xm20::m.gameGain = std::max(0.f, std::min(1.f, nAudVolume / 10000.f));
        LeaveCriticalSection(&xm20::m.lock);
    }
    return 0;
}
static int XM20Settings(InterfaceInfo*){return 0;}
struct AudOut AudOutSDL={XM20SoundBlank,XM20SoundCheck,XM20SoundInit,XM20SoundSetCallback,
                        XM20SoundPlay,XM20SoundStop,XM20SoundExit,XM20SetVolume,XM20Settings,
                        _T("XMVSF native WASAPI output")};
