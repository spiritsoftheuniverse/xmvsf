// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Progear: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program (Capcom's
// driver 1.80) keeps a table of its songs at 0x9006, four bytes a song, after a word of how many there are (0xDE) at
// 0x9000, as Capcom's other games do.
#include "progear.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0xDE;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG): attract mode, the ladder movie through the five stages into the second
    // loop, and a game lost. Those not heard yet are listed by their numbers only.
    TheGame().songs = {
        {0x0E, "title", "The title (and the show; it comes back after a game over)"},
        {0x03, "choose", "The choice of pilot and gunner"},
        {0x04, "stage_1", "Stage 1"},
        {0x05, "stage_2", "Stage 2"},
        {0x06, "stage_3", "Stage 3"},
        {0x07, "stage_4", "Stage 4"},
        {0x08, "stage_5", "Stage 5"},
        {0x02, "boss", "A boss (stages 1 to 4)"},
        {0x0C, "last_boss", "The last boss (stage 5)"},
        {0x09, "stage_clear", "A stage cleared", true},
    };
}

} // namespace cps2recomp
