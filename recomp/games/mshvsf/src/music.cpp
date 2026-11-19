// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h).
// The sound program (driver 1.31) keeps a table of its songs at 0x9006, four bytes a song: the word at 0x9000 is how
// many (0x932), and a song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x932;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts, and the characters' words traced):
    // the one-player game to its end and the show. Each character has a tune, 0C and its number (the word kCharacter /
    // 2): the screen before a match plays player 2's character on point's, the round its partner's; heard for Apocalypse,
    // Cyclops, Hulk, Wolverine, Shuma-Gorath, Spider-Man, Blackheart, Ken, Chun-Li, Zangief, Sakura and Cyber-Akuma, the
    // others by their numbers. The hidden characters have their other selves' (20 on are other music).
    TheGame().songs = {
        {0x0C, "apocalypse", "Apocalypse's theme"},
        {0x0D, "cyclops", "Cyclops's theme"},
        {0x0E, "captain_america", "Captain America's theme"},
        {0x0F, "hulk", "Hulk's theme"},
        {0x10, "wolverine", "Wolverine's theme"},
        {0x11, "shuma_gorath", "Shuma-Gorath's theme"},
        {0x12, "omega_red", "Omega Red's theme"},
        {0x13, "spiderman", "Spider-Man's theme"},
        {0x14, "blackheart", "Blackheart's theme"},
        {0x15, "ryu", "Ryu's theme"},
        {0x16, "ken", "Ken's theme"},
        {0x17, "chunli", "Chun-Li's theme"},
        {0x18, "dhalsim", "Dhalsim's theme"},
        {0x19, "zangief", "Zangief's theme"},
        {0x1A, "bison", "M. Bison's theme"},
        {0x1B, "akuma", "Akuma's theme"},
        {0x1C, "sakura", "Sakura's theme"},
        {0x1E, "dan", "Dan's theme"},
        {0x1F, "cyber_akuma", "Cyber-Akuma's theme"},
        // Between the fights
        {0x07, "qsound", "The QSound logo", true},
        {0x01, "opening", "The opening"},
        {0x02, "player_select", "Player select"},
        {0x04, "next_stage", "Before each stage"},
        {0x05, "winner", "Winner", true},
        {0x0B, "warning", "Before the last fights"},
        {0x32, "last_win", "After the last fight", true},
        {0x24, "ending", "Ending"},
        {0x20, "staff_roll", "Staff roll"},
        {0x0A, "game_over", "Game over"},
    };
}

} // namespace cps2recomp
