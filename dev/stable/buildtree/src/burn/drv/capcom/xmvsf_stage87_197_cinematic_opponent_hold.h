#ifndef XMVSF_STAGE87_197_CINEMATIC_OPPONENT_HOLD_H
#define XMVSF_STAGE87_197_CINEMATIC_OPPONENT_HOLD_H
// 87.197 opt-in DHC freeze bridge:
// A global emulator pause would also stop Ken's authentic 0212 fall,
// and blindly altering a CPS2-wide freeze timer is unverified. Instead,
// hold the OPPOSING native point fighter's complete 0x400-byte original
// RAM actor record across the outgoing 3200 -> incoming ROM 2E boundary.
// No ROM, timer, player-1 Ken, projectiles or other P2 slot is patched.
// Freeze is strictly transaction- and actor-ID scoped and fail-open on
// ownership or scene change. The actor is restored on any DHC abort.
// This is deliberately not a claim to halt every background/projectile.
//
// While held, clear the saved opponent's OFFENSIVE descriptors to avoid
// repeatedly reapplying an active hitbox. The original snapshot is
// restored exactly on release; only DHC time ever owns the mutation.
#include <cstdint>
#include <cstring>
namespace XM87197Hold {
static inline unsigned rb(const std::uint8_t* r,unsigned a){
    return r[(a&65535u)^1u];
}
static inline unsigned rw(const std::uint8_t* r,unsigned a){
    return (rb(r,a)<<8u)|rb(r,a+1u);
}
static inline void wb(std::uint8_t* r,unsigned a,unsigned v){
    r[(a&65535u)^1u]=std::uint8_t(v);
}
struct Freeze {
    bool active=false;
    unsigned side=0u,base=0u,fighter=0u,start=0u,last=~0u;
    unsigned held=0u;
    std::uint8_t snapshot[0x400u]={};
    static bool phaseHolds(unsigned phase){
        // Native exit and real jump/input only. Once ROM accepts
        // reserve 2E or host aborts, the opponent must run again.
        return phase==3u || phase==4u || phase==5u ||
               phase==8u || phase==12u;
    }
    static unsigned opponentBase(unsigned side){
        return 0x4000u+((side^1u)&1u)*0x400u;
    }
    bool validActor(const std::uint8_t* ram)const{
        return active && ram && rb(ram,0x5000u)==8u &&
               rw(ram,base+0x52u)==fighter;
    }
    bool capture(std::uint8_t* ram,unsigned actorSide,unsigned now,
                 unsigned phase,unsigned pointId,unsigned reserveId){
        if(active || !ram || actorSide>=2u || phase!=3u ||
           pointId!=0x12u || reserveId!=0x14u ||
           rb(ram,0x5000u)!=8u)return false;
        const unsigned actor=opponentBase(actorSide);
        const unsigned id=rw(ram,actor+0x52u);
        if(!id || id>0x40u || (id&1u))return false;
        side=actorSide;base=actor;fighter=id;
        start=now;last=~0u;held=0u;
        std::memcpy(snapshot,ram+base,0x400u);
        active=true;
        // The first displayed tag frame is frozen too.
        suppress(ram);
        return true;
    }
    void suppress(std::uint8_t* ram)const{
        if(!validActor(ram))return;
        wb(ram,base+0x70u,0u);wb(ram,base+0x71u,0u);
        wb(ram,base+0x72u,0u);wb(ram,base+0x73u,0u);
        wb(ram,base+0x82u,0u);
    }
    // Called ahead of native CPU dispatch to suppress any carried
    // opposing attack box, without pausing the CPU/renderer/host input.
    void beforeCPU(std::uint8_t* ram,unsigned phase){
        if(active && phaseHolds(phase))suppress(ram);
    }
    // Post-CPU frame: the original 68000 has already run Ken's entire
    // gravity and script update; rewind only P2's native point object.
    bool hold(std::uint8_t* ram,unsigned now,unsigned phase){
        if(!active || !validActor(ram) || !phaseHolds(phase) ||
           now<start || now-start>180u)return false;
        if(now==last)return true;
        std::memcpy(ram+base,snapshot,0x400u);
        suppress(ram);
        ++held;last=now;
        return true;
    }
    // Finish when native incoming 2E is verified; copy back the
    // exact captured opponent object, including original hitbox bytes,
    // only if the original fighter still owns the same slot.
    bool release(std::uint8_t* ram){
        if(!active)return false;
        const bool same=validActor(ram);
        if(same)std::memcpy(ram+base,snapshot,0x400u);
        reset();
        return same;
    }
    void reset(){
        active=false;side=0u;base=0u;fighter=0u;
        start=0u;last=~0u;held=0u;
    }
};
} // XM87197Hold
#endif
