// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: the hidden characters chosen with no code (the Game page's "Hidden
// characters"): Start held as a player chooses a character gives its hidden other self, where it has one (Captain
// America U.S. Agent, Spider-Man Armored Spider-Man, Blackheart Mephisto, Zangief Mech-Zangief, Sakura Dark Sakura,
// Dan Shadow, Akuma Cyber-Akuma). The choice is the game's own (a button pressed on the cursor's character takes it,
// 00A66E), with the character changed as the game takes it.
#include "mshvsf.h"

namespace cps2recomp {

namespace {
    using namespace mshvsf;

    constexpr uint32_t kCursor = 0x0C; // word of the choosing record: the character under the player's cursor
    bool g_start[2] = {false, false};  // each player's Start held this frame

    uint16_t Hidden(uint16_t who) {
        switch (who) {
        case kCaptainAmerica: return kUSAgent;
        case kSpiderMan: return kArmoredSpiderMan;
        case kBlackheart: return kMephisto;
        case kZangief: return kMechZangief;
        case kSakura: return kDarkSakura;
        case kDan: return kShadow;
        case kAkuma: return kCyberAkuma;
        default: return who;
        }
    }

    // 00A66E: a player's choosing record (A4) takes the character under its cursor
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
    AddHook(HookAt(0x00A66E), "Chosen", Chosen);
    TheGame().features.push_back({"hidden_characters", "Game", "Hidden characters",
                                  "The hidden characters with no code: hold Start as you choose Captain America for U.S. Agent, "
                                  "Spider-Man for Armored Spider-Man, Blackheart for Mephisto, Zangief for Mech-Zangief, Sakura for "
                                  "Dark Sakura, Dan for Shadow, or Akuma for Cyber-Akuma, the last opponent."});
}

} // namespace cps2recomp
