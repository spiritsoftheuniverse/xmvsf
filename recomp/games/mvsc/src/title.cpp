// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel vs. Capcom: "Straight to the title" (the Game page). When the board starts (switched on, or reset
// from the menu), everything before the title is run unseen and unheard, as fast as it goes: the board's memory
// test, the warning, the notices and the introduction. The picture comes back with the title; nothing of the game is
// changed, only not shown.
//
// This game's show has no step word to read (it runs as one of the game's tasks, its steps in the task's own record):
// the show placing the title's picture (mvsc.h, kShowStep; frame 1,420 from switching on) ends the skipping, as does
// a credit (the game then shows the title that asks for Start). Only the board's start begins it (Machine::frame is 0
// again at a reset).
#include "alternative.h"
#include "mvsc.h"

namespace cps2recomp {

void AttractTitle(); // modes.cpp

namespace {
    using namespace mvsc;

    constexpr int kSkipMost = 8000; // frames after which the skipping is given up

    bool g_skipping = false;
    bool g_booted = false; // the board has read its settings (the memory test is over)
    bool g_title = false;  // the show has placed the title
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
    // (only the board's first frame: a game must never be run unseen. A reset while it skips starts it again)
    if (machine.frame <= 1) {
        g_skipping = FeatureOn("skip_intro") && !modes::AttractMode(); // (attract mode is the show itself)
        g_booted = g_title = false;
        g_frames = 0;
    }
    if (g_skipping && !FeatureOn("skip_intro")) {
        g_skipping = false; // (switched off while it skips: the picture comes back at once)
    }
    if (g_skipping && (g_title || (g_booted && Ram8(kCredits) != 0) || ++g_frames > kSkipMost)) {
        g_skipping = false;
    }
    Fast(g_skipping);
}

// The board has read its settings (board.cpp's hook, where the board's start reads them)
void TitleBooted() {
    g_booted = true;
}

void RegisterTitle() {
    AddHook(HookAt(0x0D1302), "ShowTitle", [](M68K &) {
        g_title = true;
        AttractTitle();
        return false;
    });
    TheGame().features.push_back({"skip_intro", "Game", "Straight to the title",
                                  "When the game starts, and when it is reset, goes straight to the title: the board's memory test, "
                                  "the notices and the introduction are run unseen, in a second or so.",
                                  0});
}

} // namespace cps2recomp
