// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the fighting modes of the menu's "Alternative modes" page (survival, the endless battle, the ring out) are
// the toolkit's (fight/battle.h); this says where the game keeps what they need, and makes the ring out's own movements
// of a player at its step.
#include "battle.h"
#include "alternative.h"
#include "cybots.h"
#include "state.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace cybots;

    constexpr int32_t kFlungAcross = 0xC000; // knocked out: away over the end, three quarters of a pixel a step
    constexpr int32_t kFlungUp = 0x60000;    //   and up, six pixels a step at first
    constexpr int32_t kPull = 0x6000;        //   pulled down by as much as a jump is (+1E's steps through one)
    constexpr int kGoneBelow = 400;          // how far under the floor one falling stops (long out of the picture)
    // The part of a player's record that is how it moves and is drawn (its steps, its frame, its speeds, its boxes): taken
    // from its record on the old stage when it comes down onto the next, but for its place (X and Y with their last
    // frame's, +1A to +29) and the computer's play (+200 to +2FF); +370 on is who it is and its opponent
    constexpr uint32_t kMovingFrom = 0x03, kMovingTo = 0x370, kPlaceFrom = 0x1A, kPlaceTo = 0x2A, kScriptFrom = 0x200, kScriptTo = 0x300;

    // A player flung out flies by the port's own steps (the game's routine for it not run): its speeds, kept with the
    // game's state
    int32_t g_speedX[2] = {0, 0}, g_speedY[2] = {0, 0};

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    bool InGame() {
        return Ram8(kInGame) != 0 && Ram16(kMode) == kGameRunning;
    }

    // The round's result: a knock-out (the game puts health at FFFF, below nothing: 0463EA), or the clock run out
    // with the round over (kStep2 18 on), by health
    uint8_t ResultNow() {
        if (!InGame() || Ram16(kStep) != kGameMatch) {
            return 0;
        }
        const int16_t h1 = static_cast<int16_t>(Ram16(Player(0) + kHealth)), h2 = static_cast<int16_t>(Ram16(Player(1) + kHealth));
        if (h1 < 0 || h2 < 0) {
            return static_cast<uint8_t>((h2 < 0 ? 1 : 0) | (h1 < 0 ? 2 : 0));
        }
        if (Ram8(kRoundClock) == 0 && Ram16(kStep2) > kRoundFought) {
            return h1 > h2 ? 1 : h2 > h1 ? 2 : 3;
        }
        return 0;
    }
} // namespace

// The ring out's own movements (fight/battle.h, RingOutMove) at a player's step (A6): true when it has made the step,
// the game's own routine for the player not run (training.cpp's PlayerStep, at 019D70)
bool RingOutStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const int n = PlayerOf(player);
    if (n < 0) {
        return false;
    }
    const RingOut ringOut = RingOutMove(player);
    if (ringOut.move == RingOut::kFlungOut) {
        // Knocked over the end of the stage: up and away, no longer kept in (drawn as it was hit)
        g_speedX[n] = ringOut.side * kFlungAcross;
        g_speedY[n] = kFlungUp;
        return true;
    }
    if (ringOut.move == RingOut::kFalling) {
        // Through the air and the floor
        if (static_cast<int16_t>(Ram16(player + kY)) > static_cast<int16_t>(Ram16(kFloor)) - kGoneBelow) {
            SetRam32(player + kX, Ram32(player + kX) + static_cast<uint32_t>(g_speedX[n]));
            SetRam32(player + kY, Ram32(player + kY) + static_cast<uint32_t>(g_speedY[n]));
            g_speedY[n] -= kPull;
        }
        return true;
    }
    if (ringOut.move == RingOut::kDropsIn) {
        // Coming down onto the next stage: as it was in its jump over the old stage, at its place here; the game's jump
        // goes on from there and lands it
        for (uint32_t at = kMovingFrom; at < kMovingTo; at++) {
            if ((at < kPlaceFrom || at >= kPlaceTo) && (at < kScriptFrom || at >= kScriptTo)) {
                SetRam8(player + at, ringOut.record[at]);
            }
        }
        SetRam32(player + kX, static_cast<uint32_t>(ringOut.x) << 16);
        SetRam32(player + kY, static_cast<uint32_t>(ringOut.y) << 16);
    }
    return false;
}

void RingOutState(Archive &ar) {
    ar.Mark(0x52494E47); // RING
    ar.Pod(g_speedX);
    ar.Pod(g_speedY);
}

void RegisterBattle() {
    battle::Description d;
    d.resultNow = ResultNow;
    d.cameraXAt = CameraXAt;
    d.cameraYAt = CameraYAt;
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.playerRecords = 2;
    d.thrown = kThrown;
    d.thrownSize = kThrownSize;
    d.thrownRecords = kThrownRecords;
    d.active = 0x00;
    d.x = kX;
    d.y = kY;
    d.character = kMachine;
    d.computer = kComputer;
    // (who is in charge: the player itself while free, attacking or in a move of its own, and while it reels from a hit
    // on its feet: a hit's push back moves it a few pixels, which at a wall (half its width from the stage's end, 3E4)
    // is past the picture's edge, and would count as a ring out. Only a knock-down (+04 8) or a throw (0C) sends a
    // player over an end: the ring out's "knocked about", as in the other games)
    d.ownMaster = [](uint32_t player) {
        const uint8_t sub = Ram8(player + kSub);
        return Ram8(player + kState) == kFree || sub == kSubAttack || sub == kSubSpecial || sub == kSubReel;
    };
    d.halfWidth = kHalfWidth;
    d.health = kHealth;
    d.healthFull = kHealthFull;
    d.healthBack = 0x19; // (a sixth of it all)
    d.stages = kStages;
    d.stageToRound = 2300;
    d.characterSpan = kMachines;
    d.choosable = [](int) { return true; };
    d.rosterNumber = [](int choice) { return choice >= 1 && choice <= kMachines ? choice - 1 : -1; };
    d.fighterChoices = {"Random"};
    d.fighterChoices.insert(d.fighterChoices.end(), std::begin(kMachineNames), std::end(kMachineNames));
    d.inGame = InGame;
    d.choosing = modes::Choosing;
    d.toRound = modes::ToRound;
    d.playSound = modes::PlaySound;
    d.wanted = modes::Wanted;
    d.cameraAt = modes::CameraAt;
    d.endModes = modes::End;
    d.stageViewer = modes::StageViewer;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideEnds = WideEnds;
    d.notTheStage = NotTheStage;
    d.thePanel = ThePanel;
    d.notTheFighters = NotTheFighters;
    d.playerSets = [](int n) { return 0xFFu << (n * 0x10); };
    d.inAir = InAir;
    d.atJumpTop = [](uint32_t player) {
        return InAir(player) && static_cast<int32_t>(Ram32(player + kY)) <= static_cast<int32_t>(Ram32(player + kLastY));
    };
    d.canAct = [](int side) {
        const uint32_t player = Player(side);
        const uint8_t sub = Ram8(player + kSub);
        return Ram8(player + kState) == kFree && (sub == kSubStand || sub == kSubCrouch || sub == kSubAir);
    };
    d.difficultyNow = [] { return static_cast<int>(Ram8(kDifficulty) & 7); };
    battle::Register(d);
}

} // namespace cps2recomp
