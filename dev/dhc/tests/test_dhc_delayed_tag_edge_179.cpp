// Stage 87.179 ROM-free C++11 synthetic timing/ownership regression.
// Does not execute original ROM code or claim in-game acceptance.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_179_delayed_tag_edge.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_176_dhc_input_isolation.h"
#include "../../stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_178_tag_latch_recovery.h"
using namespace XM87179Edge;
struct OneFrameInputPipeline {
    unsigned native=0, port=0;
    unsigned cpuFrame(unsigned sampledPort) {
        native=port;
        port=sampledPort;
        return native;
    }
};
int main(){
    // Observed 87.178 success: queue frame 2192 captured port LP+MP,
    // controller 8 was armed after CPU; 2193 native edge was 0030.
    OneFrameInputPipeline direct;
    assert(direct.cpuFrame(0x03u)==0u);
    assert(direct.cpuFrame(0u)==0x03u);
    // Observed failures queued 3107/3502/4403/5017 and delayed
    // to 3114/3511/4406/5023: controller 8 was armed after
    // zero punches, so the NEXT ROM frame saw CA=0000.
    // New plan: prepare pulse on the next pre-CPU sampling frame,
    // arm controller 8 AFTER that CPU frame, and see it at n+2.
    for(unsigned safeWindow : {3114u,3511u,4406u,5023u}){
        const unsigned due=safeWindow+1u;
        assert(eligible(11u,true,true,false,
               0x12u,0x14u,0x2E08u,0x2A28u,0x0Au,
               0u,0u,0u,due,due));
        assert(!eligible(11u,true,true,false,
               0x12u,0x14u,0x2E08u,0x2A28u,0x0Au,
               0u,0u,0u,safeWindow,due));
        assert(!eligible(11u,true,true,false,
               0x12u,0x14u,0x2E08u,0x2A28u,0x0Au,
               0u,0u,0u,due+1u,due));
        OneFrameInputPipeline late;
        assert(late.cpuFrame(0u)==0u); // safe-window frame
        assert(late.cpuFrame(nativePunchPair())==0u); // prime frame
        assert(late.cpuFrame(0u)==nativePunchPair()); // controller frame
    }
    // No forged input in other roster, proof mode, non-Ryu state,
    // during an already active controller, or with held punches.
    assert(!eligible(11u,true,true,true,0x12u,0x14u,0x2E08u,
           0x2A28u,0x0Au,0u,0u,0u,10u,10u));
    assert(!eligible(2u,true,true,false,0x12u,0x14u,0x2E08u,
           0x2A28u,0x0Au,0u,0u,0u,10u,10u));
    assert(!eligible(11u,true,true,false,0x14u,0x12u,0x2E08u,
           0x2A28u,0x0Au,0u,0u,0u,10u,10u));
    assert(!eligible(11u,true,true,false,0x12u,0x14u,0x2E08u,
           0x2A28u,0x0Au,8u,0u,0u,10u,10u));
    assert(!eligible(11u,true,true,false,0x12u,0x14u,0x2E08u,
           0x2A28u,0x0Au,0u,0u,0x0030u,10u,10u));
    assert(!eligible(11u,true,true,false,0x12u,0x14u,0x2E08u,
           0x002Au,0x0Au,0u,0u,0u,10u,10u));
    assert(XM87176Input::neutralize(11u,0x12u,0x14u,true,true,10u,0u));
    assert(nativePunchPair()==0x03u);
    assert(facingForward(1u)==1u);
    assert(facingForward(0u)==2u);
    // Original 87.178 failures: Ryu 2E08 / 002A, controller256=00,
    // +259 remains 08, and captured A0 was native 2A28.
    assert(canRestoreStagedMode(2u,true,true,0x12u,0x14u,
           0x12u,0x14u,0x2E08u,0x2E08u,0x002Au,0x2A28u,
           0x0Au,0x0Au,0u,8u,0u));
    assert(XM87178Recovery::staleHostTag259(2u,true,true,
           0x12u,0x14u,0x12u,0x14u,0x2E08u,0x0Au,0u,8u,0u));
    // Absolutely no rollback if ROM owns another action, real tag,
    // changed D6, modified roster, unconsumed controller, or no latch.
    for(unsigned action : {0x0002u,0x3200u,0x2C04u})
        assert(!canRestoreStagedMode(2u,true,true,0x12u,0x14u,
             0x12u,0x14u,action,0x2E08u,0x002Au,0x2A28u,
             0x0Au,0x0Au,0u,8u,0u));
    assert(!canRestoreStagedMode(2u,true,true,0x12u,0x14u,
         0x12u,0x14u,0x2E08u,0x2E08u,0x002Au,0x2A28u,
         0x0Au,0x0Au,8u,8u,0u));
    assert(!canRestoreStagedMode(2u,true,true,0x12u,0x14u,
         0x12u,0x14u,0x2E08u,0x2E08u,0x002Au,0x2A28u,
         0x0Au,0x0Au,0u,0u,0u));
    std::puts("PASS 87.179: late native port edge timing and narrow host A0 recovery");
    return 0;
}
