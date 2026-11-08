#ifndef XMVSF_STAGE87_224_CHARLIE_PC_H
#define XMVSF_STAGE87_224_CHARLIE_PC_H
// TEST 87.224: bounded executed 68000 PC / native actor write observer.
// Only Charlie (fighter id 0020 / ROM bank 0028) air HP 1804 /
// super-jump HK 1A04. Captures native writes BEFORE the 87.223
// host-side Storm-like flying-screen substitution.
// Opt-in and read-only; never changes any input, actor state or opcodes.
// Wraps and forwards an existing Musashi PC callback (e.g. DHC 87.182).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../../cpu/m68k/m68k.h"

namespace XM87224CharliePC {
static inline unsigned rb(const std::uint8_t* r,unsigned a){
    return r[(a&0xffffu)^1u];
}
static inline unsigned rw(const std::uint8_t* r,unsigned a){
    return (rb(r,a)<<8u)|rb(r,a+1u);
}
static inline std::uint32_t rl(const std::uint8_t* r,unsigned a){
    return (std::uint32_t(rw(r,a))<<16u)|rw(r,a+2u);
}
struct Native {
    unsigned reaction=0,step=0,action=0,index=0,air=0,gate82=0,gate83=0;
    std::uint32_t vx=0,vy=0;
};
static inline Native read(const std::uint8_t*r,unsigned base){
    Native n;
    n.reaction=rw(r,base+0x06u);
    n.step=rw(r,base+0x08u);
    n.action=rw(r,base+0x60u);
    n.index=rw(r,base+0xe0u);
    n.vx=rl(r,base+0x14u);
    n.vy=rl(r,base+0x1cu);
    n.air=rw(r,base+0xaau);
    n.gate82=rb(r,base+0x82u);
    n.gate83=rb(r,base+0x83u);
    return n;
}
static inline unsigned changes(const Native&a,const Native&b){
    return (a.reaction!=b.reaction?1u:0u)|
           (a.step!=b.step?2u:0u)|
           (a.action!=b.action?4u:0u)|
           (a.index!=b.index?8u:0u)|
           (a.vx!=b.vx?16u:0u)|
           (a.vy!=b.vy?32u:0u)|
           (a.air!=b.air?64u:0u)|
           (a.gate82!=b.gate82?128u:0u)|
           (a.gate83!=b.gate83?256u:0u);
}
struct Recorder;
inline Recorder& state();
struct Recorder {
    bool initialized=false,enabled=false,active=false;
    unsigned observedFrames[2]={0u,0u};
    unsigned frame=0,side=0,move=0,events=0,visited=0;
    unsigned base=0;
    const std::uint8_t* ram=nullptr;
    Native previous;
    int (*prior)(int)=nullptr;
    std::FILE* log=nullptr;
    static const unsigned MAX_FRAMES_PER_MOVE=32u;
    static const unsigned MAX_EVENTS_PER_FRAME=96u;
    ~Recorder(){
        if(active)m68k_set_insn_cb(prior);
        if(log)std::fclose(log);
    }
    void init(){
        if(initialized)return;
        initialized=true;
        const char* flag=std::getenv("XM87224_CHARLIE_M68K_TRACE");
        const char* path=std::getenv("XM87224_CHARLIE_M68K_TRACE_LOG");
        if(!flag || std::strcmp(flag,"1")!=0 || !path || !*path)return;
        log=std::fopen(path,"wb");
        if(!log)return;
        enabled=true;
        std::fputs("frame\tside\tcharlie_move\tPPC\tA6\tcycles\tordinal"
                   "\tchangeMask\tprev_reaction\tnext_reaction"
                   "\tprev_step\tnext_step\tprev_action\tnext_action"
                   "\tprev_index\tnext_index\tprev_vx\tnext_vx"
                   "\tprev_vy\tnext_vy\tprev_air\tnext_air"
                   "\tprev_82\tnext_82\tprev_83\tnext_83\n",log);
    }
    void begin(const std::uint8_t* source,unsigned tick,bool xmvsf,
               bool tournament,bool runahead){
        if(active || !xmvsf || !tournament || runahead || !source)return;
        init();
        if(!enabled || rb(source,0x5000u)!=8u)return;
        unsigned selected=2u,selectedMove=0u;
        for(unsigned s=0;s<2u;++s){
            const unsigned p=s?0x4400u:0x4000u;
            const unsigned a=rw(source,p+0x60u);
            if(rw(source,p+0x52u)!=0x0020u ||
               rw(source,p+0x40u)!=0x0028u)continue;
            const unsigned slot=a==0x1804u?0u:(a==0x1a04u?1u:2u);
            if(slot==2u || observedFrames[slot]>=MAX_FRAMES_PER_MOVE)continue;
            selected=s;selectedMove=a;
            ++observedFrames[slot];
            break;
        }
        if(selected>=2u)return;
        frame=tick;side=selected;move=selectedMove;ram=source;
        base=side?0x4000u:0x4400u; // Charlie's victim
        previous=read(ram,base);
        events=0u;visited=0u;
        prior=m68k_get_insn_cb();
        m68k_set_insn_cb(&Recorder::bridge);
        active=true;
    }
    static int bridge(int cycles){
        Recorder&s=state();
        if(s.prior && s.prior!=&Recorder::bridge)s.prior(cycles);
        if(s.active && s.ram)s.observe(cycles);
        return 0;
    }
    void observe(int cycles){
        ++visited;
        const Native current=read(ram,base);
        const unsigned flags=changes(previous,current);
        if(!flags)return;
        if(events<MAX_EVENTS_PER_FRAME && log){
            const unsigned pc=SekGetPPC(-1)&0x00ffffffu;
            const unsigned a6=unsigned(m68k_get_reg(nullptr,M68K_REG_A6))&
                0x00ffffffu;
            std::fprintf(log,
                "%u\t%u\t%04X\t%06X\t%06X\t%d\t%u\t%03X"
                "\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X"
                "\t%04X\t%04X\t%08X\t%08X\t%08X\t%08X"
                "\t%04X\t%04X\t%02X\t%02X\t%02X\t%02X\n",
                frame,side+1u,move,pc,a6,cycles,visited,flags,
                previous.reaction,current.reaction,previous.step,current.step,
                previous.action,current.action,previous.index,current.index,
                previous.vx,current.vx,previous.vy,current.vy,
                previous.air,current.air,previous.gate82,current.gate82,
                previous.gate83,current.gate83);
        }
        ++events;
        previous=current;
    }
    void end(){
        if(!active)return;
        active=false;
        m68k_set_insn_cb(prior);
        prior=nullptr;
        if(log){
            if(events>MAX_EVENTS_PER_FRAME)
                std::fprintf(log,"# frame=%u dropped_native_write_events=%u\n",
                    frame,events-MAX_EVENTS_PER_FRAME);
            std::fflush(log);
        }
        ram=nullptr;
    }
};
inline Recorder& state(){static Recorder s;return s;}
} // XM87224CharliePC
#endif
