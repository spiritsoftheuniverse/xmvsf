// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Vampire Savior Definitive: other games' stages, as those games show them (fight/guest_stages.h: their boards run
// beside this one, game.cfg's guests). The first Darkstalkers' (dstlk: its engine is Night Warriors' own, not Vampire
// Savior's, so its stages are not taken in as the characters are) and Street Fighter Alpha 2's; each guest's facts are
// in its own game's folder (games/dstlk/guest.h, games/sfa2/guest.h). Here is what is this game's: its camera, when a
// stage is shown, its objects that are not its stage's, its stage tunes, and which stage is wanted.
//
// Which: a stage chosen by name (the Stage page's, the stage viewer's: the guests' after Vampire Savior's twelve), or, with
// the "Darkstalkers' stages" feature, a match's stage is the opponent's own in Darkstalkers, where it has one (player
// 2's in a two-player match; the computer's against it): twelve of the eighteen have.
#include "definitive.h"
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

    // This game's match step (kMatchStep) while its versus screen is up: from the match's start to its first round's
    // (a two-player match's steps, logged: 0A, then 0 and 2 as the round is set up, 4 its round)
    constexpr uint16_t kMatchVersus = 0x0A;

    // A character's (this game's number) stage in Darkstalkers, 1 the first (0: none)
    int DarkstalkersStageOf(uint8_t character) {
        switch (character) {
        case 0x01: return 1;  // Demitri
        case 0x02: return 2;  // Gallon (Jon Talbain)
        case 0x12: return 2;  // ... Dark Gallon
        case 0x03: return 3;  // Victor
        case 0x04: return 4;  // Zabel (Lord Raptor)
        case 0x05: return 5;  // Morrigan
        case 0x06: return 6;  // Anakaris
        case 0x07: return 7;  // Felicia
        case 0x08: return 8;  // Bishamon
        case 0x18: return 8;  // ... Oboro Bishamon
        case 0x09: return 9;  // Aulbath (Rikuo)
        case 0x0A: return 10; // Sasquatch
        case 0x10: return 11; // Phobos (Huitzil)
        case 0x11: return 12; // Pyron
        default: return 0;
        }
    }

    // The stage wanted now, 1 the guests' first (Darkstalkers' are first: game.cfg's guests); 0 this game's own
    int Wanted() {
        if (BattleMode()) {
            // (a mode's stages are this game's own, but the player's stage when it is one of the guests': BattleStage)
            const int chosen = BattleStage();
            return chosen > kStages && chosen <= kStages + kMoreStages ? chosen - kStages : 0;
        }
        // (a stage chosen by name: training's, the viewer's; after this game's twelve)
        const int wanted = session::WantedStage();
        if (wanted > kStages && wanted <= kStages + kMoreStages) {
            return wanted - kStages;
        }
        if (wanted >= 1 && wanted <= kStages) {
            return 0;
        }
        if (!FeatureOn("dstlk_stages") || Ram16(kMode) != 2 || Ram16(kGameStep) != kGameMatch) {
            return 0;
        }
        // The opponent's: the computer's, or player 2's
        const int against = Ram8(Player(0) + kComputer) ? 0 : 1;
        return DarkstalkersStageOf(Ram8(Player(against) + kCharacter));
    }
} // namespace

namespace definitive {

void RegisterStages() {
    Game &game = TheGame();
    // (Display: it changes nothing of the game's play; online, each player sees the stage of their own choice)
    game.features.push_back({"dstlk_stages", "Display", "Darkstalkers' stages",
                             "A match is fought on the opponent's own stage in the first Darkstalkers, where it has one (twelve "
                             "of the eighteen have): its picture and its music, as that game shows them. Needs Darkstalkers' ROM "
                             "set (dstlk) beside this game's, or chosen on its page in the launcher. The menu's Stage page and "
                             "the stage viewer's list have them too, and Street Fighter Alpha 2's with its set.",
                             0});
    guest_stages::Host host;
    host.cameraX = kCameraX;
    host.cameraY = kCameraY;
    host.cameraLeft = kCameraLeft;
    host.cameraRight = kCameraRight;
    // (in a match of this game's, not its versus screen; or the stage viewer)
    host.shown = [] { return StageViewer() || (Ram16(kMode) == 2 && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) != kMatchVersus); };
    host.viewer = StageViewer;
    host.panel = ThePanel;
    host.notTheStage = NotTheStage;
    host.stageTunes.assign(std::begin(family::kStageTunes), std::end(family::kStageTunes));
    host.wanted = Wanted;
    host.feature = "dstlk_stages";
    guest_stages::Register(host, guests::AllBut("vampire_savior")); // (fight/all_guests.h: family.h's kMoreStageNames the same)
}

} // namespace definitive

} // namespace cps2recomp
