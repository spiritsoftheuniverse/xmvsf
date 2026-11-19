// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: Fullmetal Madness: names for what is known of the game's memory and routines, each found by experiment
// in this game (PROGRESS.md). Its variables are reached from A5 = FF8000.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>

namespace cybots {

using cps2recomp::HookAt;
using cps2recomp::M68K;

// --- where the game is ---

// Words from A5, each a step (in fours) of the one before. The first is the step of whatever the game is doing: the
// board's start steps it 4 to 14 (00A996 on, frames 39-262), then 0 (00146E, frame 453) and its show loops 4 to 30
// (00AE06 on): 10 a fight it shows (frames 1301-2349), 1C the introduction, whose own step (kStep) is 68 for the
// title, from white (frame 4014), 20 to 2C the rankings, 30 a second fight; then 4 again. A game started by Start runs
// its own routine (00C3A6), which steps it from 0 again. Traced with --watch, no coin in.
constexpr uint32_t kMode = 0xFF8000, kStep = 0xFF8004, kStep2 = 0xFF8008, kStep3 = 0xFF800C;
enum : uint16_t { kBootLast = 0x14, kShowTitle = 0x1C, kShowTitleStep = 0x68 };

// A byte: 1 while a game is being played (00C3A6 sets it as Start begins one; the round's clock reads it, 00EFC4)
constexpr uint32_t kInGame = 0xFF80BC;

// Credits paid for and not yet used, a BCD byte (RAM dumps after one, two and three coins: 1, 2, 3; 0012D2 adds a
// coin's, at most 9)
constexpr uint32_t kCredits = 0xFF80B6;

// The game's own free play, one of the operator's coin modes (the test menu's System page: 00-08 one coin for 1-9
// credits, 09-10 two to nine coins a credit, 11 two coins to start and one to continue, 12 free play; 00 on a new
// board). The board's start (008EE4) keeps the mode doubled in kCoinMode, the coins a credit for each chute in the two
// words (from the table at 0092B4: 0101 for free play), and for free play sets kFreePlay (009064); the game tests
// kFreePlay with the credits wherever it counts them (004BB2 on).
constexpr uint32_t kCoinMode = 0xFF80AC, kFreePlay = 0xFF80AA, kCoinsPerCredit1 = 0xFF8084, kCoinsPerCredit2 = 0xFF808C;
constexpr uint8_t kCoinFreePlay = 0x24;

// The round's clock: a word of BCD seconds left (the high byte; 99 at a round's start, 00EF94) and the fraction of the
// second under it, which 00EFCE takes the timer speed's step (D0, the table at 00EF8C) off; when it runs under nothing a
// second comes off (00EFF4, sbcd). A5+6B9A not zero stops it (and the damage: the round over, the game's own pauses).
// Found with --watch through a fight.
constexpr uint32_t kRoundClock = 0xFFEBA0, kClockFraction = 0xFFEBA1;
constexpr uint32_t kClockStep = 0x00EFCE;
constexpr uint32_t kNoFighting = 0xFFEB9A;

// --- the players ---

// Each player's record, 0x400 long, the first at FF81A0. The health word is at +44 (0x98 full, 001FEC sets it as a
// round starts) and the bar shown at +46. Found by --watch on the FBNeo training script's health byte (FF85E5).
constexpr uint32_t kPlayer1 = 0xFF81A0, kPlayerSize = 0x400;
inline uint32_t Player(int n) {
    return kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}
constexpr uint32_t kHealth = 0x44, kHealthShown = 0x46;
constexpr uint16_t kHealthFull = 0x98;

// Every place the game takes health off a player (lst.py -r 'sub.*0x44\(a'), each with the register the victim is in
// and the one holding the damage. Health below nothing after it is a knockout (the game sets it to FFFF), so one-hit
// knockouts take off more than a full bar.
//   002F16, 002F9A, 003036, 0030A2  a throw's and the like (A4 the victim: 0x374(a6), the other player)
//   01B81C, 01B88E                  damage taken over time (A6 the victim, D2 from its +172)
//   0463E4                          a hit (A6 the victim), the one most hits go through: --watch on both players'
//                                   health through fights
//   045E02, 045E5A                  both players lose their health at once (each the other's plus one)
struct DamageSite {
    uint32_t address;
    uint8_t victim, damage; // address register, data register
};
inline constexpr DamageSite kDamageSites[] = {
    {HookAt(0x002F16), 4, 2}, {HookAt(0x002F9A), 4, 2}, {HookAt(0x003036), 4, 2}, {HookAt(0x0030A2), 4, 2}, {HookAt(0x01B81C), 6, 2},
    {HookAt(0x01B88E), 6, 2}, {HookAt(0x0463E4), 6, 3}, {HookAt(0x045E02), 6, 3}, {HookAt(0x045E5A), 3, 6},
};


// --- a game ---

// A game's steps under kMode (4 in a game, run by its own routine 00C3A6): kStep 0 while the players choose, 4 the
// match (8 between matches); kStep2 04-0C the match's introduction (the story), 10 a round's ("PHASE 1", "GO"), 14 while
// it is fought, 18-20 over (watched through a ladder: the round is fought from "GO" to the knock-out)
enum : uint16_t { kGameRunning = 4, kGameChoosing = 0, kGameMatch = 4, kRoundFought = 0x14 };

// Who is in: a byte, bit 0 player 1, bit 1 player 2 (RAM dumps before and after each Start: 0, 1, 3)
constexpr uint32_t kPlayersIn = 0xFF80E5;

// The control panel as the game read it this frame, a word a player: bits 0-3 the stick (right, left, down, up), 4-6
// the first three buttons (Low attack, High attack, Boost), 8 the fourth (Weapon). Found holding right and the first
// button for player 1, down and the second for player 2; the word after each is a copy.
constexpr uint32_t kControls[2] = {0xFF806E, 0xFF8072};

// The sound queue: a ring of sixteen-byte requests at A5-6BC8, the word at A5-6BCC the place of the next (0055FA puts
// one in; 005544 sends the one at A5-6BCA to the sound CPU, and steps it by 0x10 in 0xFF0)
constexpr uint32_t kSoundQueue = 0xFF9438, kSoundQueueEnd = 0xFF9434;
constexpr uint16_t kSoundQueueMask = 0xFF0;

// The camera: an object whose X (the picture's left edge in the stage: 0 to 280 across, the stages are 1024 wide) is at
// +1A and its Y at +1E; which object it is goes by the stage (FFEC1A, FFECDA, FFED9A...): the word at A5+6B1C points to
// it, and the objects of the other planes follow its steps (01803E on: some as fast, some slower). 017F3A steps it
// each frame towards the players. Found from RAM dumps as the players walked to each end on several stages, and the
// fighters' objects against their X. The players stop at 1C and 3E4; they start a round at 1A8 and 258.
constexpr uint32_t kCameraPointer = 0xFFEB1C;
inline uint32_t CameraObject() {
    return 0xFF0000 | cps2recomp::Ram16(kCameraPointer);
}
inline uint32_t CameraXAt() {
    return CameraObject() + 0x1A;
}
inline uint32_t CameraYAt() {
    return CameraObject() + 0x1E;
}
inline int CameraX() {
    return static_cast<int16_t>(cps2recomp::Ram16(CameraXAt()));
}
inline int CameraY() {
    return static_cast<int16_t>(cps2recomp::Ram16(CameraYAt()));
}
constexpr int kCameraLeft = 0, kCameraRight = 0x280;
constexpr int kStageLeft = 0x1C, kStageRight = 0x3E4;
constexpr uint16_t kStartX[2] = {0x1A8, 0x258};

// The stage: a byte. A one-player game takes it from the story's table (0020E4 on: the path at +389, the place in it at
// +38A, tables from 0021E0); a two-player one at random from the table at 0021D0. Each seen with the byte held at it:
// 0A and 0B are no stage.
constexpr uint32_t kStage = 0xFFEA2D;
constexpr int kStages = 14;
inline constexpr uint8_t kStageNumbers[kStages] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0C, 0x0D, 0x0E, 0x0F};
inline constexpr const char *kStageNames[kStages] = {
    "The city",        "The desert (BOTS)", "The space station", "Town 3",     "The forest",    "The fortress",   "The ice cave",
    "The jungle base", "The volcano",       "Space",             "The desert at sunset", "The ice cave at night", "The burning ruins",
    "The burning city"};
