#ifndef XMVSF_STAGE20_1_VS_TRANSITION_H
#define XMVSF_STAGE20_1_VS_TRANSITION_H
// Earlier audio handoff without changing the Stage20 WASAPI mixer, game clock,
// save states, character input or ROM. This code only READS 68000 state.
// Stage17 phase is present in the proven Stage18 source path (Training only).
#include <cstdio>
#include <cstdint>

struct XM20VsTracker {
    unsigned int polls=0, selectionSamples=0, nonActorSamples=0;
    unsigned int selectionStatus=0xffffffffu, previousStatus=0xffffffffu;
    unsigned int previousPointer=0xffffffffu;
    bool armed=false, triggered=false, combatNotified=false;
    FILE* log=nullptr;
};
static XM20VsTracker xm20Vs;

static inline void xm20VsLog(const char* why,unsigned int status,unsigned int p1ptr) {
    if(!xm20Vs.log) xm20Vs.log=std::fopen("xmvsf_stage20_1_vs.log","w");
    if(xm20Vs.log){
        std::fprintf(xm20Vs.log,"frame=%u reason=%s status=%02X p1ptr=%08X training=%d selectPhase=%u selected=%u armed=%d\n",
            xm20Vs.polls,why,status,p1ptr,(int)xm14TrainingMode,
            xm17.phase,xm20Vs.selectionStatus,(int)xm20Vs.armed);
        std::fflush(xm20Vs.log);
    }
}

static inline void xm20VsCheck(int ready,int paused) {
    if(!ready || paused || bBurnRunAheadFrame || xm20Vs.combatNotified)return;
    cpu_core_config* cpu=GetCpuCoreConfig(0);
    if(!cpu || !cpu->open || !cpu->close || !cpu->read)return;
    cpu->open(0);
    // FF5000 first transitions out of character select to the native
    // pre-fight/VS sequence at 02. The fighter pointer often remains
    // unset until COMBAT; waiting for it moves the BGM one screen late.
    const unsigned status=cpu->read(0xFF5000u)&255u;
    const unsigned p1ptr=((cpu->read(0xFFA014u)&255u)<<24)|
       ((cpu->read(0xFFA015u)&255u)<<16)|
       ((cpu->read(0xFFA016u)&255u)<<8)|(cpu->read(0xFFA017u)&255u);
    cpu->close();
    ++xm20Vs.polls;
    if(status==0x08u) {
        // Combat always terminates the VS track. If this session skipped
        // pre-combat statuses, cut menu music, but do NOT start VS music late.
        if(!xm20Vs.triggered)xm20VsLog("COMBAT_WITHOUT_VS_PHASE",status,p1ptr);
        else xm20VsLog("VS_TO_COMBAT",status,p1ptr);
        xm20Vs.triggered=true;
        xm20Vs.combatNotified=true;
        xmvsfHost::enterCombat();
        return;
    }
    if(!xm20Vs.triggered && status>=0x02u && status<0x08u) {
        xm20VsLog("VS_NATIVE_PRECOMBAT_PHASE",status,p1ptr);
        xm20Vs.triggered=true;
        xmvsfHost::enterVs();
        xm20Vs.previousStatus=status;
        xm20Vs.previousPointer=p1ptr;
        return;
    }
    // Observe the pre- versus character-select phase without treating
    // a late actor pointer as screen identity.
    if(!xm20Vs.triggered &&
       (status!=xm20Vs.previousStatus || p1ptr!=xm20Vs.previousPointer) &&
       xm20Vs.polls<6000u)
        xm20VsLog("SELECTION_PHASE",status,p1ptr);
    xm20Vs.previousStatus=status;
    xm20Vs.previousPointer=p1ptr;
}
#endif
