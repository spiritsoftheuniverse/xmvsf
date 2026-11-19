// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: the "Alternative modes" page, attract mode and the stage viewer are the toolkit's
// (fight/alternative.h); this says where the game keeps what they need. Its steps are not the Alpha games' four words
// (xmvsf.h), so it says itself when a round is fought, the players are choosing and a game is on.
#include "alternative.h"
#include "marvel.h"
#include "session.h"
#include "state.h"
#include "xmvsf.h"
#include "marvel_stages.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace xmvsf;

    // Which part of the show's loop is on (xmvsf.h, kShowStep): the rest (notices, rankings), the introduction, the
    // fight after the title; and whether the title has been placed since the introduction began
    enum Part : uint8_t { kPartRest, kPartIntro, kPartFight };
    struct Show {
        Part part = kPartRest;
        bool title = false;
    } g_show;

    bool AttractStep(int show) {
        const bool match = Ram16(kGameStep) == kGameMatch;
        if (g_show.part == kPartRest && Ram8(kShowStep) == 2) {
            g_show = {kPartIntro, false};
        } else if (g_show.part == kPartIntro && g_show.title && match) {
            g_show = {kPartFight, false};
        } else if (g_show.part == kPartFight && !match) {
            g_show.part = kPartRest;
        }
        // (the parts not wanted run unseen)
        return show == modes::kShowIntro ? g_show.part != kPartIntro : g_show.part != kPartFight;
    }

    // 00BAEA: the stage of the next match has been taken (both players have chosen) (00BABC: from the table of stages at 088BFA, by a count that
    // goes on from match to match; the computer's last fight is on Apocalypse's, set after this at 00BD06). A mode's
    // or the session's stage is put in its place.
    bool StageChosen(M68K &) {
        const int stage = session::WantedStage();
        if (stage >= 1 && stage <= kStages) {
            SetRam8(kStage, static_cast<uint8_t>(stage - 1));
        } else if (stage > kStages) {
            SetRam8(kStage, 0); // (another game's: fought on the first, its stage drawn behind: stages.cpp)
        }
        // ... and the characters a mode wants (by the game's own word for them, kCharacter), as the players' picks (each
        // round is made from them: 00FFC0)
        const MatchWanted &wanted = modes::Wanted();
        for (int n = 0; n < 2; n++) {
            const int both[2] = {wanted.characters[n], wanted.partners[n]};
            for (int pick = 0; pick < 2; pick++) {
                if (both[pick] >= 0 && both[pick] < 2 * kCharacters) {
                    SetRam16(kPicks + static_cast<uint32_t>(n * 4 + pick * 2), static_cast<uint16_t>(both[pick]));
                }
            }
        }
        return false;
    }
} // namespace

// The show has placed the title (title.cpp's hook)
void AttractTitle() {
    g_show.title = true;
}

void ModesState(Archive &ar) {
    ar.Mark(0x53484F57); // SHOW
    ar.Pod(g_show);
}

void RegisterModes() {
    using namespace xmvsf;
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
    marvel_stages::AddStages(d, "xmvsf", kStages);
    // (the stages have no tunes of their own: a round plays player 2's character's. The viewer offers each character's
    // theme, 0B on: music.cpp)
    static const char *const kThemes[] = {"Wolverine's theme", "Cyclops's theme", "Storm's theme", "Rogue's theme", "Gambit's theme",
                                          "Sabretooth's theme", "Juggernaut's theme", "Magneto's theme", "Apocalypse's theme",
                                          "Ryu's theme", "Ken's theme", "Chun-Li's theme", "Dhalsim's theme", "Zangief's theme",
                                          "M. Bison's theme", "Akuma's theme", "Charlie's theme", "Cammy's theme"};
    for (size_t n = 0; n < std::size(kThemes); n++) {
        d.otherTuneNames.push_back(kThemes[n]);
        d.otherTunes.push_back(static_cast<uint16_t>(0x0B + n));
    }
    d.otherTuneNames.push_back("Player select");
    d.otherTunes.push_back(0x02);
    d.cameraEnds = [](int stage, int &left, int &right) { CameraEnds(stage - 1, left, right); };
    d.apart = 0x100; // (the players either side of the picture's middle: the camera's word + 0x40 + 0xC0)
    d.notTheStage = NotTheStage;
    d.wideSide = marvel::WideSide;
    d.wideMargin = marvel::WideMargin;
    d.wideForSpare = marvel::WideForSpare;
    d.wideForPort = marvel::WideForPort;
    modes::Register(d);
    AddHook(HookAt(0x00BAEA), "StageChosen", StageChosen);
}

} // namespace cps2recomp
