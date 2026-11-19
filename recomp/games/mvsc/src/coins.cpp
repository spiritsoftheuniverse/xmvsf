// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel vs. Capcom: free play (an optional feature: the launcher's Features page and the in-game menu switch
// it).
#include "mvsc.h"

namespace cps2recomp {

namespace {
    using namespace mvsc;

    bool forced = false; // free play is on because of the feature, not the operator's settings
} // namespace

// The game has free play of its own: one of the operator's coin modes (mvsc.h). The feature leaves the operator's
// settings alone and sets what the game sets for that mode; from then on Start lets a player in without a coin and
// the screen says "FREE" where it would count credits.
void CoinsFrame(Machine &) {
    if (FeatureOn("free_play")) {
        if (!Ram8(kFreePlay)) {
            SetRam8(kFreePlay, 1);
            forced = true;
        }
    } else if (forced) {
        SetRam8(kFreePlay, Ram8(kCoinMode) == kCoinFreePlay);
        forced = false;
    }
}

void RegisterCoins() {
    TheGame().features.push_back({"free_play", "Game", "Free play", "Start works without a coin.", kFeatureEnhanced});
}

} // namespace cps2recomp
