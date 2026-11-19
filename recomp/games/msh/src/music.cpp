// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program
// (driver 1.05) keeps a table of its songs at 0x9006, four bytes a song, as Street Fighter Alpha's 1.05A does: the
// word at 0x9000 is how many (0x400), and a song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x400;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts): the one-player game to its end, the
    // show with its demo sound on, and --music-test for the ones that end (`true`). A round on a character's stage plays
    // that character's theme (09 and the character's number); a one-player game fights each opponent on its own stage.
    TheGame().songs = {
        {0x09, "spiderman", "Spider-Man's theme"},
        {0x0A, "captain_america", "Captain America's theme"},
        {0x0B, "hulk", "Hulk's theme"},
        {0x0C, "iron_man", "Iron Man's theme"},
        {0x0D, "wolverine", "Wolverine's theme"},
        {0x0E, "psylocke", "Psylocke's theme"},
        {0x0F, "blackheart", "Blackheart's theme"},
        {0x10, "shuma_gorath", "Shuma-Gorath's theme"},
        {0x11, "juggernaut", "Juggernaut's theme"},
        {0x12, "magneto", "Magneto's theme"},
        {0x13, "doom", "Dr. Doom's theme"},
        {0x14, "thanos", "Thanos's theme"},
        // Between the fights
        {0x2F, "qsound", "The QSound logo", true},
        {0x07, "rankings", "The rankings"},
        {0x01, "opening", "The opening"},
        {0x02, "player_select", "Player select"},
        {0x08, "next_opponent", "The next opponent", true},
        {0x04, "versus", "Before each fight", true},
        {0x1E, "winner", "Winner"},
        {0x2B, "before_thanos", "Before Thanos"},
        {0x2E, "ending", "Ending"},
        {0x06, "game_over", "Game over", true},
    };
}

} // namespace cps2recomp
