// XMVSF 87.487: calibrated VS +180f full-HUD timing; F10 diagnostic only,
// shared by native game frames and SDL3. No ROM, input or timing writes.
#pragma once
#include "../../dev/stable/buildtree/src/burner/sdl/xm20_vs_early_phase.h"
#include "../../dev/stable/buildtree/src/burn/drv/capcom/xmvsf_stage87_392_hud_shock.h"
#include <cstdint>
namespace cps2recomp::xmvsf_vs {
// The uploaded F10 capture only states when the debugger was pressed
// (native frame 1451). It does NOT contain the earlier versus PCM
// timestamp, so the precise relative delay is not derivable from it.
// Keep the original FBNeo audio timeline until a paired timestamp exists.
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
            "XMVSF 87.485 HUD_COMBAT_FIRST_INK frame=%u\n", frame);
    }
    // Captured in one session: first VS PCM at frame 910, user-marked
    // intended shock at frame 1090. The delay is RELATIVE to the real
    // VS PCM start, not tied to F10 or to absolute frame 1090.
    static constexpr unsigned kSongToShockFrames = 180u;
    unsigned Step(unsigned phase, unsigned frame) const {
        const auto &clock = XM87392::timeline();
        if (!clock.primed) return XM87392::RevealFrames;
        // All native top-HUD sources are completely concealed beforehand.
        if (phase != 8u || !firstInkSeen || !clock.armed)
            return 0u;
        unsigned due = clock.start + kSongToShockFrames;
        // If graphics become drawable late, conceal their first frame.
        if (firstInkFrame >= due) due = firstInkFrame + 1u;
        if (frame < due) return 0u;
        const unsigned step = frame - due + 1u;
        return step < XM87392::RevealFrames
            ? step : XM87392::RevealFrames;
    }
};
inline CombatHudClock &CombatHud() {
    static CombatHudClock value;
    return value;
}
// Only used for source identity: FF5104 is the recorded native gameplay
// step (0008 during F10); FF5000 is a DIFFERENT byte and must never be
// silently conflated with it. Keep the transition's strict classifier.
inline int HudObjectPhase(int phase, unsigned nativeGameStep) {
    return nativeGameStep == 8u ? 8 : phase;
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
