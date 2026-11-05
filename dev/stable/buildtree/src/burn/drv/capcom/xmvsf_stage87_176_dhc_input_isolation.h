#ifndef XMVSF_STAGE87_176_DHC_INPUT_ISOLATION_H
#define XMVSF_STAGE87_176_DHC_INPUT_ISOLATION_H
// Stage 87.176: one-frame-late physical PP edge must not re-enter outgoing
// Ryu's 2E08 after the DHC command was queued by the host input matcher.
// This is a pure selector. The caller still uses CpsRwGetInp (no direct
// CPS2 RAM modification); isolated input is a neutral P1 port for that frame.
// Keep short native Ken QCF+PP/KK input and charge-free arrival untouched.
namespace XM87176Input {
// XM87144::Phase numeric values at 87.175: 2=TAG_CONTROLLER, 3=SWAP,
// 4=LANDING, 5=KEN_EXIT, 8=DELIVER_NATIVE_INPUT.
static inline bool neutralize(unsigned phase, unsigned point, unsigned reserve,
                              bool enabled, bool earlyCancel,
                              unsigned now, unsigned inputBegin) {
    if(!enabled || !earlyCancel)return false; // no ordinary / proof-mode tags
    const bool ryuPoint=point==0x12u && reserve==0x14u;
    const bool kenPoint=point==0x14u && reserve==0x12u;
    if(phase==2u || phase==3u)return ryuPoint || kenPoint;
    if(phase==4u || phase==5u)return kenPoint;
    if(phase==8u && kenPoint && inputBegin && now<inputBegin)return true;
    return false;
}
} // XM87176Input
#endif
