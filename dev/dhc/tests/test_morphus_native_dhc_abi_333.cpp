// 87.333 independent C++11 native-hook ABI guard unit test.
// No new steps added to the existing warm BUILD_TEST pipeline.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_333_morphus_native_dhc_abi.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace XM87333NativeDHC;
int main() {
    std::vector<std::uint8_t> code(0x20000u,0u);
    const Check* s=sites();
    for(unsigned site=0;site<5u;++site)
        for(unsigned i=0;i<s[site].size;++i)
            code[s[site].address+(i^1u)]=s[site].bytesBE[i];
    const Proof full=verify(code.data(),unsigned(code.size()));
    assert(full.all() && full.outgoing() && full.incoming());
    assert(kUSIncomingResume==kUSIncoming+6u);
    assert(kUSOutgoingNext==kUSOutgoing+6u);
    assert(!verify(code.data(),kUSIncoming).incoming());
    code[kUSVoiceSkipJump]^=0x80u;
    const Proof damaged=verify(code.data(),unsigned(code.size()));
    assert(!damaged.all() && damaged.outgoing() && !damaged.incoming());
    code[kUSOutgoing+(1u^1u)]^=0x01u;
    assert(!verify(code.data(),unsigned(code.size())).outgoing());
    std::puts("XMVSF 87.333 US native DHC original-opcode ABI guards PASS");
}
