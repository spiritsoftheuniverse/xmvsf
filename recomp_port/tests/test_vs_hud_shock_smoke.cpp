// XMVSF 87.487: calibrated automatic reveal at VS PCM +180 emu frames.
// F10 is a read-only marker and is NEVER a runtime shock trigger.
#include "../../recomp/port/xmvsf_vs_handoff.h"
#include "../../recomp/port/xmvsf_versus_audio.h"
#include <cassert>
static_assert(sizeof(port::xmvsf_home::VersusAudio) > 0);
int main() {
    auto &clock = XM87392::timeline();
    clock.reset();
    cps2recomp::xmvsf_vs::State state;
    using namespace cps2recomp::xmvsf_vs;
    state.Observe(4, 2, 2, 2, 900, true);
    assert(state.sequence == 0);
    state.Observe(5, 7, 7, 2, 901, true);
    assert(state.sequence == 1 && clock.primed);
    // Observed calibration is FF5104=0008 and FF5000=04. The second
    // byte may still be 04 when the marked automatic shock should start.
    state.Observe(8, 7, 7, 4, 910, true);
    assert(state.sequence == 1);
    clock.startWithSong(1, 910);
    clock.noteHudSource(916);
    assert(clock.step(1029) == 0);
    assert(clock.step(1030) == 1); // legacy 120f clock, not our chosen cue
    assert(clock.step(1059) == 30);
    assert(HudStep(2, 1090) == 0);
    assert(HudStep(6, 1090) == 0);
    assert(HudStep(8, 915) == 0);
    NoteCombatHudInk(2, 915);
    assert(!CombatHud().firstInkSeen);
    NoteCombatHudInk(8, 916);
    assert(CombatHud().firstInkSeen && CombatHud().firstInkFrame == 916);
    assert(HudStep(8, 916) == 0);
    assert(HudStep(8, 1029) == 0);
    assert(HudStep(8, 1030) == 0);
    assert(HudStep(8, 1089) == 0); // entire native HUD concealed
    assert(HudStep(8, 1090) == 1); // EXACT user-marked instant
    assert(HudStep(8, 1091) == 2);
    assert(HudStep(8, 1104) == 15);
    assert(HudStep(8, 1119) == 30);
    // FBNeo full upper-HUD classification for recorded native game step
    // even if FF5000 remains 04; non-HP tiles are included in the mask.
    assert(HudObjectPhase(4, 8) == 8);
    assert(HudObjectPhase(2, 8) == 8);
    assert(HudObjectPhase(4, 4) == 4);
    assert(XM87392::objectForPhase(
        HudObjectPhase(4, 8), 7, 3, 0x15BA9, 176, 0, 1, 1, false, false));
    assert(!XM87392::objectForPhase(
        HudObjectPhase(4, 4), 7, 3, 0x15BA9, 176, 0, 1, 1, false, false));
    assert(XM87392::verifiedTransitionHud(7, 2, 0x159B5, 144, 20, 1, 1));
    assert(!XM87392::verifiedTransitionHud(7, 3, 0x15BA9, 176, 0, 1, 1));
    state.Observe(8, 7, 7, 0x0a, 1140, true);
    assert(!CombatHud().firstInkSeen && !clock.primed);
    state.Observe(4, 2, 2, 2, 1200, true);
    state.Observe(5, 7, 7, 2, 1201, true);
    assert(state.sequence == 2);
    assert(HudStep(8, 1202) == 0);
    assert(!XM87392::visible(0, 0, 0));
    assert(XM87392::visible(0, 0, 30));
    return 0;
}
