// 87.180 ROM-free C++11 regression of leaked DHC controller +259.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_178_tag_latch_recovery.h"
using XM87178Recovery::staleHostTag259;
static bool recover(unsigned action,unsigned d6,unsigned c256=0u,
                    unsigned c259=8u,unsigned original259=0u){
    return staleHostTag259(2u,true,true,0x12u,0x14u,
            0x12u,0x14u,action,d6,c256,c259,original259);
}
int main(){
    // Captured 87.179 wrong native actions changed D6 from 0A to 00.
    // Before 87.180 that prevented clearing the host-owned +259=08.
    assert(recover(0x2C02u,0x00u));
    assert(recover(0x2C04u,0x00u));
    // Earlier 87.178 0002 and rejected Ryu 2E08 cases still recover.
    assert(recover(0x0002u,0x0Au));
    assert(recover(0x2E08u,0x0Au));
    // Never clear native tag or other unrelated D6=00 actions,
    // unconsumed controller8, a naturally unset +259, or a pre-existing +259.
    assert(!recover(0x3200u,0x00u));
    assert(!recover(0x0002u,0x00u));
    assert(!recover(0x2C00u,0x00u));
    assert(!recover(0x2C02u,0x04u));
    assert(!recover(0x2C02u,0x00u,8u));
    assert(!recover(0x2C02u,0x00u,0u,0u));
    assert(!recover(0x2C02u,0x00u,0u,8u,8u));
    assert(!staleHostTag259(2u,false,true,0x12u,0x14u,
              0x12u,0x14u,0x2C02u,0u,0u,8u,0u));
    assert(!staleHostTag259(2u,true,true,0x14u,0x12u,
              0x12u,0x14u,0x2C02u,0u,0u,8u,0u));
    assert(!staleHostTag259(3u,true,true,0x12u,0x14u,
              0x12u,0x14u,0x2C02u,0u,0u,8u,0u));
    std::puts("PASS 87.180: wrong-action D6=00 stale host latch recovery and safety guards");
}
