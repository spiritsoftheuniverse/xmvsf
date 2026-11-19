// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: some regions ignore the operator's difficulty setting.
//
// The computer's difficulty picks a row from a table. The routine at 005C7A reads the board's difficulty
// (FF82BC, 0..7) into d0 and multiplies it by 64 (005C7E, lsl.w #6), adds a per-stage index times four, and
// loads the default table pointer 0x315E00 into A0 (005C8A). A block at 005C90..005CB4 then, for the regions
// whose region word (FF82FA) is 2 (USA), 4, 6 or 8 (Asia), swaps A0 to a harder table at 0x316000 -- so those
// versions do not follow the difficulty setting as Japan's (FF82FA = 0) does. The row is read at 005CB6
// (move.l (a0,d0.w),$b40(a5)); the block touches nothing but A0.
//
// bankbank's difficulty patch for the ROM (hs2u.03, patched CRC 2ef3f7db) replaces that block with NOPs, so
// every region keeps the default table. This does the same thing switchably, without touching the ROM: just
// before the row is read, put the default table back in A0 when the fix is on. Only A0 is ever affected, so
// the result is identical to the patch. Off by default: the USA set plays as the board does.
#include "hsf2.h"

namespace cps2recomp {

namespace {
    constexpr uint32_t kReadRow      = 0x005CB6; // move.l (a0,d0.w),$b40(a5): the difficulty row is read here
    constexpr uint32_t kDefaultTable = 0x00315E00; // the table the region block would swap away for 2/4/6/8

    // 005CB6: force the default difficulty table, as bankbank's patch does by NOPping the region block above.
    bool DifficultyFix(M68K &c) {
        if (FeatureOn("difficulty_fix")) {
            c.a[0] = kDefaultTable;
        }
        return false; // the game's own code reads the row from A0
    }
} // namespace

void RegisterDifficulty() {
    TheGame().features.push_back({"difficulty_fix", "Game", "Difficulty fix",
        "Make every region follow the difficulty setting. The USA and Asia versions otherwise fight on a "
        "harder curve than the setting asks for, as Japan's does not. The same as bankbank's ROM fix.",
        kFeatureOwn});
    AddHook(HookAt(0x005CB6), "DifficultyFix", DifficultyFix);
}

} // namespace cps2recomp
