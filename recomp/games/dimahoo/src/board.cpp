// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Dimahoo: the board's settings (the Board page), as in the other games: each "As the board is set" unless chosen, and
// a choice is what the test menu would have set. The game plays by the variables the settings record is copied into
// (dimahoo.h), reading each where it uses it: a choice is put in them before every frame, and the board's own as the
// test menu opens, so that the test menu (which copies them as it opens) shows and saves the board's own.
#include "dimahoo.h"
#include "state.h"

namespace cps2recomp {

bool ChoicesLeftOut(); // modes.cpp
bool AttractModeOn();
bool ScoreModeOn();

namespace {
    using namespace dimahoo;

    constexpr const char *kPage = "Board";
    struct Field {
        const char *name;
        uint32_t variable;
        uint32_t record;          // (its byte in the settings record)
        const uint8_t *values;    // (the value of each choice after "As the board is set")
    };
    constexpr uint8_t kDifficulty[] = {0, 1, 2, 3, 4, 5, 6, 7};
    constexpr uint8_t kShipStock[] = {0, 1, 2, 3, 4};    // (1 to 5 ships)
    constexpr uint8_t kBombStock[] = {0, 1, 2, 3, 4};
    constexpr uint8_t kExtend[] = {0, 1, 2, 3, 4, 5};
    constexpr uint8_t kLoops[] = {0, 1};
    constexpr uint8_t kContinue[] = {1, 0};              // (off, on)
    constexpr uint8_t kDemoSound[] = {0, 2, 1};          // (off, half, on)
    constexpr Field kFields[] = {
        {"board_difficulty", kSetDifficulty, 0x10, kDifficulty}, {"board_ships", kSetShips, 0x11, kShipStock},
        {"board_bombs", kSetBombs, 0x12, kBombStock},           {"board_extend", kSetExtend, 0x13, kExtend},
        {"board_loops", kSetLoops, 0x14, kLoops},               {"board_continue", kSetContinue, 0x01, kContinue},
        {"board_demo_sound", kSetDemoSound, 0x03, kDemoSound},
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
        return field.values[choice - 1];
    }

    // The test menu takes its copies of the variables as it opens: as the switch is seen, the board's own go back in them
    // once, and the menu is left alone until it is left (the board then starts again, and its start clears the flag the
    // menu sets). (Kept with the state.)
    bool g_inTest = false;
} // namespace

void BoardFrame(Machine &machine) {
    const bool test = machine.testSwitch || Ram8(kGameOn) == kGameOnTest;
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
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How hard the game is, 1 to 8. A new board: 4.", 0,
                             {kAsSet, "1, easiest", "2", "3", "4", "5", "6", "7", "8, hardest"}});
    game.features.push_back({"board_ships", kPage, "Ships", "Ships for each credit (the test menu's player stock). A new board: 3.", 0,
                             {kAsSet, "1", "2", "3", "4", "5"}});
    game.features.push_back({"board_bombs", kPage, "Bombs", "Bombs each ship starts with (the test menu's bomber stock). A new board: 3.", 0,
                             {kAsSet, "0", "1", "2", "3", "4"}});
    game.features.push_back({"board_extend", kPage, "Extra ships", "The scores that give an extra ship. A new board: 2,000,000 and 5,000,000.", 0,
                             {kAsSet, "2,000,000 and 5,000,000", "2,000,000 only", "4,000,000 only", "Every 2,000,000", "Every 4,000,000",
                              "None"}});
    game.features.push_back({"board_loops", kPage, "Loops", "Whether the six stages are played once or twice (the test menu's stage type). A new "
                                                            "board: once.",
                             0, {kAsSet, "One loop", "Two loops"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound, or at half the volume.", 0,
                             {kAsSet, "Off", "Half", "On"}});
    // (the collection's other games have rows of these names with other meanings)
    OwnFeatures({"board_difficulty", "board_ships", "board_bombs", "board_extend", "board_demo_sound"});
}

} // namespace cps2recomp
