// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: the game described to the toolkit's code for the engine of Capcom's Marvel games
// (fight/marvel.h): the training aids' view of the players and their boxes, the ring out's movements, widescreen and
// the model viewer are done there, from where this game keeps what they need (msh.h).
#include "marvel.h"
#include "msh.h"

#include <iterator>

// Widescreen at each stage's ends (by the stage byte; fight/wide_ends.h, wide::Stage): how much further in than 64
// the camera stops for the plane the players stand in (the scroll screen going fastest with the camera) and its floor,
// at most 36; and the other scroll screens' places across with the camera at the board's ends, each a far plane drawn
// moved ({0, 0}: it goes with the view, as one going as fast as the players' plane does). Measured by
// tools/stage_ends.sh with every camera kept (kCameraKept).
static constexpr cps2recomp::wide::Stage kWideStages[] = {
    {{20, 20}, {{0, 0}, {0, 0}, {144, 240}}}, // 1
    {{0, 0}, {{0, 0}, {0, 128}, {120, 264}}}, // 2
    {{32, 32}, {{96, 288}, {0, 0}, {0, 0}}}, // 3
    {{36, 36}, {{0, 0}, {0, 0}, {0, 0}}}, // 4
    {{0, 0}, {{0, 0}, {272, 368}, {0, 0}}}, // 5
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 6
    {{22, 22}, {{40, 88}, {0, 0}, {0, 0}}}, // 7
    {{0, 0}, {{56, 296}, {0, 0}, {2000, 2480}}}, // 8
    {{0, 0}, {{176, 464}, {0, 128}, {0, 0}}}, // 9
    {{36, 36}, {{144, 240}, {0, 0}, {0, 0}}}, // 10
    {{36, 36}, {{0, 0}, {0, 0}, {52, 76}}}, // 11 (the Infinity Gem chamber: its floor is on the 16-pixel scroll screen)
    {{24, 8}, {{40, 88}, {16, 112}, {0, 0}}}, // 12
};

namespace cps2recomp {

namespace {
    using namespace msh;

    // A frame put in a player's record as 001840 puts it: its picture, the long after the time, and the rest of the
    // frame's longs, which are its boxes' numbers (two attacks, four places it can be hit, one it can be thrown in)
    void CopyFrame(Machine &machine, uint32_t player, uint32_t frame) {
        const auto long_ = [&](uint32_t at) { return (static_cast<uint32_t>(machine.Read16(at)) << 16) | machine.Read16(at + 2); };
        SetRam16(player + 0x3C, machine.Read16(frame));
        SetRam32(player + 0x40, long_(frame + 0x04));
        SetRam32(player + 0x68, long_(frame + 0x08));
        SetRam32(player + 0x6C, long_(frame + 0x0C));
        SetRam32(player + 0x74, long_(frame + 0x10));
        SetRam32(player + 0x78, long_(frame + 0x14));
        SetRam32(player + 0x7C, long_(frame + 0x18));
        SetRam32(player + 0x80, long_(frame + 0x1C));
    }
} // namespace

void DescribeEngine() {
    marvel::Engine e;
    e.player1 = kPlayer1;
    e.playerSize = kPlayerSize;
    e.records = 2;
    e.x = kX;
    e.y = kY;
    e.speedX = kSpeedX;
    e.speedY = kSpeedY;
    e.pull = kPullY;
    e.control = kControl;
    e.frame = kFrame;
    e.flip = kFlip;
    e.character = kCharacter;
    e.action = kAction;
    e.attackPhase = kAttackPhase;
    e.hitStop = kHitStop;
    e.boxTable = kBoxTable;
    e.attackBoxes = kAttackBoxes;
    e.hurtBoxes = kHurtBoxes;
    e.pushBox = kPushBox;
    e.canHit = kCanHit;
    e.canBeHit = kCanBeHit;
    e.pushTable = kPushTable;
    e.thrownList = kThrownList;
    e.thrownListSpace = kThrownListSpace;
    e.panel[0] = kPanel[0];
    e.panel[1] = kPanel[1];
    e.characterNames.assign(std::begin(kCharacterNames), std::end(kCharacterNames));
    e.stageNames.assign(std::begin(kStageNames), std::end(kStageNames));
    e.cameraRecord = CameraRecord;
    e.wideStages = kWideStages;
    e.wideStageCount = static_cast<int>(std::size(kWideStages));
    for (const KeptAt &kept : kCameraKept) {
        e.keepCameraPlaces.push_back({kept.at, kept.after});
    }
    e.pictureLeft = kPictureLeft;
    e.wall = kWall;
    e.planes = kPlanes;
    e.ground = Ground;
    e.cameraEnds = CameraEnds;
    e.stage = kStage;
    e.cameraWay = kCameraWay;
    e.followsOne = kCameraFollowsOne;
    e.clock = kRoundClock;
    e.playing = [] { return Ram8(kPlaying) != 0; };
    e.inFight = [] { return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameMatch && Ram8(kFight) != 0; };
    e.notTheStage = NotTheStage;
    e.thePanel = ThePanel;
    e.player1Set = kPlayer1Set;
    e.player2Set = kPlayer2Set;
    e.shadowSet = kShadowSet;
    e.palettes = kPalettesInSet;
    // (the animations: 09E6FE, a long a character, read where 00DBC8 starts one; frames of 0x20 bytes, 001840)
    e.animationLists = 0x09E6FE;
    e.frameBytes = 0x20;
    e.groups = 30;
    e.perGroup = 96;
    e.romHigh = 0x400000;
    e.copyFrame = CopyFrame;
    marvel::Describe(e);
}

} // namespace cps2recomp
