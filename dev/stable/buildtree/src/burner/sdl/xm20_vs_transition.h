#ifndef XMVSF_STAGE20_1_VS_TRANSITION_H
#define XMVSF_STAGE20_1_VS_TRANSITION_H
// 87.324 replay-grounded correction: FF5104 is big-endian WORD 0004,
// not an 8-bit register. Uploaded 87.320 checkpoint: FF5104=00 FF5105=04,
// FF5180=07 FF51A0=03. Previous byte-only read made VS detection impossible.
// Trigger on the actual native selection->precombat step edge, log M68K
// PCs per replay frame; do not invent an FF5000 timing window.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include "../../cpu/m68000_intf.h"
#include "xm20_vs_early_phase.h"

struct XM20VsTracker {
    unsigned int polls=0;
    unsigned int previousStatus=0xffffffffu,previousPointer=0xffffffffu;
    unsigned int gameStep=0xffffffffu,p1Choice=0xffffffffu,p2Choice=0xffffffffu;
    XM20VsEarly::Gate gate{};
    bool triggered=false,combatNotified=false,missedLogged=false;
    FILE* log=nullptr;
    FILE* trace=nullptr;
    unsigned traceCount=0;
};
static XM20VsTracker xm20Vs;

static inline void xm20VsLog(const char* why,unsigned status,unsigned p1ptr) {
    if(!xm20Vs.log)xm20Vs.log=std::fopen("xmvsf_stage20_1_vs.log","w");
    if(xm20Vs.log){
        std::fprintf(xm20Vs.log,
            "frame=%u reason=%s status=%02X p1ptr=%08X gameStep=%02X chooserP1=%02X chooserP2=%02X selecting=%u owned=%u\n",
            xm20Vs.polls,why,status,p1ptr,
            xm20Vs.gameStep,xm20Vs.p1Choice,xm20Vs.p2Choice,
            (unsigned)xm20Vs.gate.sawActiveChooser,(unsigned)xm20Vs.triggered);
        std::fflush(xm20Vs.log);
    }
}

