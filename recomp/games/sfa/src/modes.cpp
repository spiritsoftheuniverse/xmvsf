// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's
// (fight/alternative.h); this says where the game keeps what they need, which parts of its loop are which, and makes
// the match a mode wants (its characters and stage) where the game makes one.
//
// The game's loop has no fights of its own beside its introduction: the introduction is two demonstration fights
// with pictures between them and the title at its end, played twice in a loop with other fighters (kIntro,
// kSecondIntro). "The intro only" leaves the fights out of it; "the fights only" plays the four fights one after
// another and nothing else.
#include "alternative.h"
#include "session.h"
#include "sfa.h"

#include "all_guests.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace sfa;

    // Which demonstration fights the introduction shows: a byte of pairs (0, 2, 4, 6), stepped on by two as each
    // introduction begins (001CFE), the fight of the pair in another (0 or 4, turned over after each fight: 002964).
    // 002A52 takes the two fighters and the stage from a table by them.
    constexpr uint32_t kAttractPair = 0xFF0CEA;

    // The introduction's step moved on to `step`, as the game moves it (the steps under it cleared)
    void IntroTo(uint16_t step) {
        SetRam16(kGameStep, step);
        for (uint32_t at = kGameStep + 2; at < kMode + 0x10; at += 2) {
            SetRam16(at, 0);
        }
    }

    // The parts of the loop left out for what is to be shown
    bool AttractStep(int show) {
        const uint16_t part = Ram16(kMode), step = Ram16(kGameStep);
        if (part < 0x0C) {
            return true; // (the loop's own start)
        }
        const bool intro = part == kIntro || part == kSecondIntro;
        if (show == modes::kShowIntro) {
            if (!intro) {
                modes::GoTo(kIntro);
            } else if (Ram16(kMatchStep) == 0 && (step == kIntroFight || step == kIntroFight2)) {
                IntroTo(static_cast<uint16_t>(step + 2)); // (its fights left out)
            }
        } else if (!intro) {
            modes::GoTo(part < kSecondIntro ? kSecondIntro : kIntro);
        } else if (Ram16(kMatchStep) == 0 && (step == kIntroPictures || step >= kIntroMore)) {
            // (the pictures between the fights left out; after the second fight the other introduction's)
            if (step == kIntroPictures) {
                IntroTo(kIntroFight2);
            } else {
                // (the next pair of fighters, as the left-out part of the loop would have stepped it)
                SetRam8(kAttractPair, static_cast<uint8_t>((Ram8(kAttractPair) + 2) & 6));
                modes::GoTo(part == kIntro ? kSecondIntro : kIntro);
            }
        }
        return false;
    }

    // 006FD0: the match is being made (006F6E: the computer's next opponent for one player, or the two players' own
    // characters), the stage about to be taken from the second player's character (A1) as the routine's last step.
    // What a mode wants is put in place: the characters, their colours, and the stage; and that last step is done here.
    bool MatchMade(M68K &c) {
        const MatchWanted &wanted = modes::Wanted();
        for (int n = 0; n < 2; n++) {
            if (wanted.characters[n] >= 0 && wanted.characters[n] < kCharacters) {
                SetRam8(Player(n) + kCharacter, static_cast<uint8_t>(wanted.characters[n]));
            }
            if (wanted.colours[n] >= 0) {
                SetRam8(Player(n) + kColourChoice, static_cast<uint8_t>(wanted.colours[n]));
            }
        }
        const int stage = session::WantedStage();
        if (stage < 1 || stage > kStages) {
            return false; // (the game's own: the second player's character's)
        }
        // (006FD0-006FDC: the stage word from A1's character, as the game works it out, but the one wanted put in)
        SetLow16(c.d[1], static_cast<uint16_t>(static_cast<int8_t>(Ram8((c.a[1] & 0xFFFFFF) + kCharacter)) * 2));
        SetRam16(kStage, static_cast<uint16_t>((stage - 1) * 2));
        Return(c);
        return true;
    }

    // 0070A0: the end of a one-player game's next opponent being chosen (00702E), its stage taken from its character
    // (00709C): the matches after the first, which do not come by MatchMade's place. The player's stage (the menu's
    // Stage page) is put in its place: the opponent stays.
    bool NextStageChosen(M68K &) {
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            SetRam16(kStage, static_cast<uint16_t>((stage - 1) * 2));
        }
        return false;
    }
} // namespace

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
    const std::vector<const char *> others = guests::NamesBut("sfa");
    d.stageNames.insert(d.stageNames.end(), others.begin(), others.end());
    d.infiniteStages = kStages;
    d.stageTunes.assign(std::begin(kStageTunes), std::end(kStageTunes));
    d.stageTunes.resize(d.stageNames.size(), kStageTunes[0]);
    d.otherTuneNames = {"Character select", "Versus"};
    d.otherTunes = {kTuneChoose, kTuneVersus};
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.notTheStage = NotTheStage; // (sfa.h)
    d.viewerLayers = kLayersWithoutPanel;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideForSpare = WideForSpare;
    d.wideForPort = WideForPort;
    // (the board starts up through steps 2 to 8 and then begins its loop at 0C; a coin in or a game begun ends it)
    d.attractBegun = [](int) { return Ram16(kMode) >= 0x0C; };
    d.attractOver = [] { return Ram8(kPlayersIn) != 0 || Ram8(kCredits) != 0; };
    d.attractStep = AttractStep;
    d.attractWords = "The game's own show on a loop, with no coin in: the introduction and its demonstration fights, the title, "
                     "the rankings.";
    d.attractShowWords = "What attract mode plays: everything the board does, the introduction over and over (without its "
                         "fights), or one demonstration fight after another.";
    d.startTraining = session::StartSession;
    d.modesWords = "Other ways to run the game: training, attract mode, viewers for the stages and the characters, the colour "
                   "editor, survival, endless battle, ring out.";
    modes::Register(d);
    AddHook(HookAt(0x006FD0), "MatchMade", MatchMade);
    AddHook(HookAt(0x0070A0), "NextStageChosen", NextStageChosen);
}

} // namespace cps2recomp
