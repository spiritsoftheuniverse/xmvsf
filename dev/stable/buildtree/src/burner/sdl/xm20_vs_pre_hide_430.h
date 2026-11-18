#ifndef XMVSF_STAGE20_VS_PRE_HIDE_430_H
#define XMVSF_STAGE20_VS_PRE_HIDE_430_H
// TEST 87.430: Conceal the existing source-qualified native HUD from
// the FIRST observed prematch scene instead of relying on a chooser
// handoff that may be skipped while the stage is loading.
// This gate NEVER starts audio or the shock reveal and never changes
// what counts as HUD (87.417 forest/intro-wall exclusions remain frozen).
namespace XM87430PreHide {
inline bool shouldPrime(unsigned status,unsigned matchStep,bool enabled,
                        bool alreadyPrimed,bool combatNotified){
    return enabled && !alreadyPrimed && !combatNotified &&
           matchStep==0x0008u &&
           (status==0x02u || status==0x04u || status==0x06u);
}
}
#endif
