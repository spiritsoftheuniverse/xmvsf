#ifndef XMVSF_STAGE87_177_DHC_3P_H
#define XMVSF_STAGE87_177_DHC_3P_H
// Stage 87.177: QCF+3P is a DHC request, not a dash and NOT the injected
// Ken input. The existing native Ken QCF+2P remains unchanged.
// Pure, ROM-free predicates. Never alter a normal-tag or non-DHC input.
namespace XM87177Dhc3P {
static inline bool knownSuperWindow(unsigned point, unsigned reserve,
                                    unsigned action, unsigned mode,
                                    bool candidate) {
    return candidate && point==0x12u && reserve==0x14u &&
           action==0x2E08u && (mode==0x2A28u || mode==0x2A02u);
}
static inline unsigned bitCount(unsigned bits){
    unsigned n=0u;
    for(;bits;bits>>=1u)n+=bits&1u;
    return n;
}
// Existing 2P/2K chords remain authorized. ONLY pure 3P gets the added
// DHC-only exception; 3K, mixed P+K, and incomplete chords remain excluded.
static inline bool matchesChords(unsigned rawButtons,bool kicks,
                                 bool allowTriplePunch) {
    if(rawButtons&~0x3Fu)return false;
    const unsigned raw=rawButtons;
    const unsigned group=kicks?(raw>>3u)&7u:raw&7u;
    if(raw!=(kicks?(group<<3u):group))return false;
    if(bitCount(group)==2u)return true;
    return !kicks && allowTriplePunch && raw==0x07u;
}
// The incoming partner may not dash simply because the outgoing DHC
// request uses 3P. Shield the current *physical port* before the native
// 68000 sees it, but ONLY when the ongoing native Ryu super permits DHC.
static inline bool shieldPhysical3P(unsigned rawButtons,bool dhcEnabled,
                                    bool earlyCancel,bool nativeWindow) {
    return dhcEnabled && earlyCancel && nativeWindow &&
           rawButtons==0x07u;
}
} // namespace XM87177Dhc3P
#endif
