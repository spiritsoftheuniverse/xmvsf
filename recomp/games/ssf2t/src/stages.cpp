// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Other games' stages behind Super Turbo's fighters, as those games show them (fight/guest_stages.h): every other fighting
// game's in the collection, in fight/all_guests.h's order (game.cfg's guests; each one's facts in its own folder's
// guest.h). Here is what is this game's: its camera, when a stage is shown, its objects that are not its stage's, its
// fights' tunes, and which stage is wanted.
//
// Which: a stage chosen by name (the menu's Stage page's, the stage viewer's: the guests'
// after this game's sixteen), or, with the "Other games' stages" feature, an arcade match's stage is one of theirs, the
// same one for the same match. Under one of theirs the game fights on its first, Ryu's (modes.cpp, PutStage).
#include "guest_stages.h"
#include "alternative.h"
#include "session.h"
#include "ssf2t.h"

#include "all_guests.h"

namespace cps2recomp {

bool BattleMode(); // (fight/battle.h)
int BattleStage();

namespace {
    using namespace ssf2t;

    // A match's stage: from a round's introduction to its end (its versus screen is the game's own)
    bool InMatch() {
        return Ram16(kMode) == kModeGame && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) >= 6 && Ram16(kMatchStep) <= 0x10;
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
        const int from = (Ram16(kStage) * 7 + Ram8(Player(0) + kCharacter) * 3 + Ram8(Player(1) + kCharacter)) % stages;
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
    host.shown = [] { return modes::StageViewer() || InMatch(); };
    host.viewer = modes::StageViewer;
    // (what is laid over another game's stage: the fighters, their shadows and the panel; not this game's own stage's
    // pieces; the panel over the other stage's pieces in front of the fighters)
    host.notTheStage = NotTheStage;
    host.panel = ThePanel;
    host.stageTunes.assign(std::begin(kStageTunes), std::end(kStageTunes));
    host.wanted = Wanted;
    host.feature = "other_stages";
    guest_stages::Register(host, guests::AllBut("ssf2t"));
}

} // namespace cps2recomp
