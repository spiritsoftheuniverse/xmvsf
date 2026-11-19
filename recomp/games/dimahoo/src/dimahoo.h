// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Dimahoo (dimahoou, USA 000121): names for what is known of the game's memory, with how each was found (PROGRESS.md
// has more). The game is Eighting/Raizing's. Its base register A5 holds 928000, inside the board's video memory
// (900000-92FFFF): its variables live there, past the scroll screens and object tables it uses, and the toolkit's Ram8
// and the like reach them by their 68000 address. Work RAM (FF0000 on) holds the ships' and enemies' records. Its code
// is sprinkled with `cmp.w a4,d7 / cmp.w d4,d1 / cmpa.w d5,a3`, which does nothing. The picture is vertical (ROT270).
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

namespace cps2recomp::dimahoo {

// The base register: every variable below is A5 + its offset
constexpr uint32_t kBase = 0x928000;
constexpr uint32_t At(uint32_t offset) {
    return kBase + offset;
}

// --- coins and the board's settings ---

// Credits (byte, up to 9): 013B20 adds them as coins come (A5+8CC counts the coins, A5+8D4 how many make a credit);
// 013A8E takes them at Start. Found from dumps of the video memory after 1 to 6 coins.
constexpr uint32_t kCredits = At(0x8CD);
// The game's own free play (byte): the settings routine (01120E) sets it when the coin mode is 0x12, the last of the test
// menu's (0112CA); Start then takes no credit (013A6A) and the screens say FREE PLAY. The coin mode is copied to A5+8D9.
constexpr uint32_t kFreePlay = At(0x8CF), kCoinMode = At(0x8D9);
constexpr uint8_t kCoinModeFreePlay = 0x12;

// The board's settings: the record the settings memory keeps (A5+8FC), copied into the variables below by 01120E, which
// the boot calls at 00BBAC (and returns to 00BBB2). Found by stepping each option of the test menu's Configuration pages
// with Right and reading the test menu's copy (A5+7E6C on: it edits that, taken from the variables as it opens):
//   System: +00 coin mode (0-0x12; 0x12 free play; 0, 1 coin 1 credit, a new board's)  +01 continue (0 on, 1 off) -> A95
//           +02 monitor -> A96  +03 demo sound (0 off, 1 on, 2 half; on) -> A93  +04 sound mode -> A94
//           +05 chute type
//   Game:   +10 difficulty (0-7, shown 1-8; 3) -> A92   +11 player stock (0-4: 1-5 ships; 2, three ships) -> A8D
//           +12 bomber stock (0-4; 3) -> A8E           +13 extend type (0: 2,000,000 and 5,000,000; 1: 2,000,000 only;
//               2: 4,000,000 only; 3: every 2,000,000; 4: every 4,000,000; 5: none) -> A8F
//           +14 stage type (0: one loop, 1: two loops) -> A90   +15 invincible (0 off, 1 on) -> A91
// The game reads the variables where it uses them (the ships at a game's start, 02A4F6; the bombs at each ship's start,
// 02A4E8; the extend table at a game's start; the loops at the last stage's end, 008BB4; invincibility at a hit,
// 0292E4).
constexpr uint32_t kSettings = At(0x8FC);
constexpr uint32_t kSetContinue = At(0xA95), kSetDemoSound = At(0xA93), kSetDifficulty = At(0xA92), kSetShips = At(0xA8D),
                   kSetBombs = At(0xA8E), kSetExtend = At(0xA8F), kSetLoops = At(0xA90), kSetInvincible = At(0xA91);
// The settings routine's end, where the boot goes on (00BBAC calls 01120E)
constexpr uint32_t kSettingsRead = 0x00BBB2;

// --- what the game is doing ---

// The step (word): 1 the boot, 3 the title with credits or free play, 5 a game, 6 the show's demonstration; the rest of
// the show leaves the word it reads from 000002 (2E36). Found from dumps through the show and a game, and the code that
// sets it (00BC76, 008428, 01388A tests it).
constexpr uint32_t kStep = At(0x40);
enum : uint16_t {
    kStepBoot = 1,
    kStepTitle = 3,
    kStepGame = 5,
    kStepDemo = 6,
};
// A game is on (byte): 0139F6 sets it to 1 as Start begins one, 00840C clears it; the test menu sets it to FF. Found from
// dumps before and in the test menu.
constexpr uint32_t kGameOn = At(0x306);
constexpr uint8_t kGameOnTest = 0xFF;
// Who plays (byte): bit 0 player 1, bit 1 player 2, set as each takes Start (013A98)
constexpr uint32_t kPlayersIn = At(0x307);
// The scene of a game (word: the routine it runs): 04 the choice of character, 0C a stage played, 14 the change of stage,
// 18 the continue, 1C the game over, 20 after it, 24 the ending (008BF8). Found from dumps through a game and five stages.
constexpr uint32_t kScene = At(0x30A);
enum : uint16_t {
    kSceneChoose = 0x04,
    kScenePlay = 0x0C,
    kSceneStageChange = 0x14,
    kSceneContinue = 0x18,
    kSceneGameOver = 0x1C,
};
// The stage played (word, 0 the first of six: 008BDE counts it on at a stage's end) and the stage it is (word, A5+31E:
// 008B7E looks it up in the table at 008CCC by the route at A5+320, eight bytes a route: the first three stages in one
// of six orders, then 3, 4, 5). The route stays 0 in play here (008B64 only keeps it under 6), so the stage is its
// count. After the sixth, a second loop (A5+322) when the board is set to two (008BB4), else the ending.
constexpr uint32_t kStageCount = At(0x31A), kStage = At(0x31E), kRoute = At(0x320), kLoop = At(0x322);
constexpr int kStages = 6;
// 02EDBE: as the choice of character ends, `jsr $8B7E` looks the stage up from its count (008B64 has just cleared it):
// a count put in here starts the game at that stage
constexpr uint32_t kGameBegins = 0x02EDBE;
// The music playing (word: 00CB6C keeps the last asked for): C3-C7 the stages (and on), C9 and CA a boss (from its
// "has appeared" on to its end), CD a stage cleared, D1 the title. Found from dumps through five stages.
constexpr uint32_t kMusic = At(0x79F4);
// 038E7E: the show's title begins (its time, A5+42, set to 0x1E0); found from a watch on the time
constexpr uint32_t kTitleShown = 0x038E7E;

// --- the players ---

constexpr int kPlayers2 = 2;
// The players' records: 0x100 bytes from A5+46 (player 1) and A5+146 (player 2), 02A3F2 starts them. In each:
//   +00 byte: 0 out, 1 playing, FF a ship lost (02A1C0) or waiting for Start
//   +02 byte: the player (0 or 1)
//   +04 long: the score (binary, as shown; 02A1C0's table takes from the next extend's)
//   +08 byte: ships in stock, besides the one in play (from A8D at a game's start, 02A4F6; 02A2EA takes one as the next
//       ship comes)
//   +09 byte: bombs (from A8E at each ship's start, 02A194; 029FDE takes one as a bomb goes off)
//   +0A byte: the magic gauge, items taken toward the next magic level: 03F550 adds one for a small item, 03F53C ten for a
//       big one, and at 48 (03F554) it starts again, the sound 0x0C plays and MAGIC LEVEL UP shows (A1+32 a copy, held
//       at 48). Found from a watch through the first stage, and pictures.
//   +0F byte: the character (0 Solo-Bang, 1 Karte, 2 Miyamoto, 3 Grimlen: the choice's cursor, found from dumps with
//       Right pressed on it)
//   +F0 long: the ship's record in work RAM (FF1000, FF1C00)
constexpr uint32_t kPlayer1 = At(0x46), kPlayerSize = 0x100;
inline uint32_t Player(int n) {
    return kPlayer1 + kPlayerSize * static_cast<uint32_t>(n);
}
constexpr uint32_t kPlayerState = 0x00, kPlayerScore = 0x04, kPlayerShips = 0x08, kPlayerBombs = 0x09,
                   kPlayerMagic = 0x0A, kPlayerCharacter = 0x0F;
constexpr int kMagicLevel = 48;
constexpr int kCharacters = 4;
constexpr int kMostBombs = 4; // (the test menu's most)
// The time left to choose a character (word, from 0x4EB at 02EE7C)
constexpr uint32_t kChooseTime = At(0x330);

// The ships' records (work RAM, 0xC00 apart): player 1's at FF1000, player 2's at FF1C00 (02A3F8). In each:
//   +48 byte: bit 5 hit this frame (the hit tests set it), bit 7 the barrier a bomb leaves
//   +E8 long: the player's record
constexpr uint32_t kShip1 = 0xFF1000, kShipSize = 0xC00;
inline uint32_t Ship(int n) {
    return kShip1 + kShipSize * static_cast<uint32_t>(n);
}
// 0292E4: a ship (A6) hit with no barrier: unless the board's invincibility is on (A5+A91), it starts its blast (the
// ship's step 0x22). 029310 goes on unhurt, as the board's invincibility does.
constexpr uint32_t kShipHit = 0x0292E4, kShipUnhurt = 0x029310;
// A hit is let by while either of these bytes is set (029208: after the hit tests, a ship's hit bit is acted on only with
// both clear, its step under 0x22 and no barrier): A5+25A while a bomb goes off (found from a trace through bombs: set
// for each one's 134 frames, while the ship's box is the bomb's 40 by 88), A5+325 at another time not yet seen.
constexpr uint32_t kHitsLetBy = At(0x25A), kHitsLetBy2 = At(0x325);
// 029FDE: `subq.b #1,0x9(a3)`, a bomb taken (A3 the player's record) as it goes off
constexpr uint32_t kBombTaken = 0x029FDE;

// The hit tests, each run by a ship (A6) once a frame (029202, 0291FC), found from a watch on the ship's +48. Places are
// whole pixels with the picture stood up as the monitor shows it: across (the picture's rows, as the board draws it
// lying down, less the pan: kPan below) and up (its columns, 64 more: a ship held up goes to 384, at the picture's
// column 320). Found from the ship's place while it was moved, against pictures.
//   002E60: the ship's box, +08 to +0A across and +0C to +0E up (6 by 10 pixels), against the enemies' shots: points
//       (+32 across, +30 up), found through a grid of lists (the word at FFE076 its address in work RAM: a row of 16
//       cells for each 32 pixels up, a cell for each 32 across; a cell's word the first node, a node's first word the
//       next and its second the shot) and left alone while their +48 has bit 7. A shot inside sets the ship's hit bit.
//   0031DE: the same box against the enemies' bodies: a grid of the same kind (FFE078; a cell for each 64 pixels up and
//       each 64 across), whose nodes hold a kind (+02, tested against the ship's +49) and a record (+04) with the
//       body's box at +08 to +0A across and +0C to +0E up.
// The grids are made during the frame and are gone before it ends: the aids read them at the tests.
constexpr uint32_t kShotTest = 0x002E60, kBodyTest = 0x0031DE;
constexpr uint32_t kShotGrid = 0xFFE076, kBodyGrid = 0xFFE078;
constexpr uint32_t kShotGridSize = 0x400, kBodyGridSize = 0x100;
constexpr uint32_t kShipBoxLeft = 0x08, kShipBoxRight = 0x0A, kShipBoxLow = 0x0C, kShipBoxHigh = 0x0E;
constexpr int kUpOffset = 64;
// The stage is wider than the picture and pans across with the ships (word, A5+68E: 0310B6 sets it every frame; -16 with
// a ship at the left, on through 0 and up as it goes right): a place across is at the picture's row across - pan - 16.
// Found with a ship held at several places across (with the pan -12, 2 and 8 it was drawn 4, 18 and 24 rows above its
// place across) and checked against pictures of a ship moved by made-up controls, every frame it stood still.
constexpr uint32_t kPan = At(0x68E);
constexpr int kPanOffset = 16;

// The scroll screens that are the stage: 2 and 3 (scroll screen 1 is the panel: its bars, the scores and the ships
// left; found with the stage viewer, one screen at a time)
constexpr unsigned kStageLayers = 0xC;

} // namespace cps2recomp::dimahoo
