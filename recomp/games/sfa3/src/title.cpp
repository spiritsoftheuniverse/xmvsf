// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: "Straight to the title" (the Game page), as in the other games. When the board starts
// (switched on, or reset from the menu), everything before the title is run unseen and unheard, as fast as it goes:
// the board's memory test, the warning, the notices and the introduction. The picture comes back with the title;
// nothing of the game is changed, only not shown.
//
// The game's step (sfa3.h, kMode) goes 2 to 8 in twos while the board starts (the warning, frames 100-470), then 0 and
// into its show: 6 to 0x0E the notices, 0x10 the introduction (from frame 877), which ends with the title coming in
// from white (its own count of frames, kIntroFrames, at 0x5B0: frame 2337), then 0x12 on (the title at 0x14, the
// demonstration fights and rankings). With a coin in it goes from wherever it is to the title that asks for Start. The
// board's start and its show share their numbers, so the start's 0 after its 8 is waited for first. Only the board's
// start begins it (Machine::frame is 0 again at a reset): the step is 0 for a few frames between the screens of a game
// too.
#include "alternative.h"
#include "sfa3.h"

namespace cps2recomp {

namespace {
    using namespace sfa3;

    constexpr uint16_t kLastBootStep = 0x08, kIntroduction = 0x10;
    constexpr uint16_t kTitleFromWhite = 0x5B0;
    constexpr int kSkipMost = 6000; // frames after which the skipping is given up

    bool g_skipping = false;
    bool g_lastBootStep = false; // the board's start reached its last step
    bool g_booted = false;       // the board's own start (the warning) is over
    bool g_fast = false;         // this file asked for the game to run unseen
    int g_frames = 0;

    void Fast(bool on) {
        if (on != g_fast) {
            g_fast = on;
            SetFastForward(on);
        }
    }
} // namespace

void TitleFrame(Machine &machine) {
    const uint16_t mode = Ram16(kMode);
    // (only the board's first frame: the step is 0 again between screens of a game, and a game must never be run
    // unseen. A reset while it skips starts it again: what the last start had seen is not this one's)
    if (machine.frame <= 1 && mode == 0) {
        g_skipping = FeatureOn("skip_intro") && !modes::AttractMode(); // (attract mode is the show itself)
        g_lastBootStep = g_booted = false;
        g_frames = 0;
    }
    // (the feature switched off while it skips: the picture comes back at once)
    if (g_skipping && !FeatureOn("skip_intro")) {
        g_skipping = false;
    }
    if (g_skipping) {
        const bool wasBooted = g_booted;
        g_lastBootStep = g_lastBootStep || mode == kLastBootStep;
        g_booted = g_booted || (g_lastBootStep && mode == 0);
        const bool title = wasBooted && mode == kIntroduction && Ram16(kIntroFrames) >= kTitleFromWhite;
        if (title || (wasBooted && (mode > kIntroduction || mode == 2 || Ram8(kCredits) != 0)) || ++g_frames > kSkipMost) {
            g_skipping = false;
        }
    }
    Fast(g_skipping);
}

void RegisterTitle() {
    TheGame().features.push_back({"skip_intro", "Game", "Straight to the title",
                                  "When the game starts, and when it is reset, goes straight to the title: the board's memory test, "
                                  "the warning, the notices and the introduction are run unseen, in a second or so.",
                                  0});
}

} // namespace cps2recomp
