#ifndef XMVSF_STAGE87_175_NATIVE_ACCEPT_OBSERVER_H
#define XMVSF_STAGE87_175_NATIVE_ACCEPT_OBSERVER_H
// 87.175: opt-in read-only, post-CPU vs post-DHC native state observer.
// No CPS2 RAM writes, no CpsCode patches, no control injection, no super registry.
// Requires XM87142::ram* reader from xmvsf_stage87_142_rom_dispatch.h.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87175Observer {
struct Actor {
    unsigned fighter=0, action=0, mode=0, phase08=0, d6=0, a4=0;
    unsigned script=0, controller256=0, controller259=0;
    unsigned c0=0, c2=0, c4=0, c6=0, c8=0, ca=0;
};
static inline Actor read(const std::uint8_t* r, unsigned base) {
    Actor a;
    if (!r) return a;
    a.fighter=XM87142::ramW(r,base+0x52u);
    a.action=XM87142::ramW(r,base+0x60u);
    a.mode=XM87142::ramW(r,base+0xA0u);
    a.phase08=XM87142::ramW(r,base+0x08u);
    a.d6=XM87142::ramB(r,base+0xD6u);
    a.a4=XM87142::ramB(r,base+0xA4u);
    a.script=XM87142::ramL(r,base+0x34u);
    a.controller256=XM87142::ramB(r,base+0x256u);
    a.controller259=XM87142::ramB(r,base+0x259u);
    a.c0=XM87142::ramW(r,base+0xC0u);
    a.c2=XM87142::ramW(r,base+0xC2u);
    a.c4=XM87142::ramW(r,base+0xC4u);
    a.c6=XM87142::ramW(r,base+0xC6u);
    a.c8=XM87142::ramW(r,base+0xC8u);
    a.ca=XM87142::ramW(r,base+0xCAu);
    return a;
}
static inline bool differs(const Actor& a,const Actor& b){
    return a.fighter!=b.fighter || a.action!=b.action ||
        a.mode!=b.mode || a.phase08!=b.phase08 || a.d6!=b.d6 ||
        a.a4!=b.a4 || a.script!=b.script ||
        a.controller256!=b.controller256 || a.controller259!=b.controller259 ||
        a.c0!=b.c0 || a.c2!=b.c2 || a.c4!=b.c4 ||
        a.c6!=b.c6 || a.c8!=b.c8 || a.ca!=b.ca;
}
static inline const char* slotName(unsigned slot){
    return (slot&1u)?"RESERVE":"POINT";
}
static inline unsigned slotBase(unsigned slot){
    return (slot&1u?0x4800u:0x4000u)+(slot>>1u)*0x400u;
}
struct State {
    bool initialized=false,enabled=false,seeded=false,pending=false;
    unsigned lastFrame=~0u;
    Actor previous[4],beforeHost[4];
    const char* phaseBefore="IDLE";
    std::FILE* log=nullptr;
    ~State(){if(log){std::fflush(log);std::fclose(log);}}
    void init(){
        if(initialized)return;
        initialized=true;
        const char* setting=std::getenv("XM87175_DHC_OBSERVER");
        const char* path=std::getenv("XM87175_DHC_OBSERVER_LOG");
        if(!setting || std::strcmp(setting,"1")!=0 || !path || !*path)
            return;
        // Append: never truncate the user's previous F9/replay evidence.
        log=std::fopen(path,"ab");
        if(!log){
            std::fprintf(stderr,"XM87.175 observer cannot open log: %s\n",path);
            return;
        }
        enabled=true;
        std::fprintf(log,"# XM87.175 OBSERVER SESSION: post CPU versus post DHC host; "
            "script ownership is observational only; ROM acceptance not inferred from host writes\n");
        std::fputs("frame\tmovie_frame\tstage\tside\tslot\tfighter\tphase\tinputSynthetic"
            "\tinjectedDir\tinjectedButtons\td6\tmodeA0\tphase08\taction"
            "\tscript34\tcontroller256\tcontroller259\ta4\tc0\tc2\tc4\tc6\tc8\tca\n",log);
        std::fflush(log);
    }
    void row(unsigned frame,unsigned movie,unsigned slot,const char*stage,
             const char*phase,bool synthetic,unsigned dir,unsigned buttons,
             const Actor&a){
        if(!log)return;
        std::fprintf(log,"%u\t%u\t%s\t%u\t%s\t%04X\t%s\t%u\t%02X\t%02X"
            "\t%02X\t%04X\t%04X\t%04X\t%08X\t%02X\t%02X\t%02X"
            "\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\n",
            frame,movie,stage,slot/2u+1u,slotName(slot),a.fighter,
            phase?phase:"UNKNOWN",synthetic?1u:0u,dir,buttons,
            a.d6,a.mode,a.phase08,a.action,a.script,a.controller256,
            a.controller259,a.a4,a.c0,a.c2,a.c4,a.c6,a.c8,a.ca);
    }
    // Call after the 68000 CPU has executed and before XM87144::state().frame.
    void postCpu(const std::uint8_t*ram,unsigned frame,unsigned movie,
                 const char*phase,bool synthetic,unsigned dir,unsigned buttons){
        init();
        pending=false;
        if(!enabled || !ram)return;
        if(lastFrame!=~0u && frame<=lastFrame){seeded=false;}
        lastFrame=frame;
        if(XM87142::ramB(ram,0x5000u)!=8u){seeded=false;return;}
        phaseBefore=phase?phase:"UNKNOWN";
        const bool phaseActive=phase && std::strcmp(phase,"IDLE")!=0;
        for(unsigned slot=0;slot<4u;++slot){
            Actor a=read(ram,slotBase(slot));
            beforeHost[slot]=a;
            if(!seeded || differs(a,previous[slot]) || synthetic || phaseActive)
                row(frame,movie,slot,"POST_CPU_PRE_DHC",phase,synthetic,dir,buttons,a);
        }
        pending=true;
        seeded=true;
    }
    // Call immediately after XM87144::state().frame: explicit host-write check.
    void postDhc(const std::uint8_t*ram,unsigned frame,unsigned movie,
                 const char*phase,bool synthetic,unsigned dir,unsigned buttons){
        if(!pending || !enabled || !ram || frame!=lastFrame)return;
        const bool phaseChanged=std::strcmp(phaseBefore,phase?phase:"UNKNOWN")!=0;
        for(unsigned slot=0;slot<4u;++slot){
            Actor a=read(ram,slotBase(slot));
            const bool ramChanged=differs(a,beforeHost[slot]);
            if(ramChanged || (phaseChanged && slot==0u))
                row(frame,movie,slot,ramChanged?"POST_HOST_DHC_CHANGED":
                    "POST_HOST_DHC_PHASE_ONLY",phase,synthetic,dir,buttons,a);
            previous[slot]=a;
        }
        pending=false;
        // Persist diagnostics at each host-frame boundary, including failures.
        if(log)std::fflush(log);
    }
};
inline State& state(){static State s;return s;}
} // namespace XM87175Observer
#endif
