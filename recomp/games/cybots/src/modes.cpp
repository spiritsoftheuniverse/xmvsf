// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's (fight/alternative.h);
// this says where the game keeps what they need, which parts of its show are which, and makes the match a mode wants
// (its machines and stage) where the game makes one. As Night Warriors'.
#include "alternative.h"
#include "cybots.h"
#include "session.h"

#include <iterator>

namespace cps2recomp {

void HiddenMachine(int n); // choosing.cpp

namespace {
    using namespace cybots;

    // The show's loop (kMode, traced with no coin in, pictures every 250 frames beside the words): 04-0C a machine shown
    // part by part, 10 a fight, 14-18 the ranking, 1C the introduction to the title, 20-28 another machine, 2C another
    // fight, 30; then 04 again
    constexpr uint16_t kLoopStart = 0x04, kFightFirst = 0x10, kRanking = 0x14, kIntro = 0x1C, kFightSecond = 0x2C, kLoopEnd = 0x30;

    // The parts of the loop left out for what is to be shown
    bool AttractStep(int show) {
        const uint16_t part = Ram16(kMode);
        if (part < kLoopStart) {
            return true; // (the loop's own start)
        }
        if (show == modes::kShowIntro) {
            if (part != kIntro) {
                modes::GoTo(kIntro);
            }
        } else if (part < kFightFirst) {
            modes::GoTo(kFightFirst);
        } else if (part > kFightFirst && part < kFightSecond) {
            modes::GoTo(kFightSecond);
        } else if (part >= kLoopEnd) {
            modes::GoTo(kFightFirst);
        }
        return false;
    }

    // A game's own routine is running (00C3A6)
    bool InGame() {
        return Ram8(kInGame) != 0 && Ram16(kMode) == kGameRunning;
    }

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // 002188: a player is made for the match from its choices (A6); the stage is chosen by now. What a mode wants is
    // put in its place: the machines (the player's own choice, or the computer's player as made) and the stage.
    bool PlayerMade(M68K &c) {
        const int n = PlayerOf(c.a[6]);
        if (n < 0) {
            return false;
        }
        HiddenMachine(n);
        const MatchWanted &wanted = modes::Wanted();
        for (int k = 0; k < 2; k++) {
            const int machine = wanted.characters[k];
            if (machine < 0 || machine >= kMachines) {
                continue;
            }
            const uint32_t player = Player(k);
            if (k == n) {
                SetRam8(player + kMachineChoice, static_cast<uint8_t>(machine)); // (002188 copies it on)
            } else {
                for (uint32_t at = kMachine; at <= kWeapon; at++) {
                    SetRam8(player + at, static_cast<uint8_t>(machine));
                }
            }
        }
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            SetRam8(kStage, kStageNumbers[stage - 1]);
        }
        return false;
    }
} // namespace

void RegisterModes() {
    modes::Description d;
    d.steps = kMode;
    d.roundFoughtNow = [] { return InGame() && Ram16(kStep) == kGameMatch && Ram16(kStep2) == kRoundFought; };
    d.choosingNow = [] { return InGame() && Ram16(kStep) == kGameChoosing && Ram8(kPlayersIn) == 3; };
    d.running = [] { return Ram16(kMode) != 0 || Ram8(kPlayersIn) != 0; };
    d.credits = kCredits;
    d.playersIn = kPlayersIn;
    d.soundQueue = kSoundQueue;
    d.soundQueueEnd = kSoundQueueEnd;
    d.soundQueueMask = kSoundQueueMask;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.stageNames.assign(std::begin(kStageNames), std::end(kStageNames));
    // (each stage's tune, as a two-player fight on it plays it: music.cpp)
    d.stageTunes = {0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1A, 0x1B, 0x1C, 0x13, 0x13, 0x1A, 0x12};
    d.otherTuneNames = {"Pilot and machine select", "The last fight"};
    d.otherTunes = {0x02, 0x32};
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.notTheStage = NotTheStage;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideForSpare = WideForSpare;
    d.wideForPort = WideForPort;
    d.attractBegun = [](int frames) { return frames > 500 && Ram16(kMode) >= kLoopStart; };
    // (over with a credit in or a game begun: the show's own fights have players in too)
    d.attractOver = [] { return Ram8(kInGame) != 0 || Ram8(kCredits) != 0; };
    d.attractStep = AttractStep;
    d.attractWords = "The game's own show on a loop, with no coin in: the machines, the introduction, the title, fights and the "
                     "ranking.";
    d.startTraining = session::StartSession;
    d.modesWords = "Other ways to run the game: training, attract mode, a viewer for the stages, survival, endless battle, ring out.";
    modes::Register(d);
    AddHook(HookAt(kPlayerMade), "PlayerMade", PlayerMade);
}

} // namespace cps2recomp
