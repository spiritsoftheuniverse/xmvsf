#ifndef XMVSF_STAGE87_233_AIRBORNE_ENTRY_H
#define XMVSF_STAGE87_233_AIRBORNE_ENTRY_H
// TEST 87.233: 68k-native offscreen entry begins BEFORE the ground-dash
// mode exists. This is a pure observer/policy, with no RAM writes and
// no Wolverine-specific guessed actor ID. Gameplay writes are made only
// by the existing 87.232 native-floor staging function after a decision.
#include <cstdint>

namespace XM87233AirReturn {
static inline bool groundDash(unsigned mode){
    return (mode&0xff00u)==0x1400u || (mode&0xff00u)==0x1600u;
}
static inline bool validX(std::uint32_t raw){
    const std::int64_t v=std::int64_t(std::int32_t(raw));
    return v>=-0x02000000LL && v<0x10000000LL;
}
static inline bool outsideView(std::uint32_t raw,std::uint32_t other){
    if(!validX(raw)||!validX(other))return false;
    const std::int64_t delta=std::int64_t(std::int32_t(raw))-
                              std::int64_t(std::int32_t(other));
    // Same 384+64 original-screen clearance as TEST 87.232.
    return delta<=-0x01c00000LL || delta>=0x01c00000LL;
}
struct Native {
    bool combat=false,active=false,opponentActive=false,allowed=true;
    unsigned fighter=~0u,control=0,action=0,mode=0;
    unsigned y=0,floor=0,air=0;
    std::uint32_t x=0,opponentX=0;
};
enum Decision {
    NONE=0, OUTSIDE_AIR_CANDIDATE, GROUND_BEFORE_NATIVE_DASH,
    REGROUND_BEFORE_NATIVE_DASH, WAIT_FOR_NATIVE_DASH,
    NATIVE_GROUND_DASH_READY, NATIVE_GROUND_DASH_BAD_X,
    RETURN_ABORTED, NATIVE_DASH_NOT_OBSERVED
};
struct Tracker {
    unsigned fighter=~0u,outsideFrames=0,pendingFrames=0,regrounds=0;
    bool pending=false,blockedUntilBackInView=false;
    std::uint32_t anchor=0;
    void reset(){*this=Tracker{};}
    static bool safe(const Native& n){
        // No native hitstun, tag-in, attack, grab or DHC-controlled move.
        // Original normal attacks start at 100x/120x/140x; aerial normals
        // at 180x/1Axx. Normal movement/jump/return remain below 1000.
        return n.combat && n.active && n.opponentActive && n.allowed &&
            n.fighter<=0x24u && (n.fighter&1u)==0u &&
            n.control==0u && n.action<0x1000u &&
            n.floor>0u && n.floor<2048u &&
            validX(n.opponentX);
    }
    Decision observe(const Native& n){
        if(fighter!=n.fighter){reset();fighter=n.fighter;}
        const bool base=safe(n);
        const bool separated=base && outsideView(n.x,n.opponentX);
        if(!separated && validX(n.x))blockedUntilBackInView=false;
        if(pending){
            // A different move/reaction or a return to the camera cancels
            // rather than forcing an airborne attack onto the floor.
            if(!base || (validX(n.x) && !separated)){
                pending=false;outsideFrames=0u;
                return RETURN_ABORTED;
            }
            if(groundDash(n.mode)){
                pending=false;outsideFrames=0u;
                blockedUntilBackInView=true;
                // The native 68000 has accepted the original 1400/1600
                // ground-dash dispatch. No fabricated script or velocity.
                return validX(n.x)?NATIVE_GROUND_DASH_READY:
                                  NATIVE_GROUND_DASH_BAD_X;
            }
            if(!validX(n.x)){
                pending=false;outsideFrames=0u;
                blockedUntilBackInView=true;
                return RETURN_ABORTED;
            }
            if(++pendingFrames>24u){
                pending=false;outsideFrames=0u;
                blockedUntilBackInView=true;
                return NATIVE_DASH_NOT_OBSERVED;
            }
            if((n.air&0xff00u)!=0u || n.y!=n.floor){
                if(regrounds++<4u)return REGROUND_BEFORE_NATIVE_DASH;
                pending=false;outsideFrames=0u;
                blockedUntilBackInView=true;
                return NATIVE_DASH_NOT_OBSERVED;
            }
            return WAIT_FOR_NATIVE_DASH;
        }
        if(!base || !validX(n.x) || groundDash(n.mode) ||
           blockedUntilBackInView || !separated ||
           (((n.air&0xff00u)==0u) && n.y==n.floor)){
            outsideFrames=0u;
            return NONE;
        }
        // Two distinct committed native frames, not one transient image.
        if(++outsideFrames<2u)return OUTSIDE_AIR_CANDIDATE;
        pending=true;outsideFrames=0u;pendingFrames=0u;regrounds=0u;
        anchor=n.x;
        return GROUND_BEFORE_NATIVE_DASH;
    }
};
} // namespace XM87233AirReturn
#endif
