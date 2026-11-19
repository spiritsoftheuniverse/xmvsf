// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's
// (fight/alternative.h); this says where the game keeps what they need, which parts of its loop are which, and makes
// the match a mode wants (its characters and stage) where the game makes one.
//
// The game's loop (kMode, traced with no coin in; the steps between are fades): 0C and 0E the notices, 10 the
// introduction (the title at its end), 14 the title and a demonstration fight (kMatchStep 2 while it is fought), 18
// the score rankings, 1A-24 more rankings and notices, 26 the advisory, 2A another demonstration fight, 2E the rankings
// again, then 0C and on.
#include "alternative.h"
#include "session.h"
#include "sfa3.h"

#include "all_guests.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace sfa3;

    enum : uint16_t { kLoopStart = 0x0C, kIntro = 0x10, kFirstFight = 0x14, kSecondFight = 0x2A, kLoopEnd = 0x32 };
    // The loop's own counts: a byte stepped by two at the end of each half of the loop (0043EC: after the first fight's
    // rankings, 1A, and at the loop's end), whose bit 1 sends the loop back to its start, and beside it another stepped
    // with it (back to 0 past 0A), which picks a page of the show (025886). Found as the only bytes stepping alike
    // between RAM dumps before each fight of three loops; --watch for their writer. The demonstration fights' fighters
    // are drawn at random (004B3A: never the same as the last), so no count picks them.
    constexpr uint32_t kHalves = 0xFF0E76, kAttractPage = 0xFF0E77;

    // A half of the loop left out: its end's stepping done as the game does it, so the loop's counts are the board's
    void HalfDone() {
        SetRam8(kHalves, static_cast<uint8_t>(Ram8(kHalves) + 2));
        const uint8_t page = static_cast<uint8_t>(Ram8(kAttractPage) + 2);
        SetRam8(kAttractPage, page > 0x0A ? 0 : page);
    }

    // The parts of the loop left out for what is to be shown
    bool AttractStep(int show) {
        const uint16_t part = Ram16(kMode);
        if (part < kLoopStart) {
            return true; // (the loop's own start)
        }
        if (show == modes::kShowIntro) {
            if (part < kIntro || part > kIntro + 2) {
                modes::GoTo(kIntro);
            }
        } else if (part < kFirstFight) {
            modes::GoTo(kFirstFight);
        } else if (part > kFirstFight + 2 && part < kSecondFight) {
            HalfDone();
            modes::GoTo(kSecondFight);
        } else if (part > kSecondFight + 2) {
            HalfDone();
            modes::GoTo(kFirstFight);
        }
        return false;
    }

    // 00B7EE: the match has been made (00B778: the computer's next opponent for one player, or the two players' own
    // characters), the stage about to be taken from a character. The characters and colours a mode wants are put in
    // place.
    bool MatchMade(M68K &) {
        const MatchWanted &wanted = modes::Wanted();
        for (int n = 0; n < 2; n++) {
            if (wanted.characters[n] >= 0 && wanted.characters[n] < kCharacters) {
                SetRam8(Player(n) + kCharacter, static_cast<uint8_t>(wanted.characters[n]));
            }
            if (wanted.colours[n] >= 0) {
                SetRam8(Player(n) + kColourChoice, static_cast<uint8_t>(wanted.colours[n]));
            }
        }
        return false;
    }

    // 0090B8: a round is set up, the stage taken from the one the match making chose (A5+174, which the match making
    // sets more than one way). The one wanted (a mode's, the training session's) put in its place first.
    bool StageTaken(M68K &) {
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            SetRam16(kStageNext, static_cast<uint16_t>(kStageNumbers[stage - 1] * 2));
        } else if (stage > kStages) {
            SetRam16(kStageNext, 0); // (another game's: fought on Ryu's, its stage drawn behind: stages.cpp)
        }
        return false;
    }

    // 003AB2: the fight's tune has been taken (D1), a character's theme (003A30). The game has no tune of a stage's own:
    // in the stage viewer the stage's own is its character's (sfa3.h, kStageTunes), whoever stands on it unseen.
    bool FightTune(M68K &c) {
        const int stage = session::WantedStage();
        if (modes::StageViewer() && stage >= 1 && stage <= kStages) {
            SetLow8(c.d[1], static_cast<uint8_t>(kStageTunes[stage - 1]));
        }
        return false;
    }
} // namespace

bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    // (and the faces beside the bars, in the players' own colour sets: tiles 16C00-177FF, which no fighter's picture uses)
    return (set >= 0x15 && set <= 0x18) || (set >= 0x1A && set <= 0x1F) || (tile >= 0x16C00 && tile < 0x17800);
}
// (a stage's own objects: colour sets 10-13, and 0F, which the stages use too (Ryu's castle has two pieces in it, entries
// in the middle of the castle's, and 22 of the 26 stages draw something in it); the fighters have 00-09, each player five,
// their effects in their own; the panel 15-1F)
bool IsStagePiece(uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return set == 0x0F || (set >= 0x10 && set <= 0x13);
}
bool NotTheStage(uint32_t, uint16_t attributes) {
    return !IsStagePiece(attributes);
}

void RegisterModes() {
    modes::Description d;
    d.steps = kMode;
    d.gameChoosing = kGameChoosing;
    d.gameMatch = kGameMatch;
    d.matchRound = kMatchRound;
    d.roundFought = kRoundFought;
    d.credits = kCredits;
    d.playersIn = kPlayersIn;
    d.soundQueue = kSoundQueue;
    d.soundQueueEnd = kSoundQueueEnd;
    d.tuneOff = kTuneOff;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    // (this game's stages, then the other games': stages.cpp, fight/all_guests.h. Not in the Infinite, whose stages are
    // this game's spare machines'; each plays the first stage's tune in this game's place: its own plays)
    d.stageNames.assign(std::begin(kStageNames), std::end(kStageNames));
    const std::vector<const char *> others = guests::NamesBut("sfa3");
    d.stageNames.insert(d.stageNames.end(), others.begin(), others.end());
    d.infiniteStages = kStages;
    d.stageTunes.assign(std::begin(kStageTunes), std::end(kStageTunes));
    d.stageTunes.resize(d.stageNames.size(), kStageTunes[0]);
    d.otherTuneNames = {"Character select", "Versus"};
    d.otherTunes = {kTuneChoose, kTuneVersus};
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.notTheStage = NotTheStage;
    // (the stage's pieces are objects too: the castle, the grass, the trees)
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideForSpare = WideForSpare;
    d.wideForPort = WideForPort;
    // (the board starts up through steps 2 to 8 and then begins its loop; a coin in or a game begun ends it)
    d.attractBegun = [](int frames) { return frames >= 480; };
    d.attractOver = [] { return Ram8(kPlayersIn) != 0 || Ram8(kCredits) != 0; };
    d.attractStep = AttractStep;
    d.attractWords = "The game's own show on a loop, with no coin in: the introduction, the title, demonstration fights, the "
                     "rankings.";
    d.attractShowWords = "What attract mode plays: everything the board does, the introduction over and over, or one "
                         "demonstration fight after another.";
    d.startTraining = session::StartSession;
    d.modesWords = "Other ways to run the game: training, attract mode, viewers for the stages and the characters, the colour "
                   "editor, survival, endless battle, ring out, the tag team battle.";
    modes::Register(d);
    AddHook(HookAt(0x00B7EE), "MatchMade", MatchMade);
    AddHook(HookAt(0x0090B8), "StageTaken", StageTaken);
    AddHook(HookAt(0x003AB2), "FightTune", FightTune);
}

} // namespace cps2recomp
