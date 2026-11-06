#ifndef XMVSF_STAGE87_186_FIRST_ATTACK_GATE_H
#define XMVSF_STAGE87_186_FIRST_ATTACK_GATE_H
// 87.186 TEST-ONLY: the 87.185 input gate's timer ceiling 0x3C
// omitted exactly the first four genuine original Ryu ATTACK frames
// (+4E=40/3F/3E/3D). Zero user DHC attempts at these offsets exist in
// 87.185 data; this is an experiment, not a confirmed cancellation.
// Never used for startup (2E08/step02), recovery, or a consumed punch.
// The original host DHC logic still requires verified Ken ROM script,
// Ryu-Ken ownership, chain eligibility, native super and clean C0/CA.
#include "xmvsf_stage87_184_super_action_lifetime.h"
namespace XM87186FirstAttack {
static inline bool eligible(const XM87184Life::Sample&s,
                            unsigned remaining,unsigned c0,unsigned ca) {
    return XM87184Life::identify(s)==XM87184Life::ATTACK &&
           remaining>=0x3Du && remaining<=0x40u &&
           ((c0|ca)&0x0070u)==0u;
}
} // namespace XM87186FirstAttack
#endif
