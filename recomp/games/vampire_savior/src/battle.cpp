// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Vampire Savior: the fighting modes of the menu's "Alternative modes" page (survival, the endless battle, the ring
// out, the team battle and its tag-team modes) are the toolkit's (fight/battle.h); this says where the game keeps what they need, and makes the ring out's
// own movements of a player at its step (RingOutStep, from frames.cpp's PlayerStep).
#include "battle.h"
#include "modes.h"
#include "widescreen.h"

namespace cps2recomp {

bool AttackInRange(M68K &c, uint32_t player); // fight.cpp
bool CanTag(int player); // livetag.cpp
void TagIn(int player, int character, int colours);
void TagNoEntrances();
void TagDifficulty(int level);
void TagUntouchable(int player);
uint16_t TagCall(int character);
bool TagSetUp(M68K &c);
bool TaggedIn(int player);
void TagOffStage(int player, bool off);

namespace {
    using namespace vsav;

    constexpr uint32_t kSetReelAnimation = At(0x027EC0); // D0: the character's animation of that number in its list of reelings
    constexpr uint32_t kAnimate = At(0x027F70);          // the animation on by a step
    constexpr uint32_t kFly = At(0x015466);              // a step through the air: across and up by the speeds, the speeds by theirs
    constexpr uint8_t kReelLaunched = 42;                // thrown back by a knock-down (rising)
    constexpr uint8_t kReelFalling = 65;                 //   and coming down
    constexpr int32_t kPull = -0x6800;                   // what is taken off the speed up each step
    constexpr int32_t kFlungAcross = 0xC000;             // knocked out: away over the end, three quarters of a pixel a step
    constexpr int32_t kFlungUp = 0x60000;                //   and up, six pixels a step at first
    constexpr int kGoneBelow = 400;                      // how far under the floor one falling stops (long out of the picture)
    // The part of a player's record that is how it moves and is drawn (its steps, its frame, its speeds, its
    // boxes): taken from its record on the old stage when it comes down onto the next, but for its place
    constexpr uint32_t kMovingFrom = 0x04, kMovingTo = 0x100;

