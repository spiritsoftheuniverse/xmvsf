// XMVSF 87.483: the original FBNeo chooser->VS edge and HUD clock,
// shared by native game frames and SDL3. No ROM, input or timing writes.
#pragma once
#include "../../dev/stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h"
#include "../../dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_392_hud_shock.h"
#include <cstdint>
namespace cps2recomp::xmvsf_vs {
// The F10 capture starts at emulated frame 1451: game step 0008,
// both chooser records 07, and verified native ROM HP tiles 0x159B*.
// The music's pre-combat 120f deadline must NEVER reveal HUD ink
// before the original, drawable 08-state HUD actually arrives.
struct CombatHudClock {
    bool firstInkSeen = false;
    unsigned firstInkFrame = 0u;

    void Reset() {
        firstInkSeen = false;
        firstInkFrame = 0u;
    }
    void NativeInk(unsigned phase, unsigned frame) {
        if (phase != 8u || firstInkSeen ||
            !XM87392::timeline().primed) return;
        firstInkSeen = true;
        firstInkFrame = frame;
        std::fprintf(stderr,
            "XMVSF 87.483 HUD_COMBAT_FIRST_INK frame=%u "
            "native_status=08 source=verified_rom_hp\n", frame);
    }
    unsigned Step(unsigned phase, unsigned frame) const {
        const unsigned original = XM87392::timeline().step(frame);
        if (!XM87392::timeline().primed) return original;
        // Preserve all pre-combat invisibility, including early VS HUD art.
        if (phase != 8u || !firstInkSeen || frame <= firstInkFrame)
            return 0u;
        unsigned elapsed = frame - firstInkFrame;
        if (elapsed > XM87392::RevealFrames)
            elapsed = XM87392::RevealFrames;
        // Both original requirements must be satisfied: first queued
        // versus PCM +120f, AND actual verified live HUD ink.
        return original < elapsed ? original : elapsed;
    }
};
inline CombatHudClock &CombatHud() {
    static CombatHudClock value;
    return value;
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
