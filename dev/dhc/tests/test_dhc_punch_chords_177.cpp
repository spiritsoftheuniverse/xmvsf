// ROM-free C++11 regression: DHC 3P and all punch pairs; native 2P preserved.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_177_dhc_punch_chords.h"
using namespace XM87177Chords;
int main(){
    unsigned validPunches=0;
    for(unsigned m=0;m<64u;++m){
        const bool validP=(m==0x03u||m==0x05u||m==0x06u||m==0x07u);
        const bool validTwoP=(m==0x03u||m==0x05u||m==0x06u);
        assert(valid(false,m)==validTwoP); // no 3P outside DHC
        assert(valid(false,m,true)==validP); // 3P during native DHC only
        if(validP)++validPunches;
        const bool validK=(m==0x18u||m==0x28u||m==0x30u);
        assert(valid(true,m)==validK);
    }
    assert(validPunches==4u);
    assert(!valid(false,0x47u,true)); // reject high bits too
    assert(!valid(false,0x0Fu,true)); // no mixed P+K
    assert(!valid(true,0x38u)); // 3K superjump not DHC
    assert(nativeKenTwoPunchMask()==0x03u); // verified Ken input, never 3P dash
    assert(shieldPrequeue3P(true,true,0,true,0x12,0x14,0x2E08,0x2A28,7));
    assert(shieldPrequeue3P(true,true,0,true,0x12,0x14,0x2E08,0x2A02,7));
    for(unsigned p=1;p<12u;++p)
        assert(!shieldPrequeue3P(true,true,p,true,0x12,0x14,0x2E08,0x2A28,7));
    assert(!shieldPrequeue3P(false,true,0,true,0x12,0x14,0x2E08,0x2A28,7));
    assert(!shieldPrequeue3P(true,false,0,true,0x12,0x14,0x2E08,0x2A28,7));
    assert(!shieldPrequeue3P(true,true,0,false,0x12,0x14,0x2E08,0x2A28,7));
    assert(!shieldPrequeue3P(true,true,0,true,0x14,0x12,0x2E08,0x2A28,7));
    assert(!shieldPrequeue3P(true,true,0,true,0x12,0x14,0x2E00,0x2A28,7));
    assert(!shieldPrequeue3P(true,true,0,true,0x12,0x14,0x2E08,0x2A00,7));
    assert(!shieldPrequeue3P(true,true,0,true,0x12,0x14,0x2E08,0x2A28,0x0Fu));
    assert(!shieldPrequeue3P(true,true,0,true,0x12,0x14,0x2E08,0x2A28,0x03u));
    std::puts("PASS 87.177: 4 punch chords; no 3P dash in DHC; original 2P Ken input");
    return 0;
}
