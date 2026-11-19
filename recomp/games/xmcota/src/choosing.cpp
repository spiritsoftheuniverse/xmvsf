// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: the hidden characters, Juggernaut, Magneto and Akuma, chosen with no code (the Game page's
// "Hidden characters"): Start held as a player chooses a character gives Juggernaut with light punch, Magneto with medium
// punch and Akuma with hard punch. The choice is the game's own (a button pressed takes the character for the cursor's
// place, from the table at 050AF0), with the place changed to the hidden character's as the game reads it: the table has
// a place for each, after the ten the cursor reaches.
#include "xmcota.h"

namespace cps2recomp {

namespace {
    using namespace xmcota;

    int g_hidden[2] = {-1, -1}; // each player's hidden character asked for this frame (its place in the table), or -1

    // 05064E: a player's choosing (A6, its side in the byte +26) takes the character for its cursor's place (A3), into
    // the player's record (A2)
    bool Chosen(M68K &c) {
        const int n = Ram8((c.a[6] & 0xFFFFFF) + 0x26);
        const uint32_t place = c.a[3] & 0xFFFFFF;
        if ((n != 0 && n != 1) || place != kCursor + 2 * static_cast<uint32_t>(n)) {
            return false;
        }
        if (FeatureOn("hidden_characters") && g_hidden[n] >= 0) {
            SetRam8(place, static_cast<uint8_t>(g_hidden[n]));
        }
        return false;
    }
} // namespace

void ChoosingBeforeFrame(Machine &machine) {
    for (int n = 0; n < 2; n++) {
        const uint16_t held = machine.players[n];
        g_hidden[n] = !(held & kStart)    ? -1
                      : (held & kPunch1) ? kOrdinary
                      : (held & kPunch2) ? kOrdinary + 1
                      : (held & kPunch3) ? kOrdinary + 2
                                         : -1;
    }
}

void RegisterChoosing() {
    AddHook(HookAt(0x05064E), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The bosses and the hidden character with no code: hold Start as you choose a character, and press "
                                  "light punch for Juggernaut, medium punch for Magneto or hard punch for Akuma."});
}

} // namespace cps2recomp
