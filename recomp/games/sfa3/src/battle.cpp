// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: the fighting modes of the menu's "Alternative modes" page (survival, the endless battle, the
// ring out, the tag team battle) are the toolkit's (fight/battle.h); this says where the game keeps what they need, and
// makes the ring out's own movements of a player at its step of a fight.
#include "alternative.h"
#include "battle.h"
#include "sfa3.h"

#include <iterator>

namespace cps2recomp {

bool AttackInRange(M68K &c, uint32_t player); // guard.cpp
bool CanTag(int player);                      // livetag.cpp
void TagIn(int player, int character, int colours);
bool TaggedIn(int player);
void TagDifficulty(int level);
void TagOffStage(int player, bool off);
void TagUntouchable(int player);
uint16_t TagCall(int character);

namespace {
    using namespace sfa3;

    constexpr uint8_t kReelLaunched = 0x12;  // a knock-down's reeling animation in the air (028F1E: hit in the air)
    constexpr int32_t kPull = -0x6800;       // what is taken off the speed up each step
    constexpr int32_t kFlungAcross = 0xC000; // knocked out: away over the end, three quarters of a pixel a step
    constexpr int32_t kFlungUp = 0x60000;    //   and up, six pixels a step at first
    constexpr int kGoneBelow = 400;          // how far under the floor one falling stops (long out of the picture)
    // The part of a player's record that is how it moves and is drawn (its steps, its frame, its speeds, its boxes):
    // taken from its record on the old stage when it comes down onto the next, but for its place and its opponent
    constexpr uint32_t kMovingFrom = 0x04, kMovingTo = 0x100;
    // The game's random number, a word (01BDA6 steps it)
    constexpr uint32_t kRandom = 0xFF80AE;

    // The panel and the stage's pieces: everything but the fighters
    bool NotTheFighters(uint32_t tile, uint16_t attributes) {
        return ThePanel(tile, attributes) || IsStagePiece(attributes) || (attributes & 0x1F) >= 0x14;
    }
} // namespace

// The ring out's own movements (fight/battle.h, RingOutMove) at a player's step (A6): true when it has made the step,
// the game's own routine for the player not run (training.cpp's PlayerStep, at 027CBA)
bool RingOutStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const RingOut ringOut = RingOutMove(player);
    if (ringOut.move == RingOut::kFlungOut) {
        // Knocked over the end of the stage: a knock-down's throw into the air, up and away (no longer kept in)
        SetRam32(player + kState, 0x02020202); // (reeling, knocked into the air)
        SetRam8(player + kReelKind, kReelKnockedDown);
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
    d.playerRecords = 4;
    d.thrown = kThrown;
    d.thrownSize = kThrownSize;
    d.thrownRecords = kThrownRecords * 2; // (the two pools follow one another)
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
    // (the slots that are not characters of this version, and Gen's other stance, are never picked)
    d.choosable = [](int n) { return n >= 0 && n < kCharacters && kCharacterNames[n] && n != 0x13; };
    d.rosterNumber = [](int choice) { return choice >= 1 && choice <= kFighters ? static_cast<int>(kFighterNumbers[choice - 1]) : -1; };
    d.fighterChoices = {"Random"};
    for (const uint8_t number : kFighterNumbers) {
        d.fighterChoices.push_back(kCharacterNames[number]);
    }
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
    // (the players' own objects are in their colour sets: +0D, player 1's from 0, player 2's from 5, five each)
    d.playerSets = [](int n) { return 0x1Fu << (Ram8(Player(n) + kColourSet) & 0x1F); };
    d.inAir = [](uint32_t player) { return Ram8(player + kInAir) != 0; };
    d.atJumpTop = [](uint32_t player) { return Ram8(player + kInAir) && static_cast<int32_t>(Ram32(player + kSpeedY)) <= 0; };
    d.attackComing = [](Machine &machine, int side) { return AttackInRange(machine.cpu, Player(side)); };
    d.computerPlaysModes = true; // (the game's computer plays the tag style and the ring out itself: the modes' tactics, computer.cpp)
    d.difficultyNow = [] { return static_cast<int>(Ram8(0xFF80BE)); }; // (the game's copy of the board's, taken as a game begins)
    d.canAct = [](int side) { return CanAct(Player(side)); };
    d.teams = true;
    d.teamLinesY = 0.19f; // (under the bars and the names)
    // (no entrances: a team battle's fighters begin ready, livetag.cpp's TagEntrance)
    d.difficulty = TagDifficulty;
    d.canTag = CanTag;
    d.tagIn = TagIn;
    d.taggedIn = TaggedIn;
    d.offStage = TagOffStage;
    d.untouchable = TagUntouchable;
    d.tagCall = TagCall;
    d.speedX = kSpeedX;
    d.speedY = kSpeedY;
    d.floor = kGround;
    d.meter = kSpecialGauge; // (the super combo gauge, which the setting up leaves as it was)
    d.meterSize = 2;
    battle::Register(d);
    battle::RegisterTeamTag(0.25f);
}

} // namespace cps2recomp
