#ifndef XMVSF_STAGE87_184_SUPER_ACTION_LIFETIME_H
#define XMVSF_STAGE87_184_SUPER_ACTION_LIFETIME_H
// 87.184 original-ROM Shinkuu Hadoken action lifetime (Ryu fighter 12).
// From six uninterrupted 87.183 F9 native captures:
// 2E08 phase 02 starts at 0D3070; 2E08 phase 04 uses 0D30B0..0D30E0;
// 2E0A phase 06 uses 0D30F4..0D3194; end = first action outside both.
// Natural durations 163..165 frames, but NEVER gate using guessed frames.
// Recovery DHC is experimental; terminal 0D3194 is not taggable.
// No original RAM, input history, script, ROM, or movie writes.
#include <cstdint>
#include <cstdlib>
#include <cstring>
namespace XM87184Life {
enum Phase { OUTSIDE=0, STARTUP=1, ATTACK=2, RECOVERY=3 };
enum Event { NOTHING=0, ENTER=1, ATTACK_BEGIN=2, RECOVERY_BEGIN=3,
             ACTION_EXIT=4 };
struct Sample {
    unsigned fighter,action,mode,step,script,d6;
    // C++11-safe constructor; do not require C++14 aggregate rules.
    Sample(unsigned f=0,unsigned a=0,unsigned m=0,unsigned st=0,
           unsigned sc=0,unsigned d=0)
      : fighter(f),action(a),mode(m),step(st),script(sc),d6(d) {}
};
static inline Phase identify(const Sample& s) {
    if(s.fighter!=0x12u || s.d6!=0x0Au ||
       (s.mode!=0x2A28u && s.mode!=0x2A02u))return OUTSIDE;
    if(s.action==0x2E08u && s.step==0x02u &&
       s.script>=0x0D3070u && s.script<=0x0D30A0u)return STARTUP;
    if(s.action==0x2E08u && s.step==0x04u &&
       s.script>=0x0D30B0u && s.script<=0x0D30E0u)return ATTACK;
    if(s.action==0x2E0Au && s.step==0x06u &&
       s.script>=0x0D30F4u && s.script<=0x0D3194u)return RECOVERY;
    return OUTSIDE;
}
static inline bool root(const Sample&s){
    return identify(s)==STARTUP && s.script==0x0D3070u;
}
static inline bool recoveryOptIn(){
    const char* p=std::getenv("XM87184_EXPERIMENTAL_RECOVERY_TAG");
    return p && std::strcmp(p,"1")==0;
}
// The terminal 0D3194 recovery animation can enter ordinary neutral
// during the next CPU frame; never start controller 8 that late.
static inline bool canTag(const Sample&s,bool allowRecovery){
    const Phase p=identify(s);
    return p==ATTACK || (allowRecovery && p==RECOVERY &&
                        s.script<0x0D3194u);
}
static inline bool isRecoveryTag(const Sample&s,bool allowRecovery){
    return allowRecovery && identify(s)==RECOVERY &&
           s.script<0x0D3194u;
}
struct Tracker {
    bool live=false;
    Phase phase=OUTSIDE;
    unsigned startAt=0,attackAt=0,recoveryAt=0,endAt=0;
    unsigned generation=0;
    Event update(unsigned frame,const Sample&s){
        const Phase current=identify(s);
        if(!live){
            if(!root(s))return NOTHING;
            live=true;phase=STARTUP;startAt=frame;
            attackAt=recoveryAt=endAt=0;
            ++generation;
            return ENTER;
        }
        if(current==OUTSIDE){
            live=false;phase=OUTSIDE;endAt=frame;
            return ACTION_EXIT;
        }
        if(current==ATTACK && phase!=ATTACK){
            phase=ATTACK;attackAt=frame;
            return ATTACK_BEGIN;
        }
        if(current==RECOVERY && phase!=RECOVERY){
            phase=RECOVERY;recoveryAt=frame;
            return RECOVERY_BEGIN;
        }
        // A new native root while another lifecycle remained live is
        // a distinct action, never carry a stale timer or DHC candidate.
        if(root(s) && phase!=STARTUP){
            live=true;phase=STARTUP;startAt=frame;
            attackAt=recoveryAt=endAt=0;++generation;
            return ENTER;
        }
        phase=current;
        return NOTHING;
    }
    void reset(){*this=Tracker();}
};
} // namespace XM87184Life
#endif
