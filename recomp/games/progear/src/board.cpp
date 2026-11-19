// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Progear: the board's settings (the Board page), as in the other games: each "As the board is set" unless chosen, and
// a choice is what the test menu would have set. The game plays by the variables the settings record is copied into
// (progear.h), reading each where it uses it (the ships at a game's start, the extend as the score grows): a choice is
// put in them before every frame, and the board's own as the test menu opens, so that the test menu (which copies them
// as it opens) shows and saves the board's own.
#include "progear.h"
#include "state.h"

namespace cps2recomp {

bool ChoicesLeftOut(); // modes.cpp
bool AttractModeOn();
bool ScoreModeOn();

namespace progear {
// The test menu is on (word: 046926 sets it as the test switch is seen, the board's reset clears it; found from dumps
// before and in the menu)
constexpr uint32_t kTestMode = 0xFF38BC;
} // namespace progear

namespace {
    using namespace progear;

    constexpr const char *kPage = "Board";
    struct Field {
        const char *name;
        uint32_t variable;
        uint32_t record;          // (its byte in the settings record)
        const uint8_t *values;    // (the value of each choice after "As the board is set")
    };
    constexpr uint8_t kDifficulty[] = {0, 1, 2, 3, 4, 5, 6, 7};
    constexpr uint8_t kShipStock[] = {3, 4, 0, 1, 2};    // (1, 2, 3, 4, 5 ships)
    constexpr uint8_t kExtend[] = {0, 2, 1, 3};          // (3M and every 4M; every 1.5M; 4M and 9M; none)
    constexpr uint8_t kOffOn[] = {0, 1};                 // (rapid fire)
    constexpr uint8_t kContinue[] = {1, 0};              // (off, on)
    constexpr uint8_t kDemoSound[] = {0, 2, 1};          // (off, half, on)
    constexpr Field kFields[] = {
        {"board_difficulty", kSetDifficulty, 0x10, kDifficulty}, {"board_ships", kSetShips, 0x11, kShipStock},
        {"board_extend", kSetExtend, 0x12, kExtend},             {"board_rapid_fire", kSetRapid, 0x13, kOffOn},
        {"board_continue", kSetContinue, 0x01, kContinue},       {"board_demo_sound", kSetDemoSound, 0x03, kDemoSound},
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
} // namespace

// The test menu takes its copies of the variables as it opens: as the switch is seen, the board's own go back in them
// once, and the menu is left alone until it is left (the board then starts again). (Kept with the state.)
namespace {
    bool g_inTest = false;
}

void BoardFrame(Machine &machine) {
    const bool test = machine.testSwitch || Ram16(kTestMode) != 0;
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
                             {kAsSet, "1", "2", "3", "4", "5"}});
    game.features.push_back({"board_extend", kPage, "Extra ships", "The scores that give an extra ship. A new board: at 3,000,000, then every 4,000,000.",
                             0, {kAsSet, "3,000,000, then every 4,000,000", "Every 1,500,000", "4,000,000 and 9,000,000", "None"}});
    game.features.push_back({"board_rapid_fire", kPage, "Rapid fire", "Whether button 3 shoots by itself while held. A new board: off.", 0,
                             {kAsSet, "Off", "On"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound, or at half the volume.", 0,
                             {kAsSet, "Off", "Half", "On"}});
    // (the collection's other games have rows of these names with other meanings)
    OwnFeatures({"board_difficulty", "board_ships", "board_extend", "board_demo_sound"});
}

} // namespace cps2recomp
