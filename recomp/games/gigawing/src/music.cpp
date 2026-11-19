// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Giga Wing: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program (Capcom's
// driver 1.40) keeps a table of its songs at 0x9006, four bytes a song, after a word of how many there are (0xAF) at
// 0x9000; a song whose data starts with a 0 is music.
#include "gigawing.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0xAF;
    // Found by playing: attract mode, each stage from its start (start at stage), a game lost and continued
    // (CPS2RECOMP_SOUNDTRACK_LOG). Those not heard yet are listed by their numbers only.
    TheGame().songs = {
        {0x01, "title", "The title"},
        {0x15, "medallion", "Attract mode: the Medallion's story"},
        {0x02, "choose", "The choice of fighter"},
        {0x0B, "pilot_story", "A pilot's story, before each stage"},
        {0x03, "stage_1", "Stage 1"},
        {0x04, "stage_2", "Stage 2"},
        {0x05, "stage_3", "Stage 3"},
        {0x12, "stage_3_later", "Stage 3, a stretch after its start"},
        {0x06, "stage_4", "Stage 4"},
        {0x07, "stage_5", "Stage 5"},
        {0x08, "stage_6", "Stage 6: the bosses again"},
        {0x09, "stage_7", "Stage 7: the Medallion"},
        {0x13, "midboss", "A big enemy part way through a stage (stages 2, 4 and 5)"},
        {0x0A, "boss", "A boss"},
        {0x14, "stage_clear", "A stage cleared", true},
        {0x10, "continue", "Continue and game over"},
    };
}

} // namespace cps2recomp
