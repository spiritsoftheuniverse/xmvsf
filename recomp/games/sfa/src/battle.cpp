// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the fighting modes of the menu's "Alternative modes" page (survival, the endless battle, the
// ring out) are the toolkit's (fight/battle.h); this says where the game keeps what they need, and makes the ring out's
// own movements of a player at its step of a fight.
#include "alternative.h"
#include "battle.h"
#include "sfa.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace sfa;

    constexpr uint32_t kSetReelAnimation = 0x01E94A; // D0: the character's animation of that number in its third list (the reelings)
    constexpr uint32_t kAnimate = 0x01E96E;          // the animation on by a step
    constexpr uint32_t kFly = 0x01EA5E;              // a step through the air: the speeds by their pulls, the place by the speeds
    constexpr uint8_t kReelLaunched = 0x13;          // a knock-down's reeling animation, thrown back into the air (the table
                                                     // 01F41A, by the hit's kind: 01D8AC)
    constexpr int32_t kPull = -0x6800;               // what is taken off the speed up each step
    constexpr int32_t kFlungAcross = 0xC000;         // knocked out: away over the end, three quarters of a pixel a step
    constexpr int32_t kFlungUp = 0x60000;            //   and up, six pixels a step at first
    constexpr int kGoneBelow = 400;                  // how far under the floor one falling stops (long out of the picture)
    constexpr uint32_t kReelKind = 0x44;             // byte: the kind of reeling (4 knocked into the air: 01DB66)
    // The part of a player's record that is how it moves and is drawn (its steps, its frame, its speeds, its boxes):
    // taken from its record on the old stage when it comes down onto the next, but for its place and its opponent
    constexpr uint32_t kMovingFrom = 0x04, kMovingTo = 0x100;

    // (the pool of what the players throw: sfa.h; Ryu's fireball was the first)
    constexpr uint32_t kActive = 0x00;
    // The game's random number, a word (016460 steps it)
    constexpr uint32_t kRandom = 0xFF81AE;

    // The panel and the stage's pieces: everything but the fighters
    bool NotTheFighters(uint32_t tile, uint16_t attributes) {
        return ThePanel(tile, attributes) || IsStagePiece(attributes);
    }
} // namespace

// The ring out's own movements (fight/battle.h, RingOutMove) at a player's step (A6): true when it has made the step,
// the game's own routine for the player not run (training.cpp's PlayerStep, at 01CE48 and 021DF4)
bool RingOutStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const RingOut ringOut = RingOutMove(player);
    if (ringOut.move == RingOut::kFlungOut) {
        // Knocked over the end of the stage: a knock-down's throw into the air, up and away (no longer kept in)
        SetRam32(player + kState, 0x02020002); // (reeling, knocked into the air: 01DE8A's step of flying)
        SetRam8(player + kReelKind, 4);
        SetRam8(player + kInAir, 0xFF);
        SetRam8(player + kHitStop, 0);
        SetRam32(player + kSpeedX, static_cast<uint32_t>(ringOut.side * kFlungAcross));
        SetRam32(player + kSpeedY, static_cast<uint32_t>(kFlungUp));
        SetRam32(player + kPullX, 0);
        SetRam32(player + kPullY, static_cast<uint32_t>(kPull));
        c.d[0] = kReelLaunched;
        Call(c, kSetReelAnimation);
        return true;
    }
    if (ringOut.move == RingOut::kFalling) {
        // Through the air and the floor
        if (static_cast<int16_t>(Ram16(player + kY)) > static_cast<int16_t>(kGround) - kGoneBelow) {
            Call(c, kFly);
        }
        Call(c, kAnimate);
        return true;
    }
    if (ringOut.move == RingOut::kDropsIn) {
        // Coming down onto the next stage: as it was in its jump over the old stage, at its place here; the game's jump
        // goes on from there and lands it
        for (uint32_t at = kMovingFrom; at < kMovingTo; at++) {
            if ((at < kX || at >= kY + 4) && (at < kOpponent || at >= kOpponent + 2)) {
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
    d.result = kRoundResult;
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
    d.active = kActive;
    d.x = kX;
    d.y = kY;
    d.character = kCharacter;
    d.colours = kColourChoice;
    d.random = kRandom;
    d.randomNext = battle::CapcomRandomNext;
    d.computer = kComputer;
    d.control = kControl;
    d.controlOwn = kControlOwn;
    d.halfWidth = kHalfWidth;
    d.health = kHealth;
    d.healthCeiling = kHealthShown;
    d.healthFull = kHealthFull;
    d.healthBack = 0x18; // (a sixth of it all)
    d.ceilingShown = true;
    d.stages = kStages;
    d.stageToRound = 2000; // (about 1,800 from switching on, and a few)
    d.characterSpan = kCharacters;
    d.choosable = [](int) { return true; };
    d.rosterNumber = [](int choice) { return choice >= 1 && choice <= kCharacters ? choice - 1 : -1; };
    d.fighterChoices = {"Random"};
    d.fighterChoices.insert(d.fighterChoices.end(), std::begin(kCharacterNames), std::end(kCharacterNames));
    d.tuneOff = kTuneOff;
    d.inGame = [] { return Ram16(kMode) == 2; };
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
    // (the players' own objects are in their colour sets: +0D, player 1's from 0, player 2's from 8, eight each)
    d.panelLayers = 0xF & ~kLayersWithoutPanel; // (the bars, names and clock are the first scroll screen's: sfa.h)
    d.playerSets = [](int n) { return 0xFFu << (Ram8(Player(n) + kColourSet) & 0x1F); };
    d.inAir = [](uint32_t player) { return Ram8(player + kInAir) != 0; };
    d.atJumpTop = [](uint32_t player) { return Ram8(player + kInAir) && static_cast<int32_t>(Ram32(player + kSpeedY)) <= 0; };
    battle::Register(d);
}

} // namespace cps2recomp
