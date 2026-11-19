// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel vs. Capcom: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program
// (driver 1.30) keeps a table of its songs at 0x9006, four bytes a song: the word at 0x9000 is how many (0x900), and a
// song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x900;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts, and the characters' words traced):
    // the one-player game to its end and the show. As in Marvel Super Heroes vs. Street Fighter, each character has a
    // tune, 0C and its number (the word kCharacter / 2): the screen before a match plays player 2's first character's,
    // the round its second's; heard for Onslaught and every one of the players' characters but Roll and Zangief, those
    // two by their numbers. The other hidden characters have their other selves' (20 on are other music).
    TheGame().songs = {
        {0x0C, "onslaught", "Onslaught's theme"},
        {0x0D, "war_machine", "War Machine's theme"},
        {0x0E, "captain_america", "Captain America's theme"},
        {0x0F, "hulk", "Hulk's theme"},
        {0x10, "wolverine", "Wolverine's theme"},
        {0x11, "gambit", "Gambit's theme"},
        {0x12, "venom", "Venom's theme"},
        {0x13, "spiderman", "Spider-Man's theme"},
        {0x14, "roll", "Roll's theme"},
        {0x15, "ryu", "Ryu's theme"},
        {0x16, "captain_commando", "Captain Commando's theme"},
        {0x17, "chunli", "Chun-Li's theme"},
        {0x18, "jin", "Jin's theme"},
        {0x19, "zangief", "Zangief's theme"},
        {0x1A, "strider", "Strider Hiryu's theme"},
        {0x1B, "morrigan", "Morrigan's theme"},
        {0x1C, "megaman", "Mega Man's theme"},
        // Between the fights
        {0x07, "qsound", "The QSound logo", true},
        {0x01, "opening", "The opening"},
        {0x02, "player_select", "Player select"},
        {0x04, "next_stage", "Before each stage"},
        {0x05, "winner", "Winner", true},
        {0x0B, "warning", "Before Onslaught"},
        {0x2E, "last_win", "After Onslaught"},
        {0x20, "ending", "Ending"},
        {0x1D, "staff_roll", "Staff roll"},
        {0x0A, "game_over", "Game over"},
        {0x2F, "rankings", "The rankings"},
    };
}

} // namespace cps2recomp
