#ifndef XMVSF_STAGE87_333_MORPHUS_NATIVE_DHC_ABI_H
#define XMVSF_STAGE87_333_MORPHUS_NATIVE_DHC_ABI_H
// 87.333 — VERIFIED *US* Motorola 68000 hook ABI, from uploaded 87.332
// decrypted CpsCode evidence and Morphus56k native-hooks-2p.json + dhc.s.
//
// Data in CpsCode is WORD-SWAPPED relative to canonical M68K big-endian.
// The checked bytes are original US code, not donor-injected instructions.
// These guards are necessary, but NOT sufficient, for a future live patch:
// register/stack, A5, native actor identity and meter must also be verified.
// Do not use a global +14 offset for other M68K subroutine calls.
// This header makes no writes or changes to game execution.
#include <cstdint>
#include <cstring>

namespace XM87333NativeDHC {
enum : unsigned {
    kUSIncoming = 0x011DBEu,
    kUSIncomingResume = 0x011DC4u,
    kUSVoiceSkipJump = 0x011DCAu,
    kUSOutgoing = 0x013AB6u,
    kUSOutgoingNext = 0x013ABCu,
    kRequiredChecks = 0x1Fu
};
struct Check {
    unsigned address;
    unsigned size;
    const std::uint8_t* bytesBE;
    const char* purpose;
};
static inline const Check* sites() {
    static const std::uint8_t incoming[] = {
       0x30,0x2E,0x00,0x52,0xD0,0x40
    }; // move.w $52(a6),d0 / add.w d0,d0
    static const std::uint8_t resume[] = {
       0x20,0x7B,0x00,0x0A,0x4E,0x90
    }; // original table lookup / jsr (a0)
    static const std::uint8_t voice[] = {
       0x4E,0xF9,0x00,0x07,0x8B,0xB6
    }; // US JMP voice-tail; NOT EURO target $78BA4
    static const std::uint8_t outgoing[] = {
       0x0C,0x2E,0x00,0x04,0x00,0xA4
    }; // cmpi.b #4,$a4(a6)
    static const std::uint8_t outgoingNext[] = {
       0x67,0x4A,0x0C,0x2E,0x00,0x06,0x00,0xA4,
       0x67,0x42,0x20,0x6E,0x02,0x24
    }; // branch / compare / fetch partner pointer
    static const Check a[] = {
       {kUSIncoming,         unsigned(sizeof(incoming)),     incoming,     "INCOMING_HYPER"},
       {kUSIncomingResume,   unsigned(sizeof(resume)),       resume,       "INCOMING_RESUME"},
       {kUSVoiceSkipJump,    unsigned(sizeof(voice)),        voice,        "US_VOICE_JUMP"},
       {kUSOutgoing,         unsigned(sizeof(outgoing)),     outgoing,     "OUTGOING_CHAIN"},
       {kUSOutgoingNext,     unsigned(sizeof(outgoingNext)), outgoingNext, "OUTGOING_BRANCH"}
    };
    return a;
}
static inline bool matches(const std::uint8_t* cpsCode,
                           unsigned len,const Check& s) {
    if(!cpsCode || (s.address & 1u) || (s.size & 1u) ||
       !s.size || s.address>len || len-s.address<s.size)return false;
    for(unsigned i=0;i<s.size;++i)
        if(cpsCode[s.address+(i^1u)]!=s.bytesBE[i])return false;
    return true;
}
struct Proof {
    unsigned passed=0u;
    bool all()const{return passed==kRequiredChecks;}
    bool outgoing()const{return (passed & (1u<<3))!=0u &&
                                  (passed & (1u<<4))!=0u;}
    bool incoming()const{return (passed & (1u<<0))!=0u &&
                                  (passed & (1u<<1))!=0u &&
                                  (passed & (1u<<2))!=0u;}
};
static inline Proof verify(const std::uint8_t* cpsCode,unsigned len){
    Proof result;
    const Check* s=sites();
    for(unsigned i=0u;i<5u;++i)
        if(matches(cpsCode,len,s[i]))result.passed|=(1u<<i);
    return result;
}
} // namespace XM87333NativeDHC
#endif
