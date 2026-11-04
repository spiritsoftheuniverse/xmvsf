#ifndef XMVSF_NATIVE_PCM_H
#define XMVSF_NATIVE_PCM_H
// Stage21: SDL-independent, PCM-only RIFF/WAV loader used by XMVSF WASAPI.
// Loads the three already-provided 48kHz stereo signed-16 assets. No resampling.
// Header is portable C++ to make it unit-testable independently of Windows.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <climits>

namespace xmvsfPcm {
inline uint16_t u16(const unsigned char* b) {
    return static_cast<uint16_t>(b[0] | (static_cast<uint16_t>(b[1]) << 8));
}
inline uint32_t u32(const unsigned char* b) {
    return static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
           (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24);
}
// Contract: returns malloc-allocated interleaved S16 stereo PCM; caller free().
// On failure, both outputs are zero/null. Hard limit prevents unbounded allocation.
inline bool load48kStereo16(const char* path, unsigned char** pcm, uint32_t* frames) {
    if (!pcm || !frames || !path) return false;
    *pcm = nullptr; *frames = 0;
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    unsigned char riff[12] = {};
    bool ok = false, fmtOk = false;
    long dataAt = -1;
    uint32_t dataLen = 0;
    do {
        if (std::fread(riff, 1, sizeof(riff), f) != sizeof(riff) ||
            std::memcmp(riff, "RIFF", 4) != 0 ||
            std::memcmp(riff+8, "WAVE", 4) != 0) break;
        // Trust only chunks inside the declared RIFF payload.
        const uint64_t limit = static_cast<uint64_t>(u32(riff+4)) + 8u;
        if (limit < 44 || limit > static_cast<uint64_t>(LONG_MAX)) break;
        while (true) {
            const long here = std::ftell(f);
            if (here < 0 || static_cast<uint64_t>(here)+8u > limit) break;
            unsigned char ch[8] = {};
            if (std::fread(ch,1,8,f)!=8) break;
            const uint32_t len = u32(ch+4);
            const long start = std::ftell(f);
            if (start < 0 || static_cast<uint64_t>(start)+len+(len&1u)>limit) break;
            if (std::memcmp(ch,"fmt ",4)==0 && len>=16 && len<=4096) {
                unsigned char format[16] = {};
                if(std::fread(format,1,16,f)!=16) break;
                fmtOk = u16(format)==1 && u16(format+2)==2 &&
                        u32(format+4)==48000 && u32(format+8)==192000 &&
                        u16(format+12)==4 && u16(format+14)==16;
            } else if(std::memcmp(ch,"data",4)==0 && dataAt<0) {
                dataAt=start; dataLen=len;
            }
            const uint64_t next=static_cast<uint64_t>(start)+len+(len&1u);
            if(next>LONG_MAX || std::fseek(f,static_cast<long>(next),SEEK_SET)!=0) break;
        }
        if (!fmtOk || dataAt<0 || dataLen==0 || dataLen>256u*1024u*1024u || (dataLen&3u)) break;
        if (std::fseek(f,dataAt,SEEK_SET)!=0) break;
        unsigned char* mem=static_cast<unsigned char*>(std::malloc(dataLen));
        if(!mem) break;
        if(std::fread(mem,1,dataLen,f)!=dataLen) {std::free(mem); break;}
        *pcm=mem; *frames=dataLen/4; ok=true;
    } while(false);
    std::fclose(f);
    return ok;
}
// XM87118 loadEffectStereo16: optional 16-bit PCM mono/stereo WAV at 8..192 kHz.
// Resample a short SFX to the existing 48 kHz stereo WASAPI mixer, no extra device.
inline bool loadEffectStereo16(const char* path,unsigned char** pcm,uint32_t* frames){
    if(!path || !pcm || !frames)return false;
    *pcm=nullptr;*frames=0;
    FILE* f=std::fopen(path,"rb");if(!f)return false;
    unsigned char riff[12]={};unsigned channels=0,rate=0,block=0;
    long dataAt=-1;uint32_t dataLen=0;bool ok=false;
    do{
        if(std::fread(riff,1,12,f)!=12||std::memcmp(riff,"RIFF",4)||
           std::memcmp(riff+8,"WAVE",4))break;
        const uint64_t limit=uint64_t(u32(riff+4))+8u;
        if(limit<44 || limit>uint64_t(LONG_MAX))break;
        while(true){
            long at=std::ftell(f);if(at<0||uint64_t(at)+8>limit)break;
            unsigned char header[8]={};if(std::fread(header,1,8,f)!=8)break;
            uint32_t len=u32(header+4);long pos=std::ftell(f);
            if(pos<0||uint64_t(pos)+len+(len&1u)>limit)break;
            if(!std::memcmp(header,"fmt ",4)&&len>=16&&len<=4096){
                unsigned char format[16]={};if(std::fread(format,1,16,f)!=16)break;
                unsigned c=u16(format+2),r=u32(format+4),align=u16(format+12);
                if(u16(format)==1 && (c==1||c==2) && r>=8000 && r<=192000 &&
                   u16(format+14)==16 && align==c*2u && u32(format+8)==r*align){
                    channels=c;rate=r;block=align;
                }
            }else if(!std::memcmp(header,"data",4)&&dataAt<0){dataAt=pos;dataLen=len;}
            long next=static_cast<long>(uint64_t(pos)+len+(len&1u));
            if(std::fseek(f,next,SEEK_SET)!=0)break;
        }
        if(!channels||!rate||!block||dataAt<0||!dataLen||dataLen>32u*1024u*1024u||
           dataLen%block)break;
        const uint32_t count=dataLen/block;
        const uint64_t target=(uint64_t(count)*48000u+rate-1u)/rate;
        if(target==0||target>48000u*120u||target*4u>SIZE_MAX)break;
        unsigned char* raw=static_cast<unsigned char*>(std::malloc(dataLen));
        if(!raw)break;
        if(std::fseek(f,dataAt,SEEK_SET)||std::fread(raw,1,dataLen,f)!=dataLen){
            std::free(raw);break;
        }
        unsigned char* dest=static_cast<unsigned char*>(std::malloc(size_t(target)*4u));
        if(!dest){std::free(raw);break;}
        for(uint64_t i=0;i<target;++i){
            const double at=double(i)*rate/48000.0;
            const uint32_t lo=uint32_t(at)<count?uint32_t(at):count-1u;
            const uint32_t hi=lo+1u<count?lo+1u:lo;
            const double t=at>double(lo)?(at-double(lo)<1.0?at-double(lo):0.0):0.0;
            for(unsigned ch=0;ch<2;++ch){
                const unsigned srcCh=channels==1?0:ch;
                const size_t a=size_t(lo)*block+srcCh*2u;
                const size_t b=size_t(hi)*block+srcCh*2u;
                const int16_t p=int16_t(u16(raw+a)),q=int16_t(u16(raw+b));
                int sample=int(double(p)*(1.0-t)+double(q)*t);
                if(sample>32767)sample=32767;
                if(sample< -32768)sample=-32768;
                const uint16_t v=uint16_t(int16_t(sample));
                dest[i*4u+ch*2u]=static_cast<unsigned char>(v&255u);
                dest[i*4u+ch*2u+1u]=static_cast<unsigned char>(v>>8);
            }
        }
        std::free(raw);*pcm=dest;*frames=uint32_t(target);ok=true;
    }while(false);
    std::fclose(f);return ok;
}
} // namespace xmvsfPcm
#endif
