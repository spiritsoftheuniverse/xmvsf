// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: a player handed to the game's computer in the middle of a round (the fighting modes:
// fight/battle.h, which say who plays a fighter each frame by its byte kPerson).
//
// A player's routine (078030) runs a person's step (078270) or the computer's (07BB22) by that byte, and both keep how
// far on they are in the record's kFighting byte: 0 at a round's start, then 2 for the fight. The computer's first
// step (07BB34) sets up what the computer plays by (its tables for the character and the plan it follows: 07D6F4,
// 07E36E, 07BB82, the two words from 24EAE8), which a person's never does; a player handed over at 2 goes straight to
// the computer's fight step with none of it, and the computer's code reads its plan from nowhere (the game stops). So
// where the computer's step is about to run (07804A) for a player with no plan in its record (the long kPlanTable: the
// record is cleared as a match begins, 003616, and only the computer's set-up writes it, 07E3A8), the game is sent
// through that set-up first (07D6F4, 07E36E, 07BB82, the two words from 24EAE8; not its tables from 005078 nor its
// shadow, which the player has, nor the standing animation it starts). In the game itself a player only changes hands between rounds, at 0: this never runs.
#include "ssf2t.h"


namespace cps2recomp {

namespace {
    using namespace ssf2t;

    constexpr uint32_t kComputerStep = At(0x07804A);
    constexpr uint32_t kCharacterTables = At(0x07D6F4), kPlan = At(0x07E36E), kPlanRound = At(0x07BB82);
    constexpr uint32_t kPlanWords = At(0x24EAE8); // (two words a character, to +23A and +23C)
    constexpr uint32_t kComputerStepReturns = At(0x078052);
    constexpr uint32_t kPlanTable = family::kPlanTable; // a player's long: the computer's plan for the character (07E3A8)
    constexpr uint32_t kSetUp = 0x273;                  // a byte the computer's set-up puts at 2 (where kPlanTable is none)
    bool SetUp(uint32_t player) {
        return kPlanTable ? Ram32(player + kPlanTable) != 0 : Ram8(player + kSetUp) != 0;
    }
    constexpr uint32_t kPersonStep = At(0x078270);
    constexpr uint32_t kReadingFrom = 0x080, kReadingTo = 0x0E0; // a person's reading of the controls for its moves

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // 07804A: the computer's step is to come for the player at A6
    bool ComputerStep(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (PlayerOf(player) < 0 || SetUp(player) || Ram8(player + kFighting) != 2) {
            return false;
        }
        // (the game's own first step of the computer, 07BB40-07BB74, but for the animation it starts and its two bytes
        // at 07BB4E: the player is in the middle of whatever it was doing, and a standing animation under another
        // state's step left that state's routine reading the wrong table, Fei Long's at 095F84. Its three routines are
        // run by the game in their order, each returning into the next as if called (07D6F4, 07E36E, 07BB82; none uses
        // what the one before left in a register, nor +273 before 07E36E), and the last returns where 07BB22's would.
        // Not called from C++: the board's time they take is then counted as the game's own code counts it, and the
        // interpreted run of the same code is alike)
        const uint32_t words = kPlanWords + static_cast<uint32_t>(Ram8(player + kCharacter)) * 4;
        SetRam16(player + 0x23A, static_cast<uint16_t>(c.Read(words, 1)));
        SetRam16(player + 0x23C, static_cast<uint16_t>(c.Read(words + 2, 1)));
        SetRam8(player + 0x273, 2);
        c.d[0] = 0; // (moveq #0,d0)
        // (a game whose set-up has fewer parts goes through those it has: the places 0 are none, family.h)
        c.Push32(kComputerStepReturns);
        if constexpr (kPlanRound != 0) {
            c.Push32(kPlanRound);
        }
        if constexpr (kCharacterTables != 0) {
            c.Push32(kPlan);
            c.pc = kCharacterTables;
        } else {
            c.pc = kPlan;
        }
        return true;
    }

    // 078270: a person's step is to come for the player at A6. One the computer played until now (a plan in its record)
    // has its reading of the controls for its moves started again from nothing, and its plan let go, once it stands or
    // crouches: the computer's moves keep their own counts in those bytes (Cammy's at 0B4B2C, +80), which a person's
    // reading then took for how far on it was (Cammy's at 08DCBC, a jump through its table into the middle of an
    // instruction). Not before: a move under way keeps which it is there (Fei Long's, read at 095B34). Given back to the
    // computer, the player is set up for it again (above). In the game itself, as above, this never runs.
    bool PersonStep(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (PlayerOf(player) < 0 || !SetUp(player)) {
            return false;
        }
        const uint8_t state = Ram8(player + kState);
        if (state != kStand && state != kCrouch) {
            return false;
        }
        for (uint32_t at = kReadingFrom; at < kReadingTo; at++) {
            SetRam8(player + at, 0);
        }
        if (kPlanTable) {
            SetRam32(player + kPlanTable, 0);
        } else {
            SetRam8(player + kSetUp, 0);
        }
        return false;
    }
} // namespace

void RegisterComputer() {
    AddHook(HookAt(kComputerStep), "ComputerStep", ComputerStep);
    AddHook(HookAt(kPersonStep), "PersonStep", PersonStep);
}

} // namespace cps2recomp
