// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Other games' stages behind Alpha 3's fighters, as those games show them (fight/guest_stages.h: each run from its own
// stage code alone): every other fighting game's in the collection, in fight/all_guests.h's order (game.cfg's guests; each
// one's facts in its own folder's guest.h). Here is what is this game's: its camera, when a stage is shown, its objects
// that are not its stage's, its fights' tunes, and which stage is wanted.
//
// Which: a stage chosen by name (the menu's Stage page's, the stage viewer's: the guests'
// after this game's twenty-six), or, with the "Other games' stages" feature, an arcade match's stage is one of theirs,
// the same one for the same match. Under one of theirs the game fights on its first, Ryu's (modes.cpp, StageTaken).
#include "alternative.h"
#include "guest_stages.h"
#include "session.h"
#include "sfa3.h"

#include "all_guests.h"

namespace cps2recomp {

bool BattleMode(); // (fight/battle.h)
int BattleStage();

namespace {
    using namespace sfa3;

    // A match's stage: shown while a round is on (its versus screen and the choice before are the game's own)
    bool InMatch() {
        return Ram16(kMode) == 2 && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchRound;
    }

    // The stage wanted now, 1 the guests' first (fight/all_guests.h's order); 0 this game's own
    int Wanted() {
        if (BattleMode()) {
            // (a mode's stages are this game's own, but the player's stage when it is one of the guests': BattleStage)
            const int chosen = BattleStage();
            return chosen > kStages && chosen <= kStages + guest_stages::Stages() ? chosen - kStages : 0;
        }
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
        // One of theirs, the same for the same match: from the stage the game chose and who is fought there
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
    host.cameraLeft = kCameraLeft;
    host.cameraRight = kCameraRight;
    // (the stage viewer's too only once its round is on: on its way there, through the character choice, the camera is
    // somewhere else, 768 up)
    host.shown = InMatch;
    host.viewer = modes::StageViewer;
    // (what is laid over another game's stage: the fighters and the panel; not this game's own stage's pieces)
    host.panel = ThePanel;
    host.notTheStage = [](uint32_t tile, uint16_t attributes) {
        return ThePanel(tile, attributes) || (!IsStagePiece(attributes) && (attributes & 0x1F) < 0x14);
    };
    // (its fights' tunes are its characters' themes: the stages' list of them, sfa3.h)
    host.stageTunes.assign(std::begin(kStageTunes), std::end(kStageTunes));
    host.wanted = Wanted;
    host.feature = "other_stages";
    guest_stages::Register(host, guests::AllBut("sfa3"));
}

} // namespace cps2recomp
