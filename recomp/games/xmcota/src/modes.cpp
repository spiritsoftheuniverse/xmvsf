// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's
// (fight/alternative.h); this says where the game keeps what they need. Its steps are not the Alpha games' four words
// (xmcota.h), so it says itself when a round is fought, the players are choosing and a game is on.
#include "alternative.h"
#include "marvel.h"
#include "session.h"
#include "state.h"
#include "xmcota.h"
#include "marvel_stages.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace xmcota;

    // Which part of the show's loop is on: the rest (notices, rankings), the introduction, the fight after the title;
    // and whether the title has been begun since the introduction began
    enum Part : uint8_t { kPartRest, kPartIntro, kPartFight };
    struct Show {
        Part part = kPartRest;
        bool title = false;
    } g_show;

    bool AttractStep(int show) {
        const bool match = Ram16(kGameStep) == kGameMatch;
        if (g_show.part == kPartRest && Ram8(kShowScene) != 0) {
            g_show = {kPartIntro, false};
        } else if (g_show.part == kPartIntro && g_show.title && match) {
            g_show = {kPartFight, false};
        } else if (g_show.part == kPartFight && !match) {
            g_show.part = kPartRest;
        }
        // (the parts not wanted run unseen)
        return show == modes::kShowIntro ? g_show.part != kPartIntro : g_show.part != kPartFight;
    }

    // 0047AE: the match is about to begin, its stage taken (by the character the player faces: 0045CC, or by the
    // two-player game's roulette: 00468A). A mode's or the session's stage is put in its place, and the characters a mode
    // wants (by the game's own word for them, kCharacter) in the players' records, which the round is made from.
    bool StageChosen(M68K &) {
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            SetRam8(kStage, static_cast<uint8_t>(stage - 1));
        } else if (stage > kStages) {
            SetRam8(kStage, 0); // (another game's: fought on the first, its stage drawn behind: stages.cpp)
        }
        const MatchWanted &wanted = modes::Wanted();
        for (int n = 0; n < 2; n++) {
            if (wanted.characters[n] >= 0 && wanted.characters[n] < 2 * kCharacters) {
                SetRam16(Player(n) + kCharacter, static_cast<uint16_t>(wanted.characters[n]));
            }
            if (wanted.colours[n] >= 0) {
                SetRam8(Player(n) + kColourChoice, static_cast<uint8_t>(wanted.colours[n]));
            }
        }
        return false;
    }
} // namespace

// Before every frame: a match on Avalon with no Magneto in it (only a mode or a training session puts one there) begins
// as if he had come, as a challenger's joining there has it begin (xmcota.h, kMagnetoCame)
void ModesBeforeFrame(Machine &) {
    if (Ram8(kPlaying) && Ram16(kGameStep) == kGameMatch && Ram8(kStage) == kAvalon && Ram16(Player(0) + kCharacter) != kMagneto &&
        Ram16(Player(1) + kCharacter) != kMagneto) {
        SetRam8(kMagnetoCame, 0xFF);
    }
}

// The show has begun its title (title.cpp's hook)
void AttractTitle() {
    g_show.title = true;
}

void ModesState(Archive &ar) {
    ar.Mark(0x53484F57); // SHOW
    ar.Pod(g_show);
}

void RegisterModes() {
    modes::Description d;
    d.roundFoughtNow = [] { return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameMatch && Ram8(kFight) == kFighting; };
    d.choosingNow = [] { return Ram8(kPlaying) != 0 && Ram16(kGameStep) == kGameChoosing && Ram8(kPlayersIn) == 3; };
    d.running = [] { return Ram8(kPlaying) != 0; };
    d.credits = kCredits;
    d.playersIn = kPlayersIn;
    d.soundQueue = kSoundQueue;
    d.soundQueueEnd = kSoundQueueEnd;
    d.soundQueueMask = kSoundQueueMask;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.xLong = true;
    d.attractBegun = [](int frames) {
        g_show = Show();
        return frames > 100;
    };
    d.attractStep = AttractStep;
    d.attractWords = "The game's own show on a loop, with no coin in: the introduction, the title, a fight and the rankings.";
    d.attractOver = [] { return Ram8(kPlaying) != 0 || Ram8(kCredits) != 0; };
    d.startTraining = session::StartSession;
    d.modesWords = "Other ways to run the game: training, attract mode, viewers for the stages and the characters, and the "
                   "colour editor.";
    // (its stages, then the other games': stages.cpp, fight/marvel_stages.h)
    d.stageNames.assign(std::begin(kStageNames), std::end(kStageNames));
    marvel_stages::AddStages(d, "xmcota", kStages);
    d.cameraEnds = [](int stage, int &left, int &right) { CameraEnds(stage - 1, left, right); };
    d.apart = 0x100; // (the players either side of the picture's middle: the camera's word + 0x40 + 0xC0)
    d.notTheStage = NotTheStage;
    d.wideSide = marvel::WideSide;
    d.wideMargin = marvel::WideMargin;
    d.wideForSpare = marvel::WideForSpare;
    d.wideForPort = marvel::WideForPort;
    modes::Register(d);
    AddHook(HookAt(0x0047AE), "StageChosen", StageChosen);
}

} // namespace cps2recomp
