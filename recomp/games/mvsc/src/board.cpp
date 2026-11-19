// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel vs. Capcom: the board's settings (the operator's, in its test menu: Configuration, System and Game)
// as features on the launcher's Board page, as in the other games. Each is "As the board is set" unless chosen, and
// a choice is what the test menu would have set: the game reads its settings from the settings record once, as the
// board starts (007E7A on, called from 000FAC), into the variables below; the hook at 000FB2, where that call
// returns, puts the choices in those variables. The board's settings memory itself is not changed, nor what the
// test menu shows and saves (the test menu edits a copy of its own).
//
// The settings record (FFF648) and the variable each goes to (each option stepped in the test menu, saved, and the
// variables read after the next start; the names read from pictures; in brackets the board's own, a new board's):
//   +01 continue    FFF959  0 on, 1 off (on)           +10 difficulty  FFF956  0 easy, 1 normal, 2-6 hard 1-5, 7 expert (normal)
//   +03 demo sound  FFF957  0 off, 1 on (on)           +11 timer speed FFF95D  0 slow, 1 normal, 2 fast, 3 fastest (normal)
//                                                      +12 damage      FFF95E  0 low, 1 normal, 2 high, 3 maximum (normal)
//                                                      +13 mode select FFF963  0 manual only, 1 free select, 2 easy only
//                                                                              (free select)
//                                                      +14 join-in     FFF961  0 on, 1 off (on)
//                                                      +15 event       FFF95F  0 off, 1 one match (off; the test menu sets
//                                                                              continue and join-in off with it)
//                                                      +16 game speed  FFF960  0 normal, 1 turbo 1, 2 turbo 2, 3 free
//                                                                              select, 4 free select 2 (free select)
//                                                      +17 flickering  FFF9B8  0 on, 1 off (on)
#include "mvsc.h"

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
        {"board_difficulty", 0xFFF956, 0, false}, {"board_damage", 0xFFF95E, 0, false},   {"board_timer", 0xFFF95D, 0, false},
        {"board_speed", 0xFFF960, 0, false},      {"board_mode_select", 0xFFF963, 0, false}, {"board_event", 0xFFF95F, 0, false},
        {"board_join_in", 0xFFF961, 0, true},     {"board_continue", 0xFFF959, 0, true},   {"board_demo_sound", 0xFFF957, 0, false},
        {"board_flickering", 0xFFF9B8, 0, true},
    };
    constexpr uint32_t kContinue = 0xFFF959, kEvent = 0xFFF95F, kJoinIn = 0xFFF961;
    constexpr uint8_t kContinueOff = 1;

    // The board's own values of what was chosen, to put back for its test menu (kept with the game's state: they say
    // what the variables in RAM hold)
    uint8_t g_board[std::size(kSettings)];
    uint8_t g_continue = 0, g_joinIn = 0;
    bool g_chosen = false; // the choices are in the variables
    bool g_again = false;  // they were taken out for the test menu, and go back in when a game begins

    void PutChoices() {
        g_chosen = false;
        for (size_t n = 0; n < std::size(kSettings); n++) {
            g_board[n] = Ram8(kSettings[n].variable);
        }
        g_continue = Ram8(kContinue);
        g_joinIn = Ram8(kJoinIn);
        for (const Setting &setting : kSettings) {
            const int choice = FeatureValue(setting.name);
            if (choice > 0) {
                SetRam8(setting.variable, static_cast<uint8_t>(setting.inverted ? 2 - choice : setting.first + choice - 1));
                g_chosen = true;
            }
        }
        if (g_chosen && Ram8(kEvent)) {
            SetRam8(kContinue, kContinueOff); // (no continue nor join-in in the one-match event, as the test menu sets it)
            SetRam8(kJoinIn, 1);
        }
    }

    // 000FB2: the board's settings have been read
    bool SettingsRead(M68K &) {
        g_again = false;
        PutChoices();
        TitleBooted();
        return false;
    }
    // 0D4388: a game begins (Start has let a player in; the game is marked as being played)
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
        SetRam8(kJoinIn, g_joinIn);
        g_chosen = false;
        g_again = true;
    }
}

void BoardState(Archive &ar) {
    ar.Mark(0x424F4152); // BOAR
    ar.Bytes(g_board, sizeof g_board);
    ar.Pod(g_continue);
    ar.Pod(g_joinIn);
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
    game.features.push_back({"board_event", kPage, "Event", "Off, or one match only (and no continue nor join-in).", 0, {kAsSet, "Off", "One match"}});
    game.features.push_back({"board_join_in", kPage, "Join-in", "Whether a second player can join a game against the computer.", 0,
                             {kAsSet, "Off", "On"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_flickering", kPage, "Flickering",
                             "Whether the game shows some of its see-through effects by flickering them.", 0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(0x000FB2), "SettingsRead", SettingsRead);
    AddHook(HookAt(0x0D4388), "GameBegins", GameBegins);
}

} // namespace cps2recomp
