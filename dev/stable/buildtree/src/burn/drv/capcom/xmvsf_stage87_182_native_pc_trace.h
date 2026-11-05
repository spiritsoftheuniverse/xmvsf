#ifndef XMVSF_STAGE87_182_NATIVE_PC_TRACE_H
#define XMVSF_STAGE87_182_NATIVE_PC_TRACE_H
// Stage 87.182: opt-in, bounded 68000 executed-PC trace for the *armed*
// Ryu->Ken DHC controller-8 handoff. No gameplay, original RAM, opcode,
// input history, super script, or CPU cycle budget modifications.
// Uses EXISTING Musashi instruction-completion callback, not the disabled
// debug instruction hook. Restores any prior callback after each CPU frame.
// Requires XM87142's read-only native RAM reader, SekGetPPC, and Musashi.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../../cpu/m68k/m68k.h"

namespace XM87182PC {
struct Entry {
    unsigned pc=0,nextPc=0,a6=0,d0=0,cycles=0;
    unsigned action=0,mode=0,d6=0,ctrl256=0,ctrl259=0;
    unsigned c0=0,c2=0,c6=0,ca=0;
};
static inline bool watch(unsigned pc) {
    return (pc>=0x01038Eu && pc<=0x0104E4u) || // dispatch/input producer
           (pc>=0x011B00u && pc<=0x011B72u) || // original tag phase
           (pc>=0x013C30u && pc<=0x0145E0u) || // original motion matchers
           (pc>=0x032800u && pc<=0x034300u);   // Ryu command/arbitration
}
static inline const char* region(unsigned pc) {
    if(pc>=0x01038Eu && pc<=0x0104E4u)return "INPUT_AND_SCRIPT";
    if(pc>=0x011B00u && pc<=0x011B72u)return "NATIVE_TAG_CONTROLLER";
    if(pc>=0x013C30u && pc<=0x0145E0u)return "NATIVE_MOTION_MATCHER";
    if(pc>=0x032800u && pc<=0x034300u)return "RYU_COMMAND_DISPATCH";
    return "OTHER";
}
struct State {
    enum { LIMIT=4096 };
    bool initialized=false,enabled=false,active=false;
    std::FILE* log=nullptr;
    const std::uint8_t*ram=nullptr;
    unsigned frame=0,phase=0,count=0,dropped=0,visited=0;
    int (*prior)(int)=nullptr;
    Entry entries[LIMIT];
    ~State(){
        // Normal execution always ends the hook before leaving Cps2Frame.
        if(active)m68k_set_insn_cb(prior);
        if(log){std::fflush(log);std::fclose(log);}
    }
    void init(){
        if(initialized)return;
        initialized=true;
        const char* on=std::getenv("XM87182_NATIVE_PC_TRACE");
        const char* path=std::getenv("XM87182_NATIVE_PC_TRACE_LOG");
        if(!on || std::strcmp(on,"1")!=0 || !path || !*path)return;
        log=std::fopen(path,"ab");
        if(!log){
            std::fprintf(stderr,"XM87.182 cannot open native PC trace: %s\n",path);
            return;
        }
        enabled=true;
        std::fputs("# XM87.182 NATIVE PC SESSION: real executed PPC, "
                   "post-instruction RAM, DHC controller-8 window only\n",log);
        std::fputs("frame\tphase\tordinal\tregion\tPPC\tPC_next\tA6\tD0"
                   "\tcycles\tRyuAction\tRyuA0\tRyuD6\tcontroller256"
                   "\tcontroller259\tC0\tC2\tC6\tCA\n",log);
        std::fflush(log);
    }
    void record(int cycles){
        ++visited;
        // SekGetPPC records the PC of the just executed original 68000
        // instruction, not the next instruction's PC.
        const unsigned pc=SekGetPPC(-1)&0xFFFFFFu;
        if(!watch(pc))return;
        if(count>=LIMIT){++dropped;return;}
        Entry& e=entries[count++];
        e.pc=pc;e.nextPc=SekGetPC(-1)&0xFFFFFFu;
        e.a6=(unsigned)m68k_get_reg(nullptr,M68K_REG_A6)&0xFFFFFFu;
        e.d0=(unsigned)m68k_get_reg(nullptr,M68K_REG_D0);
        e.cycles=(unsigned)cycles;
        // Snapshot AFTER the executed instruction: read-only, so the
        // exact PC that modified action/D6/A0 can be correlated.
        e.action=XM87142::ramW(ram,0x4060u);
        e.mode=XM87142::ramW(ram,0x40A0u);
        e.d6=XM87142::ramB(ram,0x40D6u);
        e.ctrl256=XM87142::ramB(ram,0x4256u);
        e.ctrl259=XM87142::ramB(ram,0x4259u);
        e.c0=XM87142::ramW(ram,0x40C0u);
        e.c2=XM87142::ramW(ram,0x40C2u);
        e.c6=XM87142::ramW(ram,0x40C6u);
        e.ca=XM87142::ramW(ram,0x40CAu);
    }
    // Invoke before native input sampling, after SekOpen(0), only for
    // an already armed, early-cancel Ryu Shinkuu -> Ken DHC.
    void begin(const std::uint8_t*source,unsigned now,unsigned dhcPhase,
               bool earlyCancel,bool runahead){
        init();
        if(active || !enabled || runahead || !earlyCancel || !source ||
           dhcPhase!=2u || XM87142::ramB(source,0x5000u)!=8u ||
           XM87142::ramW(source,0x4052u)!=0x12u ||
           XM87142::ramW(source,0x4852u)!=0x14u)return;
        ram=source;frame=now;phase=dhcPhase;
        count=0;dropped=0;visited=0;
        prior=m68k_get_insn_cb();
        // Never recurse, and never replace another callback without
        // forwarding it and restoring it at the CPU frame boundary.
        m68k_set_insn_cb(&XM87182PC::State::bridge);
        active=true;
    }
    static int bridge(int cycles){
        State& s=state();
        if(s.prior && s.prior!=&XM87182PC::State::bridge)
            s.prior(cycles);
        if(s.active && s.ram)s.record(cycles);
        return 0; // underlying Musashi ignores return; no cycle changes
    }
    void end(){
        if(!active)return;
        active=false;
        m68k_set_insn_cb(prior);
        prior=nullptr;
        if(log){
            std::fprintf(log,"# DHC_FRAME frame=%u watched=%u dropped=%u"
                    " allInstructions=%u\n",frame,count,dropped,visited);
            for(unsigned i=0;i<count;++i){
                const Entry& e=entries[i];
                std::fprintf(log,"%u\t%u\t%u\t%s\t%06X\t%06X"
                     "\t%06X\t%08X\t%u\t%04X\t%04X\t%02X"
                     "\t%02X\t%02X\t%04X\t%04X\t%04X\t%04X\n",
                    frame,phase,i,region(e.pc),e.pc,e.nextPc,e.a6,e.d0,
                    e.cycles,e.action,e.mode,e.d6,e.ctrl256,e.ctrl259,
                    e.c0,e.c2,e.c6,e.ca);
            }
            std::fflush(log);
        }
        ram=nullptr;
    }
};
// Forward declaration for the static callback that references State.
inline State& state(){static State instance;return instance;}
} // namespace XM87182PC
#endif
