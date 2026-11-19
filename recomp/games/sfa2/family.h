// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 2 as one of the games made from its program, which build its sources (src/): what is its
// own. The places its sources name are its own, so there is no list of them (src/places.h).
#pragma once

#include <array>
#include <cstdint>

#include "game.h"

namespace sfa2::family {

struct Place {
    uint32_t sfa2, here;
};
inline constexpr std::array<Place, 0> kPlaces = {};

// Who a player is (a player's byte +82), by number: seen on the name plates with each number put in. 18 to 23 are
// other forms of six of them (the classic Chun-Li, Shin Akuma and Evil Ryu among them), each with its first form's
// stage.
inline constexpr int kCharacters = 24;
inline constexpr const char *kCharacterNames[kCharacters] = {
    "Ryu", "Ken", "Akuma", "Charlie", "Chun-Li", "Adon", "Sodom", "Guy", "Birdie", "Rose", "M. Bison", "Sagat", "Dan", "Sakura",
    "Rolento", "Dhalsim", "Zangief", "Gen", "Chun-Li (classic)", "Gen (another form)", "Shin Akuma", "Zangief (another form)",
    "Dhalsim (another form)", "Evil Ryu"};
// Whose other form each of those after the first eighteen is (src/livetag.cpp: its call as it tags out is its first
// form's)
inline constexpr uint8_t kFormOf[kCharacters - 18] = {0x04, 0x11, 0x02, 0x10, 0x0F, 0x00};

// The hidden characters the "Hidden characters" feature gives for one who chooses the first form (src/training.cpp),
// and what it says. (Gen's other form is left out: it is the style he changes to in a fight.)
struct HiddenForm {
    uint8_t first, other;
};
inline constexpr HiddenForm kHiddenForms[] = {{0, 23}, {2, 20}, {4, 18}, {15, 22}, {16, 21}};
inline constexpr const char *kHiddenWords =
    "Choosing Ryu, Akuma, Chun-Li, Dhalsim or Zangief gives their hidden form instead: Evil Ryu, Shin Akuma, the classic "
    "Chun-Li, and Dhalsim's and Zangief's other forms. For player 1, player 2 or both.";

// The characters with Super Street Fighter II's ways (Zangief's and Dhalsim's other forms): they never guard in the
// air, never rise quickly, and a throw of them ends their stun's count (src/guard.cpp, throws.cpp, player.cpp; the
// game tests their numbers, 023554, 022AF0, 0261BE). A player's record at `player`
inline bool Classic(uint32_t player) {
    const uint8_t who = cps2recomp::Ram8(player + 0x82);
    return who == 0x15 || who == 0x16;
}

// When the game's own mode of play (A5+138) is not the usual one, the guard-range test looks at both other
// records after the players' (helpers) after the things thrown (src/guard.cpp, 0234CA)
inline constexpr bool kGuardBothHelpers = true;

// A stun's length: by chance and by character (a row of the table at 09814C for each, the row from the character's
// own routine, 09AE84), src/player.cpp (022A12)
inline constexpr bool kStunByCharacter = true;

// How much the super combo gauge of a player knocked down by a hit goes up, after a hit taken in the air (src/player.cpp,
// ReelDown) (0227F2)
inline constexpr uint8_t kDownGauge = 3;

// A routine of this game's own that a round's end calls after the winner's count of rounds won (src/fight.cpp,
// RoundFighting): none
inline constexpr uint32_t kRoundWonToo = 0;

// A byte of a player's record that, set in player 1's, has the pushing apart look at player 2 first, once (src/collision.cpp,
// PushApart): none
inline constexpr uint32_t kPushOtherWay = 0;

// Box numbers in a frame are read as signed bytes by the hit test (ext.w: 0103EE), src/collision.cpp
inline constexpr bool kBoxNumbersUnsigned = false;

// A helper's hit (the attacker's +3E set), which builds no stun, also marks the victim as a special's knock-down does
// (+258: no quick rise), src/damage.cpp: no
inline constexpr bool kHelperHitMarks = false;

// The checks of the game's for M. Bison and for Chun-Li by number (src/player.cpp: Bison's standing animation and his
// own as a hit lands, 021756, 022140; Chun-Li's every step frozen by a hit, 021632)
inline bool Bison(uint8_t who) {
    return who == 0x0A;
}
inline bool ChunLi(uint8_t who) {
    return who == 0x04 || who == 0x12;
}

// A byte of the player's record that this game also clears as a hit lands (src/player.cpp, ReelBegins): none
inline constexpr uint32_t kReelClearsToo = 0;
// A hit of kind 0B ends there in this game (its animation, the colours put back, +250-252 cleared): no
inline constexpr bool kReelElevenEnds = false;
// How far the player's colour set goes on as auto guard takes a hit (src/player.cpp, ReelFrozen: 0225DC)
inline constexpr uint8_t kAutoGuardSets = 4;
// In the game's other modes of play, the byte of the other's record that a throw's reach takes for its box turned
// about (src/throws.cpp): none (always its facing, +0B)
inline constexpr uint32_t kThrowTurnedInModes = 0;

// The board's damage levels, as its test menu names them (src/board.cpp)
inline constexpr const char *kDamageLevels[4] = {"Low", "Medium", "Normal", "High"};
inline constexpr bool kDamageLevelsOwn = false; // (Alpha 2's own names, which the collection's other games share)

// The ROM file whose colours PalMod changes, and which set it is (src/colours.cpp)
inline constexpr const char *kColoursFile = "sz2u.07", *kColoursFileWhose = "the USA set's, 960430";

} // namespace sfa2::family
