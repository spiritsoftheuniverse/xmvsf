// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Progear (progearu, USA 010117): names for what is known of the game's memory, with how each was found (PROGRESS.md
// has more). The game is Cave's: its variables are at fixed addresses in work RAM (no base register), the objects are
// records in work RAM too, and its code is sprinkled with `cmpi.l #$63A1B8D3,d1`, which does nothing (as Giga Wing's
// is with another number). Heights are counted up from the picture's foot, and places are in 64ths of a pixel.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

namespace cps2recomp::progear {

// --- coins and the board's settings ---

// Credits (byte, up to 9): a coin adds them at 0465DC / 04677A; Start takes them at 0464DC (found from dumps after
// 1, 2, 3 coins: the board asks two coins a credit)
constexpr uint32_t kCredits = 0xFF2389;
// The game's own free play (byte): set by the settings routine (01154A) when the coin mode is 0x12, the last of the test
// menu's; Start then takes no credit (04641A) and the screens say FREE PLAY. The coin mode (byte) is the settings
// record's first byte, copied at 0114FC.
constexpr uint32_t kFreePlay = 0xFF238B, kCoinMode = 0xFF2395;
constexpr uint8_t kCoinModeFreePlay = 0x12;

// The board's settings: the record the settings memory keeps (FF275E), copied into the variables below by 011476 as the
// board starts and when the test menu is left. Found by stepping each option of the test menu's Configuration pages
// (Shot 1 steps a value down) and reading the variables after each press:
//   System: +00 coin mode (0-0x12; 0x12 free play; 9 a new board's: 2 coins 1 credit)  +01 continue (0 on, 1 off)
//           +02 monitor  +03 demo sound (0 off, 1 on, 2 half; on)  +04 sound mode  +05 chute type
//   Game:   +10 difficulty (0-7, shown 1-8; 1)
//           +11 ship stock (0: 3, 1: 4, 2: 5, 3: 1, 4: 2; 0, three ships: 00F904 reads it through a table at 00F914)
//           +12 extend (0: first at 3,000,000 and every 4,000,000 after; 1: at 4,000,000 and 9,000,000; 2: every
//               1,500,000; 3: none)
//           +13 rapid fire on button 3 (0 off, 1 on)
// The test menu does not edit these: it copies them into its own (FF06D6 on, 001D76) as it opens and back at Save &
// Exit (001CB6), so a value put in them before it opens is what it shows.
constexpr uint32_t kSettings = 0xFF275E;
constexpr uint32_t kSetContinue = 0xFF28EF, kSetDemoSound = 0xFF28ED, kSetDifficulty = 0xFF28EC, kSetShips = 0xFF28F3,
                   kSetExtend = 0xFF28F4, kSetRapid = 0xFF28F9;

// --- what the game is doing ---

// The mode (word; the jump table at 00DE28 runs on it). Found by tracing it through the board's start, the show and a
// game:
//   00 the warning, 0E the logos, 01 the title in the show, 05 a demonstration, 0D the title with credits or free play
//   (then 04: Start is taken there, 00E90E), 0B the choice of pilot and gunner and the stage's start, 07 a stage played,
//   08 continue and game over, 0C the ranking.
constexpr uint32_t kMode = 0xFF3ECE;
enum : uint16_t {
    kModeWarning = 0x00,
    kModeShowTitle = 0x01,
    kModeTitleStart = 0x04,
    kModeDemo = 0x05,
    kModePlay = 0x07,
    kModeContinue = 0x08,
    kModeChoose = 0x0B,
    kModeTitle = 0x0D,
    kModeLogos = 0x0E,
};

// Who plays (word): 00E9A4 puts E0 there when player 1 starts, D0 when player 2 does; bit 4 of its low byte is set while
// player 1 is out and bit 5 while player 2 is (00F762, 00F77C give the ships only to a player whose bit is clear; 00F4BC
// clears it when one joins in). 0 in the show's title.
constexpr uint32_t kPlayers = 0xFF3ED0;
constexpr uint16_t kPlayer1Out = 0x10, kPlayer2Out = 0x20;

// The stage (word, 0 the first of five): 07CDCA counts it on at a stage's end, with its double and its quadruple beside
// it (FF3FA4, FF3FA6: the indexes the stage's tables are read with); a 5 is the ending (07CDE6). Found from a trace
// through stages 1 to 3.
constexpr uint32_t kStage = 0xFF3FA2, kStage2 = 0xFF3FA4, kStage4 = 0xFF3FA6;
constexpr int kStages = 5;
// 00E9C2: Start is taken on the title (the mode becomes 0B, the choice of pilot); the stage is 0, and is read from here
constexpr uint32_t kGameBegins = 0x00E9C2;

// The boss (byte): the stage's controller (a record at FF2952) sets +09 when the stage's scroll reaches its boss's place
// (06AC5A; the table at 06ABC6 has, for each stage, where the music turns and where the boss comes), and the stage's end
// clears it (found from a trace: set through each boss, clear between)
constexpr uint32_t kBossOn = 0xFF295B;

// --- the players ---

constexpr int kPlayers2 = 2;
// Ships (word a player, FF3F72 on, with the one in play): set at a game's start from the ship stock (00F76C), one taken
// as a ship's blast ends (03A1A4; the game goes to the continue at 0)
constexpr uint32_t kShips = 0xFF3F72;
// Scores (BCD long a player, FF3DC6 and FF3DCA: 00EA88 clears them; the panel shows them with a 0 more)
constexpr uint32_t kScore1 = 0xFF3DC6;
// The pilot (word a player, FF39E4 on: 0 Ring and the gambler's plane, 1 Bolt and the militant's) and the gunner (word a
// player, FF39E8 on: 0 Chain, type alpha; 1 Nail, beta; 2 Rivet, gamma), found from dumps on the choice with Down
// pressed. The cursor objects keep their own copies (FF5C81, FF5C85), which Shot takes.
constexpr uint32_t kPilot = 0xFF39E4, kGunner = 0xFF39E8, kPilotCursor = 0xFF5C81, kGunnerCursor = 0xFF5C85;
constexpr int kPilots = 2, kGunners = 3;

// The players' ships: records of 0x60 bytes (player 1's at FF42E2, player 2's at FF4342: 0057AC tests both). In each:
//   +00 word: flags (bit 15 in use; bit 12, bit 4 of the byte, hit: set by the hit test 04DF7A at 04E03C; 0393DC clears
//       it and blows the ship up)
//   +02 word: up from the picture's foot, +04 word: across, both in 64ths of a pixel
//   +10, +12, +14, +16 words: the hit box from the middle: up, down, right, left (in 64ths)
//   +20 word: the shot's power, twice its level: 0 to 8 (five levels), 2 more for each P picked up (0395EC), 0 at a
//       ship's start (0390CC); +52 the highest it has been since (03925C), FF3ECA a copy for the panel (039048). Found
//       from a trace through the first stages.
//   +22 word: bombs (3 at each ship's start, 0396FA takes one)
//   +3E byte: frames left out of harm (a ship's start; FF while bit 6 of FFFFAC, a test switch, is set)
constexpr uint32_t kShip1 = 0xFF42E2, kShipSize = 0x60;
inline uint32_t Ship(int n) {
    return kShip1 + kShipSize * static_cast<uint32_t>(n);
}
constexpr uint32_t kShipFlags = 0x00, kShipY = 0x02, kShipX = 0x04, kShipBoxUp = 0x10, kShipBoxDown = 0x12,
                   kShipBoxRight = 0x14, kShipBoxLeft = 0x16, kShipPower = 0x20, kShipBombs = 0x22, kShipPowerHighest = 0x52,
                   kShipSafe = 0x3E;
constexpr int kMostPower = 4; // (levels 1 to 5)
constexpr int kUnit = 64; // (64ths of a pixel)
constexpr int kMostBombs = 9;
// 0393DC: `bclr #4,(a6)` then `bne` to the ship's blast (039FAA): a ship (A6) hit this frame. 0393E4 goes on unhurt.
constexpr uint32_t kShipHitTaken = 0x0393DC, kShipUnhurt = 0x0393E4;
// The game's tasks: slots of 0x40 bytes from FF2912, a task's number times 0x40 in (0092F0 starts one: the word its number
// with bit 15 set, then a word 0 and three longs, D1-D3). Task 2 is player 1's ship (038FAC: 005464 in the table of
// tasks at 005454) and 3 player 2's; the take-off of the first stage starts 2 (05C5BA), and the change of stage starts
// both again for the players that had a ship (06C202, putting it at 0C00 across: 06C216).
// The take-off is on (word): set as the gunner is taken (04704E), cleared as the first stage's take-off ends (05C50A) and
// at a change of stage (06C1CC); while it is set there is no panel, no Start for the other player (046350) and no hit
// test for the ships (039338)
constexpr uint32_t kTakeOff = 0xFF390E;
constexpr uint32_t kTasks = 0xFF2912, kTaskSize = 0x40;
constexpr int kShipTask = 2;
constexpr uint16_t kShipStartX = 0x0C00;
// 0396FA: `subq.w #1,0x22(a6)`, a bomb taken as it goes off
constexpr uint32_t kBombTaken = 0x0396FA;

// The enemies' bodies: 0x50 records of 0x20 bytes from FF5C50 (07CD84 clears them up to FF6650), as many in use as FF38E4
// and FF38E6 together (04DFC4); in use when bit 15 of the first word is set, and hitting when bit 14 is too; +02 up, +04
// across, and the box as a ship's: +10 up, +12 down, +14 right, +16 left of the middle (all in 64ths). The hit test
// (04DFE8) adds 0x2800 to each place before comparing, the ship's too.
constexpr uint32_t kBodies = 0xFF5C50, kBodyCount1 = 0xFF38E4, kBodyCount2 = 0xFF38E6;
constexpr uint32_t kBodySize = 0x20;
constexpr int kBodyRecords = 0x50;
// 04DF7A: the hit test of a ship (A4) against the bodies and the shots, once a frame (0057E4: both ships in turn when
// both play). The bodies' list is made during the frame and cleared before it ends.
constexpr uint32_t kHitTest = 0x04DF7A;
// The enemies' shots: records of 0x40 bytes from FF7A90 (040A0A walks them each frame, at most 0x104 by FFFF38): in use
// when bit 15 of the first word is set and none of 5180 (040ABA: a shot going or gone), +02 up and +04 across. Each is a
// point: 040B02 tests it against the box FFFF7C holds for the ship (its +10-+16 round its middle: 2 pixels each way), and
// 040B48 sets the ship's hit bit.
constexpr uint32_t kShots = 0xFF7A90, kShotSize = 0x40;
constexpr int kShotCount = 0x104;

// The scroll screens that are the stage: all three (the panel is objects; scroll screen 1 has stage 4's sky, found with
// the stage viewer)
constexpr unsigned kStageLayers = 0xE;

} // namespace cps2recomp::progear
