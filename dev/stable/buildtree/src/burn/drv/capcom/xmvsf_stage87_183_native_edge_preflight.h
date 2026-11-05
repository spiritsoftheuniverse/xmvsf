#ifndef XMVSF_STAGE87_183_NATIVE_EDGE_PREFLIGHT_H
#define XMVSF_STAGE87_183_NATIVE_EDGE_PREFLIGHT_H
// 87.183: DHC safety preflight using ROM-owned native input history.
// The real 87.182 F9/observer recording shows 20/20 successful native
// controller-8 handoffs started with C0=0001 and CA=0000. All three
// armed failures had C0 punch bits (0011/0031) and CA punch bits
// (0010/0030) already accepted by the original CPU.
// This checks native input state BEFORE host writes +256/+259/+A0;
// it never edits native input words or generates replacement punches.
namespace XM87183Preflight {
static inline unsigned heldPunches(unsigned c0) {return c0 & 0x0070u;}
static inline unsigned pressedPunches(unsigned ca) {return ca & 0x0070u;}
static inline bool freshNativeTagEdge(unsigned c0,unsigned ca) {
    return heldPunches(c0)==0u && pressedPunches(ca)==0u;
}
static inline bool shouldDecline(bool experimentalEarlyCancel,
        bool unsafeDiagnosticReplay,unsigned nativeC0,unsigned nativeCA) {
    return experimentalEarlyCancel && !unsafeDiagnosticReplay &&
           !freshNativeTagEdge(nativeC0,nativeCA);
}
} // namespace XM87183Preflight
#endif
