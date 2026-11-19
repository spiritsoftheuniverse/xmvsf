// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: the hidden character with no code (the Game page's "Hidden characters"). The old versions
// are on the game's own menu here; what is left hidden is Akuma, the Super Turbo version's seventeenth character, whom
// the game gives for a code on the stick before the choice. With the feature, the three punches pressed at once as a
// Super Turbo character is chosen give Akuma.
//
// The choice is the game's own: a button newly pressed chooses the character and its colours (00C2E0 on: 00C364 where
// the ways meet, the player's +8C set), and the Super Turbo version then waits for the button to be let go (00C252: its
// count +8D; held through it the eighth set of colours), going on to 00C228 when it is. With the code's flag (+88) the
// wait instead looks for the three punches pressed at once within half a second and makes the player Akuma with his
// first colours (00C2CE: +32C 0, +32B 0x10). The feature does that itself, on the wait's first frame, when the three
// punches were down as the character was chosen.
#include "hsf2.h"

#include "state.h"

namespace cps2recomp {

namespace {
    using namespace hsf2;

    constexpr uint32_t kButtonsNow = 0x310; // word: the buttons held this frame (the punches 0x70: 00C2BA)
    constexpr uint16_t kPunches = 0x070;
    constexpr uint32_t kChose = 0x00C364, kWait = 0x00C280, kChoiceFinal = 0x00C228;
    bool g_punches[2] = {false, false}; // the three punches chose each player's character (kept with the game's state)

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // 00C364: a button has chosen the character of the player at A6
    bool Chose(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n >= 0) {
            g_punches[n] = (Ram16(player + kButtonsNow) & kPunches) == kPunches;
        }
        return false;
    }

    // 00C280: the Super Turbo version's wait for the button to be let go
    bool Wait(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n < 0 || !g_punches[n]) {
            return false;
        }
        g_punches[n] = false;
        if (!FeatureOn("hidden_characters") || Ram8(player + kVersion) != kSuperTurbo) {
            return false;
        }
        SetRam8(player + kColourChoice, 0);
        SetRam8(player + kCharacter, kAkuma);
        c.pc = kChoiceFinal;
        return true;
    }
} // namespace

void ChoosingState(Archive &ar) {
    ar.Mark(0x43484F4F); // CHOO
    ar.Pod(g_punches);
}

void RegisterChoosing() {
    AddHook(HookAt(kChose), "Chose", Chose);
    AddHook(HookAt(kWait), "ChoiceWait", Wait);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden character with no code: the three punches pressed at once as you choose a Super Turbo "
                                  "character give Akuma."});
}

} // namespace cps2recomp
