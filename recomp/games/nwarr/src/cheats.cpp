// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: cheats (optional features: the launcher's Features page and the in-game menu switch them). Made-up
// play needs them to go up the ladder to its end ('make explore', game.cfg's explore features). As Pocket Fighter's.
#include "nwarr.h"
#include "session.h"

namespace cps2recomp {

namespace {
    using namespace nwarr;

    // Which player's record an address is (0, 1), or -1
    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // Every place the game takes health off a player, in family.h's slots (Night Warriors has 35, lst.py -r 'sub.w
    // d4,0x42(aN)|subq.w #1,0x42(aN)'): a hit's (0C1384, the one most hits go through), a throw's and a character's own
    // moves' (each in its own routine), the thrown player's (A6), and the slow drain of damage still to come (02C184:
    // the word at +188 taken a point at a time); each with the register the player is in, 0x10 more for a subq of one.
    // Health below nothing after it is a knockout (0C1388), so one-hit knockouts take off more than a full bar; with no
    // knockouts (a training session, the modes that want it) a hit takes no more than the health there is.
    struct Site {
        uint32_t address;
        uint8_t code;
    };
    constexpr Site kSites[] = {
        {HookAt(family::kDamage00), family::kDamageRegs[0]}, {HookAt(family::kDamage01), family::kDamageRegs[1]}, {HookAt(family::kDamage02), family::kDamageRegs[2]},
        {HookAt(family::kDamage03), family::kDamageRegs[3]}, {HookAt(family::kDamage04), family::kDamageRegs[4]}, {HookAt(family::kDamage05), family::kDamageRegs[5]},
        {HookAt(family::kDamage06), family::kDamageRegs[6]}, {HookAt(family::kDamage07), family::kDamageRegs[7]}, {HookAt(family::kDamage08), family::kDamageRegs[8]},
        {HookAt(family::kDamage09), family::kDamageRegs[9]}, {HookAt(family::kDamage10), family::kDamageRegs[10]}, {HookAt(family::kDamage11), family::kDamageRegs[11]},
        {HookAt(family::kDamage12), family::kDamageRegs[12]}, {HookAt(family::kDamage13), family::kDamageRegs[13]}, {HookAt(family::kDamage14), family::kDamageRegs[14]},
        {HookAt(family::kDamage15), family::kDamageRegs[15]}, {HookAt(family::kDamage16), family::kDamageRegs[16]}, {HookAt(family::kDamage17), family::kDamageRegs[17]},
        {HookAt(family::kDamage18), family::kDamageRegs[18]}, {HookAt(family::kDamage19), family::kDamageRegs[19]}, {HookAt(family::kDamage20), family::kDamageRegs[20]},
        {HookAt(family::kDamage21), family::kDamageRegs[21]}, {HookAt(family::kDamage22), family::kDamageRegs[22]}, {HookAt(family::kDamage23), family::kDamageRegs[23]},
        {HookAt(family::kDamage24), family::kDamageRegs[24]}, {HookAt(family::kDamage25), family::kDamageRegs[25]}, {HookAt(family::kDamage26), family::kDamageRegs[26]},
        {HookAt(family::kDamage27), family::kDamageRegs[27]}, {HookAt(family::kDamage28), family::kDamageRegs[28]}, {HookAt(family::kDamage29), family::kDamageRegs[29]},
        {HookAt(family::kDamage30), family::kDamageRegs[30]}, {HookAt(family::kDamage31), family::kDamageRegs[31]}, {HookAt(family::kDamage32), family::kDamageRegs[32]},
        {HookAt(family::kDamage33), family::kDamageRegs[33]}, {HookAt(family::kDamage34), family::kDamageRegs[34]}, {HookAt(family::kDamage35), family::kDamageRegs[35]},
        {HookAt(family::kDamage36), family::kDamageRegs[36]}, {HookAt(family::kDamage37), family::kDamageRegs[37]}, {HookAt(family::kDamage38), family::kDamageRegs[38]},
        {HookAt(family::kDamage39), family::kDamageRegs[39]},
    };

    bool Damage(M68K &c) {
        const Site *found = nullptr;
        for (const Site &site : kSites) {
            found = site.address && site.address == c.pc ? &site : found;
        }
        if (!found) {
            return false;
        }
        const int reg = found->code & 7;
        const bool one = (found->code & 0x10) != 0;
        const int player = PlayerOf(c.a[reg]);
        if (player < 0) {
            return false;
        }
        const uint32_t health = Player(player) + kHealth;
        const bool kept = FeatureOn(player == 0 ? "no_damage" : "no_damage_2");
        if (one) {
            // (the drain: the point is given back first, to the bar's word beside it too)
            if (kept || (session::NoKnockOut() && static_cast<int16_t>(Ram16(health)) <= 1)) {
                SetRam16(health, static_cast<uint16_t>(Ram16(health) + 1));
                SetRam16(health + 2, static_cast<uint16_t>(Ram16(health + 2) + 1));
            }
            return false;
        }
        if (c.d[4] & 0xFFFF) {
            session::Hit(player, c.d[4] & 0xFFFF);
        }
        if (kept) {
            SetLow16(c.d[4], 0);
        } else if (player == 1 && FeatureOn("one_hit")) {
            SetLow16(c.d[4], kHealthFull + 1);
        }
        const int16_t left = static_cast<int16_t>(Ram16(health));
        if (session::NoKnockOut() && static_cast<int16_t>(c.d[4] & 0xFFFF) > left) {
            SetLow16(c.d[4], static_cast<uint32_t>(left > 0 ? left : 0));
        }
        return false;
    }
} // namespace

void RegisterCheats() {
    for (const Site &site : kSites) {
        if (site.address) {
            AddHook(site.address, "Damage", Damage);
        }
    }
    // 008798: subq.b #1 off the frames left of the round's second; at nothing a second comes off the clock (0087AC)
    // and the frames start again. With the cheat the frame is given back first, so the clock stands still.
    AddHook(HookAt(At(0x008798)), "ClockFrame", [](M68K &) {
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
