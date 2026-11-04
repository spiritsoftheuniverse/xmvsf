#ifndef XMVSF_STAGE87_47_MOVEMENT_CUES_H
#define XMVSF_STAGE87_47_MOVEMENT_CUES_H
// 87.47A READ ONLY: accepted ROM movement cues, not raw input guesses.
// CPS-2 68k CPU offsets: current action +A0, superjump mode +AA,
// world X +0C, world Y +10. CpsRamFF raw host bytes are word-swapped.
// ROM xvsu.03k/04k 961023: 01273E tests +AA; 012772 writes +A0=1C
// for superjump; 0137B2 dispatches +A0=3C airborne dash.
#include <cstdint>
#include <cstdio>
namespace XM8747 {
struct Fighter {
    unsigned x,y;
    unsigned char action,superMode;
};
static inline Fighter Read(const unsigned char* raw,unsigned base) {
    Fighter s={0,0,0,0};
    if(!raw)return s;
    // CPU word +A0/+AA high bytes are raw offsets +A1/+AB.
    s.action=raw[base+0xA1u];s.superMode=raw[base+0xABu];
    s.x=unsigned(raw[base+0x0Cu])|(unsigned(raw[base+0x0Du])<<8);
    s.y=unsigned(raw[base+0x10u])|(unsigned(raw[base+0x11u])<<8);
    return s;
}
enum Event { None=0, GroundDash=1, SuperJump=2, AirDash=3, SuperJumpLand=4, BackDashCandidate=5 };
struct Tracker {
    Fighter prev[2];
    bool ready[2];
    unsigned lastFrame;
    int lastPhase;
    Tracker():lastFrame(~0u),lastPhase(-1){ready[0]=ready[1]=false;}
};
static inline Event Identify(const Fighter& prev,const Fighter& now,bool valid) {
    if(!valid)return None;
    // A0=14 is recorded as ground dash; A0=16 is a ROM alternative,
    // but not demonstrated by the archived RAM capture.
    if(now.action==0x14 && prev.action!=0x14 && !now.superMode) return GroundDash;
    if(now.action==0x16 && prev.action!=0x16 && !now.superMode) return BackDashCandidate;
    // An airborne dash supersedes a superjump. Check BEFORE jump flag.
    if(now.action==0x3C && prev.action!=0x3C) return AirDash;
    // ROM explicitly sets +AA=1 for superjump, can progress to 2 in air.
    if(!prev.superMode && now.superMode &&
       (now.action==0x1A || now.action==0x1C))return SuperJump;
    if(prev.superMode && !now.superMode)return SuperJumpLand;
    return None;
}
static inline const char* Name(Event e) {
    switch(e){case GroundDash:return "GROUND_DASH";
      case SuperJump:return "SUPER_JUMP";
      case AirDash:return "AIR_DASH";
      case SuperJumpLand:return "LAND";
      case BackDashCandidate:return "BACK_DASH_CANDIDATE";
      default:return "NONE";}
}
static inline void Observe(const unsigned char* raw,unsigned frame,int phase,int firstDrawPass){
    static Tracker t;
    if(!raw || !firstDrawPass || t.lastFrame==frame)return;
    t.lastFrame=frame;
    if(phase!=8){t.ready[0]=t.ready[1]=false;t.lastPhase=phase;return;}
    for(unsigned p=0;p<2u;++p){
        const unsigned base=0x4000u+p*0x400u;
        // Only confirmed active fighter records. No RAM writes.
        if(!raw[base]){t.ready[p]=false;continue;}
        const Fighter cur=Read(raw,base);
        const Event e=Identify(t.prev[p],cur,t.ready[p]);
        if(e!=None){
            std::fprintf(stderr,
              "XM87.47 CUE frame=%u p=%u event=%s action=%02X super=%u x=%u y=%u previous=%02X/%u\n",
              frame,p+1,Name(e),cur.action,cur.superMode,cur.x,cur.y,
              unsigned(t.prev[p].action),unsigned(t.prev[p].superMode));
        }
        t.prev[p]=cur;t.ready[p]=true;
    }
    t.lastPhase=phase;
}
}
#endif