// Read-only exact replay/M68K diagnostic. No new save states or CPU writes.
static inline void xm20VsTrace(unsigned status,unsigned step,unsigned p1,
                              unsigned p2,unsigned p1ptr,unsigned pc,
                              unsigned ppc,unsigned opcode,unsigned stage,
                              unsigned timer){
    const char* target=std::getenv("XMVSF_VS_M68K_TRACE_LOG");
    if(!target||!*target||xm20Vs.traceCount>=1200u)return;
    if(!xm20Vs.trace){
        xm20Vs.trace=std::fopen(target,"wb");
        if(!xm20Vs.trace)return;
        std::fprintf(xm20Vs.trace,
            "movie_frame\tnative_frame\tpc\tppc\topcode\tstatus_FF5000\tstep_FF5104_WORD\tP1_FF5180\tP2_FF51A0\tstage_FF5113\ttimer_FF5008\tactor_ptr\ttrigger\n");
    }
    const XM8764::Context& replay=XM8764::context();
    const unsigned movieFrame=replay.allFrames?replay.allFrames-1u:0u;
    std::fprintf(xm20Vs.trace,
        "%u\t%u\t%06X\t%06X\t%04X\t%02X\t%04X\t%02X\t%02X\t%02X\t%02X\t%08X\t%d\n",
        movieFrame,(unsigned)GetCurrentFrame(),pc,ppc,opcode,status,step,
        p1,p2,stage,timer,p1ptr,(int)xm20Vs.triggered);
    std::fflush(xm20Vs.trace);
    ++xm20Vs.traceCount;
}
static inline void xm20VsClose(){
    if(xm20Vs.trace){std::fclose(xm20Vs.trace);xm20Vs.trace=nullptr;}
    if(xm20Vs.log){std::fclose(xm20Vs.log);xm20Vs.log=nullptr;}
}
static inline void xm20VsCheck(int ready,int paused) {
    if(!ready || paused || bBurnRunAheadFrame || xm20Vs.combatNotified)return;
    cpu_core_config* cpu=GetCpuCoreConfig(0);
    if(!cpu || !cpu->open || !cpu->close || !cpu->read)return;
    cpu->open(0);
    const unsigned status=cpu->read(0xFF5000u)&255u;
    const unsigned p1ptr=((cpu->read(0xFFA014u)&255u)<<24)|
       ((cpu->read(0xFFA015u)&255u)<<16)|
       ((cpu->read(0xFFA016u)&255u)<<8)|(cpu->read(0xFFA017u)&255u);
    // Original game: FF5104 step 04 choosing, 08 match setup.
    // FF5180 and FF51A0 are separate 0x20-byte chooser records.
    // Byte zero reaches 07 after each player's full team and speed picks.
    const unsigned gameStep=((cpu->read(0xFF5104u)&255u)<<8)|
                            (cpu->read(0xFF5105u)&255u); // 68000 big-endian WORD
    const unsigned p1Choice=cpu->read(0xFF5180u)&255u;
    const unsigned p2Choice=cpu->read(0xFF51A0u)&255u;
    // Exact M68K PC/PPC after this native frame; ROM opcode read only.
    const unsigned pc=SekGetPC(-1),ppc=SekGetPPC(-1);
    const unsigned opcode=pc<0x400000u?
        ((cpu->read(pc)&255u)<<8)|(cpu->read(pc+1u)&255u):0u;
    const unsigned stage=cpu->read(0xFF5113u)&255u;
    const unsigned timer=cpu->read(0xFF5008u)&255u;
    cpu->close();
    ++xm20Vs.polls;
    const unsigned oldStep=xm20Vs.gameStep;
    xm20Vs.gameStep=gameStep;
    xm20Vs.p1Choice=p1Choice;
    xm20Vs.p2Choice=p2Choice;
    xm20VsTrace(status,gameStep,p1Choice,p2Choice,p1ptr,pc,ppc,opcode,stage,timer);

    if(status==0x08u){
        xm20VsLog(xm20Vs.triggered?"VS_TO_COMBAT":
                                     "COMBAT_WITHOUT_VS_PHASE",status,p1ptr);
        xm20Vs.triggered=true;
        xm20Vs.combatNotified=true;
        xmvsfHost::enterCombat();
        return;
    }
    const XM20VsEarly::Event entrance=
        xm20Vs.gate.observe(gameStep,p1Choice,p2Choice);
    // Use only the observed 16-bit M68K chooser->match handoff,
    // not the old guessed FF5000 threshold or late status fallback.
    const bool screenEntry=status<0x08u &&
        (entrance==XM20VsEarly::VS_SCREEN_EDGE ||
         entrance==XM20VsEarly::MATCH_SETUP_EDGE);
    if(!xm20Vs.triggered && screenEntry){
        xm20VsLog(entrance==XM20VsEarly::VS_SCREEN_EDGE?
            "VS_M68K_HANDOFF_0004_TO_INTERMEDIATE":
            "VS_M68K_HANDOFF_0004_TO_0008",status,p1ptr);
        xm20Vs.triggered=true;
        xmvsfHost::enterVs(); // reset and start versus music here ONLY
    }
    // No status-only fallback. A missing ROM handoff remains a logged miss.
    if(!xm20Vs.triggered && !xm20Vs.missedLogged && status>=0x08u){
        xm20Vs.missedLogged=true;
        xm20VsLog("VS_M68K_EDGE_MISSING",status,p1ptr);
    }
    if(!xm20Vs.triggered &&
       (status!=xm20Vs.previousStatus ||
        p1ptr!=xm20Vs.previousPointer ||
        oldStep!=gameStep) &&
       xm20Vs.polls<6000u)
        xm20VsLog("SELECTION_PHASE",status,p1ptr);
    xm20Vs.previousStatus=status;
    xm20Vs.previousPointer=p1ptr;
}
#endif
