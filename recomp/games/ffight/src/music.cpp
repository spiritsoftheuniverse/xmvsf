// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Final Fight: the board's music numbers (from sound_program's song table at 0x1003, 0x60 entries of 2 bytes) and
// what the player sees for each in the custom soundtrack window.
#include "game.h"

namespace cps2recomp {

void RegisterMusic(Game &g) {
    g.songTable = 0x1003;
    g.songCount = 0x60;
    g.songEntry = 2;
    g.songs = {
        {0x52, "opening", "Opening / Title", true},
        {0x55, "player_select", "Player Select"},
        {0x57, "stage_start", "Stage Start", true},
        {0x50, "stage_1", "Stage 1: Slum"},
        {0x41, "stage_1_underground", "Stage 1: Slum (Underground)"},
        {0x42, "stage_2", "Stage 2: Subway"},
        {0x43, "stage_2_ring", "Stage 2: Subway (Ring)"},
        {0x40, "stage_3", "Stage 3: West Side / Stage 6: Up Town"},
        {0x4C, "bonus", "Bonus Stage"},
        {0x48, "stage_4", "Stage 4: Industrial Area"},
        {0x45, "stage_5", "Stage 5: Bay Area"},
        {0x44, "boss", "Boss Theme"},
        {0x46, "theme_46", "Stage / Boss Theme (0x46)"},
        {0x47, "theme_47", "Stage / Boss Theme (0x47)"},
        {0x49, "final_boss", "Final Boss (Belger)"},
        {0x51, "stage_clear", "Stage Clear", true},
        {0x58, "ending", "Ending"},
        {0x53, "continue", "Continue"},
        {0x54, "game_over", "Game Over", true},
    };
}

} // namespace cps2recomp