// 002188: a player is made for the match from its choices (A6 the record): +370 the pilot from +389, +371 the
// machine (and its arms and weapon, +372 and +373) from +38C. In a one-player game the computer's player has been made
// already (0021A2, from the story's tables), and the stage chosen.
constexpr uint32_t kPlayerMade = 0x002188;

// --- the players' records ---

// The rest of the record, from dumps of players standing, crouching, jumping, attacking, hit and guarding, and the
// FBNeo training script for this set
constexpr uint32_t kState = 0x03;    // 0 free (kSub: 0 standing or walking, 4 crouching, 8 in the air, 0C landing),
constexpr uint32_t kSub = 0x04;      // 4 busy (kSub: 0 attacking, 4 a special move, 8 knocked down, 0C thrown, 10 reeling)
enum : uint8_t { kFree = 0, kBusy = 4 };
enum : uint8_t { kSubStand = 0, kSubCrouch = 4, kSubAir = 8, kSubLanding = 0x0C };
enum : uint8_t { kSubAttack = 0, kSubSpecial = 4, kSubDown = 8, kSubThrown = 0x0C, kSubReel = 0x10 };
constexpr uint32_t kFacesRight = 0x09; // byte: 1 facing right (a box's offset across is kept as if facing left)
constexpr uint32_t kX = 0x1A, kY = 0x1E; // words (each the high word of a long with 16 bits of fraction)
constexpr uint32_t kLastX = 0x22, kLastY = 0x26; // the same, as the last frame left them (a trace through a jump)
// Bytes: 1 while the game plays the player. +380 says it for the match (1 for player 2 in a one-player game, 0 in a
// two-player one); 0196E6 copies it to +253 as a round is set up, and the computer's play goes by +253 (0196E6, 046264).
// +10C is the side, 0 or 1.
constexpr uint32_t kComputerMatch = 0x380, kComputer = 0x253;
// The computer's play of a player (A6): 0197AE runs it each frame (020506) while +253 is set; 020484 starts it (the
// script for the machine at +200 and the rest), which the game does only as a round is set up (019720)
constexpr uint32_t kComputerPlays = 0x0197AE, kComputerStarts = 0x020484, kComputerScript = 0x200;
// The floor's height: a word, A5+6A20, by the stage (30 on most, 28 on the forest and the fortress: 003908 stands the
// players on it as a round starts). kGround is the height the picture's line 192 shows with the camera at its lowest
// (the boxes and the zoom are drawn from it), not the floor.
constexpr uint32_t kFloor = 0xFFEA20;
constexpr uint16_t kGround = 0x30;
inline bool InAir(uint32_t player) {
    return static_cast<int16_t>(cps2recomp::Ram16(player + kY)) > static_cast<int16_t>(cps2recomp::Ram16(kFloor));
}
constexpr uint32_t kPilot = 0x370, kMachine = 0x371, kArm = 0x372, kWeapon = 0x373;
// Choosing: +381 is 1 while the player chooses, +383 its step (0 the pilot, 4 on the machine), +382 1 once it has
// chosen (dumps of player 1's record every 40 frames through the choice, with pictures)
constexpr uint32_t kChoosing = 0x381, kChosen = 0x382;
constexpr uint32_t kPilotChoice = 0x389, kMachineChoice = 0x38C, kColourChoice = 0x38B;
// The machines by number (+371), as the FBNeo training script names them
constexpr int kMachines = 16;
inline constexpr const char *kMachineNames[kMachines] = {"Blodia", "Reptos", "Fordy",    "Guldin", "Swordsman", "Lightning",
                                                         "Jackal", "Vise",   "Cyclone",  "Riot",   "Tarantula", "Helion",
                                                         "Super-8", "Gaits", "Warlock",  "Killer Bee"};
