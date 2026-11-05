#ifndef XMVSF_STAGE87_179_DELAYED_TAG_EDGE_H
#define XMVSF_STAGE87_179_DELAYED_TAG_EDGE_H
// Stage 87.179: first deliver a fresh ROM-visible 2P edge, THEN arm
// controller 8 on the same host frame. CPS2's native 68000 input history
// receives the edge on the FOLLOWING frame alongside controller 8.
// Only delayed early-cancel Ryu(12)->Ken(14). Never force action 2E/32.
namespace XM87179Edge {
static inline bool eligible(unsigned phase,bool enabled,bool early,
  bool proof,unsigned point,unsigned reserve,unsigned action,unsigned mode,
  unsigned d6,unsigned controller256,unsigned controller259,
  unsigned nativeHeld,unsigned now,unsigned due){
    return enabled && early && !proof && phase==11u &&
      now==due && point==0x12u && reserve==0x14u &&
      action==0x2E08u && (mode==0x2A28u || mode==0x2A02u) &&
      d6==0x0Au && controller256==0u && controller259==0u &&
      (nativeHeld & 0x0070u)==0u;
}
static inline bool canRestoreStagedMode(unsigned phase,bool enabled,bool early,
  unsigned point,unsigned reserve,unsigned outgoing,unsigned incoming,
  unsigned action,unsigned expectedAction,unsigned mode,unsigned oldMode,
  unsigned d6,unsigned oldD6,unsigned controller256,unsigned controller259,
  unsigned old259){
    return enabled && early && phase==2u &&
       point==0x12u && reserve==0x14u &&
       outgoing==point && incoming==reserve &&
       action==0x2E08u && expectedAction==action &&
       mode==0x002Au &&
       (oldMode==0x2A28u || oldMode==0x2A02u) &&
       d6==0x0Au && oldD6==d6 &&
       controller256==0u && controller259==0x08u &&
       old259==0u;
}
static inline unsigned facingForward(unsigned facingByte){
    return (facingByte&1u)?1u:2u;
}
static inline unsigned nativePunchPair(){return 0x03u;}
} // namespace XM87179Edge
#endif
