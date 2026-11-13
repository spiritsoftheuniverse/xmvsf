// Standalone C++11 policy regression, not a new native build gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_334_morphus_2v2_precommit.h"
#include <cassert>
#include <cstdio>
using namespace XM87334Native;
int main(){
    auto r=resolve(0x14u,1u,0x03u); // Ken QCF, two punches
    assert(r.ok() && r.hyperD6==0x0Cu && !r.terminalThrow);
    r=resolve(0x12u,2u,0x18u); // Ryu QCB, two kicks
    assert(r.ok() && r.hyperD6==0x0Cu);
    r=resolve(0x1Eu,3u,0x03u); // Akuma ground Shun Goku Satsu
    assert(r.ok() && r.hyperD6==0x10u && r.terminalThrow);
    assert(resolve(0x1Eu,1u,0x03u).reason==AIR_ONLY);
    assert(resolve(0x06u,2u,0x18u).reason==CONDITIONAL);
    assert(resolve(0x14u,5u,0x03u).reason==NO_HYPER);
    assert(resolve(0x14u,7u,0x03u).reason==MOTION_NOT_MAPPED);
    assert(resolve(0x14u,1u,0x01u).reason==BAD_BUTTONS);
    assert(resolve(0x14u,1u,0x1Bu).reason==BAD_BUTTONS);
    assert(resolve(0x13u,1u,0x03u).reason==BAD_FIGHTER);
    Pending p;
    assert(p.stage(1u,0x12u,0x14u,99u,resolve(0x14u,1u,0x03u)));
    assert(!p.stage(1u,0x12u,0x14u,100u,resolve(0x14u,1u,0x03u)));
    assert(!p.matchesAfterOriginalSwap(0u,0x14u,0x12u));
    assert(!p.matchesAfterOriginalSwap(1u,0x12u,0x14u));
    assert(p.matchesAfterOriginalSwap(1u,0x14u,0x12u));
    p.markConsumed();
    assert(!p.matchesAfterOriginalSwap(1u,0x14u,0x12u));
    p.clear();assert(!p.armed);
    std::puts("XMVSF 87.334 native two-member D6 precommit policy PASS");
}
