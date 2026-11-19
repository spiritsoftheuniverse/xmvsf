// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: the hidden characters, Dr. Doom and Thanos, chosen with no code (the Game page's "Hidden
// characters"): Start held as a player chooses a character gives Dr. Doom with a punch and Thanos with a kick. The
// choice is the game's own (a button pressed on the cursor's character takes it, 008F1C), with the character changed
// as the game takes it.
#include "msh.h"

namespace cps2recomp {

namespace {
    using namespace msh;

    bool g_start[2] = {false, false}; // each player's Start held this frame
    bool g_kick[2] = {false, false};  // ... and a kick

    // 008F1C: a player's choosing record (A4) takes the character under its cursor (the record's word +0C, which the
    // cursor's moves have put in the player's record too: 008FA8)
    bool Chosen(M68K &c) {
        const uint32_t record = c.a[4] & 0xFFFFFF;
        if (record != kChoosing && record != kChoosing + kChoosingSize) {
            return false;
        }
        const int n = record == kChoosing ? 0 : 1;
        if (FeatureOn("hidden_characters") && g_start[n]) {
            const uint16_t who = g_kick[n] ? kThanos : kDrDoom;
            SetRam16(record + kCursor, who);
            SetRam16(Player(n) + kCharacter, who);
        }
        return false;
    }
} // namespace

void ChoosingBeforeFrame(Machine &machine) {
    for (int n = 0; n < 2; n++) {
        g_start[n] = (machine.players[n] & kStart) != 0;
        g_kick[n] = (machine.players[n] & (kKick1 | kKick2 | kKick3)) != 0;
    }
}

void RegisterChoosing() {
    AddHook(HookAt(0x008F1C), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden characters with no code: hold Start as you choose a character, and press a punch for "
                                  "Dr. Doom or a kick for Thanos."});
}

} // namespace cps2recomp
