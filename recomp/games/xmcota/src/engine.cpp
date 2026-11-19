// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: the game described to the toolkit's code for the engine of Capcom's Marvel games
// (fight/marvel.h): the training aids' view of the players and their boxes, the ring out's movements, widescreen and
// the model viewer are done there, from where this game keeps what they need (xmcota.h). It is the engine's first game,
// and some of it is its own: a frame is eight bytes, not Marvel Super Heroes' 0x20, and each stage's routine keeps its
// camera between its ends itself.
#include "marvel.h"
#include "xmcota.h"

#include <iterator>


// Widescreen at each stage's ends (by the stage byte; fight/wide_ends.h, wide::Stage): how much further in than 64
// the camera stops for the plane the players stand in (the scroll screen going fastest with the camera) and its floor,
// at most 36; and the other scroll screens' places across with the camera at the board's ends, each a far plane drawn
// moved ({0, 0}: it goes with the view, as one going as fast as the players' plane does). Measured by
// tools/stage_ends.sh.
static constexpr cps2recomp::wide::Stage kWideStages[] = {
    {{0, 0}, {{0, 0}, {0, 0}, {0, 0}}}, // 1
    {{12, 12}, {{0, 0}, {0, 0}, {96, 288}}}, // 2
    {{0, 0}, {{16, 112}, {0, 0}, {0, 0}}}, // 3
    {{0, 0}, {{487, 664}, {0, 0}, {0, 0}}}, // 4
    {{0, 0}, {{0, 0}, {0, 0}, {144, 240}}}, // 5
    {{0, 0}, {{144, 240}, {0, 0}, {96, 288}}}, // 6
    {{28, 28}, {{0, 0}, {339, 556}, {352, 544}}}, // 7
    {{20, 20}, {{51, 332}, {0, 0}, {352, 544}}}, // 8
    {{36, 36}, {{0, 0}, {0, 0}, {32, 128}}}, // 9
    {{36, 36}, {{0, 0}, {0, 0}, {16, 112}}}, // 10
    {{28, 28}, {{16, 112}, {0, 0}, {40, 88}}}, // 11
    {{0, 0}, {{0, 0}, {448, 640}, {1248, 1344}}}, // 12
};
namespace cps2recomp {

namespace {
    using namespace xmcota;

    // A frame put in a player's record as 001806 puts it: the frame is the long of its picture (whose record has the
    // frame's longs after a first: its flags and its boxes' numbers), a word, and its time
    void CopyFrame(Machine &machine, uint32_t player, uint32_t frame) {
        const auto long_ = [&](uint32_t at) { return (static_cast<uint32_t>(machine.Read16(at)) << 16) | machine.Read16(at + 2); };
        const uint32_t picture = long_(frame);
        SetRam32(player + 0x40, picture);
        SetRam16(player + 0x3C, machine.Read16(frame + 0x04));
        const uint32_t from = picture & 0xFFFFFF;
        for (uint32_t at = 0; at < 0x1C; at += 4) {
            SetRam32(player + 0x64 + at, long_(from + 0x04 + at));
        }
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
    e.actionStand = 0x00;
    e.actionWalk = 0x02;
    e.actionJump = 0x04;
    e.actionCrouch = 0x08;
    e.actionGuard = 0x1C;
    e.actionCrouchGuard = 0x1E;
    e.attackPhase = kAttackPhase;
    e.attackPhaseMask = kAttacking;
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
    e.cameraX = kCameraX;
    e.cameraY = kCameraY;
    e.cameraLeft = kCameraLeft;
    e.cameraRight = kCameraRight;
    e.cameraTop = kCameraTop;
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
    e.cameraWayDown = kCameraWayDown;
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
    // (the animations: 0C14EA, a long a character, read where 009ED8 starts one; frames of 8 bytes, 001806: the picture,
    // a word, the time, its top bit saying the frame after is the long that follows)
    e.animationLists = 0x0C14EA;
    e.frameBytes = 0x08;
    e.frameTimeAt = 0x06;
    e.framePictureAt = 0x00;
    e.framePictureMask = 0x00FFFFFF;
    e.groups = 30;
    e.perGroup = 96;
    e.romHigh = 0x400000;
    e.copyFrame = CopyFrame;
    marvel::Describe(e);
}

} // namespace cps2recomp
