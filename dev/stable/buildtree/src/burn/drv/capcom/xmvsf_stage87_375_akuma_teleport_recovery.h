#ifndef XMVSF_STAGE87_375_AKUMA_TELEPORT_RECOVERY_H
#define XMVSF_STAGE87_375_AKUMA_TELEPORT_RECOVERY_H
// XMVSF 87.375: Akuma native teleport recovery 10 frames -> 1 frame.
// F10 87.374/001: teleport motion action 2C38 (step 2->4), followed
// by 2C3A / step 6 / script 0EB372 / sprite 4FAE for ten frames.
// F10 87.374/002 confirms same ROM tail across opposite directions.
// LP pressed in 2C3A was ignored; first normal action came at its exit.
//
// Apply exactly ONCE, on the first post-CPU appearance of 2C3A,
// while leaving the original ROM controller to select neutral/input.
// This does not alter position, velocity, facing, action, phase, input,
// script or animation; importantly 2C38 travel is never shortened.
// Version 3 unaffected. Only the original US ROM. Zero never written.
// Disable for comparison: XM87375_AKUMA_TELEPORT_RECOVERY=0.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87375Teleport {
struct Side {
    unsigned previousAction=~0u,previousId=~0u,lastFrame=~0u;
    unsigned start=0u;
    bool shortened=false;
};
struct State {
    Side side[2];
    void reset(){side[0]=Side{};side[1]=Side{};}
};
inline State& state(){static State s;return s;}
inline bool enabled(){
    const char* v=std::getenv("XM87375_AKUMA_TELEPORT_RECOVERY");
    return !v || std::strcmp(v,"0")!=0;
}
inline void writeWord(std::uint8_t* ram,unsigned address,unsigned value){
    ram[(address&65535u)^1u]=std::uint8_t(value>>8u);
    ram[((address+1u)&65535u)^1u]=std::uint8_t(value);
}
inline void afterCpu(std::uint8_t* ram,const std::uint8_t* rom,
                     unsigned romSize,unsigned now,const char* driver,
                     bool version4,bool runahead){
    State& st=state();
    if(!version4 || runahead || !ram || !rom || !driver ||
       std::strcmp(driver,"xmvsfu")!=0 ||
       XM87142::ramB(ram,0x5000u)!=8u){
        st.reset();return;
    }
    const unsigned target=0x0EB372u;
    // Verify the actual US ROM frame; never trust just a RAM pointer
    // on an unrecognized regional variant or a corrupted actor.
    const bool targetVerified=target<=romSize &&
        romSize-target>=16u &&
        XM87142::romW(rom,romSize,target+6u)==0x4FAEu;
    for(unsigned side=0u;side<2u;++side){
        Side& t=st.side[side];
        if(t.lastFrame==now)continue;
        if(t.lastFrame!=~0u && now<t.lastFrame)t=Side{};
        const unsigned p=0x4000u+0x400u*side;
        const unsigned id=XM87142::ramW(ram,p+0x52u);
        const unsigned action=XM87142::ramW(ram,p+0x60u);
        // Report the real original 68000 return from recovery, not a
        // host-written idle action or an assumed one-frame success.
        if(t.shortened && (action!=0x2C3Au || id!=0x1Eu)){
            if(id==0x1Eu){
                std::fprintf(stderr,
                    "XM87.375 AKUMA_TELEPORT_EXIT frame=%u side=%u "
                    "recovery_frames=%u target=1 next_action=%04X\n",
                    now,side+1u,now-t.start,action);
            }
            t.shortened=false;
        }
        const bool entry=enabled() && targetVerified &&
            t.previousId==0x1Eu && id==0x1Eu &&
            t.previousAction==0x2C38u && action==0x2C3Au &&
            XM87142::ramW(ram,p+0x08u)==6u &&
            (XM87142::ramW(ram,p+0xA0u)&0xFF00u)==0x2A00u &&
            XM87142::ramL(ram,p+0x34u)==target &&
            XM87142::ramL(ram,p+0x38u)==target &&
            XM87142::ramW(ram,p+0x42u)==0x4FAEu;
        if(entry){
            const unsigned before=XM87142::ramW(ram,p+0x3Eu);
            if(before>1u && before<0x1000u){
                // The native animation runner's positive countdown.
                // No synthetic action/neutral/hitbox or movement writes.
                writeWord(ram,p+0x3Eu,1u);
                t.shortened=true;t.start=now;
                std::fprintf(stderr,
                    "XM87.375 AKUMA_TELEPORT_TAIL frame=%u side=%u "
                    "2C3A/0006 script=0EB372 native_timer=%u->1\n",
                    now,side+1u,before);
            }else{
                std::fprintf(stderr,
                    "XM87.375 AKUMA_TELEPORT_TIMER_UNCHANGED frame=%u "
                    "side=%u timer=%u (not a positive hold)\n",
                    now,side+1u,before);
            }
        }
        t.previousAction=action;t.previousId=id;t.lastFrame=now;
    }
}
} // namespace XM87375Teleport
#endif
