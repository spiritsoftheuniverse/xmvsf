// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: the game described to the toolkit's code for the engine of Capcom's Marvel games
// (fight/marvel.h): the training aids' view of the players and their boxes, the ring out's movements, widescreen and
// the model viewer are done there, from where this game keeps what they need (xmvsf.h).
#include "marvel.h"
#include "xmvsf.h"

#include <iterator>

// Widescreen at each stage's ends (by the stage byte; fight/wide_ends.h, wide::Stage): how much further in than 64
// the camera stops for the plane the players stand in (the scroll screen going fastest with the camera) and its floor,
// at most 36; and the other scroll screens' places across with the camera at the board's ends, each a far plane drawn
// moved ({0, 0}: it goes with the view, as one going as fast as the players' plane does). Measured by
// tools/stage_ends.sh with every camera kept (kCameraKept).
static constexpr cps2recomp::wide::Stage kWideStages[] = {
    {{0, 0}, {{24, 360}, {0, 0}, {768, 1152}}}, // 1
    {{0, 0}, {{0, 128}, {0, 0}, {64, 320}}}, // 2
    {{0, 0}, {{16, 112}, {0, 0}, {96, 288}}}, // 3
    {{0, 0}, {{16, 112}, {96, 288}, {0, 0}}}, // 4
    {{0, 0}, {{0, 0}, {0, 0}, {96, 288}}}, // 5
    {{0, 0}, {{96, 288}, {0, 0}, {0, 0}}}, // 6
    {{10, 0}, {{24, 360}, {0, 0}, {12, 372}}}, // 7
    {{15, 15}, {{492, 660}, {0, 0}, {96, 288}}}, // 8
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 9
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 10
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 11
};

namespace cps2recomp {

using namespace xmvsf;

void DescribeEngine() {
    marvel::Engine e;
    e.player1 = kPlayer1;
    e.playerSize = kPlayerSize;
    e.records = 4; // (the point characters first, their partners after)
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
    e.planes = 0x01E7EC; // (longs by a character's byte +30: the camera's word of the plane it stands in)
    e.ground = [] { return static_cast<int>(kGround); };
    e.cameraEnds = CameraEnds;
    e.stage = kStage;
    e.cameraWay = kCameraWay;
    e.followsOne = kCameraFollowsOne;
    e.cameraWayTwice = true; // (070232 goes by the byte two after it when a player's +257 says)
    e.clock = kRoundClock;
    e.playing = [] { return Ram8(kPlaying) != 0; };
    e.inFight = [] { return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameMatch && Ram8(kFight) != 0; };
    e.notTheStage = NotTheStage;
    e.thePanel = ThePanel;
    e.player1Set = kPlayer1Set;
    e.player2Set = kPlayer2Set;
    e.shadowSet = kShadowSet;
    e.palettes = kPalettesInSet;
    // (the animations: groups of words of offsets from a list a character at 08A918, which 010380 starts one from;
    // frames of 0x10 bytes, copied into the record by 001AE6)
    e.animationLists = 0x08A918;
    e.frameBytes = 0x10;
    e.groups = 26;
    e.perGroup = 96;
    e.copyFrame = marvel::CopyFrameBoxSets; // (001AE6)
    marvel::Describe(e);
}

} // namespace cps2recomp
