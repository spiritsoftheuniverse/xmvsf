// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Giga Wing: the board's settings (the Board page), as in the other games: each "As the board is set" unless chosen,
// and a choice is what the test menu would have set. The game plays by the variables the settings record is copied
// into (gigawing.h), reading each where it uses it (the ships at a game's start, the bombs at each ship's): a choice
// is put in them before every frame, and the board's own as the test menu opens, so that the test menu shows and saves
// the board's own.
#include "gigawing.h"
#include "state.h"

namespace cps2recomp {

bool ChoicesLeftOut(); // modes.cpp
bool AttractModeOn();
bool ScoreModeOn();

namespace {
    using namespace gigawing;

    constexpr const char *kPage = "Board";
    struct Field {
        const char *name;
        uint32_t variable;
        uint32_t record; // (its byte in the settings record)
        int first;       // (the value of the first choice after "As the board is set")
    };
    constexpr Field kFields[] = {
        {"board_difficulty", kSetDifficulty, 0x10, 0}, {"board_ships", kSetShips, 0x11, 2}, {"board_bombs", kSetBombs, 0x12, 0},
        {"board_continue", kSetContinue, 0x01, 0},      {"board_demo_sound", kSetDemoSound, 0x03, 0},
    };

    // The value each variable is to have now (-1: the board's own, left alone)
    int Wanted(const Field &field) {
        // (attract mode with its sound, a score mode with no continue)
        if (field.variable == kSetDemoSound && AttractModeOn()) {
            return 1;
        }
        if (field.variable == kSetContinue && ScoreModeOn()) {
            return 1;
        }
        const int choice = FeatureValue(field.name);
        if (choice <= 0) {
            return -1;
        }
        if (field.variable == kSetContinue) {
            return choice == 1 ? 1 : 0; // ("Off" is 1 on the board, "On" 0)
        }
        if (field.variable == kSetDemoSound) {
            return choice == 1 ? 0 : 1;
        }
        return field.first + choice - 1;
    }
} // namespace

// The test menu shows and edits the variables themselves (and SAVE & EXIT copies them into the record): as it opens, the
// board's own go back in them once, and the menu is left alone until it is left. (Kept with the state.)
namespace {
    bool g_inTest = false;
}

void BoardFrame(Machine &machine) {
    const bool test = machine.testSwitch || Ram8(kTestMode) != 0;
    for (const Field &field : kFields) {
        const int value = Wanted(field);
        if (value < 0) {
            continue;
        }
        if (test) {
            if (!g_inTest) {
                SetRam8(field.variable, Ram8(kSettings + field.record));
            }
            continue;
        }
        SetRam8(field.variable, static_cast<uint8_t>(value));
    }
    g_inTest = test;
}

void BoardState(Archive &ar) {
    ar.Pod(g_inTest);
}

void RegisterBoard() {
    Game &game = TheGame();
    game.pages.push_back({kPage, "", "The board's own settings, as its test menu has them (Configuration): each as the board is set unless "
                                     "chosen here."});
    constexpr const char *kAsSet = "As the board is set";
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How hard the game is, 1 to 8. A new board: 2.", 0,
                             {kAsSet, "1, easiest", "2", "3", "4", "5", "6", "7", "8, hardest"}});
    game.features.push_back({"board_ships", kPage, "Ships", "Ships for each credit (the test menu's ship stock). A new board: 3.", 0,
                             {kAsSet, "2", "3", "4", "5"}});
    game.features.push_back({"board_bombs", kPage, "Force bombs", "Force bombs each ship starts with (the force bomb stock). A new board: 2.",
                             0, {kAsSet, "0", "1", "2", "3", "4"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    // (the collection's other games have rows of these names with other meanings)
    OwnFeatures({"board_difficulty", "board_ships", "board_bombs"});
}

} // namespace cps2recomp
