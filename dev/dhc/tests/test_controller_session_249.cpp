// TEST 87.249: menu-confirmed Xbox/DS4Windows pad, NOT saved profile,
// is allowed to control native CPS2 P1 without altering persisted bindings.
// The policy is directly used by both game-input mapping and SDL polling.
#include <cassert>
#include <cstdio>
#include "../../stable/buildtree/src/burner/sdl/xmvsf_stage249_controller_policy.h"
int main() {
    using namespace XM249Policy;
    assert(!needsVirtualMap(false,false)); // legacy raw Fighter Pi/keyboard
    assert(needsVirtualMap(true,false));   // saved P1 assignment
    assert(needsVirtualMap(false,true));   // Xbox menu A with no profile
    assert(needsVirtualMap(true,true));    // menu Xbox overrides old Fighter Pi
    assert(useSemanticPad(true,false,false,false));
    assert(useSemanticPad(true,true,false,false)); // old Fighter Pi
    assert(useSemanticPad(true,true,true,false));  // saved keyboard
    assert(!useSemanticPad(true,true,false,true)); // same mapped pad: custom binds
    assert(!useSemanticPad(false,true,false,false));
    assert(!useSemanticPad(false,false,false,false));
    assert(deliver(false,true,true)); // native Xbox A reaches game even if raw 0
    assert(!deliver(true,false,true)); // stale old device binding ignored
    assert(deliver(true,false,false)); // mapped Fighter Pi still works
    assert(!deliver(false,true,false)); // no speculative custom-bind overrides
    std::puts("XMVSF 87.249 menu controller semantic handoff PASS");
    return 0;
}
