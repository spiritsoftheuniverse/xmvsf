// Opt-in, standalone pure-policy test for 87.313. NOT an extra build gate.
// g++ -std=c++11 -Wall -Wextra -pedantic dev/dhc/tests/test_vs_phase_313.cpp -o test_vs_phase_313.exe
#include "../../stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace XM20VsEarly;
    Gate normal;
    assert(normal.observe(4,1,0)==NONE);
    assert(normal.observe(4,2,0)==NONE);
    assert(normal.observe(4,7,1)==NONE);
    assert(normal.observe(4,7,2)==NONE);
    assert(normal.observe(4,7,7)==BOTH_CHOOSERS_DONE);
    assert(normal.observe(4,7,7)==NONE);
    assert(normal.observe(8,7,7)==NONE);
    Gate cpuOpponent;
    assert(cpuOpponent.observe(4,1,0)==NONE);
    assert(cpuOpponent.observe(4,7,0)==NONE);
    assert(cpuOpponent.observe(8,7,0)==MATCH_SETUP_EDGE);
    assert(cpuOpponent.observe(8,7,0)==NONE);
    Gate stale;
    assert(stale.observe(8,7,7)==NONE); // stale rematch cannot arm
    assert(stale.observe(4,7,7)==NONE); // initial fully-done state not selection edge
    assert(stale.observe(4,6,7)==NONE);
    assert(stale.observe(4,7,7)==BOTH_CHOOSERS_DONE);
    Gate incomplete;
    assert(incomplete.observe(4,1,2)==NONE);
    assert(incomplete.observe(4,7,2)==NONE);
    assert(incomplete.observe(4,7,6)==NONE);
    assert(incomplete.observe(8,7,6)==MATCH_SETUP_EDGE);
    std::puts("XMVSF 87.313: ROM chooser to VS pre-stage policy PASS");
    return 0;
}
