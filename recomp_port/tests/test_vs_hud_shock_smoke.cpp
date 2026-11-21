// 87.482: native 04->VS edge, exact original 120+30 HUD clock, SDL3 helper syntax.
#include "../../recomp/port/xmvsf_vs_handoff.h"
#include "../../recomp/port/xmvsf_versus_audio.h"
#include <cassert>
static_assert(sizeof(port::xmvsf_home::VersusAudio) > 0);
int main() {
    auto &clock = XM87392::timeline();
    clock.reset();
    cps2recomp::xmvsf_vs::State state;
    state.Observe(4, 2, 2, 2, 100, true);
    assert(state.sequence == 0);
    state.Observe(5, 7, 7, 2, 101, true);
    assert(state.sequence == 1 && clock.primed);
    state.Observe(8, 7, 7, 8, 102, true);
    assert(state.sequence == 1);
    clock.startWithSong(1, 103);
    clock.noteHudSource(104);
    assert(clock.step(222) == 0);
    assert(clock.step(223) == 1);
    assert(clock.step(252) == 30);
    assert(XM87392::verifiedTransitionHud(7, 2, 88417, 12, 20, 2, 2));
    assert(!XM87392::verifiedTransitionHud(7, 3, 50000, 12, 20, 2, 2));
    assert(!XM87392::visible(0, 0, 0));
    assert(XM87392::visible(0, 0, 30));
    return 0;
}
