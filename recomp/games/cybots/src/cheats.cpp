// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: cheats (optional features: the launcher's Features page and the in-game menu switch them). Made-up play
// needs them to go up the ladder to its end ('make explore', game.cfg's explore features). As Night Warriors'.
#include "cybots.h"
#include "session.h"

namespace cps2recomp {

namespace {
    using namespace cybots;

    // Which player's record an address is (0, 1), or -1
    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // Every place health is taken off (cybots.h, kDamageSites): the damage's register made 0 for no damage, more than
    // a full bar for one-hit knockouts; with no knockouts (a training session, the modes that want it) a hit takes no
    // more than the health there is.
    bool Damage(M68K &c) {
        const DamageSite *found = nullptr;
        for (const DamageSite &site : kDamageSites) {
            found = site.address == c.pc ? &site : found;
        }
        if (!found) {
            return false;
        }
        const int player = PlayerOf(c.a[found->victim]);
        if (player < 0) {
            return false;
        }
        uint32_t &damage = c.d[found->damage];
        if (damage & 0xFFFF) {
            session::Hit(player, damage & 0xFFFF);
        }
        if (FeatureOn(player == 0 ? "no_damage" : "no_damage_2")) {
            SetLow16(damage, 0);
        } else if (player == 1 && FeatureOn("one_hit")) {
            SetLow16(damage, kHealthFull + 1);
        }
        const int16_t health = static_cast<int16_t>(Ram16(Player(player) + kHealth));
        if (session::NoKnockOut() && static_cast<int16_t>(damage & 0xFFFF) > health) {
            SetLow16(damage, static_cast<uint32_t>(health > 0 ? health : 0));
        }
        return false;
    }
} // namespace

void RegisterCheats() {
    for (const DamageSite &site : kDamageSites) {
        AddHook(site.address, "Damage", Damage);
    }
    // 00EFCE: sub.b d0,0x6BA1(a5), the timer speed's step off the fraction of the round's second; under nothing a
    // second comes off the clock. With the cheat nothing is taken, so the clock stands still.
    AddHook(HookAt(kClockStep), "ClockFraction", [](M68K &c) {
        if (FeatureOn("infinite_time") || session::NoClock()) {
            SetLow8(c.d[0], 0);
        }
        return false;
    });
    Game &game = TheGame();
    game.features.push_back({"no_damage", "Cheats", "No damage", "Player 1's health does not go down."});
    game.features.push_back({"no_damage_2", "Cheats", "No damage, player 2", "Player 2's health does not go down (the opponent's, in a one-player game)."});
    game.features.push_back({"one_hit", "Cheats", "One-hit knockouts", "Player 2 (the opponent, in a one-player game) goes down with the first hit."});
    game.features.push_back({"infinite_time", "Cheats", "Infinite time", "The round's clock stands still."});
}

} // namespace cps2recomp
