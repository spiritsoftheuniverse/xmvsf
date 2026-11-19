// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: the board's settings (the operator's, in its test menu: Configuration, System and Game) as
// features on the launcher's Board page, as in the other games. Each is "As the board is set" unless chosen, and a
// choice is what the test menu would have set: the game reads its settings from the settings record once, as the
// board starts (08E9C6 on, called from 000C66), into the variables below; the hook at 000C6C, where that call returns,
// puts the choices in those variables. The board's settings memory itself is not changed, nor what the test menu shows
// and saves (the test menu edits a copy of its own, at FF2C24).
//
// The settings record (FFF912) and the variable each goes to (each option changed in the test menu, saved, and the
// record read after the next start; the names stepped through with pictures; in brackets the board's own):
//   +01 continue    FFFB43  0 on, 1 off (on)        +10 difficulty  FFFB40  0-7 easiest, very easy, easy, medium,
//   +03 demo sound  FFFB41  0 off, 1 on (off)                               medium hard, hard, very hard, hardest (medium)
//                                                   +11 timer speed FFFB47  0-7 1 (slow) to 8 (fast) (4, normal)
//                                                   +12 damage      FFFB48  0-7 level 1 to 8 (level 4)
//                                                   +13 max round   FFFB46  0 one round, 1 three, 2 five (three)
//                                                   +14 event       FFFB4C  0 off, 1 one match (off; FFFB49 too, from a
//                                                                           table at 08EB5C; the test menu sets join-in
//                                                                           and continue off with it)
//                                                   +15 join-in     FFFB4B  0 on, 1 off (on)
//                                                   +16 mode        FFFB4D  0 normal, 1 normal and turbo, 2 normal,
//                                                                           turbo and beginner (normal and turbo)
//                                                   +17 turbo       FFFB4A  1 turbo only (with mode 0)
#include "msh.h"

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
        {"board_difficulty", 0xFFFB40, 0, false}, {"board_damage", 0xFFFB48, 0, false},  {"board_timer", 0xFFFB47, 0, false},
        {"board_rounds", 0xFFFB46, 0, false},     {"board_event", 0xFFFB4C, 0, false},   {"board_join_in", 0xFFFB4B, 0, true},
        {"board_continue", 0xFFFB43, 0, true},    {"board_demo_sound", 0xFFFB41, 0, false},
    };
    constexpr uint32_t kContinue = 0xFFFB43, kJoinIn = 0xFFFB4B, kEvent = 0xFFFB4C, kEventToo = 0xFFFB49;
    // The mode is two variables: FFFB4D, and FFFB4A for turbo alone
    constexpr uint32_t kMode = 0xFFFB4D, kTurboOnly = 0xFFFB4A;
    constexpr uint8_t kOff = 1;

    // The board's own values of what was chosen, to put back for its test menu (kept with the game's state: they say
    // what the variables in RAM hold)
    uint8_t g_board[std::size(kSettings)];
    uint8_t g_continue = 0, g_joinIn = 0, g_eventToo = 0, g_mode = 0, g_turboOnly = 0;
    bool g_chosen = false; // the choices are in the variables
    bool g_again = false;  // they were taken out for the test menu, and go back in when a game begins

    void PutChoices() {
        g_chosen = false;
        for (size_t n = 0; n < std::size(kSettings); n++) {
            g_board[n] = Ram8(kSettings[n].variable);
        }
        g_continue = Ram8(kContinue);
        g_joinIn = Ram8(kJoinIn);
        g_eventToo = Ram8(kEventToo);
        g_mode = Ram8(kMode);
        g_turboOnly = Ram8(kTurboOnly);
        for (const Setting &setting : kSettings) {
            const int choice = FeatureValue(setting.name);
            if (choice > 0) {
                SetRam8(setting.variable, static_cast<uint8_t>(setting.inverted ? 2 - choice : setting.first + choice - 1));
                g_chosen = true;
            }
        }
        // Mode: Normal, Turbo, Normal and turbo, Normal, turbo and beginner (the test menu's order)
        static constexpr uint8_t kModes[4][2] = {{0, 0}, {0, 1}, {1, 0}, {2, 0}};
        if (const int choice = FeatureValue("board_mode"); choice > 0) {
            SetRam8(kMode, kModes[choice - 1][0]);
            SetRam8(kTurboOnly, kModes[choice - 1][1]);
            g_chosen = true;
        }
        if (g_chosen) {
            SetRam8(kEventToo, Ram8(kEvent) ? 1 : 0); // (as the board's start sets it, from the table at 08EB5C)
            if (Ram8(kEvent)) {
                // (no continue and no join-in in the one-match event, as the test menu sets them)
                SetRam8(kContinue, kOff);
                SetRam8(kJoinIn, kOff);
            }
        }
    }

    // 000C6C: the board's settings have been read
    bool SettingsRead(M68K &) {
        g_again = false;
        PutChoices();
        TitleBooted();
        return false;
    }
    // 08CDBC: a game begins (Start has let a player in; the game is about to be marked as being played)
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
        SetRam8(kEventToo, g_eventToo);
        SetRam8(kMode, g_mode);
        SetRam8(kTurboOnly, g_turboOnly);
        g_chosen = false;
        g_again = true;
    }
}

void BoardState(Archive &ar) {
    ar.Mark(0x424F4152); // BOAR
    ar.Bytes(g_board, sizeof g_board);
    ar.Pod(g_continue);
    ar.Pod(g_joinIn);
    ar.Pod(g_eventToo);
    ar.Pod(g_mode);
    ar.Pod(g_turboOnly);
    ar.Pod(g_chosen);
    ar.Pod(g_again);
}

void RegisterBoard() {
    Game &game = TheGame();
    game.pages.push_back({kPage, "", "The board's own settings, as its test menu has them (Configuration): each as the board is set "
                                     "unless chosen here. Taken when the game starts or is reset."});
    constexpr const char *kAsSet = "As the board is set";
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How well the computer fights.", 0,
                             {kAsSet, "Easiest", "Very easy", "Easy", "Medium", "Medium hard", "Hard", "Very hard", "Hardest"}});
    game.features.push_back({"board_damage", kPage, "Damage level", "How much damage a hit does.", 0,
                             {kAsSet, "Level 1", "Level 2", "Level 3", "Level 4", "Level 5", "Level 6", "Level 7", "Level 8"}});
    game.features.push_back({"board_timer", kPage, "Timer speed", "How fast the round's clock runs.", 0,
                             {kAsSet, "1 (slow)", "2", "3", "4 (normal)", "5", "6", "7", "8 (fast)"}});
    game.features.push_back({"board_rounds", kPage, "Max round", "How many rounds a match has.", 0,
                             {kAsSet, "1 round", "3 rounds", "5 rounds"}});
    game.features.push_back({"board_mode", kPage, "Mode",
                             "The game's speed: normal, turbo, or a choice for the players (and a beginner's choice too).", 0,
                             {kAsSet, "Normal", "Turbo", "Normal and turbo", "Normal, turbo and beginner"}});
    game.features.push_back({"board_event", kPage, "Event", "Off, or one match only (and no continue or join-in).", 0,
                             {kAsSet, "Off", "One match"}});
    game.features.push_back({"board_join_in", kPage, "Join-in", "Whether a second player can join a game against the computer.", 0,
                             {kAsSet, "Off", "On"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(0x000C6C), "SettingsRead", SettingsRead);
    AddHook(HookAt(0x08CDBC), "GameBegins", GameBegins);
}

} // namespace cps2recomp
