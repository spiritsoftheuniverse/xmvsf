// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: cheats (optional features: the launcher's Features page and the in-game menu switch
// them). Made-up play needs them to go up the ladder to its end ('make explore', game.cfg's explore features).
#include "session.h"
#include "mshvsf.h"

namespace cps2recomp {

namespace {
    using namespace mshvsf;

    // 018536 and 018BB2: sub.w d5,0x250(a6), a hit's damage (D5) off the health of the character hit (A6), either of
    // a side's two. Health at nothing or below after it is a knockout (the partner then comes in, or the round is
    // lost), so one-hit knockouts take off more than a full bar; with no knockouts (a training session) a hit takes
    // no more than leaves one point.
    bool Damage(M68K &c) {
        const int side = SideOf(c.a[6]);
        if (side < 0) {
            return false;
        }
        const uint32_t record = c.a[6] & 0xFFFFFF;
        if ((c.d[5] & 0xFFFF) && record == Player(side)) {
            // (what comes back: the part of the bar left red, 0199A0, which the hit takes before this)
            session::Hit(side, c.d[5] & 0xFFFF);
        }
        if (FeatureOn(side == 0 ? "no_damage" : "no_damage_2")) {
            SetLow16(c.d[5], 0);
        } else if (side == 1 && FeatureOn("one_hit")) {
            SetLow16(c.d[5], kHealthFull + 1);
        }
        const int16_t health = static_cast<int16_t>(Ram16(record + kHealth));
        if (session::NoKnockOut() && static_cast<int16_t>(c.d[5] & 0xFFFF) >= health) {
            SetLow16(c.d[5], static_cast<uint32_t>(health > 1 ? health - 1 : 0));
        }
        return false;
    }
} // namespace

void RegisterCheats() {
    AddHook(0x018536, "Damage", Damage);
    AddHook(0x018BB2, "DamageOther", Damage);
    // 010E38: the frames left of the round's second, taken one off (at nothing a second comes off the clock). With
    // the cheat the frame is given back first, so the clock stands still.
    AddHook(0x010E38, "ClockFrame", [](M68K &) {
        if (FeatureOn("infinite_time") || session::NoClock()) {
            SetRam8(kClockFrames, static_cast<uint8_t>(Ram8(kClockFrames) + 1));
        }
        return false;
    });
    Game &game = TheGame();
    game.features.push_back({"no_damage", "Cheats", "No damage", "Player 1's characters' health does not go down."});
    game.features.push_back({"no_damage_2", "Cheats", "No damage, player 2",
                             "Player 2's characters' health does not go down (the opponent's, in a one-player game)."});
    game.features.push_back({"one_hit", "Cheats", "One-hit knockouts", "Player 2's characters (the opponent's, in a one-player game) go down with the first hit."});
    game.features.push_back({"infinite_time", "Cheats", "Infinite time", "The round's clock stands still."});
}

} // namespace cps2recomp