// Boxes (the FBNeo training script for this set): the long at +32 points to the record's box tables, a word at the start
// of each kind the distance to its table (+0 attacks, 16 bytes each; +2 head, +4 body, +6 legs, +8 push, 8 bytes each);
// the record's bytes +62 to +66 name the box of each kind now. A box is words: across from X (turned when facing
// right), up from Y, half its width, half its height.
constexpr uint32_t kBoxTables = 0x32, kBoxIds = 0x62;
// What the players throw: records of 0xC0, sixteen from FF9360 (the script's pool), in use while bit 0 of +01 is set;
// their boxes as a player's (an attack, +62, and a hurt box, +63)
constexpr uint32_t kThrown = 0xFF9360, kThrownSize = 0xC0;
constexpr int kThrownRecords = 16;
// The whole picture frozen while a hit lands (the FBNeo training script)
constexpr uint32_t kHitFreeze = 0xFFEBA4;

// 003782: a player (A6) kept inside the picture: X no less than the camera's plus half its width (+118, a word) and no
// more than the picture's right edge less it (0037FC returns); +116 says which edge held it (1 left, 2 right). Found by
// --watch on player 1's X pressed against the left end (0037B6). Its stage ends are the camera's.
constexpr uint32_t kKeptInPicture = 0x003782;
constexpr uint32_t kHalfWidth = 0x118;

