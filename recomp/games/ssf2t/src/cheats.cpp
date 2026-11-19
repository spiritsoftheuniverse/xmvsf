// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: cheats (optional features: the launcher's Features page and the in-game menu switch
// them). Made-up play needs them to go up the ladder to its end ('make explore', game.cfg's explore features).
#include "battle.h"
#include "session.h"
#include "ssf2t.h"

namespace cps2recomp {

void TagKnockDown(uint32_t player); // livetag.cpp

namespace {
    using namespace ssf2t;

    // Which player's record an address is (0, 1), or -1
    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // `damage` (D6, D4 or D0, its low word) is about to come off the health of the player whose record is `victim`
    // (anything else's is left alone). The training session is told of it. Health below nothing after it is a
    // knockout, so one-hit knockouts take off more than a full bar; with no knockouts (a training session, the modes
    // that want it) a hit takes no more than the health there is.
    bool DamageTo(uint32_t &damage, uint32_t victim) {
        const int player = PlayerOf(victim);
        if (player < 0) {
            return false;
        }
        if (damage & 0xFFFF) {
            session::Hit(player, damage & 0xFFFF, 0, false);
        }
        if (FeatureOn(player == 0 ? "no_damage" : "no_damage_2")) {
            SetLow16(damage, 0);
        } else if (player == 1 && FeatureOn("one_hit")) {
            SetLow16(damage, kHealthFull + 1);
        }
        const int16_t health = static_cast<int16_t>(Ram16(Player(player) + kHealth));
        if ((session::NoKnockOut() || TeamHoldsKnockOut(player)) && static_cast<int16_t>(damage & 0xFFFF) > health) {
            SetLow16(damage, static_cast<uint32_t>(health > 0 ? health : 0));
        }
        // (the team battle's tag style: a bar emptied with a partner left is a knock-down, not a knock-out)
        if (TeamHoldsKnockOut(player) && static_cast<int16_t>(damage & 0xFFFF) >= health && !Ram8(Player(player) + kGuarded)) {
            TagKnockDown(Player(player));
        }
        return false;
    }
} // namespace

void RegisterCheats() {
    // A hit's (D6 off A2's health), a thrown thing's, and a throw's: D4 off A3's at once (two ways), or put by at +78
    // (004A58, which sees to the knockout itself) to be taken at the throw's end (004C98, 07AB6E, 07ACEA)
    AddHook(kHitDamage, "HitDamage", [](M68K &c) { return DamageTo(c.d[6], c.a[2]); });
    AddHook(kThrownDamage, "ThrownDamage", [](M68K &c) { return DamageTo(c.d[6], c.a[2]); });
    for (const uint32_t at : kThrowDamage) {
        AddHook(at, "ThrowDamage", [](M68K &c) { return DamageTo(c.d[4], c.a[3]); });
    }
    // 00FE74: subq.b #1 off the frames of the round's second (at nothing a second goes). With the cheat the frame is
    // given back first, so the clock stands still.
    AddHook(kClockTick, "ClockTick", [](M68K &) {
        if (FeatureOn("infinite_time") || session::NoClock()) {
            SetRam8(kClockFrames, static_cast<uint8_t>(Ram8(kClockFrames) + 1));
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
