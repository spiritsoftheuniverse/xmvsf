// Stage 87.334: standalone C++11 native PP/KK-to-D6 regressions.
// Not added to the normal Windows BUILD_TEST pipeline.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_334_morphus_2v2_precommit.h"
#include <cassert>
#include <cstdio>
using namespace XM87334Native;
int main(){
    unsigned n=0u;commands(&n);
    assert(n==34u); // actual direct native PP/KK command entries from donor JSON
    auto r=resolve(0x14u,1u,0x03u); // Ken QCF+PP: Shoryureppa
    assert(r.ok() && r.hyperD6==0x0Cu && !r.terminalThrow);
    r=resolve(0x14u,1u,0x18u); // Ken QCF+KK: Shinryuken, NOT 0x0C
    assert(r.ok() && r.hyperD6==0x0Eu);
    r=resolve(0x12u,1u,0x03u); // Ryu QCF+PP
    assert(r.ok() && r.hyperD6==0x0Au);
    r=resolve(0x12u,2u,0x18u); // Ryu QCB+KK
    assert(r.ok() && r.hyperD6==0x0Cu);
    r=resolve(0x0Eu,1u,0x18u); // Magneto QCF+KK
    assert(r.ok() && r.hyperD6==0x06u);
    r=resolve(0x1Eu,1u,0x03u); // Akuma ground QCF+PP
    assert(r.ok() && r.hyperD6==0x16u);
    r=resolve(0x22u,1u,0x18u); // Cammy QCF+KK
    assert(r.ok() && r.hyperD6==0x0Eu);
    assert(resolve(0x1Eu,3u,0x03u).reason==NO_HYPER); // SGS needs sequence
    assert(resolve(0x06u,2u,0x18u).reason==NO_HYPER); // conditional SGS
    assert(resolve(0x1Au,1u,0x03u).reason==NO_HYPER); // 360 not QCF
    assert(resolve(0x02u,2u,0x03u).reason==NO_HYPER); // F,DF,D not QCB
    assert(resolve(0x14u,5u,0x03u).reason==NO_HYPER);
    assert(resolve(0x14u,7u,0x03u).reason==MOTION_NOT_MAPPED);
    assert(resolve(0x14u,1u,0x01u).reason==BAD_BUTTONS);
    assert(resolve(0x14u,1u,0x1Bu).reason==BAD_BUTTONS);
    assert(resolve(0x13u,1u,0x03u).reason==BAD_FIGHTER);
    Pending p;
    assert(p.stage(1u,0x12u,0x14u,99u,resolve(0x14u,1u,0x03u)));
    assert(p.d6==0x0Cu);
    assert(!p.stage(1u,0x12u,0x14u,100u,resolve(0x14u,1u,0x03u)));
    assert(!p.matchesAfterOriginalSwap(0u,0x14u,0x12u));
    assert(!p.matchesAfterOriginalSwap(1u,0x12u,0x14u));
    assert(p.matchesAfterOriginalSwap(1u,0x14u,0x12u));
    p.markConsumed();
    assert(!p.matchesAfterOriginalSwap(1u,0x14u,0x12u));
    p.clear();assert(!p.armed);
    std::puts("XMVSF 87.334 native-input D6 precommit policy PASS");
}
