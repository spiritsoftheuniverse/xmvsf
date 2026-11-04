#ifndef XMVSF_STAGE87_32_STUN_PROBE_H
#define XMVSF_STAGE87_32_STUN_PROBE_H
// Stage87.32 read-only: preserve raw CPS-2 fighter RAM for identifying the REAL
// accumulating/decaying dizzy value. No guessed game-field offsets or writes.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
namespace XM8732 {
static inline void Capture(const unsigned char* ram, unsigned frame, int gameStatus) {
    // Opt-in. Off has no file writes and no hot-path RAM reads.
    static int armed=-1;
    static FILE* file=nullptr;
    static unsigned lastFrame=~0u, samples=0;
    if(armed<0){
        const char* out=std::getenv("XM8732_STUN_CAPTURE");
        armed=(out && *out && std::strcmp(out,"0")!=0)?1:0;
        if(armed){
            file=std::fopen(out,"wb");
            if(!file){ std::fprintf(stderr,"XM87.32 STUN_PROBE_OPEN_FAILED path=%s\n",out);armed=0; }
            else {
                const char magic[8]={'X','M','8','7','3','2','A','\n'};
                std::fwrite(magic,1,sizeof(magic),file);
                std::fprintf(stderr,"XM87.32 STUN_PROBE_READY output=%s scope=FF4000/4400/4800/4C00\n",out);
            }
        }
    }
    if(!armed || !file || !ram || gameStatus!=8 || lastFrame==frame || samples>=7200u)return;
    lastFrame=frame;
    // An intentionally simple 4100-byte record, little-endian frame then
    // the four exact 0x400-byte fighter structs. Bytes are unmodified.
    unsigned char stamp[4]={(unsigned char)frame,(unsigned char)(frame>>8),
                            (unsigned char)(frame>>16),(unsigned char)(frame>>24)};
    std::fwrite(stamp,1,4,file);
    for(int p=0;p<4;++p)std::fwrite(ram+0x4000+p*0x400,1,0x400,file);
    ++samples;
    if(samples%60u==0u){std::fflush(file);
        if(samples<=180u || samples%600u==0u)
            std::fprintf(stderr,"XM87.32 STUN_PROBE frames=%u last=%u\n",samples,frame);
    }
    if(samples==7200u){std::fflush(file);std::fclose(file);file=nullptr;
        std::fprintf(stderr,"XM87.32 STUN_PROBE_COMPLETE frames=%u\n",samples);}
}
}
#endif
