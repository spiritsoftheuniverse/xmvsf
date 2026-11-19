// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: its music, for a custom soundtrack (port/soundtrack.h, machine/soundtrack.h). The sound
// program (driver 1.06b) keeps a table of its songs at 0x9006, four bytes a song, as the other drivers do: the word at
// 0x9000 is how many (0x280). The game plays Super Turbo's music by Super Turbo's numbers, whichever versions fight
// (logged over a one-player ladder as Super Turbo's Ryu and as World Warrior's, and two-player games with each version):
// only the introduction's and the character choice's moved.
#include "game.h"

namespace cps2recomp {

void RegisterMusic() {
    TheGame().songTable = 0x9006;
    TheGame().songCount = 0x280;
    // Found by playing (CPS2RECOMP_SOUNDTRACK_LOG, with pictures where each starts): the one-player ladder to its end with
    // Ryu, two-player games, a one-player game lost (made-up play: Guile's stage went to its faster version, sixteen on,
    // as the health went low, and the continue screen came), and attract mode. Each stage's tune has a second version,
    // sixteen on, faster, once a fighter's health is low (as in Super Turbo). The other characters' endings are not
    // placed yet.
    TheGame().songs = {
        // Each stage's (hsf2.h, kStageTunes)
        {0x02, "ryu", "Ryu's stage"},
        {0x03, "honda", "E. Honda's stage"},
        {0x05, "blanka", "Blanka's stage"},
        {0x07, "guile", "Guile's stage"},
        {0x01, "ken", "Ken's stage"},
        {0x04, "chunli", "Chun-Li's stage"},
        {0x06, "zangief", "Zangief's stage"},
        {0x08, "dhalsim", "Dhalsim's stage"},
        {0x0C, "bison", "M. Bison's stage"},
        {0x0B, "sagat", "Sagat's stage"},
        {0x09, "balrog", "Balrog's stage"},
        {0x0A, "vega", "Vega's stage"},
        {0x0E, "cammy", "Cammy's stage"},
        {0x0F, "thawk", "T. Hawk's stage"},
        {0x0D, "feilong", "Fei Long's stage"},
        {0x10, "deejay", "Dee Jay's stage"},
        // ... and their faster versions, once a fighter's health is down to a third
        {0x12, "ryu_later", "Ryu's stage, faster (a fighter\'s health low)"},
        {0x13, "honda_later", "E. Honda's stage, faster (a fighter\'s health low)"},
        {0x15, "blanka_later", "Blanka's stage, faster (a fighter\'s health low)"},
        {0x17, "guile_later", "Guile's stage, faster (a fighter\'s health low)"},
        {0x11, "ken_later", "Ken's stage, faster (a fighter\'s health low)"},
        {0x14, "chunli_later", "Chun-Li's stage, faster (a fighter\'s health low)"},
        {0x16, "zangief_later", "Zangief's stage, faster (a fighter\'s health low)"},
        {0x18, "dhalsim_later", "Dhalsim's stage, faster (a fighter\'s health low)"},
        {0x1C, "bison_later", "M. Bison's stage, faster (a fighter\'s health low)"},
        {0x1B, "sagat_later", "Sagat's stage, faster (a fighter\'s health low)"},
        {0x19, "balrog_later", "Balrog's stage, faster (a fighter\'s health low)"},
        {0x1A, "vega_later", "Vega's stage, faster (a fighter\'s health low)"},
        {0x1E, "cammy_later", "Cammy's stage, faster (a fighter\'s health low)"},
        {0x1F, "thawk_later", "T. Hawk's stage, faster (a fighter\'s health low)"},
        {0x1D, "feilong_later", "Fei Long's stage, faster (a fighter\'s health low)"},
        {0x20, "deejay_later", "Dee Jay's stage, faster (a fighter\'s health low)"},
        // The last fight of the ladder, against M. Bison
        {0xD5, "final_fight", "The final fight"},
        {0xD6, "final_fight_later", "The final fight, faster (a fighter\'s health low)"},
        // Between the fights
        {0x33, "title", "The introduction and the title", true},
        {0x3D, "qsound", "The QSound notice", true},
        {0x34, "player_select", "Player select"},
        {0x35, "versus", "Versus screen", true},
        {0x36, "winner", "Winner (the winner's quote)", true},
        {0x37, "continue", "Continue?", true},
        {0x3A, "ranking", "Score ranking", true},
        // The ending and the staff roll
        {0x23, "ryu_ending", "Ryu's ending"},
        {0x3C, "staff_roll", "Staff roll"},
    };
}

} // namespace cps2recomp
