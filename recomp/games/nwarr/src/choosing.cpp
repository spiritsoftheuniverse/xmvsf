// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: the hidden characters, Huitzil and Pyron (the bosses, playable by a person: tried with the choice
// held at each), chosen with no code (the Game page's "Hidden characters"): Start held as a player chooses Victor gives
// Huitzil, as it chooses Demitri Pyron. The choice is the game's own (029012: a button pressed on the cursor's
// character, A6 the player), with the character changed where the game keeps it (kChoice, copied for the fight at
// 006934); the button still says the colours. As Pocket Fighter's.
#include "nwarr.h"

namespace cps2recomp {

namespace {
    using namespace nwarr;

    bool g_start[2] = {false, false}; // each player's Start held this frame

    bool Chosen(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
        if (n < 0 || !FeatureOn("hidden_characters") || !g_start[n]) {
            return false;
        }
        const uint8_t who = Ram8(player + kChoice);
        for (const auto &hidden : family::kHidden) {
            if (who == hidden[0]) {
                SetRam8(player + kChoice, hidden[1]);
                break;
            }
        }
        return false;
    }
} // namespace

void ChoosingBeforeFrame(Machine &machine) {
    for (int n = 0; n < 2; n++) {
        g_start[n] = (machine.players[n] & kStart) != 0;
    }
}

void RegisterChoosing() {
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters", nwarr::family::kHiddenWords});
    AddHook(HookAt(At(0x029012)), "Chosen", Chosen);
    AddHook(HookAt(At(0x029092)), "ChosenByStart", Chosen);
}

} // namespace cps2recomp
