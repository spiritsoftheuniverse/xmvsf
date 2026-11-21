// XMVSF 87.484: original FBNeo full HUD shock, immediate F10 preview,
// shared by native game frames and SDL3. No ROM, input or timing writes.
#pragma once
#include "../../dev/stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h"
#include "../../dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_392_hud_shock.h"
#include <cstdint>
namespace cps2recomp::xmvsf_vs {
// 87.484: F10's observed frame 1451 is the intended FIRST SHOCK FRAME,
// not the beginning of another 120-frame hold. F10 is an explicit diagnostic
// preview, while gameplay normally uses the first verified combat HUD ink.
struct CombatHudClock {
    bool firstInkSeen = false;
    unsigned firstInkFrame = 0u;
    bool preview = false;

    void Reset() {
        firstInkSeen = false;
        firstInkFrame = 0u;
        preview = false;
    }
    void PreviewAt(unsigned frame) {
        // The exact F10 down-edge is the start of the shock, no offset.
        firstInkSeen = true;
        firstInkFrame = frame;
        preview = true;
        std::fprintf(stderr,
            "XMVSF 87.484 HUD_F10_SHOCK_START frame=%u step=1\n", frame);
    }
    void NativeInk(unsigned phase, unsigned frame) {
        if (phase != 8u || firstInkSeen ||
            !XM87392::timeline().primed) return;
        firstInkSeen = true;
        firstInkFrame = frame;
        std::fprintf(stderr,
            "XMVSF 87.484 HUD_COMBAT_FIRST_INK frame=%u step=1\n", frame);
    }
    unsigned Step(unsigned phase, unsigned frame) const {
        const auto &clock = XM87392::timeline();
        if (!clock.primed && !preview) return XM87392::RevealFrames;
        // Keep the full top HUD hidden BEFORE the combat/preview edge.
        if (phase != 8u || !firstInkSeen || frame < firstInkFrame)
            return 0u;
        const unsigned elapsed = frame - firstInkFrame + 1u;
        return elapsed < XM87392::RevealFrames ? elapsed
                                               : XM87392::RevealFrames;
    }
};
inline CombatHudClock &CombatHud() {
    static CombatHudClock value;
    return value;
}
inline void PreviewShockAt(unsigned frame) {
    CombatHud().PreviewAt(frame);
}
inline unsigned HudStep(unsigned phase, unsigned frame) {
    return CombatHud().Step(phase, frame);
}
inline void NoteCombatHudInk(unsigned phase, unsigned frame) {
    CombatHud().NativeInk(phase, frame);
}

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
            CombatHud().Reset();
            hud.observe(fight, frame, false);
            return;
        }
        if (step == 4u && previousStep != 4u) gate = {};
        previousStep = step;
        if (frame < hud.lastFrame || fight == 0x0au || fight == 0x0cu)
            CombatHud().Reset();
        hud.observe(fight, frame, true);
        const auto event = gate.observe(step, p1, p2);
        // Exactly FBNeo Stage87.399: do not fire on the last select
        // button, nor when combat has already started.
        if (fight < 8u && (event == XM20VsEarly::VS_SCREEN_EDGE ||
                            event == XM20VsEarly::MATCH_SETUP_EDGE)) {
            CombatHud().Reset();
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
