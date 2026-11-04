#ifndef XMVSF_STAGE87_102_FLIGHT_AUDIO_H
#define XMVSF_STAGE87_102_FLIGHT_AUDIO_H
// XM87.104: critical flying-screen DOWNWARD VELOCITY onset through EXISTING WASAPI
// game stream. SDL decodes the WAV offline once; NO second audio device.
#include "xmvsf_stage87_102_flight_impact.h"
#include "burnint.h"
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
namespace XM87102FlightAudio {
struct Player {
    bool loadAttempted=false;
    bool ready=false;
    int sampleRate=0;
    std::vector<Uint8> pcm;
    unsigned offset=0;
    unsigned triggers=0;
    void load(int hz){
        if(loadAttempted)return;
        if(hz<8000||hz>192000){std::fprintf(stderr,"XM87.102 FS_WAV bad output rate=%d\n",hz);return;}
        loadAttempted=true;
        char* base=SDL_GetBasePath();
        std::string path=base?base:"";
        if(base)SDL_free(base);
        path+="assets/xmvsf/flyingscreen.wav";
        SDL_AudioSpec spec{};Uint8* data=nullptr;Uint32 length=0;
        if(!SDL_LoadWAV(path.c_str(),&spec,&data,&length)){
            std::fprintf(stderr,"XM87.102 FS_WAV_MISSING %s (%s); native audio continues\n",path.c_str(),SDL_GetError());
            return;
        }
        SDL_AudioCVT cvt{};
        const int rc=SDL_BuildAudioCVT(&cvt,spec.format,spec.channels,spec.freq,AUDIO_S16SYS,2,hz);
        if(rc<0||!length){std::fprintf(stderr,"XM87.102 FS_WAV_CONVERT_SETUP_FAILED %s\n",SDL_GetError());SDL_FreeWAV(data);return;}
        cvt.len=(int)length;
        cvt.buf=(Uint8*)SDL_malloc((size_t)cvt.len*(size_t)cvt.len_mult);
        if(!cvt.buf){SDL_FreeWAV(data);return;}
        std::memcpy(cvt.buf,data,length);SDL_FreeWAV(data);
        if(rc>0 && SDL_ConvertAudio(&cvt)<0){std::fprintf(stderr,"XM87.102 FS_WAV_CONVERT_FAILED %s\n",SDL_GetError());SDL_free(cvt.buf);return;}
        pcm.assign(cvt.buf,cvt.buf+(rc>0?cvt.len_cvt:cvt.len));SDL_free(cvt.buf);
        ready=!pcm.empty();sampleRate=hz;
        std::fprintf(stderr,"XM87.102 FS_WAV_READY path=%s bytes=%zu rate=%d\n",path.c_str(),pcm.size(),hz);
    }
    void stop(){offset=(unsigned)pcm.size();}
    void trigger(unsigned frame,unsigned fighter){
        if(!ready)return;
        offset=0;
        ++triggers;
        std::fprintf(stderr,"XM87.104 FS_VY_ACTIVATION frame=%u fighter=P%u vy=00132A00 wav=%u\n",frame,fighter,triggers);
    }
    void mix(INT16* dst,unsigned frames){
        if(!ready||!dst||offset>=pcm.size()||!frames)return;
        const unsigned bytes=frames*4u;
        const unsigned n=std::min<unsigned>(bytes,(unsigned)pcm.size()-offset);
        // 16-bit signed stereo in the SAME native WASAPI stream, no queue lag.
        SDL_MixAudioFormat((Uint8*)dst,pcm.data()+offset,AUDIO_S16SYS,n,SDL_MIX_MAXVOLUME/2);
        offset+=n;
    }
};
inline Player& player(){static Player p;return p;}
inline unsigned word(cpu_core_config* c,unsigned a){return ((c->read(a)&255u)<<8)|(c->read(a+1)&255u);}
inline void observe(unsigned tick,int hz){
    Player& wav=player();wav.load(hz);
    const char* driver=BurnDrvGetTextA(DRV_NAME);
    XM87102Impact::State& impact=XM87102Impact::state();
    // Immediately silence any unfinished sample on return to selection/menus.
    if(!driver||std::strcmp(driver,"xmvsfu")!=0){impact.reset();wav.stop();return;}
    if(bBurnRunAheadFrame)return;
    if(impact.lastTick==tick)return;
    impact.lastTick=tick;
    cpu_core_config* c=GetCpuCoreConfig(0);
    if(!c||!c->read||!c->open||!c->close)return;
    c->open(0);
    // Use the same exact native in-match guard as the 87.102 match snapshot:
    // phase 08 alone is insufficient in selection / transitional scenes.
    const unsigned phase=c->read(0xFF5000u)&255u;
    const unsigned matchPtr=(word(c,0xFFA014u)<<16)|word(c,0xFFA016u);
    const bool combat=(phase==8u && matchPtr==0x00FF4000u);
    if(!combat){c->close();impact.reset();wav.stop();return;}
    for(unsigned i=0;i<2u;++i){
        const unsigned base=0xFF4000u+0x400u*i;
        const unsigned action=word(c,base+0x60u);
        const unsigned profile=word(c,base+0xE0u);
        const unsigned y=word(c,base+0x10u);
        const unsigned reaction=word(c,base+0x06u);
        // FFxx1C: signed 16.16 downward velocity, sampled AFTER the native
        // game frame and the confirmed Tournament flight-rule velocity write.
        const std::uint32_t nativeVy=(std::uint32_t(word(c,base+0x1Cu))<<16) |
            std::uint32_t(word(c,base+0x1Eu));
        if(impact.fighter[i].tick(combat,action,profile,y,reaction,nativeVy)){
            // Only the first frame carrying Cyclops's actual downward impulse.
            wav.trigger(tick,i+1u);
        }
    }
    c->close();
}
} // namespace XM87102FlightAudio
#endif
