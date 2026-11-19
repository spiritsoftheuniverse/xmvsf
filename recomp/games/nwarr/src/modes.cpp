// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's
// (fight/alternative.h); this says where the game keeps what they need, which parts of its show are which, and makes the
// match a mode wants (its characters and stage) where the game makes one. As Pocket Fighter's.
#include "alternative.h"
#include "nwarr.h"
#include "session.h"

#include "all_guests.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace nwarr;

    // The show's loop (kMode, traced with no coin in; family.h). Night Warriors': 2 the QSound notice, 4 the moon, 6 the
    // introduction to the title (its last pictures), 8-0C between, 0E-14 four short fights, 16 the Best 5 ranking,
    // 18-1A, 0 and round again (pictures every 200 frames beside the words)
    constexpr uint16_t kLoopStart = 0x02;

    // The parts of the loop left out for what is to be shown
    bool AttractStep(int show) {
        const uint16_t part = Ram16(kMode);
        if (part <= family::kShowUnseen) {
            return true; // (the loop's own start)
        }
        if (show == modes::kShowIntro) {
            if (part < family::kIntroFirst || part > family::kIntroLast) {
                modes::GoTo(family::kIntroFirst);
            }
        } else if (part < family::kFightFirst || part > family::kFightLast) {
            modes::GoTo(family::kFightFirst);
        }
        return false;
    }

    // A game's own routine is running (02527C): its words say a round is being fought, or the players choose
    bool InGame() {
        return Ram8(kPlayersIn) != 0 && Ram16(kMode) == kGameRunning;
    }

    // 0064D6: move.w d1,0xD88(a5), the next stage (and, in a game against the computer, the next opponent, whose
    // stage it is). A mode's stage is put in its place.
    bool StageSet(M68K &c) {
        // (not in the board's own game against the computer: there the word is the next opponent too, so the player's
        // stage, the menu's Stage page, is left out; another game's is drawn behind such a match all the same)
        if (Ram8(kPlayersIn) != 3 && !modes::Wanted().stage && !BattleMode() && !session::InSession()) {
            return false;
        }
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            c.d[1] = (c.d[1] & 0xFFFF0000) | static_cast<uint32_t>(stage - 1);
        } else if (stage > kStages) {
            c.d[1] &= 0xFFFF0000; // (another game's: fought on the first, its stage drawn behind: stages.cpp)
        }
        return false;
    }

    // 006502: the stage of a two-player match (A5+DB0, less one: a word to kStage, and back). A mode's stage is put in
    // its place.
    bool VersusStageSet(M68K &c) {
        const int stage = session::WantedStage();
        if (stage < 1) {
            return false;
        }
        // (another game's: fought on the first, its stage drawn behind: stages.cpp)
        SetRam16(kStage, static_cast<uint16_t>(stage <= kStages ? stage - 1 : 0));
        Return(c);
        return true;
    }

    // 00692E: a player is made for the fight from the character it chose (A6 the record; 006934 copies kChoice to
    // kCharacter). A mode's characters are put in the choice first.
    bool PlayerMade(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
        if (n < 0) {
            return false;
        }
        const int wanted = modes::Wanted().characters[n];
        if (wanted >= 1 && wanted < kCharacters) {
            SetRam8(player + kChoice, static_cast<uint8_t>(wanted));
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
    // (its stages, then the other games': stages.cpp, fight/all_guests.h; each plays the first stage's tune in its place)
    d.stageNames.assign(std::begin(kStageNames), std::end(kStageNames));
    const std::vector<const char *> others = guests::NamesBut(family::kGuestId);
    d.stageNames.insert(d.stageNames.end(), others.begin(), others.end());
    d.infiniteStages = kStages;
    d.stageTunes.assign(std::begin(family::kStageTunes), std::end(family::kStageTunes)); // (family.h)
    d.stageTunes.resize(d.stageNames.size(), family::kStageTunes[0]);
    d.otherTuneNames = {"Character select", "Versus"};
    d.otherTunes = {family::kTuneSelect, family::kTuneVersus};
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.notTheStage = NotTheStage;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideForSpare = WideForSpare;
    d.wideForPort = WideForPort;
    d.attractBegun = [](int frames) { return frames > 500 && Ram16(kMode) >= kLoopStart; };
    d.attractOver = [] { return Ram8(kPlayersIn) != 0 || Ram8(kCredits) != 0; };
    d.attractStep = AttractStep;
    d.attractWords = "The game's own show on a loop, with no coin in: the introduction, the title, fights and the ranking.";
    d.startTraining = session::StartSession;
    d.modesWords = "Other ways to run the game: training, attract mode, a viewer for the stages, survival, endless battle, ring out.";
    modes::Register(d);
    AddHook(HookAt(kStageSet), "StageSet", StageSet);
    AddHook(HookAt(At(0x006502)), "VersusStageSet", VersusStageSet);
    AddHook(HookAt(At(0x00692E)), "PlayerMade", PlayerMade);
}

} // namespace cps2recomp
