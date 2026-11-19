// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: free play (an optional feature: the launcher's Features page and the in-game menu
// switch it).
#include "hsf2.h"

namespace cps2recomp {

namespace {
    using namespace hsf2;

    bool forced = false; // free play is on because of the feature, not the operator's settings
} // namespace

// The game has free play of its own, a coin mode its test menu never offers (hsf2.h). The feature leaves the
// operator's settings alone and sets what the board's start sets for that mode; from then on Start lets a player in
// without a coin and the screen says "FREE PLAY" where it would count credits.
void CoinsFrame(Machine &) {
    if (FeatureOn("free_play")) {
        if (!Ram8(kFreePlay)) {
            SetRam8(kFreePlay, 1);
            SetRam16(kCoinsPerCredit1, kFreePlayCoins);
            SetRam16(kCoinsPerCredit2, kFreePlayCoins);
            forced = true;
        }
    } else if (forced) {
        // Switched off: Start needs a coin again (the coin words are put right by the game at its next reset)
        SetRam8(kFreePlay, Ram8(kCoinMode) == kCoinFreePlay);
        forced = false;
    }
}

void RegisterCoins() {
    TheGame().features.push_back({"free_play", "Game", "Free play", "Start works without a coin.", kFeatureEnhanced});
}

} // namespace cps2recomp
