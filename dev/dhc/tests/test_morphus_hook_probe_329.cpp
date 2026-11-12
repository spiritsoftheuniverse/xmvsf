// Standalone C++11 test, not part of the normal incremental build.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_329_morphus_hook_probe.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace XM87329HookProbe;
int main(){
    std::vector<std::uint8_t> bytes(0x13B00u,0u);
    const Site* s=sites();
    assert(!matches(bytes.data(),unsigned(bytes.size()),s[0]));
    for(unsigned i=0;i<2u;i++){
        for(unsigned k=0;k<6u;k++)bytes[s[i].address+k]=s[i].expected[k];
        assert(matches(bytes.data(),unsigned(bytes.size()),s[i]));
    }
    char hex[32]={}; hex6(bytes.data(),unsigned(bytes.size()),s[0].address,hex);
    assert(std::strcmp(hex,"0C2E000400A4")==0);
    bytes[s[1].address]^=1u;
    assert(!matches(bytes.data(),unsigned(bytes.size()),s[1]));
    std::puts("XMVSF 87.329 Morphus read-only opcode probe: PASS");
}
