// 87.178 ROM-free C++ regression: 3P->2P prequeue, no dash, stale latch.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_177_dhc_punch_chords.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_178_tag_latch_recovery.h"
using namespace XM87177Chords;
int main(){
    // Native Ryu+Ken tag controller requires an actual press edge.
    // Captured 87.177 first 4 good DHCs carried a 2P physical edge;
    // the first 3P attempt was zeroed at CpsRwGetInp and went 0200/0002.
    assert(valid(false,0x03u,true));
    assert(valid(false,0x05u,true));
    assert(valid(false,0x06u,true));
    assert(valid(false,0x07u,true));
    assert(!valid(false,0x07u)); // outside verified DHC, 3P is not reassigned
    assert(nativeKenTwoPunchMask()==0x03u);
    assert(popcount3(nativeKenTwoPunchMask())==2u);
    assert(shieldPrequeue3P(true,true,0,true,0x12u,0x14u,
                           0x2E08u,0x2A28u,0x07u));
    // The host can see 0x07; the native port receives 0x03, not 0x00.
    for(unsigned m=0;m<64u;++m)if(m!=7u)
        assert(!shieldPrequeue3P(true,true,0,true,0x12u,0x14u,
                                 0x2E08u,0x2A28u,m));

    // Phase 9 (WAIT_RYU_SUPER_END) must neutralize held user chord.
    assert(XM87176Input::neutralize(9u,0x12u,0x14u,true,true,12232u,0u));
    assert(!XM87176Input::neutralize(9u,0x12u,0x14u,true,false,12232u,0u));
    assert(!XM87176Input::neutralize(9u,0x14u,0x12u,true,true,12232u,0u));
    // Existing nine-frame native Ken QCF+2P remains unmodified.
    for(unsigned t=0;t<9u;++t)
        assert(!XM87176Input::neutralize(8u,0x14u,0x12u,
                                        true,true,1884u+t,1884u));

    using XM87178Recovery::staleHostTag259;
    // Exact captured 14302 outcome: controller 8 consumed, Ryu
    // native action 0002 chosen, host +259 still 08.
    assert(staleHostTag259(2u,true,true,0x12u,0x14u,0x12u,0x14u,
                            0x0002u,0x0Au,0x00u,0x08u,0x00u));
    // Exact captured 12253/14783 timeout: native tag 3200 absent.
    assert(staleHostTag259(2u,true,true,0x12u,0x14u,0x12u,0x14u,
                            0x2E08u,0x0Au,0x00u,0x08u,0x00u));
    // Never alter a real tag, changed roster, native controller-8
    // awaiting consumption, nonowned latch, or ordinary gameplay.
    assert(!staleHostTag259(2u,true,true,0x12u,0x14u,0x12u,0x14u,
                            0x3200u,0x14u,0x00u,0x08u,0x00u));
    assert(!staleHostTag259(2u,true,true,0x14u,0x12u,0x12u,0x14u,
                            0x0002u,0x0Au,0x00u,0x08u,0x00u));
    assert(!staleHostTag259(2u,true,true,0x12u,0x14u,0x12u,0x14u,
                            0x0002u,0x0Au,0x08u,0x08u,0x00u));
    assert(!staleHostTag259(2u,true,true,0x12u,0x14u,0x12u,0x14u,
                            0x0002u,0x0Au,0x00u,0x08u,0x08u));
    assert(!staleHostTag259(2u,true,false,0x12u,0x14u,0x12u,0x14u,
                            0x0002u,0x0Au,0x00u,0x08u,0x00u));
    assert(!staleHostTag259(3u,true,true,0x12u,0x14u,0x12u,0x14u,
                            0x0002u,0x0Au,0x00u,0x08u,0x00u));
    std::puts("PASS 87.178: 3P maps to native 2P, phase 9 isolated, stale +259 narrowly restored");
    return 0;
}
