// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound
// program (driver 1.05A) keeps a table of its songs at 0x9006, four bytes a song, as Street Fighter Alpha's does: the
// word at 0x9000 is how many (0x315), and a song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x315;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts): the one-player game to its end, and
    // the stage viewer on the stages it missed; --music-test for the ones that end (`true`). A round on a stage plays its
    // character's theme (07 and the stage's number); a one-player game fights each opponent on its own stage.
    TheGame().songs = {
        {0x07, "wolverine", "Wolverine's theme"},
        {0x08, "psylocke", "Psylocke's theme"},
        {0x09, "colossus", "Colossus's theme"},
        {0x0A, "cyclops", "Cyclops's theme"},
        {0x0B, "storm", "Storm's theme"},
        {0x0C, "iceman", "Iceman's theme"},
        {0x0D, "spiral", "Spiral's theme"},
        {0x0E, "silver_samurai", "Silver Samurai's theme"},
        {0x0F, "omega_red", "Omega Red's theme"},
        {0x10, "sentinel", "Sentinel's theme"},
        {0x11, "juggernaut", "Juggernaut's theme"},
        {0x12, "magneto", "Magneto's theme"},
        // Between the fights
        {0x05, "winner", "Winner", true},
        {0x02, "magneto_beaten", "Magneto beaten"},
        {0x17, "ending", "Ending"},
        {0x18, "ending_2", "Ending, continued"},
        {0x19, "staff_roll", "Staff roll"},
        {0x04, "game_over", "Game over", true},
    };
}

} // namespace cps2recomp
