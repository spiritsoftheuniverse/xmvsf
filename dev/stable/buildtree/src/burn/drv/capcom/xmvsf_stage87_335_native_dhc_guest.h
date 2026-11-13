#ifndef XMVSF_STAGE87_335_NATIVE_DHC_GUEST_H
#define XMVSF_STAGE87_335_NATIVE_DHC_GUEST_H
// XMVSF 87.335 — the FIRST DIRECT NATIVE 68000 incoming DHC path.
// Transcription of the stock-state portion of Morphus56k's
// xm3_hook_dhc_commit (copyright and credit: Daniel Plas Rivera).
// Runs from a host-owned, page-aligned, mapped M68K fetch page, NOT from
// a guessed "unused" space in the game's ROM. CpsCode remains unmodified.
// Stage 87.336 replaces the incorrect zero-action JSR $1038E with the
// ORIGINAL per-fighter D6 state dispatcher, ROM-verified before emission.
// The retained $009E90 call remains from the donor handoff. The
// original EURO $125D6 helper is NOT translated by a guessed offset;
// its absence makes this an experimental startup test, not final DHC.
// Normal tag-ins execute the original $011DBE unchanged.
#include <cstdint>
#include <cstring>
namespace XM87335Guest {
enum : unsigned {
    kGuestBase=0x500000u, kPageBytes=4096u,
    kP1Entry=kGuestBase, kP2Entry=kGuestBase+0x100u,
    kExpectedBytes=80u
};
// M68K big-endian instructions; register-preserving, A5 never assigned.
// MOVEM.L D0-D7/A0-A4,-(SP); save native caller state.
// Preserve native +A0 -> +A1 before setting +A0 Hyper mode and +D6 ID.
// Clear stale tag-in fields; set outgoing X and native ground; clear motion.
// Call original US action dispatcher $1038E and stock routine $9E90;
// restore exactly the saved registers then RTS to original fighter caller.
static inline const std::uint8_t* source(unsigned* length=nullptr) {
    static const std::uint8_t be[]={
        0x48,0xE7,0xFF,0xF8,
        0x1D,0x6E,0x00,0xA0,0x00,0xA1, // original A0->A1 previous mode
        0x1D,0x7C,0x00,0x2A,0x00,0xA0,
        0x1D,0x7C,0x00,0x00,0x00,0xD6, // D6 immediate at byte 19
        0x42,0x2E,0x00,0x4D,
        0x42,0x2E,0x02,0x59,
        0x3D,0x7C,0x00,0x00,0x00,0x0C, // outgoing X at 32,33
        0x3D,0x6E,0x02,0x54,0x00,0x10,
        0x42,0xAE,0x00,0x14,
        0x42,0xAE,0x00,0x18,
        0x42,0xAE,0x00,0x1C,
        0x42,0xAE,0x00,0x20,
        0x70,0x00,0x72,0x00,
        0x4E,0xB9,0x00,0x01,0x03,0x8E, // patched at 64..67: verified D6 dispatcher
        0x4E,0xB9,0x00,0x00,0x9E,0x90,
        0x4C,0xDF,0x1F,0xFF,
        0x4E,0x75
    };
    static_assert(sizeof(be)==kExpectedBytes,"DHC native guest ABI changed");
    if(length)*length=unsigned(sizeof(be));
    return be;
}
static inline bool emit(std::uint8_t* page,unsigned pageBytes,
                        unsigned offset,unsigned d6,unsigned x,
                        unsigned nativeDispatch) {
    unsigned length=0u;
    const std::uint8_t* src=source(&length);
    if(!page || pageBytes<kPageBytes || offset>pageBytes ||
       pageBytes-offset<length || (offset&1u) || d6==0u || d6>255u ||
       x>65535u || nativeDispatch==0u || nativeDispatch>0xFFFFFEu ||
       (nativeDispatch&1u))return false;
    std::uint8_t be[kExpectedBytes]={};
    std::memcpy(be,src,length);
    be[19]=std::uint8_t(d6);
    be[32]=std::uint8_t(x>>8u);
    be[33]=std::uint8_t(x);
    be[64]=std::uint8_t(nativeDispatch>>24u);
    be[65]=std::uint8_t(nativeDispatch>>16u);
    be[66]=std::uint8_t(nativeDispatch>>8u);
    be[67]=std::uint8_t(nativeDispatch);
    for(unsigned i=0;i<length;i+=2u) {
        page[offset+i]=be[i+1u];    // Sek/Musashi word-swapped fetch view
        page[offset+i+1u]=be[i];
    }
    return true;
}
static inline bool isImage(const std::uint8_t* page,unsigned pageBytes,
                           unsigned offset,unsigned d6,unsigned x,
                           unsigned nativeDispatch){
    std::uint8_t compare[kPageBytes]={};
    return emit(compare,sizeof(compare),offset,d6,x,nativeDispatch) &&
           page && pageBytes>=offset+kExpectedBytes &&
           std::memcmp(page+offset,compare+offset,kExpectedBytes)==0;
}
} // namespace XM87335Guest
#endif