    // The walk's (kAction kActionWalk, 02289E): the way it goes (1 back), and the way the computer wants it to go
    constexpr uint32_t kWalkingBack = 0x0A, kComputerWalksBack = 0x20B;
    constexpr uint32_t kWalkBegins = At(0x0228B8); // the walk's animation and speed, by kWalkingBack
    // 022966 (the walk's first step) and 0229F2 (its steps after): the computer's walk takes the way the computer wants,
    // and that the other way than the walk's, the walk begins again that way (else it goes on, at `kSame`). The ring
    // out's line: not back towards the end it is near, coached (battle.h); else the game's code as it is.
    template <uint32_t kSame>
    bool ComputerWalkWay(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (!Ram8(player + kComputerWalksBack) || RingOutMayWalkBack(player)) {
            return false;
        }
        SetLow8(c.d[0], 0); // (forward)
        if (Ram8(player + kWalkingBack)) {
            SetRam8(player + kWalkingBack, 0);
            c.pc = kWalkBegins;
        } else {
            c.pc = kSame;
        }
        return true;
    }
} // namespace

// The ring out's own movements (fight/battle.h, RingOutMove) at a player's step (A6): true when it has made the
// step, the game's own routine for the player not run
bool RingOutStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const RingOut ringOut = RingOutMove(player);
    if (ringOut.move == RingOut::kFlungOut) {
        // Knocked over the end of the stage: a knock-down's throw back, up and away (no longer kept in)
        SetRam32(player + kState, 0x02020202); // (reeling, knocked down, in the air)
        SetRam8(player + kAirborne, 0xFF);
        SetRam8(player + kHitStop, 0);
        SetRam8(player + kAttacking, 0);
        SetRam32(player + kSpeedX, static_cast<uint32_t>(ringOut.side * kFlungAcross));
        SetRam32(player + kSpeedY, static_cast<uint32_t>(kFlungUp));
        SetRam32(player + kPullX, 0);
        SetRam32(player + kPullY, static_cast<uint32_t>(kPull));
        c.d[0] = kReelLaunched;
        Call(c, kSetReelAnimation);
        return true;
    }
    if (ringOut.move == RingOut::kFalling) {
        // Through the air and the floor; coming down from the top, the knock-down's falling
        if (static_cast<int16_t>(Ram16(player + kY)) > static_cast<int16_t>(kGround) - kGoneBelow) {
            const bool rising = static_cast<int32_t>(Ram32(player + kSpeedY)) > 0;
            Call(c, kFly);
            if (rising && static_cast<int32_t>(Ram32(player + kSpeedY)) <= 0) {
                c.d[0] = kReelFalling;
                Call(c, kSetReelAnimation);
                return true;
            }
        }
        Call(c, kAnimate);
        return true;
    }
    if (ringOut.move == RingOut::kDropsIn) {
        // Coming down onto the next stage: as it was in its jump over the old stage, at its place here; the
        // game's jump goes on from there and lands it
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
    d.result = At(0xFF810C); // (kRoundResult)
    d.cameraX = kCameraX;
    d.cameraY = kCameraY;
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.playerRecords = kPlayerRecords;
    d.thrown = kThrown;
    d.thrownSize = kThrownSize;
    d.thrownRecords = kThrownRecords;
    d.active = kActive;
    d.x = kX;
    d.y = kY;
    d.character = kCharacter;
    d.computer = kComputer;
    d.control = kControl;
    d.controlOwn = kControlOwn;
    d.halfWidth = kHalfWidth;
    d.plays = kPlays;
    d.colours = kColours;
    d.random = 0xFF8000 + AtA5(0xD4); // (014E8A)
    d.randomNext = battle::CapcomRandomNext;
    d.health = kHealth;
    d.healthCeiling = kHealthCeiling;
    d.healthFull = kHealthFull;
    d.healthBack = 0x30;
    d.stages = kStages;
    d.stageToRound = 1100; // (counted: 921 for most stages, 1,046 for Fetus of God, and a few)
    d.characterSpan = kCharacterSpan;
    d.choosable = [](int number) { return Choosable(number); };
    d.rosterNumber = [](int choice) { return choice >= 1 && choice <= static_cast<int>(std::size(kRoster)) ? kRoster[choice - 1].number : -1; };
    d.fighterChoices = RosterChoices({"Random"});
    d.tuneOff = kTuneOff;
    d.inGame = [] { return Ram16(kMode) != 0; };
    d.choosing = [] { return Ram16(kMode) == 2 && Ram16(kGameStep) == kGameChoosing && Ram8(kPlayersIn) == 3; };
    d.toRound = [](Machine &machine, int32_t &frames, bool choose) { return ToRound(machine, frames, choose); };
    d.playSound = PlaySound;
    d.wanted = Wanted;
    d.cameraAt = CameraAt;
    d.endModes = EndModes;
    d.stageViewer = StageViewer;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideEnds = WideEnds;
    d.notTheStage = NotTheStage;
    d.thePanel = ThePanel;
    d.notTheFighters = NotTheFighters;
    d.playerSets = [](int n) { return kPlayerSets[n]; };
    d.noShadow = NoShadow;
    d.inAir = [](uint32_t player) { return Ram8(player + kAirborne) != 0; };
    d.atJumpTop = [](uint32_t player) { return Ram8(player + kAirborne) && static_cast<int32_t>(Ram32(player + kSpeedY)) <= 0; };
    d.teams = true;
    d.teamLinesY = 0.19f; // (just under the bars' bats)
    d.oneBarEach = [] { SetRam8(At(0xFF80A4), 0); }; // (the board's damage gauges: 0 is one; board.cpp)
    d.canTag = CanTag;
    d.tagIn = TagIn;
    d.noEntrances = TagNoEntrances;
    d.difficulty = TagDifficulty;
    d.attackComing = [](Machine &machine, int side) { return AttackInRange(machine.cpu, Player(side)); };
    // (the game's computer plays the tag style and the ring out itself: the modes' tactics, computer.cpp)
    d.computerPlaysModes = true;
    d.difficultyNow = [] { return static_cast<int>(Ram8(At(0xFF80BE))); }; // (the game's copy of the board's, taken as a game begins)
    d.taggedIn = TaggedIn;
    d.offStage = TagOffStage;
    d.untouchable = TagUntouchable;
    d.tagCall = TagCall;
    d.downSound = 0x0540; // (the announcer's at a knock-out: queued about 26 frames after one, every knock-out seen)
    d.speedX = kSpeedX;
    d.speedY = kSpeedY;
    d.floor = kGround;
    d.meter = kStocks; // (the stocks and the gauge being filled, cleared by the setting up)
    d.meterSize = 3;
    battle::Register(d);
    AddHook(At(0x022354), "TagSetUp", TagSetUp); // (livetag.cpp: a newcomer of a swap kept where it was swapped in)
    AddHook(At(0x022966), "ComputerWalkWay", ComputerWalkWay<At(0x02297A)>);
    AddHook(At(0x0229F2), "ComputerWalkWayOn", ComputerWalkWay<At(0x022A06)>);
    battle::RegisterTeamTag(0.25f);
}

} // namespace cps2recomp
