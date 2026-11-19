// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Other games' stages behind Alpha 2's fighters, as those games show them (fight/guest_stages.h: each run from its own
// stage code alone): every other fighting game's in the collection, in fight/all_guests.h's order (game.cfg's guests; each
// one's facts in its own folder's guest.h). Here is what is this game's: its camera, when a stage is shown, its objects that are
// not its stage's, its stage tunes, and which stage is wanted.
//
// Which: a stage chosen by name (the menu's Stage page's, the stage viewer's: the guests' after this game's eighteen), or,
// with the "Other games' stages" feature, an arcade match's stage is one of theirs, the same one for the same match.
#include "guest_stages.h"
#include "modes.h"
#include "session.h"
#include "sfa2.h"

#include "all_guests.h"

namespace cps2recomp {

bool BattleMode(); // (fight/battle.h)
int BattleStage();
bool StageViewer(); // (modes.cpp)
bool ThePanel(uint32_t tile, uint16_t attributes); // (machines.cpp)

namespace {
    using namespace sfa2;

    constexpr uint32_t kCameraX = At(0xFF8290), kCameraY = At(0xFF8294);

    // A match's stage: shown while a round is on (its versus screen and the choice before are the game's own)
    bool InMatch() {
        return Ram16(kMode) == 2 && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchRound;
    }

    // The stage wanted now, 1 the guests' first (Darkstalkers' are first: game.cfg's guests); 0 this game's own
    int Wanted() {
        if (BattleMode()) {
            // (a mode's stages are this game's own, but the player's stage when it is one of the guests': BattleStage)
            const int chosen = BattleStage();
            return chosen > kStages && chosen <= kStages + guest_stages::Stages() ? chosen - kStages : 0;
        }
        // (a stage chosen by name: training's, the viewer's; after this game's eighteen)
        const int wanted = session::WantedStage();
        if (wanted > kStages && wanted <= kStages + guest_stages::Stages()) {
            return wanted - kStages;
        }
        if (wanted >= 1 && wanted <= kStages) {
            return 0;
        }
        if (!FeatureOn("other_stages") || !InMatch()) {
            return 0;
        }
        // One of theirs, the same for the same match: from the stage the game chose and who is fought there (the
        // players' characters), the first of them whose game's set is there from that one on
        const int stages = guest_stages::Stages();
        const int from = (Ram16(kStage) / 2 * 7 + Ram8(Player(0) + kCharacter) * 3 + Ram8(Player(1) + kCharacter)) % stages;
        for (int n = 0; n < stages; n++) {
            const int stage = (from + n) % stages + 1;
            if (guest_stages::Available(stage)) {
                return stage;
            }
        }
        return 0;
    }
} // namespace

void RegisterStages() {
    Game &game = TheGame();
    // (Display: it changes nothing of the game's play; online, each player sees the stage of their own choice)
    game.features.push_back({"other_stages", "Display", "Other games' stages",
                             "An arcade match is fought on a stage of another game's, as that game shows it, its music too: "
                             "the other fighting games in the collection, those whose ROM sets are there (beside this "
                             "game's, or chosen on their pages in the launcher). The menu's Stage page and the stage "
                             "viewer's list have them all after this game's own.",
                             0});
    guest_stages::Host host;
    host.cameraX = kCameraX;
    host.cameraY = kCameraY;
    host.cameraLeft = 256;
    host.cameraRight = 640;
    host.shown = [] { return StageViewer() || InMatch(); };
    host.viewer = StageViewer;
    // (what is laid over another game's stage: the fighters, in colour sets 11-26, and the panel; not this game's own
    // stage's pieces, its people and its doors, which are in the sets below and above: battle.cpp's NotTheFighters)
    // (its panel, over another game's pieces in front of the fighters: not the fighters' shadows, on the floor's line,
    // tiles 26A00-26AFF, which ThePanel takes in for the modes that lay the fighters over another stage)
    host.panel = [](uint32_t tile, uint16_t attributes) { return ThePanel(tile, attributes) && !(tile >= 0x26A00 && tile < 0x26B00); };
    host.notTheStage = [](uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return ThePanel(tile, attributes) || (set >= 11 && set < 27);
    };
    host.stageTunes.assign(std::begin(kStageTunes), std::end(kStageTunes));
    host.wanted = Wanted;
    host.feature = "other_stages";
    guest_stages::Register(host, guests::AllBut("sfa2"));
}

} // namespace cps2recomp
