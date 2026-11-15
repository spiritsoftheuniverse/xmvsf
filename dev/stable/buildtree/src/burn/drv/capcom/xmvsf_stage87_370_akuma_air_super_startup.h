#ifndef XMVSF_STAGE87_370_AKUMA_AIR_SUPER_STARTUP_H
#define XMVSF_STAGE87_370_AKUMA_AIR_SUPER_STARTUP_H
// XMVSF 87.372 - air charge-frame duration fix; ground startup preserved.
// Source: 87.370 F10 combat trace. Ryu QCF+PPP 2E08/step2 -> step4: 67f.
// Akuma 2E06 air super: 74f, opening 0EB87A, charge 0EB8BA.
// Akuma 2E02 ground super (QCB+PPP candidate): 70-71f, opening
// 0EB692, charge 0EB6A2. These are native action observations, not proof
// of the input matcher or the first frame a projectile can deal damage.
//
// Do NOT dispatch 2E08/2E04 directly: original 68000 runs every super
// stage, including meter, blackout, spawned objects and recovery.
// The 87.370 native trace shows Akuma air 2E06 holds its 0EB8BA /
// animation 64D4 charge pose for approximately 52 game frames. In 87.371
// an additional +14 frames extended the hold. The target charge pose
// is correct, but the timer must not reset to its first/full duration.
// 87.372 gives the AIR pose two native ticks (adjustable 1..16) and
// then allows the original 68000 to enter its own 2E08 attack action.
// Ground 2E02 continues with the 87.371 timing. Never force 2E08.
//
// Opt out: XM87371_AKUMA_SUPER_STARTUP=0 (both moves),
// or XM87370_AKUMA_AIR_SUPER_FAST=0 (air only).
// Tune without recompiling: XM87372_AKUMA_AIR_CHARGE_FRAMES=1..16 (default 2).
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace XM87370Akuma {
static const unsigned targetFrames=67u;
struct Move {
    unsigned action, modeMask, d6, rootScript, rootAnim;
    unsigned chargeScript, chargeAnim, extraHold, finalScript;
    const char* label;
};
static inline Move airMove(){
    return {0x2E06u,0u,0x0Au,0x0EB87Au,0x59FAu,
            0x0EB8BAu,0x64D4u,0u,0x0EB8CAu,"AIR_2E06"};
}
static inline Move groundMove(){
    return {0x2E02u,0x2A28u,0x1Au,0x0EB692u,0x67F8u,
            0x0EB6A2u,0x6832u,0u,0x0EB762u,"QCB_PPP_2E02"};
}
static inline void putW(std::uint8_t* r,unsigned p,unsigned v){
    r[(p&65535u)^1u]=std::uint8_t(v>>8u);
    r[((p+1u)&65535u)^1u]=std::uint8_t(v);
}
static inline void putL(std::uint8_t* r,unsigned p,unsigned v){
    putW(r,p,v>>16u);putW(r,p+2u,v);
}
struct Tracker {
    bool active=false,deadlineNudged=false;
    unsigned entered=0u,action=0u;
};
struct State {Tracker side[2];};
static inline State& state(){static State s;return s;}
static inline bool enabled(const Move& m){
    const char* all=std::getenv("XM87371_AKUMA_SUPER_STARTUP");
    if(all && std::strcmp(all,"0")==0)return false;
    const char* air=std::getenv("XM87370_AKUMA_AIR_SUPER_FAST");
    if(m.action==0x2E06u && air && std::strcmp(air,"0")==0)return false;
    return true;
}
// The original charge-pose ROM timer is much too long for a connecting
// airborne fireball. Clamp the replacement to a small positive count:
// zero is an original animation-runner sentinel and is never written.
static inline unsigned airChargeFrames(){
    const char* value=std::getenv("XM87372_AKUMA_AIR_CHARGE_FRAMES");
    if(!value || !*value)return 2u;
    char* end=nullptr;
    const unsigned long count=std::strtoul(value,&end,10);
    return end && *end==0 && count>=1ul && count<=16ul ?
        static_cast<unsigned>(count):2u;
}
static inline bool matches(const std::uint8_t* r,unsigned p,
                           const Move& m,bool atRoot){
    if(!r || XM87142::ramW(r,p)==0u ||
       XM87142::ramW(r,p+0x52u)!=0x001Eu ||
       XM87142::ramW(r,p+0x60u)!=m.action ||
       XM87142::ramW(r,p+0x08u)!=2u ||
       XM87142::ramB(r,p+0xD6u)!=m.d6)return false;
    const unsigned mode=XM87142::ramW(r,p+0xA0u);
    const unsigned y=XM87142::ramW(r,p+0x10u);
    const unsigned floor=XM87142::ramW(r,p+0x254u);
    if(floor<24u || floor>=2048u)return false;
    if(m.action==0x2E06u){
        // 87.368: 2A28, 87.370: 2A1C. Both are genuine airborne
        // Akuma 2E06 with identical original ROM startup records.
        if((mode!=0x2A1Cu && mode!=0x2A28u) || y+24u>=floor)return false;
    }else if(mode!=m.modeMask || y+16u<floor || y>floor+16u)return false;
    if(!atRoot)return true;
    return XM87142::ramL(r,p+0x34u)==m.rootScript &&
           XM87142::ramL(r,p+0x38u)==m.rootScript+16u &&
           XM87142::ramW(r,p+0x42u)==m.rootAnim &&
           XM87142::ramW(r,p+0x3Eu)>0u &&
           XM87142::ramW(r,p+0x3Eu)<0x1000u;
}
static inline bool beforeCpu(std::uint8_t* r,const std::uint8_t* rom,
                             unsigned romSize,unsigned frame,const char* driver,
                             bool version4,bool runahead){
    if(!version4 || runahead || !r || !rom || !driver ||
       std::strcmp(driver,"xmvsfu")!=0)return false;
    bool patched=false;
    for(unsigned side=0u;side<2u;++side){
        const unsigned p=0x4000u+side*0x400u;
        Tracker& t=state().side[side];
        const Move m=XM87142::ramW(r,p+0x60u)==0x2E06u?
            airMove():groundMove();
        if(t.active){
            if(XM87142::ramW(r,p+0x52u)!=0x001Eu ||
               XM87142::ramW(r,p+0x60u)!=t.action ||
               frame<t.entered || frame-t.entered>180u){
                t=Tracker{};
            }else if(m.action==0x2E02u && !t.deadlineNudged && enabled(m) &&
                     frame==t.entered+targetFrames &&
                     matches(r,p,m,false) &&
                     XM87142::ramL(r,p+0x34u)==m.finalScript){
                // Only the final original-ROM startup record. If native
                // frame-count drift adds one tick, shorten its *remaining*
                // animation countdown; never force a controller stage.
                const unsigned left=XM87142::ramW(r,p+0x3Eu);
                if(left>1u && left<0x100u){
                    putW(r,p+0x3Eu,1u);
                    t.deadlineNudged=true;
                    std::fprintf(stderr,
                        "XM87.371 AKUMA_FINAL_COUNTDOWN frame=%u side=%u "
                        "move=%s remaining=%u->1 target=%u\n",
                        frame,side+1u,m.label,left,targetFrames);
                }
            }
        }
        if(t.active || !enabled(m) || !matches(r,p,m,true))continue;
        const unsigned selector=m.action&0xFFu;
        const XM87142::Lookup root=XM87142::resolve(
            rom,romSize,0x1Eu,0x2Eu,selector);
        if(!root.valid || root.script!=m.rootScript ||
           root.animation!=m.rootAnim || m.chargeScript>romSize ||
           romSize-m.chargeScript<16u ||
           XM87142::romW(rom,romSize,m.chargeScript+6u)!=m.chargeAnim)
            continue;
        const unsigned duration=XM87142::romW(rom,romSize,m.chargeScript+2u);
        if(duration<1u || duration>32u || duration+m.extraHold>64u)continue;
        const unsigned appliedDuration=m.action==0x2E06u ?
            airChargeFrames():duration+m.extraHold;
        // Keep +60 action, +08 controller step, +A0 mode, +D6 native
        // move, +4E super meter/timer, and *all* projectile state intact.
        // Air: original charge pose, but close to its last frame, not
        // the first frame of a 52f hold. Ground is unchanged from 87.371.
        putL(r,p+0x34u,m.chargeScript);
        putL(r,p+0x38u,m.chargeScript+16u);
        putW(r,p+0x3Eu,appliedDuration);
        putW(r,p+0x42u,m.chargeAnim);
        t.active=true;t.deadlineNudged=false;
        t.entered=frame?frame-1u:frame;
        t.action=m.action;
        patched=true;
        std::fprintf(stderr,
            "XM87.372 AKUMA_SUPER_CHARGE frame=%u side=%u move=%s "
            "script=%06X->%06X original_duration=%u applied_duration=%u "
            "ryu_reference=%u\n",
            frame,side+1u,m.label,m.rootScript,m.chargeScript,
            duration,appliedDuration,targetFrames);
    }
    return patched;
}
static inline void afterCpu(const std::uint8_t* r,unsigned frame){
    if(!r)return;
    for(unsigned side=0u;side<2u;++side){
        Tracker& t=state().side[side];
        if(!t.active)continue;
        const unsigned p=0x4000u+side*0x400u;
        const unsigned action=XM87142::ramW(r,p+0x60u);
        const unsigned step=XM87142::ramW(r,p+0x08u);
        const bool isAir=t.action==0x2E06u;
        const unsigned attackAction=isAir?0x2E08u:0x2E02u;
        if(XM87142::ramW(r,p+0x52u)==0x001Eu &&
           action==attackAction && step>=4u){
            const unsigned elapsed=frame>=t.entered?frame-t.entered:0u;
            std::fprintf(stderr,
                "XM87.372 AKUMA_NATIVE_ATTACK_BEGIN frame=%u side=%u "
                "move=%s elapsed=%u ryu_reference=%u delta=%d\n",
                frame,side+1u,isAir?"AIR_2E06":"QCB_PPP_2E02",
                elapsed,targetFrames,int(elapsed)-int(targetFrames));
            t=Tracker{};
        }else if(XM87142::ramW(r,p+0x52u)!=0x001Eu ||
                 (action!=t.action && action!=attackAction) ||
                 frame<t.entered || frame-t.entered>180u){
            t=Tracker{};
        }
    }
}
} // namespace XM87370Akuma
#endif
