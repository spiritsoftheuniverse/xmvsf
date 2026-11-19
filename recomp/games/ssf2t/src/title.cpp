// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: "Straight to the title" (the Game page), as in the other games. When the board
// starts (switched on, or reset from the menu), everything before the title is run unseen and unheard, as fast as it
// goes: the board's memory test, the warning and the introduction. The picture comes back with the title; nothing of
// the game is changed, only not shown.
//
// The game's step (ssf2t.h, kMode) goes 2 to 10 in twos while the board starts (the warning at 4 and 6, frames 108-
// 302), then 0 (frame 805) and into the loop of its show: 2 the introduction, whose own step (kGameStep) is 16 when the
// title comes in from white (frame 2229: "SUPER", then the rest of it), 4 to 16 the notices, a demonstration fight
// and the rankings. With a coin in it goes from the introduction to 4 instead: the title that asks for Start. The
// board's start and its show share their numbers, so the start's last step (10) is waited for first. Only the board's
// start begins it (Machine::frame is 0 again at a reset): the step is 0 for a few frames between the screens of a
// game too.
#include "alternative.h"
#include "ssf2t.h"

namespace cps2recomp {

namespace {
    using namespace ssf2t;

    constexpr uint16_t kBooted = family::kBooted, kIntroduction = family::kIntro, kTitle = family::kTitleStep; // (family.h)
    constexpr int kSkipMost = 6000; // frames after which the skipping is given up

    bool g_skipping = false;
    bool g_booted = false; // the board's own start (the warning) is over
    bool g_fast = false;   // this file asked for the game to run unseen
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
        g_booted = false;
        g_frames = 0;
    }
    // (the feature switched off while it skips: the picture comes back at once)
    if (g_skipping && !FeatureOn("skip_intro")) {
        g_skipping = false;
    }
    if (g_skipping) {
        const bool wasBooted = g_booted;
        g_booted = g_booted || mode == kBooted;
        const bool title = wasBooted && mode == kIntroduction && Ram16(kGameStep) >= kTitle;
        if (title || (wasBooted && mode > kIntroduction && mode != kBooted) || ++g_frames > kSkipMost) {
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
