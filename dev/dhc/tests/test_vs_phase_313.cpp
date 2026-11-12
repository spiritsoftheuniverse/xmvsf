// 87.322: VS song must begin on native screen handoff, not the last
// character selection and never as a late match-start fallback.
// Standalone test, not an extra production build gate.
#include "../../stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace XM20VsEarly;
    Gate normal;
    assert(normal.observe(4,1,0)==NONE);
    assert(normal.observe(4,2,0)==NONE);
    assert(normal.observe(4,7,1)==NONE);
    assert(normal.observe(4,7,7)==NONE); // completion still on chooser
    assert(normal.observe(6,7,7)==VS_SCREEN_EDGE); // first VS frame
    assert(normal.observe(8,7,7)==NONE);
    Gate cpuOpponent;
    assert(cpuOpponent.observe(4,1,0)==NONE);
    assert(cpuOpponent.observe(4,7,0)==NONE);
    assert(cpuOpponent.observe(6,7,0)==VS_SCREEN_EDGE);
    Gate direct;
    assert(direct.observe(4,1,0)==NONE);
    assert(direct.observe(8,7,0)==MATCH_SETUP_EDGE); // only used if native status < 2
    assert(direct.observe(8,7,0)==NONE);
    Gate stale;
    assert(stale.observe(8,7,7)==NONE);
    assert(stale.observe(4,7,7)==NONE);
    assert(stale.observe(4,6,7)==NONE);
    assert(stale.observe(4,7,7)==NONE);
    assert(stale.observe(6,7,7)==VS_SCREEN_EDGE);
    Gate noChooser;
    assert(noChooser.observe(6,7,7)==NONE);
    std::puts("XMVSF 87.322: first native VS screen handoff gate PASS");
    return 0;
}
