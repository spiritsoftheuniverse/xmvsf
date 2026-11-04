// g++ -std=c++11 -Wall -Wextra -pedantic dev/dhc/tests/test_dhc_landing.cpp -o dhc_landing_test
#include "../xmvsf_stage87_137_dhc_landing.h"
#include <cassert>
using namespace XM87137Landing;
int main(){
    Target left=choose(300,500,48,0,1024);
    assert(left.valid && left.x==452 && left.faceRight);
    Target right=choose(700,500,48,0,1024);
    assert(right.valid && right.x==548 && !right.faceRight);
    // Target follows opponent, not outgoing point position.
    Target reposition=choose(100,600,48,0,1024);
    assert(reposition.valid && reposition.x==552);
    // Edge cases: don't invent a cross-over location.
    assert(!choose(300,20,48,0,1024).valid); // right side is also possible, but same-side policy
    assert(!choose(500,1000,48,0,1024).valid);
    assert(!choose(300,500,0,0,1024).valid);
    return 0;
}
