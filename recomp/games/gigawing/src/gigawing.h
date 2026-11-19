// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Giga Wing (gigawing, USA 990222): names for what is known of the game's memory, with how each was found
// (PROGRESS.md has more). The game is Takumi's, not Capcom's own code: it keeps its variables at A5 = FF8000 (set at
// 000130), nearly all below it, and runs its work as 83 tasks (a table of 16-byte entries at A5-162A, their routines
// in the table at 000634, eight bytes an entry; the loop at 000584). Its code is sprinkled with
// `cmpi.l #$19721027,d1`, which does nothing.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

namespace cps2recomp::gigawing {

constexpr uint32_t kVars = 0xFF8000; // A5

// --- coins and the board's settings ---

// Credits (byte, up to 9): a coin adds them at 001826; Start takes one at 057BBE (found from dumps after 1, 2, 3 coins)
constexpr uint32_t kCredits = kVars - 0x5F6B;
// The game's own free play (byte): set by the settings routine (000E48) when the coin mode is 0x12, the last of the
// test menu's; Start then takes no credit (057BBE) and the screens say FREE PLAY. The coin mode (byte) is the
// settings record's first byte, copied at 000E04.
constexpr uint32_t kFreePlay = kVars - 0x5F69, kCoinMode = kVars - 0x5F5F;
constexpr uint8_t kCoinModeFreePlay = 0x12;

// The board's settings: the record the settings memory keeps (A5-5F4C, FF20B4), copied into the variables below by
// 000D92 (at 000DE0 on) as the board starts and when the test menu is left. Found by stepping each option of the
// test menu's Configuration pages with Right and Left and reading the variables after each press:
//   System: +00 coin mode (0-0x12; 0x12 free play)   +01 continue (0 on, 1 off; on)   +02 monitor (0, 0x80 flipped)
//           +03 demo sound (0 off, 1 on; on)          +04 sound mode (0 QSound, 1 mono)   +05 chute type (0-2)
//   Game:   +10 difficulty (0-7, shown 1-8; 1)       +11 ship stock (2-5; 3)              +12 force bomb stock (0-4; 2)
// The game reads them where it uses them: the ships at a game's start (0085F4), the bombs at each ship's start
// (009236).
constexpr uint32_t kSettings = kVars - 0x5F4C;
// The test menu is on (byte, A5-5F6D: 0B895A sets it as the menu opens, and the coins are not counted meanwhile,
// 0017DC; found from dumps before and in the menu). The menu shows and edits the variables below themselves.
constexpr uint32_t kTestMode = kVars - 0x5F6D;
constexpr uint32_t kSetContinue = kVars - 0x5DBB, kSetDemoSound = kVars - 0x5DBD, kSetDifficulty = kVars - 0x5DBE,
                   kSetShips = kVars - 0x5DB8, kSetBombs = kVars - 0x5DB7;

// --- what the game is doing ---

// The show's and the game's mode (word, A5-0D82; the routine of task 82, 0072B8, jumps on it). Found by reading it
// through the board's start, the show and a game:
//   0C the QSound logo, 10 Capcom's, 11 the story ("The Medallion is the stone..."), 05 the ranking and the like, 04 the
//   title (PRESS START), 12 a demonstration, 08 the ranking after a game; 0B the choice of fighter, 0E the pilot's story
//   after it, 0D the lesson (Start skips them), 06 a stage played, 07 continue and game over, 02 after a game over.
// Before the task loop runs (the warning) it is 0. With credits or free play the board goes from the warning straight
// to the title.
constexpr uint32_t kMode = kVars - 0xD82;
enum : uint16_t {
    kModeTitle = 0x04,
    kModeShowStage = 0x05,
    kModePlay = 0x06,
    kModeContinue = 0x07,
    kModeStory = 0x11,
    kModeChoose = 0x0B,
    kModeLesson = 0x0D,
    kModePilot = 0x0E,
    kModeDemo = 0x12,
};

// The players in the game (byte, A5-0FA4): bit 0 player 1, bit 1 player 2, set when Start takes the player in
// (057C20 with free play; with coins the bits above are the credits' too); 0 in the show.
constexpr uint32_t kPlayersIn = kVars - 0xFA4;

// The stage (word, A5-0CC0): 0 the first of seven, counted on at each stage's end (found from dumps through stages
// 1-4 with the ships kept up)
constexpr uint32_t kStage = kVars - 0xCC0;
constexpr int kStages = 7;
// The last stage (6) is the Medallion's fight from its start, a boss with no clock (bit 3 of A5-0D84 never set there:
// found by tracing a game begun at it); its end is the ending
constexpr int kLastStage = 6;
// 008488: a game's start has put the stage at 0 (008484), before the lesson; the stage is loaded from it as it begins.
// (The show's demonstrations set it at 008C78.) The next stage is counted at 0915DA.
constexpr uint32_t kGameBegins = 0x008488;

// --- the players ---

constexpr int kPlayers = 2;
// Ships left (word a player, A5-0D24 on: 0085F8 sets them from the ship stock at a game's start, 008F7E takes one at a
// loss and ends the game at 0) and force bombs (word a player, A5-0D20 on: 009236 sets them from the bomb stock at each
// ship's start, 04AE98 takes one when the bomb button is pressed). Found by watching the panel's numbers.
constexpr uint32_t kShips = kVars - 0xD24, kBombs = kVars - 0xD20;
// The multiplier (BCD long a player, A5-0D1C on: the number by the medal at the top; 02369C, a gold medal picked up,
// adds the player's chain to it, at most 49,999,999) and the chain (BCD long a player, A5-0D10 on: each medal's value
// is added to it as it is picked up; 02368C puts it back to 0)
constexpr uint32_t kMultiplier = kVars - 0xD1C, kChain = kVars - 0xD10;
// The fighter (word a player, A5-0CF8 on: 0 Sinnosuke and the Raijin, 1 Ruby and the Carmine, 2 Isha and the Porchka,
// 3 Stuck and the Widerstand; Down steps it 0, 3, 2, 1 on the choice of fighter) and the shot's power (word a player,
// A5-0CF4 on: 0 at a ship's start, one more with each P picked up)
constexpr uint32_t kFighter = kVars - 0xCF8, kPower = kVars - 0xCF4;
// The choice of fighter (040B6A, each player's cursor an object: +02 its fighter, +03 the other player's): 040B8E adds
// the step (D2: 1 down, -1 up) and keeps it below 4, passing over the other player's fighter unless bit 5 of A5-0D7F is
// set; 040BBA puts it in the player's fighter word. There is a fifth, 4, the choice never reaches.
constexpr uint32_t kChooseStep = 0x040B8E, kChooseStepped = 0x040BBA, kPlayerFlags = kVars - 0xD7F;
constexpr uint8_t kPlayer2Choosing = 0x20;
constexpr int kFighters = 5;
// Scores: six BCD bytes a player (twelve digits, from A5-0F32 player 1's and A5-0F2A player 2's; the panel shows them
// without the leading zeros: found by searching dumps for its numbers, which pass ten digits late in a game)
constexpr uint32_t kScore1 = kVars - 0xF32, kScore2 = kVars - 0xF2A;
inline uint64_t Score(int n) {
    const uint32_t at = n == 0 ? kScore1 : kScore2;
    uint64_t value = 0;
    for (uint32_t k = 0; k < 6; k++) {
        const uint8_t b = Ram8(at + k);
        value = value * 100 + (b >> 4) * 10 + (b & 0xF);
    }
    return value;
}

// The players' ships: objects of 0xC0 bytes (player 1's at FFADEA, player 2's at FFAEAA; found from the pointers to
// the ships and bombs they hold, +58 and +74). In each:
//   +00 word: flags (bit 15 in use; bit 12 hit: set by 02398C, the hit test's answer; the ship then blows up and
//       FFAE0E counts the frames to the next one)
//   +0A word: across, the picture's column (0-383)          +0E word: up, from the picture's foot
//   +16, +18, +1A, +1C words: the hit box, from the middle: left, right, up, down (3 each: a 6 x 6 square)
constexpr uint32_t kShip1 = 0xFFADEA, kShipSize = 0xC0;
inline uint32_t Ship(int n) {
    return kShip1 + kShipSize * static_cast<uint32_t>(n);
}
constexpr uint32_t kShipFlags = 0x00, kShipX = 0x0A, kShipY = 0x0E, kShipBoxLeft = 0x16, kShipBoxRight = 0x18,
                   kShipBoxUp = 0x1A, kShipBoxDown = 0x1C;
constexpr uint16_t kShipHit = 0x1000;
// +8A word: the Reflect Barrier, charged up to 0x1000 while the shot button is held and run down while the barrier is
// up (found from dumps holding the button)
constexpr uint32_t kShipReflect = 0x8A;
constexpr int kReflectFull = 0x1000;

// The boss's clock: the TIME at the right while a boss is fought (word, A5+57FC: seconds left, counted down every 63
// frames by 0593F6 while bit 3 of the byte at A5-0D84 is set; at 0 the boss leaves). Found from the panel's number.
constexpr uint32_t kBossSeconds = kVars + 0x57FC, kBossFlags = kVars - 0xD84;
constexpr uint8_t kBossClock = 0x08;
// ... and bit 1 is set once the boss is over (beaten or gone: the clock's bit stays set through the stage's end)
constexpr uint8_t kBossOver = 0x02;

// The enemies' shots: a list (A5-10E0 holds the first; each starts with the next's address), each with its place at
// +06 across and +0A up, a point tested against the ships' boxes (029DE8)
constexpr uint32_t kShots = kVars - 0x10E0, kShotX = 0x06, kShotY = 0x0A;
// The enemies' bodies: 0x30 records of 0x80 bytes from A5-0A36 (023A4C), in use when (+00 & E480) is 8000, their place
// at +0A and +0E and their box +16 right, +18 left, +1A up and +1C down; a ship's middle is tested against them
constexpr uint32_t kBody = kVars - 0xA36, kBodySize = 0x80, kBodyX = 0x0A, kBodyY = 0x0E, kBodyRight = 0x16, kBodyLeft = 0x18,
                   kBodyUp = 0x1A, kBodyDown = 0x1C;
constexpr int kBodies = 0x30;
// The most bombs a ship can hold (023764 counts no further) and the shot's highest power (0237AC)
constexpr int kMostBombs = 8, kMostPower = 3;
// The scroll screens that are the stage (the panel and the words are scroll screen 1, of 8-pixel tiles)
constexpr unsigned kStageLayers = 0xC;

// 023956: a ship (A2) tested against the enemies' shots and bodies (029DE8, its box from +16-+1C); 02398C is what it
// does on a hit: sets the ship's hit bit, and answers -1
constexpr uint32_t kShipHitAnswer = 0x02398C;
// 023A4C: a ship (A0) tested against the enemies' bodies (the table at A5-0A20, 0x30 of 0x80 bytes); 023AAE sets the
// ship's hit bit when one touches it (and marks the enemy)
constexpr uint32_t kShipTouched = 0x023AAE;
// 04AE98: a force bomb is taken from the ship's count (A1 points at it)
constexpr uint32_t kBombTaken = 0x04AE98;

} // namespace cps2recomp::gigawing
