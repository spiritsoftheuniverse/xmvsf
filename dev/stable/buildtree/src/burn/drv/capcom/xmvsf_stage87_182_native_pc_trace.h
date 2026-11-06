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
    unsigned pc=0,fromPc=0,nextPc=0,a6=0,d0=0,d1=0,cycles=0,ordinal=0,changed=0;
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
struct State;
inline State& state(); // callback needs a forward declaration
struct State {
    enum { LIMIT=8192 };
    bool initialized=false,enabled=false,active=false,openingFrame=false;
    unsigned openingStaticRows=0;
    std::FILE* log=nullptr;
    const std::uint8_t*ram=nullptr;
    unsigned frame=0,phase=0,count=0,dropped=0,visited=0;
    unsigned lastAction=0,lastMode=0,lastD6=0,last256=0,last259=0;
    unsigned lastC2=0,lastCA=0,previousExecuted=0;
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
        std::fputs("frame\tphase\tordinal\tregion\tPPC\tfromPPC\tPC_next\tA6\tD0\tD1"
                   "\tcycles\tordinalCPU\tchangeMask\tRyuAction\tRyuA0\tRyuD6\tcontroller256"
                   "\tcontroller259\tC0\tC2\tC6\tCA\n",log);
        std::fflush(log);
    }
    void record(int cycles){
        ++visited;
        // SekGetPPC records the PC of the just executed original 68000
        // instruction, not the next instruction's PC.
        const unsigned pc=SekGetPPC(-1)&0xFFFFFFu;
        const unsigned previousPC=previousExecuted;
        previousExecuted=pc;
        // Read-only state deltas pinpoint the *executed instruction*
        // that first changes Ryu's action/mode/D6/input edge, even if
        // its PC is outside the known command-dispatch ranges.
        const unsigned action=XM87142::ramW(ram,0x4060u);
        const unsigned mode=XM87142::ramW(ram,0x40A0u);
        const unsigned d6=XM87142::ramB(ram,0x40D6u);
        const unsigned c256=XM87142::ramB(ram,0x4256u);
        const unsigned c259=XM87142::ramB(ram,0x4259u);
        const unsigned c2=XM87142::ramW(ram,0x40C2u);
        const unsigned ca=XM87142::ramW(ram,0x40CAu);
        const unsigned flags=
           (action!=lastAction?1u:0u)|(mode!=lastMode?2u:0u)|
           (d6!=lastD6?4u:0u)|(c256!=last256?8u:0u)|
           (c259!=last259?16u:0u)|(c2!=lastC2?32u:0u)|
           (ca!=lastCA?64u:0u);
        lastAction=action;lastMode=mode;lastD6=d6;
        last256=c256;last259=c259;lastC2=c2;lastCA=ca;
        if(!watch(pc) && !flags)return;
        // 87.186: during an unarmed startup/early attack sample,
        // retain all actual native state changes but cap non-changing
        // watched PCs at 96 per frame. The older armed-controller PPC
        // trace remains byte-for-byte identical in its selection rules.
        if(openingFrame && !flags){
            if(openingStaticRows>=96u)return;
            ++openingStaticRows;
        }
        // Keep up to 8192 trace entries per DHC frame. If the detailed
        // filtered PC trace saturates, reserve the final row for the
        // latest native actor-state change. Truncation is explicit.
        if(count>=LIMIT){
            ++dropped;
            if(!flags)return;
            Entry& e=entries[LIMIT-1];
            e.pc=pc;e.fromPc=previousPC;e.nextPc=SekGetPC(-1)&0xFFFFFFu;
            e.a6=(unsigned)m68k_get_reg(nullptr,M68K_REG_A6)&0xFFFFFFu;
            e.d0=(unsigned)m68k_get_reg(nullptr,M68K_REG_D0);e.d1=(unsigned)m68k_get_reg(nullptr,M68K_REG_D1);
            e.cycles=(unsigned)cycles;e.ordinal=visited;e.changed=flags;
            e.action=action;e.mode=mode;e.d6=d6;
            e.ctrl256=c256;e.ctrl259=c259;
            e.c0=XM87142::ramW(ram,0x40C0u);
            e.c2=c2;e.c6=XM87142::ramW(ram,0x40C6u);e.ca=ca;
            return;
        }
        Entry& e=entries[count++];
        e.pc=pc;e.fromPc=previousPC;e.nextPc=SekGetPC(-1)&0xFFFFFFu;
        e.ordinal=visited;e.changed=flags;
        e.a6=(unsigned)m68k_get_reg(nullptr,M68K_REG_A6)&0xFFFFFFu;
        e.d0=(unsigned)m68k_get_reg(nullptr,M68K_REG_D0);e.d1=(unsigned)m68k_get_reg(nullptr,M68K_REG_D1);
        e.cycles=(unsigned)cycles;
        // Snapshot AFTER the executed instruction: read-only, so the
        // exact PC that modified action/D6/A0 can be correlated.
        e.action=action;e.mode=mode;e.d6=d6;
        e.ctrl256=c256;e.ctrl259=c259;
        e.c0=XM87142::ramW(ram,0x40C0u);
        e.c2=c2;
        e.c6=XM87142::ramW(ram,0x40C6u);
        e.ca=ca;
    }
    // Invoke before native input sampling, after SekOpen(0), only for
    // an already armed, early-cancel Ryu Shinkuu -> Ken DHC.
    void begin(const std::uint8_t*source,unsigned now,unsigned dhcPhase,
               bool earlyCancel,bool runahead,bool openingRequest=false){
        init();
        if(active || !enabled || runahead || !earlyCancel || !source ||
           (dhcPhase!=2u && !openingRequest) ||
           XM87142::ramB(source,0x5000u)!=8u ||
           XM87142::ramW(source,0x4052u)!=0x12u ||
           XM87142::ramW(source,0x4852u)!=0x14u)return;
        ram=source;frame=now;phase=dhcPhase;
        openingFrame=(dhcPhase!=2u && openingRequest);
        openingStaticRows=0u;
        count=0;dropped=0;visited=0;previousExecuted=0;
        lastAction=XM87142::ramW(source,0x4060u);
        lastMode=XM87142::ramW(source,0x40A0u);
        lastD6=XM87142::ramB(source,0x40D6u);
        last256=XM87142::ramB(source,0x4256u);
        last259=XM87142::ramB(source,0x4259u);
        lastC2=XM87142::ramW(source,0x40C2u);
        lastCA=XM87142::ramW(source,0x40CAu);
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
            std::fprintf(log,openingFrame?
                    "# OPENING_FRAME frame=%u watched=%u dropped=%u allInstructions=%u\n":
                    "# DHC_FRAME frame=%u watched=%u dropped=%u allInstructions=%u\n",
                    frame,count,dropped,visited);
            for(unsigned i=0;i<count;++i){
                const Entry& e=entries[i];
                std::fprintf(log,"%u\t%u\t%u\t%s\t%06X\t%06X\t%06X"
                     "\t%06X\t%08X\t%08X\t%u\t%u\t%02X\t%04X\t%04X\t%02X"
                     "\t%02X\t%02X\t%04X\t%04X\t%04X\t%04X\n",
                    frame,phase,i,region(e.pc),e.pc,e.fromPc,e.nextPc,e.a6,e.d0,e.d1,
                    e.cycles,e.ordinal,e.changed,e.action,e.mode,e.d6,e.ctrl256,e.ctrl259,
                    e.c0,e.c2,e.c6,e.ca);
            }
            std::fflush(log);
        }
        ram=nullptr;
        openingFrame=false;
    }
};
// Forward declaration for the static callback that references State.
inline State& state(){static State instance;return instance;}
} // namespace XM87182PC
#endif
