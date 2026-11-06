// 87.195: actual native jump apex, natural Y and opponent-relative X.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_194_native_passive_jump_entry.h"
using namespace XM87194Jump;
int main(){
    Arrival a;
    assert(a.begin(0u,0x14u,100u,304,408,472,740,true));
    assert(a.targetX==692);
    assert(a.position(100u,408)==304);
    // Native ROM ascends from 408 to 380 before beginning the descent.
    assert(a.position(101u,394)==304);
    assert(a.position(102u,380)==304);
    assert(a.apexY==380);
    const int a0=a.position(103u,390);
    assert(a0>304 && a0<692);
    // Midway through the full 380..472 native descent, NOT only
    // halfway through the latter 408..472 portion.
    const int middle=a.position(104u,426);
    assert(middle>=(304+692)/2-3 && middle<=(304+692)/2+3);
    assert(!a.ready(0x14u,426));
    assert(a.position(105u,472)==692);
    assert(a.ready(0x14u,472));
    assert(a.landingFrame==105u);
    // Mirror the same physical curve to the other side.
    Arrival mirrored;
    assert(mirrored.begin(1u,0x12u,200u,704,408,472,256,false));
    assert(mirrored.targetX==304);
    assert(mirrored.position(200u,408)==704);
    assert(mirrored.position(201u,380)==704);
    const int leftMiddle=mirrored.position(202u,426);
    assert(leftMiddle>=(704+304)/2-3 && leftMiddle<=(704+304)/2+3);
    assert(mirrored.position(203u,450)<leftMiddle);
    assert(mirrored.position(204u,472)==304);
    assert(mirrored.ready(0x12u,472));
    // One immediate ROM floor snap, a stalled animation, or absent
    // descent cannot authorize reserve input or a false 2E super.
    Arrival stalled;
    assert(stalled.begin(0u,0x14u,300u,304,408,472,740,true));
    for(unsigned n=301u;n<700u;++n){
        assert(stalled.position(n,408)==304);
        assert(!stalled.ready(0x14u,408));
    }
    assert(stalled.position(700u,472)==304);
    assert(!stalled.ready(0x14u,472));
    std::puts("PASS 87.195 native apex-to-floor arc: full-descent X travel, mirrored target, no phantom landing");
}
