// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The Punisher: the board's settings (the Board page), as in the other games: each "As the board is set" unless
// chosen, and a choice is what the test menu would have set. The game plays by the variables the settings record is
// copied into as the board starts (punisher.h): a choice is put in them before every frame, and the board's own while
// the test switch is held, so that the test menu shows and saves the board's own.
//
// Some of the values the game plays by are worked out from them by the coin set-up (006A4E: the lives, the
// difficulty's level), as the board starts and on the screen before a game: the choices are in before the board's
// start does it (006848), and when a game begins with choices made, the set-up is run again so that a choice made
// meanwhile counts.
#include "beat.h"
#include "punisher.h"

#include <algorithm>
#include <iterator>

namespace cps2recomp {

bool ChoicesLeftOut(); // modes.cpp

namespace {
    using namespace punisher;

    constexpr const char *kPage = "Board";
    struct Field {
        const char *name;
        uint32_t variable;
        uint32_t record; // (its byte in the settings record)
    };
    constexpr Field kFields[] = {
        {"board_difficulty", kSetDifficulty, 0x11}, {"board_lives", kSetLives, 0x12},
        {"board_extend", kSetExtend, 0x13},         {"board_continue", kSetContinue, 0x0D},
        {"board_demo_sound", kSetDemoSound, 0x0F},  {"board_player_select", kSetPlayerSelect, 0x14},
    };

    // The value each variable is to have now (-1: the board's own, left alone), and whether any is chosen
    int Wanted(const Field &field) {
        const int choice = FeatureValue(field.name);
        // (attract mode with its sound, score attack with no continue: beat/beat.h)
        if (field.variable == kSetDemoSound && beat::AttractMode()) {
            return 1;
        }
        if (field.variable == kSetContinue && beat::ScoreAttack()) {
            return 0;
        }
        // (the viewers begin their game unseen: no players' screen)
        if (field.variable == kSetPlayerSelect && beat::Viewing()) {
            return 0;
        }
        return choice > 0 ? choice - 1 : -1;
    }

    bool PutChoices() {
        bool chosen = false;
        for (const Field &field : kFields) {
            const int value = Wanted(field);
            if (value >= 0) {
                SetRam8(field.variable, static_cast<uint8_t>(value));
                chosen = true;
            }
        }
        return chosen;
    }

    // 006848: the board's start has read its settings, and works out the rest from them next
    bool SettingsRead(M68K &) {
        PutChoices();
        return false;
    }

    // 007B90: a game begins (its step 0), before it takes its lives and difficulty: the coin set-up again with the
    // choices, if any are made
    bool GameStarts(M68K &c) {
        if (PutChoices()) {
            // (the game goes on here as it would have: its registers and its place as they were)
            uint32_t d[8], a[8];
            std::copy(std::begin(c.d), std::end(c.d), d);
            std::copy(std::begin(c.a), std::end(c.a), a);
            const uint32_t pc = c.pc;
            const uint8_t freePlay = Ram8(kFreePlay);
            Call(c, kCoinSetUp);
            SetRam8(kFreePlay, freePlay); // (free play is game.cpp's)
            std::copy(d, d + 8, std::begin(c.d));
            std::copy(a, a + 8, std::begin(c.a));
            c.pc = pc;
        }
        return false;
    }
} // namespace

void BoardFrame(Machine &machine) {
    if (machine.testSwitch) {
        // (the board's own, for the test menu to show and keep)
        for (const Field &field : kFields) {
            if (Wanted(field) >= 0) {
                SetRam8(field.variable, Ram8(kSettings + field.record));
            }
        }
        return;
    }
    PutChoices();
}


void RegisterBoard() {
    Game &game = TheGame();
    game.pages.push_back({kPage, "", "The board's own settings, as its test menu has them (System Configuration): each as the board is set "
                                     "unless chosen here."});
    constexpr const char *kAsSet = "As the board is set";
    game.features.push_back({"board_difficulty", kPage, "Difficulty", "How hard the game is. A new board: 3, normal.", 0,
                             {kAsSet, "0, extra easy", "1, very easy", "2, easy", "3, normal", "4, hard", "5, very hard", "6, extra hard",
                              "7, hardest"}});
    game.features.push_back({"board_lives", kPage, "Lives", "Lives for each credit (the test menu's number of players). A new board: 2.", 0,
                             {kAsSet, "1", "2", "3", "4"}});
    game.features.push_back({"board_extend", kPage, "Extend", "The score that gives a life. A new board: 1,800,000.", 0,
                             {kAsSet, "800,000", "1,800,000", "2,800,000", "None"}});
    game.features.push_back({"board_continue", kPage, "Continue", "Whether a game over can be continued.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_demo_sound", kPage, "Demo sound", "Whether the attract mode plays sound.", 0, {kAsSet, "Off", "On"}});
    game.features.push_back({"board_player_select", kPage, "Player select",
                             "Whether the players' screen (the Punisher for player 1, Nick Fury for player 2) is shown after Start.",
                             0, {kAsSet, "Off", "On"}});
    AddHook(HookAt(kSettingsRead), "SettingsRead", SettingsRead);
    AddHook(HookAt(kGameStarts), "GameStarts", GameStarts);
}

} // namespace cps2recomp
