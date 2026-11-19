// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel vs. Capcom: the hidden characters chosen with no code (the Game page's "Hidden characters"): Start held as a
// player chooses a character gives its hidden other self, where it has one (War Machine Gold War Machine, Hulk Orange
// Hulk, Venom Red Venom, Chun-Li Shadow Lady, Morrigan Lilith, Mega Man Roll). The choice is the game's own (a button
// pressed on the cursor's character takes it, 00BCDA), with the character changed as the game takes it.
#include "mvsc.h"

namespace cps2recomp {

namespace {
    using namespace mvsc;

    constexpr uint32_t kCursor = 0x0C; // word of the choosing record: the character under the player's cursor
    bool g_start[2] = {false, false};  // each player's Start held this frame

    uint16_t Hidden(uint16_t who) {
        switch (who) {
        case kWarMachine: return kGoldWarMachine;
        case kHulk: return kOrangeHulk;
        case kVenom: return kRedVenom;
        case kChunLi: return kShadowLady;
        case kMorrigan: return kLilith;
        case kMegaMan: return kRoll;
        default: return who;
        }
    }

    // 00BCDA: a player's choosing record (A4) takes the character under its cursor
    bool Chosen(M68K &c) {
        const uint32_t record = c.a[4] & 0xFFFFFF;
        if (record != kChoosing && record != kChoosing + kChoosingSize) {
            return false;
        }
        const int n = record == kChoosing ? 0 : 1;
        if (FeatureOn("hidden_characters") && g_start[n]) {
            SetRam16(record + kCursor, Hidden(Ram16(record + kCursor)));
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
    AddHook(HookAt(0x00BCDA), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden characters with no code: hold Start as you choose War Machine for Gold War "
                                  "Machine, Hulk for Orange Hulk, Venom for Red Venom, Chun-Li for Shadow Lady, Morrigan for "
                                  "Lilith, or Mega Man for Roll."});
}

} // namespace cps2recomp
