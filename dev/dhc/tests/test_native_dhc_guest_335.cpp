// Stage 87.335 independent native 68000 guest emitter test.
// Run explicitly if desired, NEVER added as a BUILD_TEST gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_335_native_dhc_guest.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_333_morphus_native_dhc_abi.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace XM87335Guest;
static unsigned word(const std::uint8_t* p,unsigned addr){
    return (unsigned(p[addr+1u])<<8u)|unsigned(p[addr]);
}
int main() {
    std::uint8_t mem[kPageBytes]={};
    assert(kExpectedBytes==74u);
    assert(!emit(nullptr,kPageBytes,0u,0x0Cu,0x1234u));
    assert(!emit(mem,sizeof(mem),1u,0x0Cu,0x1234u));
    assert(!emit(mem,sizeof(mem),0u,0u,0x1234u));
    assert(emit(mem,sizeof(mem),0u,0x0Cu,0x1234u));
    assert(emit(mem,sizeof(mem),0x100u,0x0Eu,0x5678u));
    assert(isImage(mem,sizeof(mem),0u,0x0Cu,0x1234u));
    assert(isImage(mem,sizeof(mem),0x100u,0x0Eu,0x5678u));
    assert(!isImage(mem,sizeof(mem),0u,0x0Eu,0x1234u));
    assert(word(mem,0u)==0x48E7u);       // native register save
    assert(word(mem,2u)==0xFFF8u);       // D0-D7/A0-A4 predecrement mask
    assert(word(mem,12u)==0x000Cu);      // P1 Ken D6 Hyper selector
    assert(word(mem,26u)==0x1234u);      // outgoing native X
    assert(word(mem,0x100u+12u)==0x000Eu); // independent P2 D6
    assert(word(mem,0x100u+26u)==0x5678u);
    assert(word(mem,68u)==0x4CDFu);      // full register restoration
    assert(word(mem,70u)==0x1FFFu);
    assert(word(mem,72u)==0x4E75u);      // return to genuine tag caller
    assert(word(mem,56u)==0x4EB9u);      // US JSR $1038E
    assert(word(mem,62u)==0x4EB9u);      // US JSR $9E90
    assert(XM87333NativeDHC::kUSIncoming==0x011DBEu);
    assert(XM87333NativeDHC::kUSIncomingResume==0x011DC4u);
    std::puts("XMVSF 87.335 Motorola guest code emission PASS");
}
