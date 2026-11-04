#ifndef XMVSF_STAGE87_79_FLIGHT_TRACE_H
#define XMVSF_STAGE87_79_FLIGHT_TRACE_H
// Read-only CPS2 flight/knockback investigation. No gameplay writes.
// Captures FF0000-FFFFFF plus 256 native CPS-2 registers once after each
// emulated combat frame. Frame log shows actor words and input-port states.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM8779Flight {
static const unsigned RAM_SIZE = 0x10000u;
static const unsigned REG_SIZE = 0x100u;
static const unsigned TOTAL_SIZE = RAM_SIZE + REG_SIZE;
static const unsigned MAX_BYTES = 64u * 1024u * 1024u;
struct Recorder {
    std::FILE* frames;
    std::FILE* changes;
    bool initialized;
    bool active;
    bool previousCombat;
    bool limited;
    unsigned combatFrames;
    unsigned lastFrame;
    unsigned lastX[2];
    unsigned lastY[2];
    unsigned char previous[TOTAL_SIZE];
    unsigned long long written;
    Recorder() : frames(NULL), changes(NULL), initialized(false), active(false),
                 previousCombat(false), limited(false), combatFrames(0), lastFrame(0),
                 written(0) {
        std::memset(previous, 0, sizeof(previous));
        lastX[0]=lastX[1]=lastY[0]=lastY[1]=0;
    }
    ~Recorder() { if(frames)std::fclose(frames); if(changes)std::fclose(changes); }
    void write32(std::uint32_t x) {
        std::uint8_t b[4] = {std::uint8_t(x),std::uint8_t(x>>8),std::uint8_t(x>>16),std::uint8_t(x>>24)};
        std::fwrite(b,1,4,changes);
    }
    static unsigned byte(const std::uint8_t* ram,unsigned offset) {
        return ram[(offset&0xffffu)^1u]; // 68000 big endian / FBNeo host-swapped
    }
    static unsigned word(const std::uint8_t* ram,unsigned offset) {
        return (byte(ram,offset)<<8) | byte(ram,offset+1u);
    }
    static int movement(unsigned now,unsigned prev) {
        const unsigned d=(now-prev)&0xffffu;
        return d>=0x8000u?int(d)-0x10000:int(d);
    }
    void init() {
        initialized=true;
        const char* f=std::getenv("XM8779_FLY_FRAMES_FILE");
        const char* b=std::getenv("XM8779_FLY_RAM_FILE");
        if(!f||!*f||!b||!*b)return;
        frames=std::fopen(f,"wb");
        if(!frames)return;
        changes=std::fopen(b,"wb");
        if(!changes){std::fclose(frames);frames=NULL;return;}
        const char magic[8]={'X','M','7','9','R','A','M','1'};
        std::fwrite(magic,1,8,changes);
        written=8;
        std::fputs("frame\tphase_FF5000\tphase_FF5001\tp1_x\tp1_y\tp1_dx\tp1_dy\tp1_animation\t"
                   "p2_x\tp2_y\tp2_dx\tp2_dy\tp2_animation\t"
                   "FF5100\tFF5182\tinput01A\tinput01C\tinput01E\t"
                   "CPSREG00\tCPSREG02\tCPSREG04\tCPSREG06\tchanged_bytes\tsegment\n",frames);
        active=true;
        std::fprintf(stderr,"XM87.79 FLY_CAPTURE_READY FF0000-FFFFFF CPSREG256 max_bytes=%u\n",MAX_BYTES);
    }
    void frame(std::uint32_t tick,const std::uint8_t* ram,const std::uint8_t* regs,
               unsigned inA,unsigned inC,unsigned inE) {
        if(!initialized)init();
        if(!active || !ram || !regs || limited)return;
        const unsigned phase0=byte(ram,0x5000u),phase1=byte(ram,0x5001u);
        const bool combat=(phase0==8u || phase1==8u);
        if(!combat){previousCombat=false;return;}
        const bool newSegment=!previousCombat || tick<=lastFrame;
        previousCombat=true;
        lastFrame=tick;
        if(newSegment)combatFrames=0;
        unsigned x[2]={word(ram,0x400cu),word(ram,0x440cu)};
        unsigned y[2]={word(ram,0x4010u),word(ram,0x4410u)};
        const int dx0=(combatFrames && !newSegment)?movement(x[0],lastX[0]):0;
        const int dy0=(combatFrames && !newSegment)?movement(y[0],lastY[0]):0;
        const int dx1=(combatFrames && !newSegment)?movement(x[1],lastX[1]):0;
        const int dy1=(combatFrames && !newSegment)?movement(y[1],lastY[1]):0;
        lastX[0]=x[0];lastX[1]=x[1];lastY[0]=y[0];lastY[1]=y[1];
        std::uint32_t count=0;
        for(unsigned j=0;j<RAM_SIZE;++j) if(ram[j]!=previous[j])++count;
        for(unsigned j=0;j<REG_SIZE;++j) if(regs[j]!=previous[RAM_SIZE+j])++count;
        const bool keyframe=newSegment || (combatFrames%120u)==0u;
        const unsigned needed=9u+(keyframe?TOTAL_SIZE:count*6u);
        if(written+needed>MAX_BYTES){
            limited=true;
            std::fprintf(stderr,"XM87.79 FLY_TRACE_LIMIT: binary RAM capture capped at %u bytes\n",MAX_BYTES);
            return;
        }
        std::fprintf(frames,"%u\t%02X\t%02X\t%u\t%u\t%d\t%d\t%04X\t%u\t%u\t%d\t%d\t%04X\t"
                            "%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%02X\t%u\t%u\n",
                     tick,phase0,phase1,x[0],y[0],dx0,dy0,word(ram,0x4006u),
                     x[1],y[1],dx1,dy1,word(ram,0x4406u),
                     byte(ram,0x5100u),byte(ram,0x5182u),inA,inC,inE,
                     regs[0],regs[2],regs[4],regs[6],count,newSegment?1u:0u);
        const unsigned char type=keyframe?1u:2u;
        std::fwrite(&type,1,1,changes);
        write32(tick);
        write32(keyframe?TOTAL_SIZE:count*6u);
        if(keyframe){
            std::fwrite(ram,1,RAM_SIZE,changes);
            std::fwrite(regs,1,REG_SIZE,changes);
        }else{
            for(unsigned j=0;j<TOTAL_SIZE;++j){
                const unsigned char v=j<RAM_SIZE?ram[j]:regs[j-RAM_SIZE];
                if(v==previous[j])continue;
                write32(j); // 00000..0FFFF FF bank; 10000..100FF CPS regs
                std::fwrite(&previous[j],1,1,changes);
                std::fwrite(&v,1,1,changes);
            }
        }
        std::memcpy(previous,ram,RAM_SIZE);
        std::memcpy(previous+RAM_SIZE,regs,REG_SIZE);
        written+=needed;
        ++combatFrames;
        if((combatFrames%60u)==0u){std::fflush(frames);std::fflush(changes);}
        if(newSegment)std::fprintf(stderr,"XM87.79 FLY_COMBAT_SEGMENT frame=%u phase=%02X/%02X\n",tick,phase0,phase1);
    }
};
static inline Recorder& state(){static Recorder recorder;return recorder;}
} // namespace XM8779Flight
#endif
