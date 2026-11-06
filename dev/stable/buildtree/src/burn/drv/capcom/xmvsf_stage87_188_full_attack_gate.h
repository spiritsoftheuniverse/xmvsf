#ifndef XMVSF_STAGE87_188_FULL_ATTACK_GATE_H
#define XMVSF_STAGE87_188_FULL_ATTACK_GATE_H
// Original 87.187 capture proves 2A02 is a valid Ryu Shinkuu ATTACK
// mode across the whole action. The old >0x1C time cutoff blocked
// later attack frames. Permit fresh, unconsumed native ATTACK input
// from +4E=3C through +4E=01; 40..3D remain separately opt-in.
// This predicate does not synthesize inputs or guarantee ROM acceptance.
#include "xmvsf_stage87_184_super_action_lifetime.h"
namespace XM87188Attack {
static inline bool eligible(const XM87184Life::Sample& s,
       bool enabled,unsigned remaining,unsigned c0,unsigned ca){
    return enabled && XM87184Life::identify(s)==XM87184Life::ATTACK &&
           remaining>0u && remaining<=0x3Cu &&
           ((c0|ca)&0x0070u)==0u;
}
}
#endif