// The player's step of a fight (A6): 019CF8 runs it for both players each frame (in an order of chance, and a second
// time for one with +1AA set); it goes on by kState (019D7C)
constexpr uint32_t kPlayerStep = 0x019D70;
// The hit, after its damage has been worked out and before it is taken (A6 the one hit; +130 not zero when it guarded:
// the damage then a quarter, or nothing: 0463CC on)
constexpr uint32_t kHitTaken = 0x0463B8;
constexpr uint32_t kGuarded = 0x130;

// --- the machines' colours ---

// A machine's colours (015152 puts them in a player's seven palettes, 900000 for player 1 and 900200 for player 2, as
// a round is set up): the table at 0153CE has a word for each machine, how far on from the table its colours are; there
// two sets of eight bytes (the player's +38B, bit 0, says which), each the number of a palette in one of seven tables,
// one table for each of the player's palettes (32 bytes a palette). A stage of ice (06 and 0D) has a table of its own,
// 0154EE, with the machines' colours as that stage's light shows them. Found from the routine, and each palette
// compared with the palette memory a fight showed (the same but for the brightness, which the game keeps).
constexpr uint32_t kColourTable = 0x0153CE, kColourTableIce = 0x0154EE;
inline constexpr uint32_t kColourBanks[] = {0x0CFB48, 0x0D03C8, 0x0D0BC8, 0x0D13C8, 0x0D1988, 0x0D1B08, 0x0D2308};
constexpr uint32_t kColoursEnd = 0x0D2A88; // (the last table's last palette any machine names, and on)
constexpr int kColourSetsEach = 2, kPalettesInSet = 7, kPaletteBytes = 0x20;
constexpr uint32_t kPalettes = 0x900000, kPalettes2 = 0x900200;

// Objects by their colour set (attributes' low five bits) and tile, from the object lists of fights: 00-07 player 1's
// (the palette loader 015152 writes its seven at 900000) and 10-17 player 2's (900200), with what they throw and the
// sparks; 18-1A the stage's pieces; 1B-1F the panel (clock, bars, names); the faces under the bars are in sets 09, 0F
// and 1C, tiles 12300-123FF
// The players' shadows: both in set 04, tiles 2A200-2A21F (on the floor's line, under each player)
inline bool IsShadow(uint32_t tile) {
    return tile >= 0x2A200 && tile < 0x2A220;
}
inline bool TheFaces(uint32_t tile) {
    return tile >= 0x12300 && tile < 0x12400;
}
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return set >= 0x1B || TheFaces(tile);
}
inline bool NotTheStage(uint32_t tile, uint16_t attributes) {
    (void)tile;
    const unsigned set = attributes & 0x1F;
    return set < 0x18 || set >= 0x1B;
}
inline bool NotTheFighters(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return ThePanel(tile, attributes) || (set >= 0x18 && set < 0x1B);
}

// --- the board's settings ---

// The settings record, at A5-4600 (FF3A00) as the test menu changes it. The board's start copies the settings memory's
// record into the variables (008EE4, called at 00A9BA, from A5-5BB8: an event sets join in, 00909A) and goes on at
// 0090A2; leaving the test menu copies them again (091256 on) and goes on at 0912F4. Each variable found by stepping
// the option in the test menu and dumping memory after each step, and the two copies read in the listing.
constexpr uint32_t kSettingsCopied = 0x0090A2, kSettingsCopiedAgain = 0x0912F4;
// The difficulty as the board's start left it (0-7: board.cpp)
constexpr uint32_t kDifficulty = 0xFF80A8;
// A game begins: Start was pressed (the game's own routine)
constexpr uint32_t kGameBegins = 0x00C3A6;

// --- for hooks ---

inline void SetLow8(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFFFF00) | (value & 0xFF);
}
inline void SetLow16(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFF0000) | (value & 0xFFFF);
}

} // namespace cybots

namespace cps2recomp {
// The wide picture (widescreen.cpp): the columns it adds on each side (0: none), how far right the stage is drawn in it,
// how far short of each end the camera stops (64, or 0: played as on the board), and a spare's frame drawn as wide as
// the port's (and the port's machine back)
int WideSide();
int WideStage();
int WideMargin();
void WideForSpare(Machine &machine);
void WideForPort(Machine &machine);
void WideEnds(int &left, int &right);
} // namespace cps2recomp
