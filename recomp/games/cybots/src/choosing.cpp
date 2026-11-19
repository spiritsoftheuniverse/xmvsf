// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the hidden machines, Super-8, Gaits, Warlock and Killer Bee (the Game page's "Hidden characters"), chosen
// with no code: Start held as a player chooses a machine gives the hidden one in the same place of the list (the first
// of the four the pilot is offered gives Super-8, the second Gaits, the third Warlock, the fourth Killer Bee). The
// choice is the game's own (00CE0C: a button on the cursor's machine; +388 the cursor, 0-3); the machine is changed
// where the game makes the player from its choice (002188, modes.cpp).
#include "cybots.h"
#include "state.h"

namespace cps2recomp {

namespace {
    using namespace cybots;

    constexpr uint8_t kFirstHidden = 0x0C; // Super-8; then Gaits, Warlock, Killer Bee
    constexpr uint32_t kCursor = 0x388;
    // Each player's Start as it chose (held on the last frame before its choice was made), kept with the game's state
    bool g_start[2] = {false, false};
} // namespace

void ChoosingBeforeFrame(Machine &machine) {
    for (int n = 0; n < 2; n++) {
        if (!Ram8(Player(n) + kChosen)) {
            g_start[n] = (machine.players[n] & kStart) != 0;
        }
    }
}

void ChoosingState(Archive &ar) {
    ar.Mark(0x48494445); // HIDE
    ar.Pod(g_start);
}

// The player's machine as chosen, changed for the hidden one when Start was held (modes.cpp's PlayerMade, at 002188)
void HiddenMachine(int n) {
    if (FeatureOn("hidden_characters") && g_start[n]) {
        SetRam8(Player(n) + kMachineChoice, static_cast<uint8_t>(kFirstHidden + (Ram8(Player(n) + kCursor) & 3)));
    }
}

void RegisterChoosing() {
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The four hidden machines with no code: hold Start as you choose a machine for the hidden one in "
                                  "its place (first: Super-8, second: Gaits, third: Warlock, fourth: Killer Bee)."});
}

} // namespace cps2recomp
