// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: the board's settings (the operator's, in its test menu: Configuration, System and Game)
// as features on the launcher's Board page, as in the other games. Each is "As the board is set" unless chosen, and a
// choice is what the test menu would have set: the game reads its settings from the settings record once, as the board
// starts (0859D6 on, called from 000C92), into the variables below; the hook at 000C98, where that call returns, puts
// the choices in those variables. The board's settings memory itself is not changed, nor what the test menu shows and
// saves (the test menu edits a copy of its own, at FF2E1C, loaded from these variables at 080F5C).
//
// The settings record (FFEB8C) and the variable each goes to (0859D6 read; each option stepped in the test menu with
// its copy dumped, the names read off pictures; in brackets the board's own):
//   +01 continue    FFEDB7  0 on, 1 off (on)        +10 difficulty  FFEDB4  0-7 easiest, very easy, easy, medium,
//   +03 demo sound  FFEDB5  0 off, 1 on (off)                               medium hard, hard, very hard, hardest (medium)
//                                                   +11 timer speed FFEDBC  0-7 1 (slow) to 8 (fast) (4, normal)
//                                                   +12 damage      FFEDBD  0-7 level 1 to 8 (level 4)
//                                                   +13 max round   FFEDBB  0 one round, 1 three, 2 five (three)
//                                                   +14 event       FFEE24  0 off, 1 one match, 2 sudden death, 3 both
//                                                                           (off; FFEDBE one match and FFEDBF sudden
//                                                                           death too, from a table at 085B38; the test
//                                                                           menu sets join-in off with it)
//                                                   +15 join-in     FFEDC0  0 on, 1 off (on)
#include "xmcota.h"

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
        {"board_difficulty", 0xFFEDB4, 0, false}, {"board_damage", 0xFFEDBD, 0, false}, {"board_timer", 0xFFEDBC, 0, false},
        {"board_rounds", 0xFFEDBB, 0, false},     {"board_event", 0xFFEE24, 0, false},  {"board_join_in", 0xFFEDC0, 0, true},
        {"board_continue", 0xFFEDB7, 0, true},    {"board_demo_sound", 0xFFEDB5, 0, false},
    };
    constexpr uint32_t kJoinIn = 0xFFEDC0, kEvent = 0xFFEE24, kOneMatch = 0xFFEDBE, kSuddenDeath = 0xFFEDBF;
    constexpr uint8_t kOff = 1;

    // The board's own values of what was chosen, to put back for its test menu (kept with the game's state: they say
    // what the variables in RAM hold)
    uint8_t g_board[std::size(kSettings)];
    uint8_t g_oneMatch = 0, g_suddenDeath = 0;
    bool g_chosen = false; // the choices are in the variables
    bool g_again = false;  // they were taken out for the test menu, and go back in when a game begins

    void PutChoices() {
        g_chosen = false;
        for (size_t n = 0; n < std::size(kSettings); n++) {
            g_board[n] = Ram8(kSettings[n].variable);
        }
        g_oneMatch = Ram8(kOneMatch);
        g_suddenDeath = Ram8(kSuddenDeath);
        for (const Setting &setting : kSettings) {
            const int choice = FeatureValue(setting.name);
            if (choice > 0) {
                SetRam8(setting.variable, static_cast<uint8_t>(setting.inverted ? 2 - choice : setting.first + choice - 1));
                g_chosen = true;
            }
        }
        if (g_chosen) {
            // (as the board's start sets them, from the table at 085B38)
            const uint8_t event = Ram8(kEvent) & 3;
            SetRam8(kOneMatch, event & 1);
            SetRam8(kSuddenDeath, (event >> 1) & 1);
            if (event) {
                SetRam8(kJoinIn, kOff); // (no join-in with an event, as the test menu sets it)
            }
        }
    }

    // 000C98: the board's settings have been read
    bool SettingsRead(M68K &) {
        g_again = false;
        PutChoices();
        TitleBooted();
        return false;
    }
    // 079EDA: a game begins (Start has let a player in; the game is about to be marked as being played)
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
        SetRam8(kOneMatch, g_oneMatch);
        SetRam8(kSuddenDeath, g_suddenDeath);
        g_chosen = false;
        g_again = true;
    }
}

void BoardState(Archive &ar) {
    ar.Mark(0x424F4152); // BOAR
    ar.Bytes(g_board, sizeof g_board);
    ar.Pod(g_oneMatch);
    ar.Pod(g_suddenDeath);
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
    game.features.push_back({"board_event", kPage, "Event",
                             "Off, one match only, sudden death, or both (and no join-in with any of them).", 0,
                             {kAsSet, "Off", "One match", "Sudden death", "One match and sudden death"}});
    game.features.push_back({"board_join_in", kPage, "Join-in", "Whether a second player can join a game against the computer.", 0,
                             {kAsSet, "Off", "On"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(0x000C98), "SettingsRead", SettingsRead);
    AddHook(HookAt(0x079EDA), "GameBegins", GameBegins);
}

} // namespace cps2recomp
