// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: a player handed to the game's computer in the middle of a round (the fighting modes:
// fight/battle.h, which say who plays a fighter each frame by its byte kPerson). Super Turbo's code (games/ssf2t's
// computer.cpp has the story); the addresses are this program's.
//
// A player's routine (066964) runs a person's step (066B32) or the computer's (06A142) by that byte, and both keep how
// far on they are in the record's kFighting byte: 0 at a round's start, then 2 for the fight. The computer's first
// step sets up what the computer plays by (06BD16, 06C922, 06A1C8, the two words from 316E3E), which a person's never
// does; a player handed over at 2 goes straight to the computer's fight step with none of it. So where the computer's
// step is about to run (06697E) for a player with no plan in its record (the long kPlanTable: the record is cleared as
// a match begins, 0039E8, and only the computer's set-up writes it, 06C952), the game is sent through that set-up
// first. In the game itself a player only changes hands between rounds, at 0: this never runs.
#include "hsf2.h"


namespace cps2recomp {

namespace {
    using namespace hsf2;

    constexpr uint32_t kComputerStep = 0x06697E;
    constexpr uint32_t kCharacterTables = 0x06BD16, kPlan = 0x06C922, kPlanRound = 0x06A1C8;
    constexpr uint32_t kPlanWords = 0x316E3E; // (two words a character, to +23A and +23C)
    constexpr uint32_t kComputerStepReturns = 0x066986;
    constexpr uint32_t kPlanTable = 0x1E0;    // a player's long: the computer's plan for the character (06C952)
    constexpr uint32_t kPersonStep = 0x066B32;
    constexpr uint32_t kReadingFrom = 0x080, kReadingTo = 0x0E0; // a person's reading of the controls for its moves

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // 06697E: the computer's step is to come for the player at A6
    bool ComputerStep(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (PlayerOf(player) < 0 || Ram32(player + kPlanTable) != 0 || Ram8(player + kFighting) != 2) {
            return false;
        }
        // (the game's own first step of the computer, 06A188-06A1BA, but for the animation it starts and its two bytes
        // at 06A196: the player is in the middle of whatever it was doing (in Super Turbo a standing animation under
        // another state's step left that state's routine reading the wrong table). Its three routines are run by the
        // game in their order, each returning into the next as if called (06BD16, 06C922, 06A1C8), and the last
        // returns where 06A142's would.
        // Not called from C++: the board's time they take is then counted as the game's own code counts it, and the
        // interpreted run of the same code is alike)
        const uint32_t words = kPlanWords + static_cast<uint32_t>(Ram8(player + kCharacter)) * 4;
        SetRam16(player + 0x23A, static_cast<uint16_t>(c.Read(words, 1)));
        SetRam16(player + 0x23C, static_cast<uint16_t>(c.Read(words + 2, 1)));
        SetRam8(player + 0x273, 2);
        c.d[0] = 0; // (moveq #0,d0)
        c.Push32(kComputerStepReturns);
        c.Push32(kPlanRound);
        c.Push32(kPlan);
        c.pc = kCharacterTables;
        return true;
    }

    // 066B32: a person's step is to come for the player at A6. One the computer played until now (a plan in its record)
    // has its reading of the controls for its moves started again from nothing, and its plan let go, once it stands or
    // crouches: the computer's moves keep their own counts in those bytes (+80), which a person's reading then takes
    // for how far on it was (in Super Turbo Cammy's went into the middle of an instruction). Not before: a move under
    // way keeps which it is there. Given back to the computer, the player is set up for it again (above). In the game
    // itself, as above, this never runs.
    bool PersonStep(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (PlayerOf(player) < 0 || Ram32(player + kPlanTable) == 0) {
            return false;
        }
        const uint8_t state = Ram8(player + kState);
        if (state != kStand && state != kCrouch) {
            return false;
        }
        for (uint32_t at = kReadingFrom; at < kReadingTo; at++) {
            SetRam8(player + at, 0);
        }
        SetRam32(player + kPlanTable, 0);
        return false;
    }
} // namespace

void RegisterComputer() {
    AddHook(HookAt(kComputerStep), "ComputerStep", ComputerStep);
    AddHook(HookAt(kPersonStep), "PersonStep", PersonStep);
}

} // namespace cps2recomp
