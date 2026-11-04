#ifndef XMVSF_STAGE87_126_STORM_VOICE_TRACE_H
#define XMVSF_STAGE87_126_STORM_VOICE_TRACE_H
// Stage 87.126: READ-ONLY QSound DSP-command provenance for Storm attack voices.
// It is unsafe to mute an unidentified QSound register or channel: voices,
// effects and music share the chip. Use a live capture to identify voice
// sample starts BEFORE enabling any selective voice suppression.
// No QSound command, ROM state, PCM buffer or game RAM is altered here.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87126Voice {
static inline unsigned byteAt(const std::uint8_t* r,unsigned p){return r[(p&0xffffu)^1u];}
static inline unsigned wordAt(const std::uint8_t* r,unsigned p){return (byteAt(r,p)<<8)|byteAt(r,p+1);}
static inline unsigned fighterIdByte(const std::uint8_t* r,unsigned b){return r[b+0x52u];}
static inline bool combat(const std::uint8_t* r){return r&&(byteAt(r,0x5000u)==8u||byteAt(r,0x5001u)==8u);}
struct State {
    bool checked=false;
    std::FILE* log=nullptr;
    unsigned count=0;
    unsigned lastFrame=0;
    ~State(){if(log)std::fclose(log);}
    void init(){
        if(checked)return;
        checked=true;
        const char* path=std::getenv("XM87126_VOICE_TRACE_FILE");
        if(!path||!*path)return;
        log=std::fopen(path,"wb");
        if(!log){std::fprintf(stderr,"XM87.126 STORM_VOICE_TRACE_OPEN_FAILED %s\n",path);return;}
        std::setvbuf(log,nullptr,_IOFBF,65536);
        std::fputs("frame\tdsp_register\tdsp_value\tp1_id_byte\tp1_id_word\tp1_bank\tp1_action\tp1_y\tp2_id_byte\tp2_id_word\tp2_bank\tp2_action\tp2_y\n",log);
        std::fprintf(stderr,"XM87.126 DF_CENTER_AND_STORM_VOICE_TRACE_READY voice_mutation=0\n");
    }
    void observe(unsigned frame,unsigned reg,unsigned value,const std::uint8_t* r){
        init();
        if(!log||!combat(r)||count>=250000u)return;
        const unsigned p1=0x4000u,p2=0x4400u;
        std::fprintf(log,"%u\t%02X\t%04X\t%02X\t%04X\t%04X\t%04X\t%u\t%02X\t%04X\t%04X\t%04X\t%u\n",
            frame,reg,value,
            fighterIdByte(r,p1),wordAt(r,p1+0x52u),wordAt(r,p1+0x40u),wordAt(r,p1+0x60u),wordAt(r,p1+0x10u),
            fighterIdByte(r,p2),wordAt(r,p2+0x52u),wordAt(r,p2+0x40u),wordAt(r,p2+0x60u),wordAt(r,p2+0x10u));
        ++count;
        if(frame!=lastFrame && (count%1024u)==0u)std::fflush(log);
        lastFrame=frame;
    }
};
static inline State& state(){static State s;return s;}
}
#endif
