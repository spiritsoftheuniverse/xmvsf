// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Dimahoo: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program (Capcom's
// driver 1.80) keeps a table of its songs at 0x9006, four bytes a song, after a word of how many there are (0xD2) at
// 0x9000, as Capcom's other games do.
#include "dimahoo.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0xD2;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG): attract mode, the ladder movie through the six stages to the ending
    // and the title again, and a game lost. Those not heard yet are listed by their numbers only.
    TheGame().songs = {
        {0xD1, "title", "The title"},
        {0xC1, "story", "The story (the show, after the logos)"},
        {0xC2, "choose", "The choice of character"},
        {0xC3, "stage_1", "Stage 1"},
        {0xC4, "stage_2", "Stage 2"},
        {0xC5, "stage_3", "Stage 3"},
        {0xC6, "stage_4", "Stage 4"},
        {0xC7, "stage_5", "Stage 5"},
        {0xC8, "stage_6", "Stage 6"},
        {0xC9, "boss", "A boss (stages 1, 3, 4 and 6)"},
        {0xCA, "boss_2", "Stage 2's boss"},
        {0xCC, "boss_3", "Stage 5's boss, and stage 6's before the last"},
        {0xCB, "last_boss", "The last boss"},
        {0xCD, "stage_clear", "A stage cleared", true},
        {0xCE, "ending", "The ending"},
        {0xCF, "staff", "The staff roll"},
    };
}

} // namespace cps2recomp
