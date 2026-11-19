// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: the game described to the toolkit's code for the engine of Capcom's Marvel games
// (fight/marvel.h): the training aids' view of the players and their boxes, the ring out's movements, widescreen and
// the model viewer are done there, from where this game keeps what they need (mshvsf.h).
#include "marvel.h"
#include "mshvsf.h"

#include <iterator>


// Widescreen at each stage's ends (by the stage byte; fight/wide_ends.h, wide::Stage): how much further in than 64
// the camera stops for the plane the players stand in (the scroll screen going fastest with the camera) and its floor,
// at most 36; and the other scroll screens' places across with the camera at the board's ends, each a far plane drawn
// moved ({0, 0}: it goes with the view, as one going as fast as the players' plane does). Measured by
// tools/stage_ends.sh with every camera kept (kCameraKept).
static constexpr cps2recomp::wide::Stage kWideStages[] = {
    {{0, 0}, {{24, 360}, {0, 0}, {24, 360}}}, // 1
    {{0, 0}, {{0, 128}, {0, 0}, {64, 320}}}, // 2
    {{0, 0}, {{16, 112}, {0, 0}, {96, 288}}}, // 3
    {{0, 0}, {{16, 112}, {96, 288}, {0, 0}}}, // 4
    {{0, 0}, {{0, 0}, {0, 0}, {96, 288}}}, // 5
    {{0, 0}, {{96, 288}, {0, 0}, {0, 0}}}, // 6
    {{10, 0}, {{24, 360}, {0, 0}, {12, 372}}}, // 7
    {{15, 15}, {{492, 660}, {0, 0}, {96, 288}}}, // 8
    {{0, 0}, {{328, 568}, {0, 0}, {0, 0}}}, // 9
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 10
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 11
    {{15, 15}, {{492, 660}, {0, 0}, {864, 1056}}}, // 12
};
namespace cps2recomp {

using namespace mshvsf;

void DescribeEngine() {
    marvel::Engine e;
    e.player1 = kPlayer1;
    e.playerSize = kPlayerSize;
    e.records = 4; // (the first characters, then the second ones)
    e.playerAt = Player; // (the one on point of each side)
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
    // (the model viewer: the players' characters first, then the hidden ones, then the last opponents and Norimaro)
    for (int n = 0; n < kCharacters; n++) {
        if (Choosable(2 * n)) {
            e.viewerOrder.push_back(n);
        }
    }
    e.viewerOrder.insert(e.viewerOrder.end(), {kCyberAkuma / 2, kApocalypse / 2, kNorimaro / 2});
    e.cameraRecord = CameraRecord;
    e.wideStages = kWideStages;
    e.wideStageCount = static_cast<int>(std::size(kWideStages));
    for (const KeptAt &kept : kCameraKept) {
        e.keepCameraPlaces.push_back({kept.at, kept.after});
    }
    e.cameraX = kCameraX;
    e.cameraY = kCameraY;
    e.cameraLeft = kCameraLeft;
    e.cameraRight = kCameraRight;
    e.cameraTop = kCameraTop;
    // (the camera's step works on a copy of its place: kept between its ends across at 0A55AC, then down at 0A55D0,
    // each copy put back after, at 0A55C4 and 0A55E8)
    e.cameraWorkX = kCameraWorkX;
    e.cameraWorkY = kCameraWorkY;
    e.keepCameraDown = HookAt(0x0A55D0);
    e.keptAcross = HookAt(0x0A55C4);
    e.keptDown = HookAt(0x0A55E8);
    e.pictureLeft = kPictureLeft;
    e.wall = kWall;
    e.planes = 0x021FAE; // (longs by a character's byte +30: the camera's word of the plane it stands in; 011BC4)
    e.ground = Ground;
    e.cameraEnds = CameraEnds;
    e.stage = kStage;
    e.cameraWay = kCameraWay;
    e.followsOne = kCameraFollowsOne;
    e.cameraWayTwice = true; // (0A1C64 goes by the byte two after it when a point character's +207 says)
    e.clock = kRoundClock;
    e.playing = [] { return Ram8(kPlaying) != 0; };
    e.inFight = [] { return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameMatch && Ram8(kFight) != 0; };
    e.notTheStage = NotTheStage;
    e.thePanel = ThePanel;
    e.player1Set = kPlayer1Set;
    e.player2Set = kPlayer2Set;
    e.shadowSet = kShadowSet;
    e.palettes = kPalettesInSet;
    // (the animations: groups of words of offsets from a list a character at 137476, which 011762 starts one from;
    // frames of 0x10 bytes, copied into the record by 001E08)
    e.animationLists = 0x137476;
    e.frameBytes = 0x10;
    e.groups = 26;
    e.perGroup = 96;
    e.copyFrame = marvel::CopyFrameBoxSets; // (001E08)
    marvel::Describe(e);
}

} // namespace cps2recomp
