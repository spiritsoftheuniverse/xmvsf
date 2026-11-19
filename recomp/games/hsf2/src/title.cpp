// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: "Straight to the title" (the Game page), as in the other games. When the board starts
// (switched on, or reset from the menu), everything before the title is run unseen and unheard, as fast as it goes: the
// board's memory test, the warning and the introduction (the anniversary's mark, the maker's, and each old game's title
// in turn). The picture comes back with the title; nothing of the game is changed, only not shown.
//
// The board's start steps the first step word (hsf2.h, kMode) 2 to 10 in twos in its own task (the warning at 4 and 6),
// then the show's task begins (frame 837) and steps it from 2: 6 the introduction, whose own step is 0x20 as the title
// comes in (frame 2501), then 8 the health warning. With a coin in, the
// title that asks for Start is a task of its own (kTitleTask), and Start begins the game's (kGameTask): either ends the
// skipping too. Only the board's start begins it (Machine::frame is 0 again at a reset).
#include "alternative.h"
#include "hsf2.h"

namespace cps2recomp {

namespace {
    using namespace hsf2;

    constexpr int kSkipMost = 6000; // frames after which the skipping is given up

    bool g_skipping = false;
    bool g_fast = false; // this file asked for the game to run unseen
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
    // (only the board's first frame: the step is 0 again between screens of a game, and a game must never be
    // run unseen. A reset while it skips starts it again: what the last start had seen is not this one's)
    if (machine.frame <= 1 && mode == 0) {
        g_skipping = FeatureOn("skip_intro") && !modes::AttractMode(); // (attract mode is the show itself)
        g_frames = 0;
    }
    // (the feature switched off while it skips: the picture comes back at once)
    if (g_skipping && !FeatureOn("skip_intro")) {
        g_skipping = false;
    }
    if (g_skipping) {
        const bool show = Ram8(kShowTask) != 0;
        const bool title = (show && ((mode == kIntro && Ram16(kMatchStep) >= kTitleStep) || mode > kIntro)) || Ram8(kTitleTask) || InGame();
        if (title || ++g_frames > kSkipMost) {
            g_skipping = false;
        }
    }
    Fast(g_skipping);
}

void RegisterTitle() {
    TheGame().features.push_back({"skip_intro", "Game", "Straight to the title",
                                  "When the game starts, and when it is reset, goes straight to the title: the board's memory test, "
                                  "the warning and the introduction are run unseen, in a second or so.",
                                  0});
}

} // namespace cps2recomp
