// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: free play (an optional feature: the launcher's Features page and the in-game menu switch it).
#include "sfa3.h"

#include "state.h"

namespace cps2recomp {

namespace {
    using namespace sfa3;

    bool g_forced = false; // free play is on because of the feature, not the operator's settings
} // namespace

// The game has free play of its own, a coin mode of its test menu (sfa3.h). The feature leaves the operator's settings
// alone and sets what the board's start sets for that mode; from then on Start lets a player in without a coin and the
// screen says "FREE PLAY" where it would count credits.
void CoinsFrame(Machine &) {
    if (FeatureOn("free_play")) {
        if (!Ram8(kFreePlay)) {
            SetRam8(kFreePlay, 1);
            SetRam16(kCoinsPerCredit1, kFreePlayCoins);
            SetRam16(kCoinsPerCredit2, kFreePlayCoins);
            g_forced = true;
        }
    } else if (g_forced) {
        // Switched off: Start needs a coin again (the coin words are put right by the game at its next reset)
        SetRam8(kFreePlay, Ram8(kCoinMode) == kCoinFreePlay);
        g_forced = false;
    }
}

void CoinsState(Archive &ar) {
    ar.Mark(0x434F494E); // COIN
    ar.Pod(g_forced);
}

void RegisterCoins() {
    TheGame().features.push_back({"free_play", "Game", "Free play", "Start works without a coin.", kFeatureEnhanced});
}

} // namespace cps2recomp
