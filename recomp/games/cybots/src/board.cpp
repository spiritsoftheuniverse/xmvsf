// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the board's settings (the operator's, in its test menu: Configuration, System and Game) as features on
// the launcher's Board page, as in the other games. Each is "As the board is set" unless chosen, and a choice is what
// the test menu would have set: the game reads its settings from the settings memory as the board starts and copies
// them into the variables below (008EE4 on, and again as its test menu is left: 091256 on); the hooks where those
// copies end (0090A2, 0912F4) put the choices in those variables. The board's settings memory itself is not changed, nor what the test menu shows and saves.
//
// The settings record (A5-4600, as the settings memory holds it) and the variable each goes to (from A5), found by
// stepping each option in the test menu and dumping memory after each step; the values' names read from the test
// menu (the board's own in brackets):
//   +0D difficulty   A8  0-7: 1 easiest to 8 hardest (3: 4 medium)    +08 continue    AF  0 off, 1 on (on)
//   +10 damage level 100 0-7: 1 minimum to 8 maximum (3: 4 medium)    +0A demo sound  A7  0 off, 1 on (on)
//   +11 timer speed  101 0-7: 1 slow to 8 fast (3: 4 normal)
//   +12 event        102 0 off, 1 on (off)
//   +13 join in      103 0 on, 1 off (on; an event sets it off, 00909C)
#include "cybots.h"

#include "machine.h"
#include "state.h"

#include <iterator>

namespace cps2recomp {

namespace {
    constexpr uint32_t kVars = 0xFF8000; // (A5)
    constexpr const char *kPage = "Board";

    struct Setting {
        const char *name;
        uint32_t variable;
    };
    constexpr Setting kSettings[] = {
        {"board_difficulty", kVars + 0xA8}, {"board_damage", kVars + 0x100},  {"board_timer", kVars + 0x101},
        {"board_event", kVars + 0x102},     {"board_join_in", kVars + 0x103}, {"board_continue", kVars + 0xAF},
        {"board_demo_sound", kVars + 0xA7},
    };

    // The board's own values of what was chosen, to put back for its test menu. (Kept with the game's state,
    // BoardState: they say what the variables in RAM hold, so a state loaded or run-ahead's frame taken back must
    // bring them back with the RAM.)
    uint8_t g_board[std::size(kSettings)];
    bool g_chosen = false; // the choices are in the variables
    bool g_again = false;  // they were taken out for the test menu, and go back in when a game begins

    // The choices put in the variables (the board's own kept first)
    void PutChoices() {
        g_chosen = false;
        for (size_t n = 0; n < std::size(kSettings); n++) {
            g_board[n] = Ram8(kSettings[n].variable);
        }
        for (const Setting &setting : kSettings) {
            const int choice = FeatureValue(setting.name);
            if (choice > 0) {
                SetRam8(setting.variable, static_cast<uint8_t>(choice - 1));
                g_chosen = true;
            }
        }
    }

    // 0912F4: the board's settings have been copied to the variables
    bool SettingsRead(M68K &) {
        g_again = false;
        PutChoices();
        return false; // (and on with the game's own code)
    }
    // 00C3A6: Start pressed, a game begins (the game's own routine)
    bool GameBegins(M68K &) {
        if (g_again) {
            g_again = false;
            PutChoices();
        }
        return false;
    }
} // namespace

// Before every frame. The board's test switch pressed (its test menu shows, changes and saves the variables) puts the
// board's own values back, so that what the test menu saves is the board's; leaving the test menu does not read the
// settings again, so the choices go back in when a game next begins.
void BoardBeforeFrame(Machine &machine) {
    if (g_chosen && machine.testSwitch) {
        for (size_t n = 0; n < std::size(kSettings); n++) {
            SetRam8(kSettings[n].variable, g_board[n]);
        }
        g_chosen = false;
        g_again = true;
    }
}

void BoardState(Archive &ar) {
    ar.Mark(0x424F4152); // BOAR
    ar.Bytes(g_board, sizeof g_board);
    ar.Pod(g_chosen);
    ar.Pod(g_again);
}

void RegisterBoard() {
    Game &game = TheGame();
    game.pages.push_back({kPage, "", "The board's own settings, as its test menu has them (Configuration): each as the board is set "
                                     "unless chosen here. Taken when the game starts or is reset."});
    constexpr const char *kAsSet = "As the board is set";
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How well the computer fights.", 0,
                             {kAsSet, "1 Easiest", "2 Very easy", "3 Easy", "4 Medium", "5 Medium hard", "6 Hard", "7 Very hard", "8 Hardest"}});
    game.features.push_back({"board_damage", kPage, "Damage level", "How much damage a hit does.", 0,
                             {kAsSet, "1 Minimum", "2", "3", "4 Medium", "5", "6", "7", "8 Maximum"}});
    game.features.push_back({"board_timer", kPage, "Timer speed", "How fast the round's clock runs.", 0,
                             {kAsSet, "1 Slow", "2", "3", "4 Normal", "5", "6", "7", "8 Fast"}});
    game.features.push_back({"board_event", kPage, "Event", "One match only, for tournaments.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_join_in", kPage, "Join in", "Whether a second player can join a game against the computer.", 0,
                             {kAsSet, "On", "Off"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(cybots::kSettingsCopied), "SettingsRead", SettingsRead);
    AddHook(HookAt(cybots::kSettingsCopiedAgain), "SettingsRead", SettingsRead);
    AddHook(HookAt(cybots::kGameBegins), "GameBegins", GameBegins);
}

} // namespace cps2recomp
