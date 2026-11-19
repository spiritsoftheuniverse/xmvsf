// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound program (driver
// 1.05A, as Night Warriors') keeps a table of its songs at 0x9006, four bytes a song: the word at 0x9000 is how many
// (0x400), and a song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x400;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts): a training fight on each of the
    // fourteen stages (two players: the stage's tune), the one-player ladder to its end with Jin and Blodia (the story
    // has tunes of its own, which take the stages' place there), and attract mode.
    TheGame().songs = {
        // A two-player fight's, by its stage
        {0x12, "city", "The city and the burning city"},
        {0x13, "desert", "The desert, the desert at sunset and the ice cave at night"},
        {0x14, "space_station", "The space station"},
        {0x15, "town_3", "Town 3"},
        {0x16, "forest", "The forest"},
        {0x17, "fortress", "The fortress"},
        {0x18, "ice_cave", "The ice cave"},
        {0x1A, "jungle_base", "The jungle base and the burning ruins"},
        {0x1B, "volcano", "The volcano"},
        {0x1C, "space", "Space"},
        // The one-player story's fights
        {0x19, "story_fight_2", "A fight of the story (the second)"},
        {0x1D, "story_fight_5", "A fight of the story (the fifth)"},
        {0x1E, "story_fight_3", "A fight of the story (the third)"},
        {0x1F, "story_fight_6", "A fight of the story (the sixth)"},
        {0x32, "last_fight", "The last fight"},
        // Between the fights
        {0x2D, "qsound", "The QSound notice", true},
        {0x02, "select", "Pilot and machine select"},
        {0x0B, "phase", "A round begins (\"Phase 1\")", true},
        {0x07, "map", "The map before a fight"},
        {0x03, "story_1", "A scene of the story (1)"},
        {0x04, "story_2", "A scene of the story (2)"},
        {0x05, "story_3", "A scene of the story (3)"},
        {0x06, "story_4", "A scene of the story (4)"},
        {0x08, "story_5", "A scene of the story (5)"},
        {0x34, "story_6", "A scene of the story (6)"},
        {0xF1, "ending_start", "After the last fight"},
        {0x39, "ending", "Ending (Jin's)"},
        {0x36, "staff_roll", "Staff roll"},
        {0x0F, "game_over", "Game over", true},
        {0x0A, "ranking", "Ranking"},
        {0x01, "attract", "Attract mode: the machines"},
        {0x33, "attract_machine", "Attract mode: a machine's parts"},
    };
}

} // namespace cps2recomp
