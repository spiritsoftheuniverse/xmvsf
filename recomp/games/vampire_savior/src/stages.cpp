// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Other games' stages behind Vampire Savior's fighters (and Vampire Savior 2's, Vampire Hunter 2's), as those games show
// them (fight/guest_stages.h; game.cfg's guests): every other fighting game's in the collection, each run from its own
// stage code alone, in fight/all_guests.h's order (each one's facts in its own folder's guest.h). Here is what is this
// game's: its camera, when a stage is shown, its objects that are not its stage's, its stage tunes, and which stage is
// wanted. Vampire Savior Definitive, built from these sources too, has its own (games/vsav_definitive/src/stages.cpp:
// its Darkstalkers' stages by the opponent); family.h says which game calls this.
//
// Which: a stage chosen by name (the menu's Stage page's, the stage viewer's: the guests' after this game's twelve), or,
// with the "Other games' stages" feature, an arcade match's stage is one of theirs, the same one for the same match.
#include "guest_stages.h"
#include "modes.h"
#include "session.h"
#include "vsav.h"

#include "all_guests.h"

namespace cps2recomp {

bool BattleMode(); // (fight/battle.h)
int BattleStage();

namespace {
    using namespace vsav;

    // This game's match step (kMatchStep) while its versus screen is up (vsav_definitive/src/stages.cpp)
    constexpr uint16_t kMatchVersus = 0x0A;

    bool InMatch() {
        return Ram16(kMode) == 2 && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) != kMatchVersus;
    }

    // The stage wanted now, 1 the guests' first (Darkstalkers' are first: game.cfg's guests); 0 this game's own
    int Wanted() {
        if (BattleMode()) {
            // (a mode's stages are this game's own, but the player's stage when it is one of the guests': BattleStage)
            const int chosen = BattleStage();
            return chosen > kStages && chosen <= kStages + kMoreStages ? chosen - kStages : 0;
        }
        const int wanted = session::WantedStage();
        if (wanted > kStages && wanted <= kStages + kMoreStages) {
            return wanted - kStages;
        }
        if (wanted >= 1 && wanted <= kStages) {
            return 0;
        }
        if (!FeatureOn("other_stages") || !InMatch()) {
            return 0;
        }
        // (not over a stage of this game's whose camera does something of its own at a round's start: the train's arrival
        // and Fetus of God's shake take the game's camera off where another game's stage can go)
        constexpr int kTrain = 10, kFetusOfGod = 11;
        const int own = Ram16(kStage) / 2;
        if (own == kTrain || own == kFetusOfGod) {
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

void RegisterOtherStages() {
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
    host.shown = [] { return StageViewer() || InMatch(); };
    host.viewer = StageViewer;
    host.panel = ThePanel;
    host.notTheStage = NotTheStage;
    host.stageTunes.assign(std::begin(family::kStageTunes), std::end(family::kStageTunes));
    host.wanted = Wanted;
    host.feature = "other_stages";
    guest_stages::Register(host, guests::AllBut("vampire_savior")); // (fight/all_guests.h: family.h's kMoreStageNames the same)
}

} // namespace cps2recomp

void vsav::family::RegisterOtherStages() {
    cps2recomp::RegisterOtherStages();
}
