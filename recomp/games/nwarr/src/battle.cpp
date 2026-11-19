// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: the fighting modes of the menu's "Alternative modes" page (survival, the endless battle, the ring out)
// are the toolkit's (fight/battle.h); this says where the game keeps what they need. As Pocket Fighter's.
#include "battle.h"
#include "alternative.h"
#include "nwarr.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace nwarr;

    constexpr uint32_t kSpeedX = 0x30, kSpeedY = 0x34; // longs, 16 bits of fraction (dumps through a jump and a walk)
    constexpr uint32_t kHalfWidth = 0x1C0; // word: half the player's width (006BBC: the picture keeps X this far in)
    constexpr int32_t kPull = -0x8000;                 // what is taken off the speed up each step
    constexpr int32_t kFlungAcross = 0x30000;          // knocked out: away over the end, three pixels a step
    constexpr int32_t kFlungUp = 0x60000;              //   and up, six pixels a step at first
    constexpr int kGoneBelow = 400;                    // how far under the floor one falling stops
    // The part of a player's record that is how it moves and is drawn: taken from its record on the old stage when it
    // comes down onto the next, but for its place and its opponent
    constexpr uint32_t kMovingFrom = 0x04, kMovingTo = 0x200;

    // The round's result, which the game keeps no byte of (a knockout sets FF8E14, 0C13BE): who has no health left,
    // or at time over who has less
    uint8_t ResultNow() {
        const auto health = [](int n) { return static_cast<int16_t>(Ram16(Player(n) + kHealth)); };
        const bool out1 = health(0) < 0, out2 = health(1) < 0;
        if (Ram8(kKnockOut) && (out1 || out2)) {
            return static_cast<uint8_t>((out2 ? 1 : 0) | (out1 ? 2 : 0));
        }
        if (Ram8(kRoundClock) == 0 && modes::RoundFought()) {
            return health(0) > health(1) ? 1 : health(1) > health(0) ? 2 : 3;
        }
        return 0;
    }
} // namespace

// The ring out's own movements (fight/battle.h, RingOutMove) at a player's step (A6): true when it has made the step,
// the game's own routine for the player not run (training.cpp's PlayerStep, at 02A220). The fighter is moved by the
// port, in the pose it was knocked out in (the game's own reel is not run meanwhile).
bool RingOutStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const RingOut ringOut = RingOutMove(player);
    const auto fly = [player] {
        const int32_t speedX = static_cast<int32_t>(Ram32(player + kSpeedX)), speedY = static_cast<int32_t>(Ram32(player + kSpeedY));
        SetRam32(player + kX, Ram32(player + kX) + static_cast<uint32_t>(speedX));
        SetRam32(player + kY, Ram32(player + kY) + static_cast<uint32_t>(speedY));
        SetRam32(player + kSpeedY, static_cast<uint32_t>(speedY + kPull));
    };
    if (ringOut.move == RingOut::kFlungOut) {
        SetRam8(player + kState, kStateReel);
        SetRam8(player + kHitStop, 0);
        SetRam8(player + kAttack, 0);
        SetRam32(player + kSpeedX, static_cast<uint32_t>(ringOut.side * kFlungAcross));
        SetRam32(player + kSpeedY, static_cast<uint32_t>(kFlungUp));
        fly();
        return true;
    }
    if (ringOut.move == RingOut::kFalling) {
        if (static_cast<int16_t>(Ram16(player + kY)) > static_cast<int16_t>(kGround) - kGoneBelow) {
            fly();
        }
        return true;
    }
    if (ringOut.move == RingOut::kDropsIn) {
        // Coming down onto the next stage: as it was in its jump over the old stage, at its place here; the game's jump
        // goes on from there and lands it
        for (uint32_t at = kMovingFrom; at < kMovingTo; at++) {
            if (at < kX || at >= kY + 4) {
                SetRam8(player + at, ringOut.record[at]);
            }
        }
        SetRam32(player + kX, static_cast<uint32_t>(ringOut.x) << 16);
        SetRam32(player + kY, static_cast<uint32_t>(ringOut.y) << 16);
    }
    return false;
}

void RegisterBattle() {
    battle::Description d;
    d.resultNow = ResultNow;
    d.cameraX = kCameraX;
    d.cameraY = kCameraY;
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.playerRecords = 2;
    d.thrown = kThrown;
    d.thrownSize = kThrownSize;
    d.thrownRecords = kThrownRecords;
    d.active = 0;
    d.x = kX;
    d.y = kY;
    d.character = kCharacter;
    d.computer = kPerson;
    d.computerIsZero = true;
    d.ownMaster = [](uint32_t player) { return Ram8(player + kState) <= kStateGuard; };
    d.halfWidth = kHalfWidth;
    d.colours = kColoursChosen;
    d.health = kHealth;
    d.healthFull = kHealthFull;
    d.healthBack = 0x18; // (a sixth of it all)
    d.stages = kStages;
    d.stageToRound = 2300;
    d.characterSpan = kCharacters - 1;
    d.choosable = [](int number) { return number >= 1 && number < kCharacters; };
    d.rosterNumber = [](int choice) { return choice >= 1 && choice < kCharacters ? choice : -1; };
    d.fighterChoices = {"Random"};
    d.fighterChoices.insert(d.fighterChoices.end(), std::begin(kCharacterNames) + 1, std::end(kCharacterNames));
    d.inGame = [] { return Ram8(kPlayersIn) != 0 && Ram16(kMode) == kGameRunning; };
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
    d.playerSets = [](int n) { return 0x1Fu << (Ram8(Player(n) + kColourSet) & 0x1F); };
    d.inAir = [](uint32_t player) { return Ram8(player + kState) == kStateJump || Ram16(player + kY) != kGround; };
    d.atJumpTop = [](uint32_t player) {
        return Ram8(player + kState) == kStateJump && static_cast<int32_t>(Ram32(player + kSpeedY)) <= 0;
    };
    battle::Register(d);
}

} // namespace cps2recomp
