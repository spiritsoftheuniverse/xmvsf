// 87.186 ROM-free C++11: verify four untested original attack frames.
// This is a native-controller experiment; it does NOT prove ROM accepts it.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_186_first_attack_gate.h"
int main(){
    XM87184Life::Sample attack(0x12u,0x2E08u,0x2A28u,4u,0x0D30B0u,0x0Au);
    XM87184Life::Sample startup(0x12u,0x2E08u,0x2A28u,2u,0x0D3070u,0x0Au);
    XM87184Life::Sample recovery(0x12u,0x2E0Au,0x2A28u,6u,0x0D30F4u,0x0Au);
    for(unsigned timer=0x3Du;timer<=0x40u;++timer)
        assert(XM87186FirstAttack::eligible(attack,timer,0x0001u,0u));
    assert(!XM87186FirstAttack::eligible(attack,0x3Cu,0x0001u,0u));
    assert(!XM87186FirstAttack::eligible(attack,0x41u,0x0001u,0u));
    assert(!XM87186FirstAttack::eligible(startup,0x40u,0x0001u,0u));
    assert(!XM87186FirstAttack::eligible(recovery,0x40u,0x0001u,0u));
    assert(!XM87186FirstAttack::eligible(attack,0x40u,0x0031u,0u));
    assert(!XM87186FirstAttack::eligible(attack,0x40u,0x0001u,0x0020u));
    std::puts("PASS 87.186: four-frame attack-only gate, safe input, no startup/recovery rewrites");
}