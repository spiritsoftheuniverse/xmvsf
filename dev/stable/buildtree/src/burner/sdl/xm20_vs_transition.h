#ifndef XMVSF_STAGE20_1_VS_TRANSITION_H
#define XMVSF_STAGE20_1_VS_TRANSITION_H
// 87.322: cue the song when the native versus presentation STARTS,
// not when final character selection finishes or the match opens.
// FF5104 leaving chooser step 04 for intermediate precombat step 05/06/07
// is the primary event. FF5000=02 is too late for a fallback.
// This observer reads original 68000 state only; no game-state writes.
#include <cstdio>
#include <cstdint>
#include "xm20_vs_early_phase.h"

struct XM20VsTracker {
    unsigned int polls=0;
    unsigned int previousStatus=0xffffffffu,previousPointer=0xffffffffu;
    unsigned int gameStep=0xffffffffu,p1Choice=0xffffffffu,p2Choice=0xffffffffu;
    XM20VsEarly::Gate gate{};
    bool triggered=false,combatNotified=false,missedLogged=false;
    FILE* log=nullptr;
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
    const unsigned gameStep=cpu->read(0xFF5104u)&255u;
    const unsigned p1Choice=cpu->read(0xFF5180u)&255u;
    const unsigned p2Choice=cpu->read(0xFF51A0u)&255u;
    cpu->close();
    ++xm20Vs.polls;
    const unsigned oldStep=xm20Vs.gameStep;
    xm20Vs.gameStep=gameStep;
    xm20Vs.p1Choice=p1Choice;
    xm20Vs.p2Choice=p2Choice;

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
    // ROM variants that do not expose intermediate step 06 may jump
    // directly 04->08. Honor that only while still in status 00/01,
    // never at the stage-load/actual combat boundary.
    const bool screenEntry=
        entrance==XM20VsEarly::VS_SCREEN_EDGE ||
        (entrance==XM20VsEarly::MATCH_SETUP_EDGE && status<0x02u) ||
        (status==0x01u && xm20Vs.previousStatus==0x00u &&
         xm20Vs.gate.sawChoosing);
    if(!xm20Vs.triggered && screenEntry){
        xm20VsLog(entrance==XM20VsEarly::VS_SCREEN_EDGE?
            "VS_SCREEN_FIRST_NATIVE_FRAME":
            entrance==XM20VsEarly::MATCH_SETUP_EDGE?
            "VS_SCREEN_DIRECT_04_08":"VS_SCREEN_STATUS_00_01",
            status,p1ptr);
        xm20Vs.triggered=true;
        xmvsfHost::enterVs(); // reset and start versus music here ONLY
    }
    // Never start the VS song on a late FF5000=02..07 fallback. This
    // used to fire as the match opened, which is exactly the wrong screen.
    if(!xm20Vs.triggered && !xm20Vs.missedLogged && status>=0x02u){
        xm20Vs.missedLogged=true;
        xm20VsLog("VS_ENTRY_MISSED_NO_LATE_MUSIC",status,p1ptr);
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
