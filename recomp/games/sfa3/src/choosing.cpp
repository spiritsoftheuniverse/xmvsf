// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: the hidden characters, Balrog, Juni and Juli, chosen with no code (the Game page's "Hidden
// characters"). The character choice's grid (8 by 8 at 01E724, the cursor's column and row at the player's +10 and +14:
// 01E6BE) has 25 faces and four cells that choose at random (bit 5 of the cell; bits 6 and 7 the players who may not
// use one); the three are on no cell. The board gives them by its "time release" (A5+EE, kept in the settings memory
// and stepped by its play time: 001604 on): from level 1 the random cells can give them too (the second half of the table
// at 01E8C4). Here Start held as a player chooses M. Bison gives Balrog, Cammy Juni and Rose Juli. The choice is the
// game's own (01E764: the button that takes the cursor's character), with the character changed before the game takes
// it.
#include "sfa3.h"

namespace cps2recomp {

namespace {
    using namespace sfa3;

    constexpr uint8_t kBison = 0x0A, kRose = 0x09, kCammy = 0x16, kBalrog = 0x15, kJuli = 0x1E, kJuni = 0x1F;
    constexpr uint32_t kRandomCell = 0x3F; // a player's byte on the character choice: set on one of the random cells
    bool g_start[2] = {false, false};      // each player's Start held this frame

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // 01E764: the player at A6 has chosen the cursor's character (+102), which the game takes from here
    bool Chosen(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n < 0 || !FeatureOn("hidden_characters") || !g_start[n] || Ram8(player + kRandomCell)) {
            return false;
        }
        const uint8_t who = Ram8(player + kCharacter);
        if (who == kBison || who == kCammy || who == kRose) {
            SetRam8(player + kCharacter, who == kBison ? kBalrog : who == kCammy ? kJuni : kJuli);
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
    AddHook(HookAt(0x01E764), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The three hidden characters with no code: hold Start as you choose M. Bison for Balrog, Cammy for "
                                  "Juni, or Rose for Juli."});
}

} // namespace cps2recomp
