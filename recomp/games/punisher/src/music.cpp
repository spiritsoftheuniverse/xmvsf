// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The Punisher: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program keeps
// a table of its songs at 0x8006, four bytes a song, after a word of how many there are (0x137), as Cadillacs and
// Dinosaurs' does; a song whose data starts with a 0 is music.
#include "punisher.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x8006;
    TheGame().songCount = 0x137;
    // Found by playing: attract mode, each stage from its start, and a game lost and continued
    // (CPS2RECOMP_SOUNDTRACK_LOG), and --music-test (with the test switch held) for the ones the board plays once
    // (`true`). Those not heard yet are listed by their numbers only.
    TheGame().songs = {
        {0x04, "story", "Attract mode: the story (and stage 5)"},
        {0xA1, "players", "After Start: the players' screen (the Punisher and Nick Fury)"},
        {0x00, "stage_1", "Stage 1: Back to the War (and stage 5's river)"},
        {0x08, "stage_2_title", "Stage 2's title"},
        {0x01, "stage_2", "Stage 2: Raid on Pantaberde"},
        {0x09, "stage_3_title", "Stage 3's title"},
        {0x02, "stage_3", "Stage 3: Waterfront Warfare"},
        {0x1A, "bonus", "The bonus round (breaking barrels, before stage 4)"},
        {0x0A, "stage_4_title", "Stage 4's title"},
        {0x03, "stage_4", "Stage 4: Death on Rails"},
        {0x0B, "stage_5_title", "Stage 5's title"},
        {0x0C, "stage_6_title", "Stage 6's title"},
        {0x05, "stage_6", "Stage 6: Final Punishment"},
        {0xA4, "stage_6_hall", "Stage 6: the hall, further on"},
        {0x06, "stage_6_machines", "Stage 6: the machine room, further on"},
        {0x1B, "boss_entrance", "A boss's entrance (stage 1)"},
        {0x18, "boss_entrance_2", "A boss's entrance (stage 2)"},
        {0x0D, "boss", "Boss fight (stages 1 and 3)"},
        {0x0E, "boss_2", "Boss fight (stages 2 and 4)"},
        {0x10, "stage_clear", "A stage cleared", true},
        {0xA2, "continue", "Continue?"},
        {0x13, "game_over", "Game over", true},
    };
}

} // namespace cps2recomp
