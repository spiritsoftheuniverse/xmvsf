// XMVSF 87.482: the original FBNeo chooser->VS edge and HUD clock,
// shared by native game frames and SDL3. No ROM, input or timing writes.
#pragma once
#include "../../dev/stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h"
#include "../../dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_392_hud_shock.h"
#include <cstdint>
namespace cps2recomp::xmvsf_vs {
struct State {
    XM20VsEarly::Gate gate{};
    unsigned previousStep = ~0u;
    std::uint64_t sequence = 0;

    void Observe(unsigned step, unsigned p1, unsigned p2,
                 unsigned fight, unsigned frame, bool active) {
        auto &hud = XM87392::timeline();
        if (!active) {
            gate = {};
            previousStep = ~0u;
            hud.observe(fight, frame, false);
            return;
        }
        if (step == 4u && previousStep != 4u) gate = {};
        previousStep = step;
        hud.observe(fight, frame, true);
        const auto event = gate.observe(step, p1, p2);
        // Exactly FBNeo Stage87.399: do not fire on the last select
        // button, nor when combat has already started.
        if (fight < 8u && (event == XM20VsEarly::VS_SCREEN_EDGE ||
                            event == XM20VsEarly::MATCH_SETUP_EDGE)) {
            hud.prepareForSong(frame);
            ++sequence;
        }
    }
};
inline State &Current() {
    static State value;
    return value;
}
} // namespace cps2recomp::xmvsf_vs
