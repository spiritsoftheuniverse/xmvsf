// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: tag-team play for four people (fight/tag.h); this says where the game keeps the character on
// point and the choosing.
#include "alternative.h"
#include "battle.h"
#include "marvel.h"
#include "session.h"
#include "state.h"
#include "tag.h"
#include "mshvsf.h"

#include <algorithm>

namespace cps2recomp {

namespace {
    using namespace mshvsf;

    // A byte of each choosing record (kChoosing): the picks taken, 0-2 (counted up as each is taken: 00A66E; traced
    // through choosing). Its first byte, the record's step, is 0 while the side is not choosing.
    constexpr uint32_t kPicksTaken = 0x03;
    // A byte of a character's record (FF3800, FF3C00): which of the side's picks it is (0 the first, 1 the second);
    // read from the record on point (the records are not swapped at a tag: traced through the tag-team movies)
    constexpr uint32_t kWhichPick = 0x260;

    // Whether a side starts with its second character, as in Marvel vs. Capcom (games/mvsc/src/tag.cpp): a player who
    // holds LP+MP+HP as the choosing ends has the game swap the side's two picks (kPicks) and their colours (00A4EE-
    // 00A55C, the same code as Marvel vs. Capcom's 00BB44), so a record's kWhichPick no longer says who chose its
    // character. 0 again while the side chooses its first character (00A6EA). Kept with save states.
    uint8_t g_swapped[2];

    // 00A4D6: the game's step has just turned from the choosing to the match; seen before the swap. Only looks.
    bool OrderChosen(M68K &) {
        if (static_cast<int8_t>(Ram8(kPlaying)) > 0) {
            for (int side = 0; side < 2; side++) {
                if ((Ram8(kPlayersIn) >> side) & 1 && (Ram16(kPanel[side]) & 0x70) == 0x70) {
                    g_swapped[side] ^= 1;
                }
            }
        }
        return false;
    }

    // 00A6EA: a side's first pick is the character under its cursor (A4: its choosing record, +02 the side). Only looks.
    bool FirstPick(M68K &c) {
        g_swapped[Ram8((c.a[4] & 0xFFFFFF) + 2) & 1] = 0;
        return false;
    }
} // namespace

void TagState(Archive &ar) {
    if (ar.version != 0 && ar.version < 11) { // (a save state from before has none)
        g_swapped[0] = g_swapped[1] = 0;
        return;
    }
    ar.Mark(0x54414F52); // TAOR
    ar.Pod(g_swapped);
}

void RegisterTag() {
    AddHook(HookAt(0x00A4D6), "TagOrderChosen", OrderChosen);
    AddHook(HookAt(0x00A6EA), "TagFirstPick", FirstPick);
    tag::Description d;
    // (which of its side's people the record on point is: who chose its character)
    d.onPoint = [](int side) {
        return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameMatch ? (Ram8(Player(side) + kWhichPick) & 1) ^ g_swapped[side] : -1;
    };
    d.picking = [](int side) {
        const uint32_t record = kChoosing + static_cast<uint32_t>(side) * kChoosingSize;
        if (Ram16(kGameStep) != kGameChoosing || Ram8(record) == 0) {
            return -1;
        }
        return Ram8(record) == kChoosingDone ? 2 : std::min<int>(Ram8(record + kPicksTaken), 2);
    };
    d.fighting = [] { return Ram16(kGameStep) == kGameMatch && Ram8(kFight) == kFighting; };
    d.otherMode = [] { return session::InSession() || modes::AttractMode() || modes::StageViewer() || BattleMode() || marvel::ModelViewer(); };
    d.labelY = 0.19f;
    tag::Register(d);
}

} // namespace cps2recomp
