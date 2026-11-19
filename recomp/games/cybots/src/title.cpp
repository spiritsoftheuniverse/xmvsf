// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: "Straight to the title" (the Game page), as in the other games. When the board starts (switched on,
// or reset from the menu), everything before the title is run unseen and unheard, as fast as it goes: the board's
// memory test, the warning, the notices and the introduction. The picture comes back with the title; nothing of the
// game is changed, only not shown.
//
// The game's step (cybots.h, kMode) is 0 while the board starts, then 4 to 14 (its start: the memory test, the warning,
// the QSound notice), 0 again (frame 453) and into the loop of its show: 4 on, 1C the introduction, whose own step
// (kStep) is 68 for the title (frame 4014), then 20 and on (the rankings, another fight). With a credit in, the show
// goes to the title that asks for Start. Only the board's start begins it (Machine::frame is 0 again at a reset).
#include "alternative.h"
#include "cybots.h"

namespace cps2recomp {

namespace {
    using namespace cybots;

    constexpr int kSkipMost = 8000; // frames after which the skipping is given up

    bool g_skipping = false;
    int g_booted = 0;    // 1: the board's start reached its last step; 2: and the show's loop began
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
    // (only the board's first frame: a game must never be run unseen. A reset while it skips starts it again)
    if (machine.frame <= 1 && mode == 0) {
        g_skipping = FeatureOn("skip_intro") && !modes::AttractMode(); // (attract mode is the show itself)
        g_booted = 0;
        g_frames = 0;
    }
    if (g_skipping && !FeatureOn("skip_intro")) {
        g_skipping = false; // (switched off while it skips: the picture comes back at once)
    }
    if (g_skipping) {
        if (g_booted == 0 && mode == kBootLast) {
            g_booted = 1;
        } else if (g_booted == 1 && mode == 0) {
            g_booted = 2;
        }
        const bool paid = Ram8(kCredits) != 0;
        const bool title = g_booted == 2 && ((mode == kShowTitle && Ram16(kStep) >= kShowTitleStep) || mode > kShowTitle || paid);
        if (title || ++g_frames > kSkipMost) {
            g_skipping = false;
        }
    }
    Fast(g_skipping);
}

void RegisterTitle() {
    TheGame().features.push_back({"skip_intro", "Game", "Straight to the title",
                                  "When the game starts, and when it is reset, goes straight to the title: the board's memory test, "
                                  "the notices and the introduction are run unseen, in a second or so.",
                                  0});
}

} // namespace cps2recomp
