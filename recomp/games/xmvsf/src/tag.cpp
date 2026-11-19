// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: tag-team play for four people (fight/tag.h); this says where the game keeps the character on
// point and the choosing.
#include "alternative.h"
#include "battle.h"
#include "marvel.h"
#include "session.h"
#include "tag.h"
#include "xmvsf.h"

#include <algorithm>

namespace cps2recomp {

namespace {
    using namespace xmvsf;

    // A byte of each choosing record (kChoosing): the picks taken, 0-2 (counted up as each is taken: 00B1E8; traced
    // through choosing). Its first byte, the record's step, is 0 while the side is not choosing.
    constexpr uint32_t kPicksTaken = 0x03;
    // A byte of the point character's record (FF4000, FF4400): which of the side's picks it is (0 the first, 1 the second),
    // swapped at a tag or a knock-out with the records' contents (traced through a tag: HP+HK, 14 frames to the swap)
    constexpr uint32_t kWhichPick = 0x220;
} // namespace

void RegisterTag() {
    tag::Description d;
    d.onPoint = [](int side) { return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameMatch ? Ram8(Player(side) + kWhichPick) & 1 : -1; };
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
