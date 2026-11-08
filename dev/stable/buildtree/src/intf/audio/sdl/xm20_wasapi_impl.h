#ifndef XMVSF_STAGE20_WASAPI_IMPL_H
#define XMVSF_STAGE20_WASAPI_IMPL_H
// Windows-only WASAPI sink + independent QPC scheduler. Included exactly once
// from the patched aud_sdl.cpp; link with ole32 and uuid.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "xmvsf_native_pcm.h" /* XMVSF_STAGE21_BOUNDARY */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <algorithm>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "../../../burner/sdl/xmvsf_stage20_api.h"

namespace xm20 {
static constexpr unsigned kGameCapacity = 1u << 17; // stereo sample frames
struct Clip {
    unsigned char* pcm = nullptr;
    uint32_t frames = 0;
    double pos = 0;
    void release() { if (pcm) std::free(pcm); pcm=nullptr; frames=0; pos=0; }
};
struct Machine {
    HANDLE thread = nullptr;
    HANDLE stop = nullptr;
    HANDLE event = nullptr;
    CRITICAL_SECTION lock;
    bool csReady = false;
    bool begun = false;
    volatile LONG threadStatus = 0;
    Clip music, move, confirm, meter;
    Clip crash, welcome, fightVoice; // authored 87.235; same WASAPI device
    double meterPos[2] = {0.0,0.0};
    bool meterOn[2] = {false,false};
    float musicGain = 0.5f, musicTarget = 0.5f, gameGain = 1.f;
    // Frame-rate independent WASAPI envelope. Derived from the current
    // audible level; a 500 ms fight handoff ends exactly at zero.
    float fadeFrom=0.5f,fadeSeconds=0.f,fadeElapsed=0.f;
    bool moveOn = false, confirmOn = false, fight = false, gameStarted = false;
    bool crashOn=false,welcomeOn=false,fightVoiceOn=false;
    uint32_t gameHz = 48000;
    int16_t* game = nullptr;
    uint64_t produced = 0;
    double consumed = 0;
    int dropped = 0;
    int underflows = 0;
};
static Machine m;
static LARGE_INTEGER qpf = {};
static double deadline = 0;
static bool clockStarted = false;

static bool clipLoad(Clip& dst, const char* filename) {
    // Resolve relative to the Windows executable; no SDL audio or filesystem API.
    char path[4096] = {};
    const DWORD n = GetModuleFileNameA(nullptr,path,static_cast<DWORD>(sizeof(path)));
    if (!n || n>=sizeof(path)) return false;
    char* slash=std::strrchr(path,'\\');
    if (!slash) slash=std::strrchr(path,'/');
    if (!slash) return false;
    *(slash+1)=0;
    const size_t used=std::strlen(path);
    if (std::snprintf(path+used,sizeof(path)-used,"assets/xmvsf/%s",filename)<0)
        return false;
    unsigned char* data=nullptr;
    uint32_t frames=0;
    if (!xmvsfPcm::load48kStereo16(path,&data,&frames) &&
        !(std::strcmp(filename,"chargemeter.wav")==0 &&
          xmvsfPcm::loadEffectStereo16(path,&data,&frames))) {
        std::fprintf(stderr,"XM21 WAV unavailable or not 48k S16 stereo: %s\n",path);
        return false;
    }
    dst.pcm=data; dst.frames=frames; dst.pos=0;
    return true;
} /* XMVSF_STAGE21_BOUNDARY */

// Authored WAVs live in repository root /audio (two parents above
// runtime/stable). Resolve relative to the EXE, independent of the CWD.
// This reuses the existing portable PCM loader, including mono/48k conversion.
static bool clipLoadRepoAudio(Clip& dst,const char* filename){
    char base[4096]={};
    const DWORD n=GetModuleFileNameA(nullptr,base,static_cast<DWORD>(sizeof(base)));
    if(!n||n>=sizeof(base))return false;
    char* slash=std::strrchr(base,'\\');
    if(!slash)slash=std::strrchr(base,'/');
    if(!slash)return false;
    *(slash+1)=0;
    const char* relatives[]={"../../audio/","../audio/","audio/"};
    for(const char* relative:relatives){
        char path[4096]={};
        const int written=std::snprintf(path,sizeof(path),"%s%s%s",base,relative,filename);
        if(written<0||size_t(written)>=sizeof(path))continue;
        unsigned char* pcm=nullptr;uint32_t frames=0;
        if(xmvsfPcm::load48kStereo16(path,&pcm,&frames) ||
           xmvsfPcm::loadEffectStereo16(path,&pcm,&frames)){
            dst.pcm=pcm;dst.frames=frames;dst.pos=0;
            std::fprintf(stderr,"XM87.235 BOOT_WAV %s frames=%u seconds=%.3f\n",
                         path,unsigned(frames),double(frames)/48000.0);
            return true;
        }
    }
    std::fprintf(stderr,"XM87.235 BOOT_WAV_MISSING %s (expected root/audio)\n",filename);
    return false;
}

static float clipSample(const Clip& c, uint32_t f, int ch) {
    if (!c.pcm || f>=c.frames) return 0.f;
    return (reinterpret_cast<const int16_t*>(c.pcm)[f*2+ch])/32768.f;
}
static float linearClip(const Clip& c, double pos, int ch) {
    if (!c.pcm || !c.frames) return 0.f;
    uint32_t lo = static_cast<uint32_t>(pos);
    if (lo>=c.frames) return 0.f;
    const uint32_t hi = std::min(lo+1,c.frames-1);
    const float t=static_cast<float>(pos-lo);
    return clipSample(c,lo,ch)*(1.f-t)+clipSample(c,hi,ch)*t;
}
static void advanceClip(Clip& c, double step, bool repeat) {
    if (!c.pcm || !c.frames) return;
    c.pos += step;
    if (repeat) { while (c.pos>=c.frames) c.pos -= c.frames; }
    else if (c.pos>=c.frames) c.pos=c.frames;
}
static inline float clamp(float f) { return std::max(-1.f,std::min(1.f,f)); }
static bool subtype(const WAVEFORMATEX* f, DWORD type) {
    if(f->wFormatTag != WAVE_FORMAT_EXTENSIBLE || f->cbSize < 22) return false;
    static const BYTE tail[8] = {0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71};
    const GUID& g = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f)->SubFormat;
    return g.Data1==type && g.Data2==0 && g.Data3==0x0010 && memcmp(g.Data4,tail,8)==0;
}
static bool isFloat(const WAVEFORMATEX* f) {
    if (f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return f->wBitsPerSample==32;
    if (f->wFormatTag == WAVE_FORMAT_EXTENSIBLE && f->cbSize>=22)
        return subtype(f,3) && f->wBitsPerSample==32;
    return false;
}
static bool isPcm(const WAVEFORMATEX* f) {
    if (f->wFormatTag == WAVE_FORMAT_PCM) return true;
    if (f->wFormatTag == WAVE_FORMAT_EXTENSIBLE && f->cbSize>=22)
        return subtype(f,1);
    return false;
}
static void putSample(BYTE* dest, unsigned channel, unsigned bits, bool floats, float value) {
    value=clamp(value);
    const unsigned stride=bits/8;
    BYTE* p = dest + channel*stride;
    if (floats) { const float v=value; memcpy(p,&v,sizeof(v)); }
    else if (bits==16) { const int16_t v = static_cast<int16_t>(value*32767.f); memcpy(p,&v,sizeof(v)); }
    else if (bits==24) {
        const int32_t v = static_cast<int32_t>(value*8388607.f);
        p[0]=static_cast<BYTE>(v);p[1]=static_cast<BYTE>(v>>8);p[2]=static_cast<BYTE>(v>>16);
    } else if (bits==32) { const int32_t v = static_cast<int32_t>(value*2147483647.f);memcpy(p,&v,sizeof(v)); }
}
static void fillFrames(BYTE* output, UINT32 count, const WAVEFORMATEX* fmt) {
    const bool floats=isFloat(fmt);
    const unsigned hz=fmt->nSamplesPerSec,channels=fmt->nChannels,bits=fmt->wBitsPerSample;
    const double musicStep=48000.0/hz;
    const double gameStep=static_cast<double>(m.gameHz)/hz;
    const uint32_t fadeFrames=240; // loop seam only, music remains continuous
    EnterCriticalSection(&m.lock);
    for (UINT32 i=0;i<count;i++) {
        if(m.fadeSeconds>0.f){
            m.fadeElapsed=std::min(m.fadeSeconds,
                                   m.fadeElapsed+1.0f/float(hz));
            const float t=m.fadeElapsed/m.fadeSeconds;
            // Smoothstep has zero slope at the endpoints, eliminating the
            // abrupt discontinuity/pop of the old 150 ms linear cutoff.
            const float ease=t*t*(3.0f-2.0f*t);
            m.musicGain=m.fadeFrom+(m.musicTarget-m.fadeFrom)*ease;
            if(m.fadeElapsed>=m.fadeSeconds){
                m.musicGain=m.musicTarget;m.fadeSeconds=0.f;
            }
        }else m.musicGain=m.musicTarget;
        float channelsOut[2]={};
        for (int ch=0;ch<2;ch++) {
            float game=0;
            // Consume emulation audio opportunistically. Its buffer can never advance the game clock.
            const uint64_t base=static_cast<uint64_t>(m.consumed);
            if (m.gameStarted && base+1<m.produced && m.produced-base<kGameCapacity) {
                const double blend=m.consumed-base;
                const auto get=[&](uint64_t p)->float{return m.game[(p%kGameCapacity)*2+ch]/32768.f;};
                game=static_cast<float>(get(base)*(1.0-blend)+get(base+1)*blend)*m.gameGain;
            }
            float music=linearClip(m.music,m.music.pos,ch);
            if(m.music.frames>fadeFrames && m.music.pos>=m.music.frames-fadeFrames) {
                const float t=static_cast<float>((m.music.pos-(m.music.frames-fadeFrames))/fadeFrames);
                music=music*(1.f-t)+linearClip(m.music,m.music.pos-(m.music.frames-fadeFrames),ch)*t;
            }
            const float movement = m.moveOn ? linearClip(m.move,m.move.pos,ch)*0.82f : 0.f;
            const float confirmation = m.confirmOn ? linearClip(m.confirm,m.confirm.pos,ch)*0.90f : 0.f;
            // XM87.118: two independent effect cursors, mixed in the existing WASAPI sink.
            const float meterSound = (m.meterOn[0] ? linearClip(m.meter,m.meterPos[0],ch)*0.56f : 0.f) +
                                     (m.meterOn[1] ? linearClip(m.meter,m.meterPos[1],ch)*0.56f : 0.f);
            const float crashSfx=m.crashOn?linearClip(m.crash,m.crash.pos,ch)*0.88f:0.f;
            const float welcomeVoice=m.welcomeOn?linearClip(m.welcome,m.welcome.pos,ch)*0.95f:0.f;
            const float fightVoice=m.fightVoiceOn?linearClip(m.fightVoice,m.fightVoice.pos,ch)*0.95f:0.f;
            const float musicDuck=(m.crashOn||m.welcomeOn||m.fightVoiceOn)?0.64f:1.0f;
            // Before confirmed combat, original QSound is muted. This also mutes game SFX;
            // selective QSound music routing is NOT implemented in this stage.
            channelsOut[ch]=clamp(game+m.musicGain*0.70f*0.70f*music*musicDuck /* 49% base, duck for authored voices */+movement+confirmation+meterSound+crashSfx+welcomeVoice+fightVoice); /* XMVSF_STAGE22_SELECTION_SFX: native game SFX always audible; sound CPU suppresses BGM */
        }
        BYTE* dest=output+static_cast<size_t>(i)*fmt->nBlockAlign;
        for(unsigned ch=0;ch<channels;ch++)putSample(dest,ch,bits,floats,ch<2?channelsOut[ch]:0.f);
        advanceClip(m.music,musicStep,true);
        if (m.moveOn) {advanceClip(m.move,musicStep,false);if(m.move.pos>=m.move.frames)m.moveOn=false;}
        if (m.confirmOn){advanceClip(m.confirm,musicStep,false);if(m.confirm.pos>=m.confirm.frames)m.confirmOn=false;}
        if (m.crashOn){advanceClip(m.crash,musicStep,false);if(m.crash.pos>=m.crash.frames)m.crashOn=false;}
        if (m.welcomeOn){advanceClip(m.welcome,musicStep,false);if(m.welcome.pos>=m.welcome.frames)m.welcomeOn=false;}
        if (m.fightVoiceOn){advanceClip(m.fightVoice,musicStep,false);if(m.fightVoice.pos>=m.fightVoice.frames)m.fightVoiceOn=false;}
        for(unsigned side=0;side<2;++side)if(m.meterOn[side]){
            m.meterPos[side]+=musicStep;
            if(m.meterPos[side]>=m.meter.frames)m.meterOn[side]=false;
        }
        if(m.gameStarted && m.consumed+1<m.produced) m.consumed+=gameStep;
        else if(m.gameStarted && m.fight) ++m.underflows;
        // Keep latency bounded if the consumer stalls or the audio device changes.
        if(m.produced>static_cast<uint64_t>(m.consumed)+m.gameHz/4u)
            m.consumed=static_cast<double>(m.produced-m.gameHz/8u);
    }
    LeaveCriticalSection(&m.lock);
}
static DWORD WINAPI audioThread(LPVOID) {
    HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if (FAILED(hr)) { fprintf(stderr,"XM20 CoInitializeEx: 0x%08lx\n",static_cast<unsigned long>(hr));return 1; }
    IMMDeviceEnumerator* en=nullptr; IMMDevice* endpoint=nullptr;
    IAudioClient* client=nullptr; IAudioRenderClient* render=nullptr; WAVEFORMATEX* fmt=nullptr;
    UINT32 bufCount=0;
    do {
        hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&en));if(FAILED(hr))break;
        hr=en->GetDefaultAudioEndpoint(eRender,eConsole,&endpoint);if(FAILED(hr))break;
        hr=endpoint->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(&client));if(FAILED(hr))break;
        hr=client->GetMixFormat(&fmt);if(FAILED(hr))break;
        if(fmt->nChannels<2 || fmt->nBlockAlign != (fmt->wBitsPerSample/8)*fmt->nChannels ||
            !(isFloat(fmt) || (isPcm(fmt) && (fmt->wBitsPerSample==16 || fmt->wBitsPerSample==24 || fmt->wBitsPerSample==32)))) {
            fprintf(stderr,"XM20 unsupported shared audio format: tag=%u channels=%u bits=%u\n",fmt->wFormatTag,fmt->nChannels,fmt->wBitsPerSample);break;
        }
        // Host mix format means no exclusive-mode changes and no second conversion service.
        hr=client->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_EVENTCALLBACK,0,0,fmt,nullptr);if(FAILED(hr))break;
        hr=client->GetBufferSize(&bufCount);if(FAILED(hr))break;
        hr=client->SetEventHandle(m.event);if(FAILED(hr))break;
        hr=client->GetService(__uuidof(IAudioRenderClient),reinterpret_cast<void**>(&render));if(FAILED(hr))break;
        BYTE* packet=nullptr;
        hr=render->GetBuffer(bufCount,&packet);if(FAILED(hr))break;
        fillFrames(packet,bufCount,fmt);
        hr=render->ReleaseBuffer(bufCount,0);if(FAILED(hr))break;
        hr=client->Start();if(FAILED(hr))break;
        InterlockedExchange(&m.threadStatus,1);
        fprintf(stderr,"XM20 WASAPI started: %luHz %u channels %u bits; game independent clock\n",
                static_cast<unsigned long>(fmt->nSamplesPerSec),fmt->nChannels,fmt->wBitsPerSample);
        HANDLE waits[]={m.stop,m.event};
        while(true) {
            DWORD wr=WaitForMultipleObjects(2,waits,FALSE,200);
            if(wr==WAIT_OBJECT_0)break;
            if(wr!=WAIT_OBJECT_0+1)continue;
            UINT32 pad=0;
            if(FAILED(client->GetCurrentPadding(&pad)))break;
            if (pad>=bufCount)continue;
            const UINT32 frames=bufCount-pad;
            if (FAILED(render->GetBuffer(frames,&packet)))break;
            fillFrames(packet,frames,fmt);
            if (FAILED(render->ReleaseBuffer(frames,0)))break;
        }
        client->Stop();
    } while(false);
    if(FAILED(hr))fprintf(stderr,"XM20 WASAPI error: 0x%08lx\n",static_cast<unsigned long>(hr));
    InterlockedExchange(&m.threadStatus,-1);
    if(render)render->Release();if(client)client->Release();if(endpoint)endpoint->Release();if(en)en->Release();
    if(fmt)CoTaskMemFree(fmt);
    CoUninitialize();return 0;
}
static void shutdown() {
    if(!m.begun)return;
    if(m.stop)SetEvent(m.stop);
    if(m.thread){WaitForSingleObject(m.thread,2000);CloseHandle(m.thread);m.thread=nullptr;}
    if(m.stop){CloseHandle(m.stop);m.stop=nullptr;}
    if(m.event){CloseHandle(m.event);m.event=nullptr;}
    m.music.release();m.move.release();m.confirm.release();m.meter.release();
    m.crash.release();m.welcome.release();m.fightVoice.release();
    if(m.game){free(m.game);m.game=nullptr;}
    if(m.csReady){DeleteCriticalSection(&m.lock);m.csReady=false;}
    m.begun=false;
}
static bool start() {
    if(m.begun)return true;
    InitializeCriticalSection(&m.lock);m.csReady=true;m.begun=true;
    m.game=reinterpret_cast<int16_t*>(calloc(kGameCapacity*2,sizeof(int16_t)));
    if (!m.game){shutdown();return false;}
    m.stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    m.event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!m.stop || !m.event){shutdown();return false;}
    clipLoad(m.music,"xmvsfMenu.wav");clipLoad(m.move,"menu_movement1.wav");clipLoad(m.confirm,"menu_selection1.wav");
    clipLoad(m.meter,"chargemeter.wav"); // optional, supplied by the user in runtime/stable/assets/xmvsf
    clipLoadRepoAudio(m.crash,"crash.wav");
    clipLoadRepoAudio(m.welcome,"xmvsf_welcome.wav");
    clipLoadRepoAudio(m.fightVoice,"fight!.wav");
    m.thread=CreateThread(nullptr,0,audioThread,nullptr,0,nullptr);
    if(!m.thread){shutdown();return false;}
    atexit(shutdown);
    return true;
}
static void queue(const int16_t* samples,unsigned frames,unsigned hz) {
    if(!samples || !frames || !m.begun || !m.csReady)return;
    EnterCriticalSection(&m.lock);
    m.gameHz=hz;
    if(!m.gameStarted) {m.gameStarted=true;m.produced=0;m.consumed=0;}
    for(unsigned i=0;i<frames;i++) {
        const uint64_t w=m.produced++;
        m.game[(w%kGameCapacity)*2]=samples[i*2];m.game[(w%kGameCapacity)*2+1]=samples[i*2+1];
    }
    if(m.produced>static_cast<uint64_t>(m.consumed)+kGameCapacity-2) {
        m.consumed=static_cast<double>(m.produced-m.gameHz/8u);++m.dropped;
    }
    LeaveCriticalSection(&m.lock);
}
} // namespace xm20
void xm20MenuStart(){
    xm20::start();
    if(!xm20::m.csReady)return;
    EnterCriticalSection(&xm20::m.lock);
    // XM86.55: return to menu means a NEW audio phase, not a continuation
    // of a combat queue or the previous character-select state. This is only
    // performed when game PCM was actually queued or combat had started.
    if(xm20::m.gameStarted || xm20::m.fight){
        xm20::m.gameStarted=false;
        xm20::m.produced=0;
        xm20::m.consumed=0;
        xm20::m.fight=false;
        xm20::m.music.pos=0;
        xm20::m.musicGain=0.5f;
        xm20::m.musicTarget=0.5f;
        xm20::m.fadeFrom=0.5f;
        xm20::m.fadeSeconds=0.f;
        xm20::m.fadeElapsed=0.f;
        xm20::m.moveOn=false;
        xm20::m.confirmOn=false;
        xm20::m.crashOn=xm20::m.welcomeOn=xm20::m.fightVoiceOn=false;
        xm20::m.crash.pos=xm20::m.welcome.pos=xm20::m.fightVoice.pos=0;
        xm20::m.meterOn[0]=xm20::m.meterOn[1]=false;
        std::fprintf(stderr,"XM86.55 AUDIO_MENU_RETURN queue_flushed=1 music_restarted=1 fight=0\n");
    }
    LeaveCriticalSection(&xm20::m.lock);
}
void xm20MenuMovement(){if(xm20::m.csReady){EnterCriticalSection(&xm20::m.lock);xm20::m.move.pos=0;xm20::m.moveOn=true;LeaveCriticalSection(&xm20::m.lock);}}
void xm20MenuConfirm(){if(xm20::m.csReady){EnterCriticalSection(&xm20::m.lock);xm20::m.confirm.pos=0;xm20::m.confirmOn=true;LeaveCriticalSection(&xm20::m.lock);}}
// Original menu, game, and these three one-shots share the same WASAPI sink.
void xm20MenuCrash(){if(xm20::m.csReady){EnterCriticalSection(&xm20::m.lock);xm20::m.crash.pos=0;xm20::m.crashOn=xm20::m.crash.frames>0;LeaveCriticalSection(&xm20::m.lock);}}
void xm20MenuWelcome(){if(xm20::m.csReady){EnterCriticalSection(&xm20::m.lock);xm20::m.welcome.pos=0;xm20::m.welcomeOn=xm20::m.welcome.frames>0;LeaveCriticalSection(&xm20::m.lock);}}
void xm20MenuFight(){if(xm20::m.csReady){EnterCriticalSection(&xm20::m.lock);xm20::m.fightVoice.pos=0;xm20::m.fightVoiceOn=xm20::m.fightVoice.frames>0;LeaveCriticalSection(&xm20::m.lock);}}
// XM87.118 METER_CHARGE_WAV: render-time SFX only, no QSound writes or SDL audio device.
void xm20MeterCharged(int player){
    if(player<0||player>1||!xm20::m.csReady)return;
    EnterCriticalSection(&xm20::m.lock);
    if(xm20::m.meter.frames){
        xm20::m.meterPos[player]=0.0;
        xm20::m.meterOn[player]=true;
        std::fprintf(stderr,"XM87.118 METER_CHARGE_WAV side=%d\n",player+1);
    }
    LeaveCriticalSection(&xm20::m.lock);
}
void xm20MenuEnterGame(){
    if(!xm20::m.csReady)return;
    EnterCriticalSection(&xm20::m.lock);
    xm20::m.fadeFrom=xm20::m.musicGain;
    xm20::m.musicTarget=1.f;
    xm20::m.fadeElapsed=0.f;
    xm20::m.fadeSeconds=0.15f; // ordinary start/upward gain stays quick
    xm20::m.fight=false;
    LeaveCriticalSection(&xm20::m.lock);
}
void xm20OnCombat(){
    if(!xm20::m.csReady)return;
    EnterCriticalSection(&xm20::m.lock);
    if(!xm20::m.fight){
        // Trigger only ONCE at the earliest verified VS/crush boundary;
        // repeated COMBAT observations cannot restart the envelope.
        xm20::m.fadeFrom=xm20::m.musicGain;
        xm20::m.musicTarget=0.f;
        xm20::m.fadeElapsed=0.f;
        xm20::m.fadeSeconds=0.5f;
        xm20::m.fight=true;
        std::fprintf(stderr,
            "XM87.206 VS MUSIC_FADE_BEGIN duration=500ms start=%.3f\n",
            xm20::m.fadeFrom);
    }
    LeaveCriticalSection(&xm20::m.lock);
}
void xm20ClockReset(){xm20::clockStarted=false;}
int xm20FrameDue(int fpsHundredths) {
    if(fpsHundredths<1000 || fpsHundredths>10000)fpsHundredths=5964;
    if(!xm20::qpf.QuadPart)QueryPerformanceFrequency(&xm20::qpf);
    LARGE_INTEGER now={};QueryPerformanceCounter(&now);
    const double interval=static_cast<double>(xm20::qpf.QuadPart)*100.0/fpsHundredths;
    if(!xm20::clockStarted){xm20::deadline=static_cast<double>(now.QuadPart);xm20::clockStarted=true;}
    if(static_cast<double>(now.QuadPart)<xm20::deadline){
        const double seconds=(xm20::deadline-now.QuadPart)/xm20::qpf.QuadPart;
        if(seconds>0.002)Sleep(static_cast<DWORD>(seconds*1000)-1);
        return 0;
    }
    xm20::deadline+=interval;
    if(static_cast<double>(now.QuadPart)>xm20::deadline+interval*2) xm20::deadline=now.QuadPart+interval;
    return 1;
}
#endif
