// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: the board's settings (the operator's, in its test menu: Configuration, System and Game)
// as features on the launcher's Board page, as in the other games. Each is "As the board is set" unless chosen, and
// a choice is what the test menu would have set: the game reads its settings from the settings record once, as the
// board starts (0AD926 on, called from 001010), into the variables below; the hook at 001016, where that call
// returns, puts the choices in those variables. The board's settings memory itself is not changed, nor what the
// test menu shows and saves (the test menu edits a copy of its own).
//
// The settings record (FFFBC0) and the variable each goes to (each option stepped in the test menu, and the test menu's
// copy of it read after each press: FF3130 on, the record's offsets; the names read from pictures; in brackets the
// board's own, the variables of a new board):
//   +01 continue    FFFED1  0 on, 1 off (on)           +10 difficulty  FFFECE  0 easy, 1 normal, 2-6 hard 1-5, 7 expert (normal)
//   +03 demo sound  FFFECF  0 off, 1 on (on)           +11 timer speed FFFED5  0 slow, 1 normal, 2 fast, 3 fastest (normal)
//                                                      +12 damage      FFFED6  0 low, 1 normal, 2 high, 3 maximum (normal)
//                                                      +13 mode select FFFEDB  0 manual only, 1 free select, 2 easy only
//                                                                              (free select)
//                                                      +14 join-in     FFFED9  0 on, 1 off (on)
//                                                      +15 event       FFFED7  0 off, 1 one match (off)
//                                                      +16 game speed  FFFED8  0 normal, 1 turbo 1, 2 turbo 2, 3 free
//                                                                              select, 4 free select 2 (free select)
#include "mshvsf.h"

#include "machine.h"
#include "state.h"

#include <iterator>

namespace cps2recomp {

void TitleBooted(); // title.cpp

namespace {
    constexpr const char *kPage = "Board";

    struct Setting {
        const char *name;
        uint32_t variable;
        int first;     // the value of the first choice after "As the board is set"
        bool inverted; // Off and On (the other games' order) are this game's 1 and 0
    };
    constexpr Setting kSettings[] = {
        {"board_difficulty", 0xFFFECE, 0, false}, {"board_damage", 0xFFFED6, 0, false},   {"board_timer", 0xFFFED5, 0, false},
        {"board_speed", 0xFFFED8, 0, false},      {"board_mode_select", 0xFFFEDB, 0, false}, {"board_event", 0xFFFED7, 0, false},
        {"board_join_in", 0xFFFED9, 0, true},     {"board_continue", 0xFFFED1, 0, true},   {"board_demo_sound", 0xFFFECF, 0, false},
    };
    constexpr uint32_t kContinue = 0xFFFED1, kEvent = 0xFFFED7;
    constexpr uint8_t kContinueOff = 1;

    // The board's own values of what was chosen, to put back for its test menu (kept with the game's state: they say
    // what the variables in RAM hold)
    uint8_t g_board[std::size(kSettings)];
    uint8_t g_continue = 0;
    bool g_chosen = false; // the choices are in the variables
    bool g_again = false;  // they were taken out for the test menu, and go back in when a game begins

    void PutChoices() {
        g_chosen = false;
        for (size_t n = 0; n < std::size(kSettings); n++) {
            g_board[n] = Ram8(kSettings[n].variable);
        }
        g_continue = Ram8(kContinue);
        for (const Setting &setting : kSettings) {
            const int choice = FeatureValue(setting.name);
            if (choice > 0) {
                SetRam8(setting.variable, static_cast<uint8_t>(setting.inverted ? 2 - choice : setting.first + choice - 1));
                g_chosen = true;
            }
        }
        if (g_chosen && Ram8(kEvent)) {
            SetRam8(kContinue, kContinueOff); // (no continue in the one-match event, as the test menu sets it)
        }
    }

    // 001016: the board's settings have been read
    bool SettingsRead(M68K &) {
        g_again = false;
        PutChoices();
        TitleBooted();
        return false;
    }
    // 0AB4DA: a game begins (Start has let a player in; the game is marked as being played)
    bool GameBegins(M68K &) {
        if (g_again) {
            g_again = false;
            PutChoices();
        }
        return false;
    }
} // namespace

// Before every frame. The board's test switch pressed puts the board's own values back, so that a game played after
// the test menu without a reset has the board's own until the choices go back in as it begins
void BoardBeforeFrame(Machine &machine) {
    if (g_chosen && machine.testSwitch) {
        for (size_t n = 0; n < std::size(kSettings); n++) {
            SetRam8(kSettings[n].variable, g_board[n]);
        }
        SetRam8(kContinue, g_continue);
        g_chosen = false;
        g_again = true;
    }
}

void BoardState(Archive &ar) {
    ar.Mark(0x424F4152); // BOAR
    ar.Bytes(g_board, sizeof g_board);
    ar.Pod(g_continue);
    ar.Pod(g_chosen);
    ar.Pod(g_again);
}

void RegisterBoard() {
    Game &game = TheGame();
    game.pages.push_back({kPage, "", "The board's own settings, as its test menu has them (Configuration): each as the board is set "
                                     "unless chosen here. Taken when the game starts or is reset."});
    constexpr const char *kAsSet = "As the board is set";
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How well the computer fights.", 0,
                             {kAsSet, "Easy", "Normal", "Hard 1", "Hard 2", "Hard 3", "Hard 4", "Hard 5", "Expert"}});
    game.features.push_back({"board_damage", kPage, "Damage level", "How much damage a hit does.", 0,
                             {kAsSet, "Low", "Normal", "High", "Maximum"}});
    game.features.push_back({"board_timer", kPage, "Timer speed", "How fast the round's clock runs.", 0,
                             {kAsSet, "Slow", "Normal", "Fast", "Fastest"}});
    game.features.push_back({"board_speed", kPage, "Game speed",
                             "The game's speed: normal, one of two turbo speeds, or a free choice of speed for the players.", 0,
                             {kAsSet, "Normal", "Turbo 1", "Turbo 2", "Free select 1", "Free select 2"}});
    game.features.push_back({"board_mode_select", kPage, "Mode select",
                             "Which controls the players may fight with: the manual ones only, the easy ones only, or "
                             "either, chosen before each game.",
                             0, {kAsSet, "Manual only", "Free select", "Easy only"}});
    game.features.push_back({"board_event", kPage, "Event", "Off, or one match only (and no continue).", 0, {kAsSet, "Off", "One match"}});
    game.features.push_back({"board_join_in", kPage, "Join-in", "Whether a second player can join a game against the computer.", 0,
                             {kAsSet, "Off", "On"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(0x001016), "SettingsRead", SettingsRead);
    AddHook(HookAt(0x0AB4DA), "GameBegins", GameBegins);
}

} // namespace cps2recomp
