// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's
// (fight/alternative.h); this says where the game keeps what they need, which parts of its loop are which, and makes
// the match a mode wants (its characters and stage) where the game makes one.
//
// The game's loop has one fight of its own, a demonstration (a game the computer plays for both players), between its
// introduction and its rankings. "The intro only" leaves everything but the introduction and the title out; "the
// fights only" plays the demonstration over and over.
#include "alternative.h"
#include "session.h"
#include "ssf2t.h"

#include "all_guests.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace ssf2t;

    // The parts of the loop left out for what is to be shown (true: a part was left, the frame not worth seeing)
    bool AttractStep(int show) {
        const uint16_t part = Ram16(kMode);
        if (part == 0 || part >= kLoopEnd) {
            return false; // (the loop's own start)
        }
        const uint16_t wanted = show == modes::kShowIntro ? kIntro : kDemo;
        if (part != wanted) {
            modes::GoTo(wanted);
            return true;
        }
        return false;
    }

    // A round is being fought; the players are choosing, both in
    bool RoundFought() {
        return Ram16(kMode) == kModeGame && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchFought;
    }
    bool Choosing() {
        return Ram16(kMode) == kModeGame && Ram16(kGameStep) == kGameChoosing && Ram8(kPlayersIn) == 3;
    }

    // The stage wanted put in kStage (none: the game's own). Not in the board's own one-player game: there the word
    // is the ladder's place too (the computer's opponent is the stage's character, 00388E, and the game asks which
    // stage it is for the last opponents and Super Street Fighter II's bonus stages), so it stays the game's own;
    // another game's stage is drawn behind such a match all the same (stages.cpp).
    void PutStage() {
        const bool ladder = Ram8(kPlayersIn) != 3 && !modes::Wanted().stage && !BattleMode() && !session::InSession();
        if (ladder) {
            return;
        }
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            SetRam16(kStage, static_cast<uint16_t>(stage - 1));
        } else if (stage > kStages) {
            SetRam16(kStage, 0); // (another game's: fought on Ryu's, its stage drawn behind: stages.cpp)
        }
    }

    // 00D714: the match's first step, the characters chosen. What a mode wants is put in place: the characters (not
    // Akuma, nor old), their colours, and the stage (the choosing has put the game's in kStage: 0039A6).
    bool MatchBegins(M68K &) {
        PutStage();
        const MatchWanted &wanted = modes::Wanted();
        for (int n = 0; n < 2; n++) {
            if (wanted.characters[n] >= 0 && wanted.characters[n] < kCharacters) {
                SetRam8(Player(n) + kCharacter, static_cast<uint8_t>(wanted.characters[n]));
                NotHidden(Player(n));
            }
            if (wanted.colours[n] >= 0) {
                SetRam8(Player(n) + kColourChoice, static_cast<uint8_t>(wanted.colours[n]));
            }
        }
        return false;
    }

    // 0CCC5C: the match's stage is being set up from kStage (again for each round): the one wanted put in first
    bool StageMade(M68K &) {
        PutStage();
        return false;
    }
} // namespace

void RegisterModes() {
    modes::Description d;
    d.steps = kMode;
    d.roundFoughtNow = RoundFought;
    d.choosingNow = Choosing;
    d.credits = kCredits;
    d.playersIn = kPlayersIn;
    d.soundQueue = kSoundQueue;
    d.soundQueueEnd = kSoundQueueEnd;
    d.tuneOff = kTuneOff;
    d.soundQueueMask = kSoundQueueMask;
    d.soundEntry = kSoundEntry;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.xLong = true;
    // (this game's stages, then the other games': stages.cpp, fight/all_guests.h. Not in the Infinite, whose stages are
    // this game's spare machines'; each plays the first stage's tune in this game's place: its own plays)
    d.stageNames.assign(std::begin(kStageNames), std::end(kStageNames));
    const std::vector<const char *> others = guests::NamesBut("ssf2t");
    d.stageNames.insert(d.stageNames.end(), others.begin(), others.end());
    d.infiniteStages = kStages;
    d.stageTunes.assign(std::begin(kStageTunes), std::end(kStageTunes));
    d.stageTunes.resize(d.stageNames.size(), kStageTunes[0]);
    d.otherTuneNames = {"Character select", "Versus"};
    d.otherTunes = {kTuneChoose, kTuneVersus};
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.notTheStage = NotTheStage; // (ssf2t.h)
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideForSpare = WideForSpare;
    d.wideForPort = WideForPort;
    // (the board's start is over at frame 805, and its loop begins; a coin in or a game begun ends attract mode)
    d.attractBegun = [](int frames) { return frames > kBootFrames; };
    d.attractOver = [] { return Ram8(kPlayersIn) != 0 || Ram8(kCredits) != 0; };
    d.attractStep = AttractStep;
    d.attractWords = "The game's own show on a loop, with no coin in: the introduction, the title, a demonstration fight, the "
                     "rankings.";
    d.attractShowWords = "What attract mode plays: everything the board does, the introduction and the title over and over, or the "
                         "demonstration fight over and over.";
    d.startTraining = session::StartSession;
    d.modesWords = "Other ways to run the game: training, attract mode, viewers for the stages and the characters, the colour "
                   "editor, survival, endless battle, ring out.";
    modes::Register(d);
    AddHook(HookAt(kMatchBegins), "MatchBegins", MatchBegins);
    AddHook(HookAt(kStageMade), "StageMade", StageMade);
}

} // namespace cps2recomp
