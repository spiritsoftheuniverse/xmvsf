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
    // A DHC selected earlier than the safe tag timer spends frames in
    // WAIT_RYU_SUPER_END (9). Keep a held 3P from leaking to the 68000
    // while the outgoing super waits for its native tag window.
    if(phase==9u)return ryuPoint;
    // 87.179 priming phase: the generated one-shot LP+MP takes precedence
    // at the inputForFrame call site; if eligibility is lost, default to
    // neutral P1 rather than leaking an unrelated physical 3P/dash.
    if(phase==11u)return ryuPoint;
    // 87.194: preserve the incoming fighter's original native jump
    // without leaking held reserve-command buttons into its parser
    // BEFORE the actual ground landing. Release the command only when
    // its original ROM-owned native descent finishes.
    if(phase==4u || phase==5u || phase==12u)return kenPoint;
    // 87.177: keep the P1 port neutral if a held physical 3P survives
    // beyond Ken's 9-frame native QCF+2P sequence. Otherwise the native
    // dash chord could leak back in while waiting for 2E ROM acceptance.
    // Keep frames [inputBegin,inputBegin+9) untouched: the real input
    // bridge must generate exactly its already verified short QCF chord.
    if(phase==8u && kenPoint && inputBegin)
        return now<inputBegin || now-inputBegin>=9u;
    return false;
}
} // XM87176Input
#endif
