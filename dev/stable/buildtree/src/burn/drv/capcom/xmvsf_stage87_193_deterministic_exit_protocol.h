#ifndef XMVSF_STAGE87_193_DETERMINISTIC_EXIT_PROTOCOL_H
#define XMVSF_STAGE87_193_DETERMINISTIC_EXIT_PROTOCOL_H
// 87.193: one outgoing DHC protocol for a native super and a reserve-owned
// command. This code has no Ryu/Ken, named-super, move-selector, timer or
// 2A-mode special cases. The executable transaction still requires the
// original separately verified fighter pair and 68000 tag dispatch.
//
// Physical chord ownership:
//   - outgoing actor: original game sees the native PUNCH tag edge;
//   - incoming actor: preserves the player's ORIGINAL super command.
// The outgoing port is normalized BEFORE CpsRwGetInp/68000 execution,
// not one frame afterward when the host queue happens to select DHC.
// The native tag controller is prepared by ONE deterministic function.
// Same layout applies to P1/P2 and to point/reserve (future mirrored
// dispatch), without pretending unverified swap routines are live.
#include "xmvsf_stage87_142_rom_dispatch.h"
#include <cstdint>
namespace XM87193Exit {
struct Chord {
    unsigned physical=0u,nativeOutgoing=0u,reserveOriginal=0u;
    bool valid=false,changed=false;
};
static inline unsigned bits3(unsigned x){
    x&=7u;return (x&1u)+((x>>1u)&1u)+((x>>2u)&1u);
}
static inline Chord route(unsigned raw){
    Chord c;c.physical=raw&0x3Fu;c.reserveOriginal=c.physical;
    if(raw&~0x3Fu)return c;
    const unsigned p=raw&7u,k=(raw>>3u)&7u;
    if((p&&k)||(!p&&!k))return c;
    if((p && bits3(p)<2u) || (k && bits3(k)<2u))return c;
    c.valid=true;
    // The original Ryu native outgoing controller-8 path needs the
    // original punch edge. Preserve working EXACT punch pairs; only
    // 3P dash and 2K/3K stray action are translated to canonical LP+MP.
    c.nativeOutgoing=p && bits3(p)==2u?c.physical:0x03u;
    c.changed=c.nativeOutgoing!=c.physical;
    return c;
}
struct PreCpu {
    bool active=false;
    unsigned nativeButtons=0u,originalButtons=0u;
};
static inline PreCpu beforeCpu(bool enabled,bool hostIdle,
        bool nativeSuperOwned,unsigned physicalButtons){
    PreCpu p;
    if(!enabled||!hostIdle||!nativeSuperOwned)return p;
    const Chord c=route(physicalButtons);
    if(!c.valid||!c.changed)return p;
    p.active=true;p.nativeButtons=c.nativeOutgoing;
    p.originalButtons=c.reserveOriginal;
    return p;
}
static inline unsigned actorBase(unsigned side){
    return side<2u?0x4000u+side*0x400u:0u;
}
static inline unsigned reserveBase(unsigned side){
    return side<2u?0x4800u+side*0x400u:0u;
}
static inline void bput(std::uint8_t*r,unsigned a,unsigned v){
    r[(a&0xFFFFu)^1u]=std::uint8_t(v);
}
static inline void wput(std::uint8_t*r,unsigned a,unsigned v){
    bput(r,a,v>>8u);bput(r,a+1u,v);
}
struct Snapshot {
    unsigned action=0u,mode=0u,script=0u,d6=0u;
    unsigned controller256=0u,controller259=0u;
    unsigned step06=0u,step08=0u,step0A=0u;
};
static inline Snapshot capture(const std::uint8_t*ram,unsigned actor){
    Snapshot s;
    if(!ram)return s;
    s.action=XM87142::ramW(ram,actor+0x60u);
    s.mode=XM87142::ramW(ram,actor+0xA0u);
    s.script=XM87142::ramL(ram,actor+0x34u);
    s.d6=XM87142::ramB(ram,actor+0xD6u);
    s.controller256=XM87142::ramB(ram,actor+0x256u);
    s.controller259=XM87142::ramB(ram,actor+0x259u);
    s.step06=XM87142::ramW(ram,actor+0x06u);
    s.step08=XM87142::ramW(ram,actor+0x08u);
    s.step0A=XM87142::ramW(ram,actor+0x0Au);
    return s;
}
// Exactly the existing proven 87.192 native-controller entry assignment,
// now one shared actor-addressed line. The original 68000 chooses 3200
// on its next execution; no invented tag sprite/script or native super.
// The caller logs/observes the real ROM result and aborts wrong actions.
static inline void armNativeController8(std::uint8_t*ram,unsigned actor){
    if(!ram)return;
    bput(ram,actor+0x256u,0x08u);
    bput(ram,actor+0x259u,0x08u);
    wput(ram,actor+0x06u,0u);
    wput(ram,actor+0x08u,0u);
    wput(ram,actor+0x0Au,0u);
    wput(ram,actor+0xA0u,0x002Au);
}
} // namespace XM87193Exit
#endif
