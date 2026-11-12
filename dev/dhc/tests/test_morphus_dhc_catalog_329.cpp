// Isolated C++11 catalog test. Not added to the normal native build gate.
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_329_morphus_dhc_catalog.h"
#include <cassert>
#include <cstdio>
using namespace XM87329Morphus;
int main() {
    assert(review(0x12u,1u)==POSSIBLE);          // Ryu QCF
    assert(review(0x14u,5u)==NO_HYPER);          // Ken HCF
    assert(review(0x06u,2u)==CONDITIONAL);       // Rogue absorbed
    assert(review(0x1Eu,1u)==AIR_ONLY);          // Akuma air-only QCF
    assert(review(0x1Eu,3u)==POSSIBLE_THROW);    // Akuma DP
    assert(review(0x1Au,1u)==POSSIBLE_THROW);    // Zangief QCF
    assert(review(0x22u,2u)==POSSIBLE);          // Cammy QCB
    assert(review(0x10u,1u)==NO_HYPER);          // Apocalypse
    assert(review(0x14u,7u)==UNKNOWN_MOTION);    // 2QCF != double-down
    assert(review(0x13u,1u)==NOT_REGISTERED);
    assert(rejectBeforeTag(NO_HYPER));
    assert(rejectBeforeTag(CONDITIONAL));
    assert(!rejectBeforeTag(UNKNOWN_MOTION));
    assert(!rejectBeforeTag(POSSIBLE_THROW));
    std::puts("XMVSF 87.329 Morphus Hyper preflight catalog: PASS");
}
