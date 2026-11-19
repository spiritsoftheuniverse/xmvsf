// SPDX-License-Identifier: GPL-3.0-only
//
// Street Fighter Alpha: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound
// program (driver 1.05A) keeps a table of its songs at 0x9006, four bytes a song, as Alpha 2's 1.05C does: the
// word at 0x9000 is how many (0x300), and a song whose data starts with a 0 is music.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x300;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts): a fight on each stage, the
    // one-player ladder to its end with every character, a lost game, a challenger and attract mode; and --music-test
    // for the ones the board plays once (`true`: quiet before 90 s). Three the board has are not placed yet: 14 (once,
    // 13 s), 25 and 34 (both over and over).
    TheGame().songs = {
        // Each character's theme, played on their stage (sfa.h, kStageTunes)
        {0x01, "ryu", "Ryu's theme"},
        {0x02, "ken", "Ken's theme"},
        {0x03, "chunli", "Chun-Li's theme"},
        {0x04, "sagat", "Sagat's theme"},
        {0x05, "sodom", "Sodom's theme"},
        {0x06, "adon", "Adon's theme"},
        {0x07, "birdie", "Birdie's theme"},
        {0x08, "guy", "Guy's theme"},
        {0x09, "charlie", "Charlie's theme"},
        {0x0A, "rose", "Rose's theme"},
        {0x0B, "akuma", "Akuma's theme"},
        {0x0C, "bison", "M. Bison's theme"},
        {0x32, "dan", "Dan's theme"},
        // Between the fights
        {0x22, "qsound", "The QSound notice", true},
        {0x0D, "attract", "The introduction (and its demonstration fights)", true},
        {0x0E, "player_select", "Player select"},
        {0x0F, "versus", "Versus screen", true},
        {0x10, "winner", "Winner (the winner's quote)", true},
        {0x12, "challenger", "Here comes a new challenger", true},
        {0x11, "continue", "Continue?"},
        {0x13, "game_over", "Game over", true},
        {0x24, "ranking", "Score ranking", true},
        // The endings, and the staff roll that follows most of them
        {0x15, "ryu_ending", "Ryu's ending"},
        {0x16, "ken_ending", "Ken's ending"},
        {0x17, "chunli_ending", "Chun-Li's ending (and its staff roll)", true},
        {0x18, "sagat_ending", "Sagat's ending"},
        {0x19, "sodom_ending", "Sodom's ending"},
        {0x1A, "adon_ending", "Adon's ending"},
        {0x1B, "birdie_ending", "Birdie's ending"},
        {0x1C, "guy_ending", "Guy's ending"},
        {0x1D, "charlie_ending", "Charlie's ending"},
        {0x1E, "rose_ending", "Rose's ending"},
        {0x1F, "akuma_ending", "Akuma's ending"},
        {0x20, "bison_ending", "M. Bison's ending"},
        {0x33, "dan_ending", "Dan's ending"},
        {0x21, "staff_roll_a", "Staff roll (after Ryu's, Akuma's, Charlie's, Guy's, Rose's, M. Bison's and Sagat's endings)"},
        {0x31, "staff_roll_b", "Staff roll (after Ken's, Adon's, Sodom's, Birdie's and Dan's endings)"},
    };
}

} // namespace cps2recomp
