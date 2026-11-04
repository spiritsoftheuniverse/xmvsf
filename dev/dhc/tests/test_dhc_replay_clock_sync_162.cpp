// Stage 87.162: F9 movie index is monotonic and independent of native
// GetCurrentFrame() resets caused by loading a training save-state.
// This bridge is observational ONLY and must never alter CPS2 inputs.
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage87_162_replay_sync.h"
#include <cassert>
static unsigned observedFromAnotherTranslationUnit(){
    return XM87162ReplaySync::movieFrame();
}
int main(){
    using namespace XM87162ReplaySync;
    assert(movieFrame()==~0u);
    publish(0u);assert(movieFrame()==0u);
    publish(1098u);assert(observedFromAnotherTranslationUnit()==1098u);
    // The native game frame can jump back, while the replay index must
    // continue to increase independently of the savestate clock.
    publish(3319u);assert(movieFrame()==3319u);
    publish(17128u);assert(movieFrame()==17128u);
    publish(~0u);assert(movieFrame()==~0u);
    return 0;
}
