#ifndef XMVSF_STAGE87_185_ACTION_GATE_H
#define XMVSF_STAGE87_185_ACTION_GATE_H
// 87.185: separate intent recognition (whole native Shinkuu lifetime)
// from safe controller-8 execution. Never use 200f candidate timeout,
// a guessed "super over" tick or late physical 3P dash normalization.
// The submitted 87.184 PC trace confirms all five 2E0A recovery tags
// cause original 68000 PC 010392 to dispatch 2E08/0D3070 instead of
// normal tag. This helper may recognize that *exact* rebound for a
// separate experimental host tag bridge, but never treats it as ROM-
// selected tag 3200 without the existing verified script handoff.
#include "xmvsf_stage87_184_super_action_lifetime.h"
namespace XM87185Gate {
static inline bool canRecognize(const XM87184Life::Sample& s) {
    const XM87184Life::Phase p=XM87184Life::identify(s);
    return p==XM87184Life::STARTUP ||
           p==XM87184Life::ATTACK ||
           (p==XM87184Life::RECOVERY && s.script<0x0D3194u);
}
static inline bool directActiveAttack(const XM87184Life::Sample& s,
      bool enabled,unsigned remaining,unsigned c0,unsigned ca) {
    return enabled && XM87184Life::identify(s)==XM87184Life::ATTACK &&
           remaining>0x1Cu && remaining<=0x3Cu &&
           ((c0|ca)&0x0070u)==0u;
}
static inline bool exactRecoveryRebound(bool enabled,
      unsigned phase,unsigned point,unsigned reserve,
      unsigned originalAction,unsigned originalStep,unsigned originalScript,
      unsigned presentAction,unsigned presentMode,unsigned presentStep,
      unsigned presentScript,unsigned d6,unsigned c256,unsigned c259,
      unsigned armedAt,unsigned now) {
    return enabled && phase==2u &&
           point==0x12u && reserve==0x14u &&
           originalAction==0x2E0Au && originalStep==6u &&
           originalScript>=0x0D30F4u && originalScript<0x0D3194u &&
           presentAction==0x2E08u && presentMode==0x2A00u &&
           presentStep==2u && presentScript==0x0D3070u &&
           d6==0x0Au && c256==0u && c259==8u &&
           now==armedAt+1u;
}
} // namespace XM87185Gate
#endif
