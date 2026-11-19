// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: free play (an optional feature: the launcher's Features page and the in-game menu switch it).
#include "alternative.h"
#include "nwarr.h"

namespace cps2recomp {

// The board has no free play of its own (nwarr.h, kCredits), so the game is kept in credits, as a coin put in would:
// it goes to the title and waits for Start, and Start lets a player in. Two, so that a second player can always join.
// (Not in attract mode, whose show runs with no coin in.) As Street Fighter Alpha's.
void CoinsFrame(Machine &) {
    if (FeatureOn("free_play") && !modes::AttractMode() && Ram8(nwarr::kCredits) < 2) {
        SetRam8(nwarr::kCredits, 2);
    }
}

void RegisterCoins() {
    TheGame().features.push_back({"free_play", "Game", "Free play", "Start works without a coin.", kFeatureEnhanced});
}

} // namespace cps2recomp
