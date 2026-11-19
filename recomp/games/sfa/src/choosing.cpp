// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the hidden characters, Akuma, M. Bison and Dan, chosen with no code (the Game page's "Hidden
// characters"): Start held as a player chooses Ryu gives Akuma, as it chooses Sagat M. Bison, and as it chooses Ken
// Dan. The choice is the game's own (0191AC: a punch or a kick newly pressed on the cursor's character, the button
// saying the colours), with the character changed before the game takes it.
#include "sfa.h"

namespace cps2recomp {

namespace {
    using namespace sfa;

    constexpr uint8_t kRyu = 0, kKen = 1, kAkuma = 2, kBison = 10, kSagat = 11, kDan = 12;
    constexpr uint32_t kHasChosen = 0x39;                 // a player's byte: not zero once it has chosen (0191EE)
    constexpr uint32_t kButtonsNow = 0xBA, kButtonsBefore = 0xBC; // bytes: the buttons held this frame and the last
    bool g_start[2] = {false, false}; // each player's Start held this frame

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // 0191AC: the player at A6 on the character choice, which chooses when a punch or a kick is newly pressed
    bool Choosing(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n < 0 || !FeatureOn("hidden_characters") || !g_start[n] || Ram8(player + kHasChosen)) {
            return false;
        }
        const uint8_t pressed = Ram8(player + kButtonsNow) & static_cast<uint8_t>(~Ram8(player + kButtonsBefore));
        if (!(pressed & 0x77)) {
            return false;
        }
        const uint8_t who = Ram8(player + kCharacter);
        if (who == kRyu || who == kKen || who == kSagat) {
            SetRam8(player + kCharacter, who == kRyu ? kAkuma : who == kKen ? kDan : kBison);
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
    AddHook(HookAt(0x0191AC), "Choosing", Choosing);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The three hidden characters with no code: hold Start as you choose Ryu for Akuma, Sagat for M. "
                                  "Bison, or Ken for Dan (the button still chooses the colours)."});
}

} // namespace cps2recomp
