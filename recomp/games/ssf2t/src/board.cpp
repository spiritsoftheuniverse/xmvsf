// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: the board's settings (the operator's, in its test menu: System Configuration) as
// features on the launcher's Board page, as in the other games. Each is "As the board is set" unless chosen, and a
// choice is what the test menu would have set: the game reads its settings from the settings memory once, as the board
// starts (00823A, called at 008FF6), into the variables below; the hook at 008FFC, where that call returns, puts the
// choices in those variables. The board's settings memory itself is not changed, nor what the test menu shows and
// saves.
//
// The settings record (A5-6578, as the settings memory holds it) and the variable each goes to (from A5; 00831C on
// copies them):
//   +1E game speed  3A9  0 turbo 1, 1 turbo 2, 2 turbo 3, 3 turbo 4, 4 free select (turbo 2)
//   +20 difficulty  2D3  0 to 7: 1 (easiest) to 8 (hardest) (4, medium)
//   +23 demo sound  2DD  0 off, 1 on (off)
//   +24 continue    2DA  0 off, 1 on (on)
//   +26 start mode  344  0 off, 1 on (off)
// (the values and their names from the test menu's page, each stepped through with the variable read after each
// step; in brackets the value the test menu shows as the board's own on a new board)
#include "ssf2t.h"

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
        int first; // the value of the first choice after "As the board is set"
    };
    constexpr Setting kSettings[] = {
        {"board_difficulty", ssf2t::At(kVars + 0x2D3), 0}, {"board_speed", ssf2t::At(kVars + 0x3A9), 0}, {"board_continue", ssf2t::At(kVars + 0x2DA), 0},
        {"board_demo_sound", ssf2t::At(kVars + 0x2DD), 0},
    };

    // The board's own values of what was chosen, to put back for its test menu. (All three are kept with the game's
    // state, BoardState: they say what the variables in RAM hold, so a state loaded or run-ahead's frame taken back
    // must bring them back with the RAM.)
    uint8_t g_board[std::size(kSettings)];
    bool g_chosen = false; // the choices are in the variables
    bool g_again = false;  // they were taken out for the test menu, and go back in when a game begins

    // The choices put in the variables (the board's own kept first)
    void PutChoices() {
        g_chosen = false;
        for (size_t n = 0; n < std::size(kSettings); n++) {
            g_board[n] = kSettings[n].variable ? Ram8(kSettings[n].variable) : 0;
        }
        for (const Setting &setting : kSettings) {
            if (!setting.variable) {
                continue; // (a setting this game has not: Super Street Fighter II's speed)
            }
            const int choice = FeatureValue(setting.name);
            if (choice > 0) {
                SetRam8(setting.variable, static_cast<uint8_t>(setting.first + choice - 1));
                g_chosen = true;
            }
        }
    }

    // 008FFC: the board's settings have been read
    bool SettingsRead(M68K &) {
        g_again = false;
        PutChoices();
        return false; // (and on with the game's own code)
    }
    // 00D69C: a game begins (Start pressed: the first step word goes to 2)
    bool GameBegins(M68K &) {
        if (g_again) {
            g_again = false;
            PutChoices();
        }
        return false;
    }
} // namespace

// Before every frame. The board's test switch pressed (its test menu shows, changes and saves the variables)
// puts the board's own values back, so that what the test menu saves is the board's; leaving the test menu does
// not read the settings again, so the choices go back in when a game next begins (demo sound, which the show
// uses, at the next reset)
void BoardBeforeFrame(Machine &machine) {
    if (!ssf2t::family::kBoardInSources) {
        return;
    }
    if (g_chosen && machine.testSwitch) {
        for (size_t n = 0; n < std::size(kSettings); n++) {
            if (kSettings[n].variable) {
                SetRam8(kSettings[n].variable, g_board[n]);
            }
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
    if (!ssf2t::family::kBoardInSources) {
        return; // (a board whose settings are DIP switches: game.cfg's board_option lines are its Board page)
    }
    Game &game = TheGame();
    game.pages.push_back({kPage, "", "The board's own settings, as its test menu has them (System Configuration): each as the board is "
                                     "set unless chosen here. Taken when the game starts or is reset."});
    constexpr const char *kAsSet = "As the board is set";
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How well the computer fights.", 0,
                             {kAsSet, "1 (easiest)", "2", "3", "4 (medium)", "5", "6", "7", "8 (hardest)"}});
    if (ssf2t::family::kTurbo) game.features.push_back({"board_speed", kPage, "Game speed",
                             "The game's speed: one of the four turbo speeds, or a free choice of speed for the players.", 0,
                             {kAsSet, "Turbo 1", "Turbo 2", "Turbo 3", "Turbo 4", "Free select"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(ssf2t::At(0x008FFC)), "SettingsRead", SettingsRead);
    AddHook(HookAt(ssf2t::At(0x00D69C)), "GameBegins", GameBegins);
}

} // namespace cps2recomp
