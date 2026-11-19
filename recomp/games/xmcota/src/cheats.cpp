// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: cheats (optional features: the launcher's Features page and the in-game menu switch
// them). Made-up play needs them to go up the ladder to its end ('make explore', game.cfg's explore features).
#include "session.h"
#include "xmcota.h"

namespace cps2recomp {

namespace {
    using namespace xmcota;

    // 00D36C and 00D8AE: sub.w d5,0x190(a6), a hit's damage (D5) off the health of the player hit (A6). Health at
    // nothing or below after it is a knockout, so one-hit knockouts take off more than a full bar; with no knockouts
    // (a training session) a hit takes no more than leaves one point.
    bool Damage(M68K &c) {
        const int side = SideOf(c.a[6]);
        if (side < 0) {
            return false;
        }
        const uint32_t record = c.a[6] & 0xFFFFFF;
        if (c.d[5] & 0xFFFF) {
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

    // 00E28A: add.w d0,0x190(a6) with D0 below nothing, health a move costs its player (A6): none with no damage, and
    // never the last point with no knockouts
    bool Cost(M68K &c) {
        const int side = SideOf(c.a[6]);
        if (side < 0) {
            return false;
        }
        const int16_t health = static_cast<int16_t>(Ram16((c.a[6] & 0xFFFFFF) + kHealth));
        if (FeatureOn(side == 0 ? "no_damage" : "no_damage_2")) {
            SetLow16(c.d[0], 0);
        } else if (session::NoKnockOut() && -static_cast<int16_t>(c.d[0] & 0xFFFF) >= health) {
            SetLow16(c.d[0], static_cast<uint32_t>(health > 1 ? 1 - health : 0));
        }
        return false;
    }
} // namespace

void RegisterCheats() {
    AddHook(0x00D36C, "Damage", Damage);
    AddHook(0x00D8AE, "DamageOther", Damage);
    AddHook(0x00E28A, "HealthCost", Cost);
    // 06CBB4: the frames left of the round's second, read to be taken one off (at nothing a second comes off the
    // clock). With the cheat the frame is given back first, so the clock stands still.
    AddHook(0x06CBB4, "ClockFrame", [](M68K &) {
        if (FeatureOn("infinite_time") || session::NoClock()) {
            SetRam8(kClockFrames, static_cast<uint8_t>(Ram8(kClockFrames) + 1));
        }
        return false;
    });
    Game &game = TheGame();
    game.features.push_back({"no_damage", "Cheats", "No damage", "Player 1's health does not go down."});
    game.features.push_back({"no_damage_2", "Cheats", "No damage, player 2",
                             "Player 2's health does not go down (the opponent's, in a one-player game)."});
    game.features.push_back({"one_hit", "Cheats", "One-hit knockouts", "Player 2 (the opponent, in a one-player game) goes down with the first hit."});
    game.features.push_back({"infinite_time", "Cheats", "Infinite time", "The round's clock stands still."});
}

} // namespace cps2recomp
