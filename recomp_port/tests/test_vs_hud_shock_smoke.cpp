// 87.483: exact FBNeo 120+30 shock, held until true combat HP pixels.
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
    // The recorded F10 clip begins with step 0008, chooser states 07/07,
    // and fully drawable 0x159B5 HP art. Never start shock on VS music
    // or on pre-fight 02/04/06 artwork even if the 120f timer is over.
    using namespace cps2recomp::xmvsf_vs;
    assert(HudStep(2, 252) == 0);
    assert(HudStep(6, 252) == 0);
    assert(HudStep(8, 252) == 0);
    NoteCombatHudInk(2, 254); // VS art must not arm combat HUD
    assert(!CombatHud().firstInkSeen);
    NoteCombatHudInk(8, 255); // first REAL native combat HP ink
    assert(CombatHud().firstInkSeen && CombatHud().firstInkFrame == 255);
    assert(HudStep(8, 255) == 0); // first drawable HUD source hidden
    assert(HudStep(8, 256) == 1); // shock front begins a frame later
    assert(HudStep(8, 270) == 15);
    assert(HudStep(8, 285) == 30);
    state.Observe(8, 7, 7, 0x0a, 290, true);
    assert(!CombatHud().firstInkSeen);
    assert(!clock.primed);
    state.Observe(4, 2, 2, 2, 300, true);
    state.Observe(5, 7, 7, 2, 301, true);
    assert(state.sequence == 2);
    assert(HudStep(8, 302) == 0);
    // Captured first HP slot 16: x E0D0, y 2024, tile 59B5,
    // attr 0002 -> resolved tile 0x159B5 at screen (144,20).
    assert(XM87392::verifiedTransitionHud(7, 2, 0x159B5, 144, 20, 1, 1));
    assert(XM87392::verifiedTransitionHud(7, 2, 88417, 12, 20, 2, 2));
    assert(!XM87392::verifiedTransitionHud(7, 3, 50000, 12, 20, 2, 2));
    assert(!XM87392::visible(0, 0, 0));
    assert(XM87392::visible(0, 0, 30));
    return 0;
}
