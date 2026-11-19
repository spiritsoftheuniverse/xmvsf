// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound
// program (driver 1.16b) keeps a table of its songs at 0x9006, four bytes a song: the word at 0x9000 is how many
// (0x700), and a song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x700;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts): the one-player game to its end,
    // two-player fights with each character on player 2's side (a round plays player 2's point character's tune: 0B
    // and the character's number), and the show
    TheGame().songs = {
        // Each character's own (Alpha Chun-Li has Chun-Li's)
        {0x0B, "wolverine", "Wolverine's theme"},
        {0x0C, "cyclops", "Cyclops's theme"},
        {0x0D, "storm", "Storm's theme"},
        {0x0E, "rogue", "Rogue's theme"},
        {0x0F, "gambit", "Gambit's theme"},
        {0x10, "sabretooth", "Sabretooth's theme"},
        {0x11, "juggernaut", "Juggernaut's theme"},
        {0x12, "magneto", "Magneto's theme"},
        {0x13, "apocalypse", "Apocalypse's theme"},
        {0x14, "ryu", "Ryu's theme"},
        {0x15, "ken", "Ken's theme"},
        {0x16, "chunli", "Chun-Li's theme"},
        {0x17, "dhalsim", "Dhalsim's theme"},
        {0x18, "zangief", "Zangief's theme"},
        {0x19, "bison", "M. Bison's theme"},
        {0x1A, "akuma", "Akuma's theme"},
        {0x1B, "charlie", "Charlie's theme"},
        {0x1C, "cammy", "Cammy's theme"},
        // Between the fights
        {0x07, "qsound", "The QSound logo", true},
        {0x01, "opening", "The opening"},
        {0x02, "player_select", "Player select"},
        {0x04, "next_stage", "Before each stage"},
        {0x05, "winner", "Winner", true},
        {0x2B, "warning", "Before Apocalypse"},
        {0x24, "ending", "Ending"},
        {0x1D, "staff_roll", "Staff roll"},
        {0x0A, "game_over", "Game over"},
    };
}

} // namespace cps2recomp
