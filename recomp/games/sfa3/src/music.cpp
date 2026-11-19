// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound
// program (driver 1.71) keeps a table of its songs at 0x9006, four bytes a song, as the earlier drivers do, but its
// music's data begin as its effects' do, and some of its music (19-1C) has no entry there: the game asks for its tunes
// from 00 to 2F (each after the command FF07), and the songs from 30 on are effects and voices (machine/soundtrack.h,
// musicBelow).
//
// A fight's tune is a character's theme (003A30: the table at 003ABA by character, player 1's or player 2's by A5+130),
// with a few of its own for the ladder's last fights; the others are asked for by kind from the table at 003AF8.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0xAC0;
    TheGame().musicBelow = 0x30;
    // Found by playing: a fight with each character, the arcade ladder to its end (E. Honda and Ryu), a lost game, a
    // challenger and attract mode (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts), the tables above, and
    // --music-test for the ones the board plays once (`true`: quiet well before a minute)
    TheGame().songs = {
        // Each character's theme, played in the fights against them (sfa3.h, kStageTunes: on their stage)
        {0x09, "ryu", "Ryu's theme"},
        {0x0A, "ken", "Ken's theme"},
        {0x0B, "akuma", "Akuma's theme"},
        {0x0C, "charlie", "Charlie's theme"},
        {0x0D, "chunli", "Chun-Li's theme"},
        {0x0E, "adon", "Adon's theme"},
        {0x0F, "sodom", "Sodom's theme"},
        {0x10, "guy", "Guy's theme"},
        {0x11, "birdie", "Birdie's theme"},
        {0x12, "rose", "Rose's theme"},
        {0x13, "bison", "M. Bison's theme"},
        {0x14, "sagat", "Sagat's theme"},
        {0x15, "dan", "Dan's theme"},
        {0x16, "sakura", "Sakura's theme"},
        {0x17, "rolento", "Rolento's theme"},
        {0x18, "dhalsim", "Dhalsim's theme"},
        {0x19, "zangief", "Zangief's theme"},
        {0x1A, "gen", "Gen's theme"},
        {0x1E, "balrog", "Balrog's theme"},
        {0x1F, "cammy", "Cammy's theme"},
        {0x21, "honda", "E. Honda's theme"},
        {0x22, "blanka", "Blanka's theme"},
        {0x23, "mika", "R. Mika's theme"},
        {0x24, "cody", "Cody's theme"},
        {0x25, "vega", "Vega's theme"},
        {0x26, "karin", "Karin's theme"},
        {0x27, "juli_juni", "Juli's and Juni's theme"},
        {0x1D, "sodom_other", "Sodom's theme, another version (003A54: for one of his kinds)"},
        // The ladder's last fights (003A5E on)
        {0x29, "rival_final_stage", "The rival's fight on the final stage (the ninth)"},
        {0x1B, "ninth_akuma_stage", "The ninth fight when it is on Akuma's stage"},
        {0x28, "tenth_ryu", "The tenth fight when it is against Ryu"},
        {0x2C, "before_final", "Before the final fight (M. Bison's words)"},
        {0x20, "final", "The final fight (M. Bison)"},
        // Between the fights
        {0x2F, "notices", "The notices at the start of the show", true},
        {0x01, "attract", "The attract mode's introduction, to the title", true},
        {0x02, "player_select", "Player select"},
        {0x08, "introduction", "A character's introduction before a match"},
        {0x2B, "rival", "A rival's words before a match"},
        {0x04, "winner", "Winner (the winner's quote)"},
        {0x03, "challenger", "Here comes a new challenger", true},
        {0x05, "continue", "Continue?"},
        {0x06, "game_over", "Game over", true},
        {0x07, "ranking", "Score ranking"},
        {0x2D, "ending", "The ending and the staff roll"},
    };
}

} // namespace cps2recomp
