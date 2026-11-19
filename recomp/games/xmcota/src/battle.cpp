// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: the fighting modes of the menu's "Alternative modes" page (survival, the endless battle, the
// ring out) are the toolkit's (fight/battle.h); this says where the game keeps what they need. The ring out's own
// movements at a player's step are the engine's shared code (fight/marvel.h, at the hook training.cpp places).
#include "alternative.h"
#include "battle.h"
#include "marvel.h"
#include "xmcota.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace xmcota;

    // The round's result as the modes want it: 1 player 1 has won, 2 player 2, 3 neither; 0 while it is fought
    uint8_t Result() {
        if (Ram8(kFight) != kDecided) {
            return 0;
        }
        const uint8_t winner = Ram8(kRoundOver);
        return winner == 0 ? 1 : winner == 1 ? 2 : 3;
    }
} // namespace

void RegisterBattle() {
    battle::Description d;
    d.resultNow = Result;
    d.cameraXAt = marvel::CameraXAt;
    d.cameraYAt = marvel::CameraYAt;
    d.cameraEnds = [](int &left, int &right) {
        left = static_cast<int16_t>(Ram16(CameraRecord() + kCameraLeft));
        right = static_cast<int16_t>(Ram16(CameraRecord() + kCameraRight));
    };
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.playerRecords = 2;
    d.active = kActive;
    d.x = kX;
    d.y = kY;
    d.character = kCharacter + 1; // (the low byte of the word)
    d.computer = kHuman;
    d.computerIsZero = true;
    d.control = kControl + 1;
    d.controlOwn = 0;
    d.health = kHealth;
    d.healthCeiling = kHealthTop;
    d.healthFull = kHealthFull;
    d.healthBack = 0x18; // (a sixth of it all)
    d.colours = kColourChoice;
    d.stages = kStages;
    d.stageToRound = 2300;
    d.characterSpan = 2 * kCharacters;
    d.choosable = [](int number) { return (number & 1) == 0 && number < 2 * kCharacters; };
    d.rosterNumber = [](int choice) { return choice >= 1 && choice <= kCharacters ? (choice - 1) * 2 : -1; };
    d.fighterChoices = {"Random"};
    d.fighterChoices.insert(d.fighterChoices.end(), std::begin(kCharacterNames), std::end(kCharacterNames));
    d.inGame = [] { return Ram8(kPlaying) != 0; };
    d.choosing = modes::Choosing;
    d.toRound = modes::ToRound;
    d.playSound = modes::PlaySound;
    d.wanted = modes::Wanted;
    d.cameraAt = modes::CameraAt;
    d.endModes = modes::End;
    d.stageViewer = modes::StageViewer;
    d.wideSide = marvel::WideSide;
    d.wideMargin = marvel::WideMargin;
    d.wideEnds = marvel::WideEnds;
    d.notTheStage = NotTheStage;
    d.thePanel = ThePanel;
    d.notTheFighters = [](uint32_t tile, uint16_t attributes) { return ThePanel(tile, attributes) || (attributes & 0x1F) >= kStageSets; };
    // (a player's colour sets: its four)
    d.playerSets = [](int n) { return 0xFu << (n == 0 ? kPlayer1Set : kPlayer2Set); };
    d.pictureLeft = kPictureLeft;
    d.yDown = true;
    d.fixedHalfWidth = kWall; // (the game keeps a player's middle this far inside the picture's edges: 00A302)
    d.inAir = marvel::InAir;
    d.atJumpTop = [](uint32_t player) { return marvel::InAir(player) && static_cast<int32_t>(Ram32(player + kSpeedY)) >= 0; };
    battle::Register(d);
}

} // namespace cps2recomp
