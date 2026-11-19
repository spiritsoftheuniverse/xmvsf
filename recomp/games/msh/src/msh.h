// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: names for what is known of the game's memory and routines, each found by experiment in this
// game (PROGRESS.md). The engine is the one X-Men vs. Street Fighter was made from (tools/portmap.py places much of
// that game's code here), and its memory is laid out alike: A5 is FF8000, the game's variables below it (A5-3800 on),
// the players' records at FF4000.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>

namespace msh {

using cps2recomp::M68K;

// --- where the game is ---

// A byte: 8 while a round is being fought (the damage routines test it before taking health: 011F30, 011F4A)
constexpr uint32_t kFight = 0xFF4800;
enum : uint8_t { kFighting = 0x08 };

// A byte, set once a game is being played (08CDC0, after Start), and the byte after it: who is in, bit 0 player 1,
// bit 1 player 2 (08CE62 sets a player's bit as Start takes its credit)
constexpr uint32_t kPlaying = 0xFF4880, kPlayersIn = 0xFF4881;
// Words, each a step of the one before (RAM dumps through a game from Start): the game's (4 while the players choose
// their characters, 8 in a match), and the match's
constexpr uint32_t kGameStep = 0xFF4884, kMatchStep = 0xFF4886;
enum : uint16_t { kGameChoosing = 4, kGameMatch = 8 };

// The round's clock: seconds left, a BCD byte (99 at a round's start, 008068; a second off at 00D4D4, sbcd), and the
// frames left of the second after it (counted down at 00D4BC; set again from a table by the timer speed, 00D4E2).
// Found by RAM dumps through a fight (the FBNeo training script holds it at 99 too).
constexpr uint32_t kRoundClock = 0xFF4808, kClockFrames = 0xFF4809;

// Credits paid for and not yet used, a byte (RAM dumps after one and two coins; nine at most, 08CF32)
constexpr uint32_t kCredits = 0xFF4C09;

// The game's own free play, one of the operator's coin modes (the test menu's System page, its first option; 12 free
// play). The board's start reads the settings record (kSettings, 08E9C6) and keeps the mode in kCoinMode; free play
// sets kFreePlay (08EA82), which the code that lets a player in tests in place of the credits (08CE34). Set alone it is
// the whole of free play: the title says FREE PLAY. (The same as X-Men vs. Street Fighter's, A5-33F6 there too.)
constexpr uint32_t kCoinMode = 0xFF4C13, kFreePlay = 0xFF4C0A;
constexpr uint8_t kCoinFreePlay = 0x12;

// The operator's settings as the settings memory holds them, read into the game's variables at the board's start
// (08E9C6 on; called from 000C66, so the call returns to 000C6C, where board.cpp and title.cpp hook). The offsets and
// where each goes are in board.cpp.
constexpr uint32_t kSettings = 0xFFF912;
// The show's scenes are tasks run one after another, each marking the record at FF2698 as it starts (087132, 08760A,
// 087B04, 087E98, 088192, 0884A6: st -0x5968(a5)); its step byte at +4. The last, 0884A6, is the title (frame 2749
// from switching on, pictures every 10 frames and --watch on the record).
constexpr uint32_t kShowStep = 0xFF269C;
constexpr uint32_t kShowTitle = 0x0884A6;

// --- the players ---

// Two records, 0x400 long: player 1's at FF4000, player 2's at FF4400 (the FBNeo training script's health places are
// in them; confirmed here by the health each shows)
constexpr uint32_t kPlayer1 = 0xFF4000, kPlayerSize = 0x400;
inline uint32_t Player(int n) {
    return kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}
// Which side a record is (0, 1), or -1 for neither
inline int SideOf(uint32_t record) {
    record &= 0xFFFFFF;
    if (record == kPlayer1) {
        return 0;
    }
    return record == kPlayer1 + kPlayerSize ? 1 : -1;
}
// Health, a word (0x90 full); a hit takes it at 011F7A and 01251E (sub.w d5,0x190(a6), the one hit in A6). The word
// after it is 0x90 throughout (the bar's other end).
constexpr uint32_t kHealth = 0x190, kHealthTop = 0x192;
constexpr uint16_t kHealthFull = 0x90;
// The hyper combo gauge: a word, the bar (0x90 a level; RAM dumps through hits), and a byte, its levels (the FBNeo
// training script keeps it at 3)
constexpr uint32_t kGaugeBar = 0x194, kGaugeLevel = 0x198;
constexpr uint8_t kGaugeFull = 3;
constexpr uint16_t kGaugeBarFull = 0x90;
// The Infinity Gems a player holds: six bytes, each a gem's number (0 power, 2 time, 4 space, 6 soul, 8 reality, A
// mind) or not one (the FBNeo training script)
constexpr uint32_t kGems = 0x1B6;
constexpr int kGemCount = 6;

// A byte: 1 while it is in use (both players' records, in a fight)
constexpr uint32_t kActive = 0x01;
// A byte: 1 a person plays the character, 0 the computer (one- and two-player fights' records compared)
constexpr uint32_t kHuman = 0x03;
// Where a player stands: a long, the word at +0C across (16 bits of fraction after it), and the word at +10 down (the
// floor's line on most stages 0x1D8; smaller is higher: Ground). Players start a round 0x100 apart, either side of the
// picture's middle. Longs, 16 bits of fraction: the speed across and down, and what is added to the speed down each
// step (RAM dumps through walking and a jump: the same as X-Men vs. Street Fighter's)
constexpr uint32_t kX = 0x0C, kY = 0x10, kSpeedX = 0x14, kSpeedY = 0x1C, kPullY = 0x20;
// A byte: bit 0 set facing right, bit 1 turned upside down (the hitbox script for this game in fbneo-training-mode
// names it; confirmed by the players' sides)
constexpr uint32_t kFlip = 0x4D;
// Who the character is: a word, even (each put in a fight and seen; 18 is Anita, who is not a fighter, and the order
// is the game's)
constexpr uint32_t kCharacter = 0x50;
constexpr int kCharacters = 12;
inline constexpr const char *kCharacterNames[kCharacters] = {
    "Spider-Man", "Captain America", "Hulk", "Iron Man", "Wolverine", "Psylocke", "Blackheart", "Shuma-Gorath", "Juggernaut",
    "Magneto", "Dr. Doom", "Thanos"};
constexpr uint16_t kDrDoom = 0x14, kThanos = 0x16;
// A word: who is in charge of the character, 0 itself (the player); 0C reeling from a hit (RAM dumps through hits)
constexpr uint32_t kControl = 0x06;
// A byte: what the character is doing (RAM dumps of standing, walking, jumping, crouching, attacking, guarding and
// being hit: the same values as X-Men vs. Street Fighter's)
constexpr uint32_t kAction = 0xA0;
constexpr uint32_t kAttackPhase = 0x6A; // byte: 80 an attack going on, C0 its recovery
constexpr uint32_t kFrame = 0x34;       // long: the animation frame being shown (in the ROM)
constexpr uint32_t kHitStop = 0x89;     // byte: steps both players stay frozen as a hit lands, counted down
// The character's boxes (the hitbox script names their places): a long, the table of its boxes, and words, each a
// box's number in it: two attacks, four places it can be hit and one it can be thrown in; a byte, its push box's
// number in kPushTable. Bytes: attacks hit only while kCanHit is not 0, and it is hit only while kCanBeHit is not 0.
constexpr uint32_t kBoxTable = 0x90, kAttackBoxes = 0x74, kHurtBoxes = 0x78, kThrowableBox = 0x80, kPushBox = 0xA2;
constexpr uint32_t kCanHit = 0x86, kCanBeHit = 0x87;
constexpr uint32_t kPushTable = 0x09E82C;
// What they throw: for each player a list of the records ending at kThrownList (player 2's 0x148 on) (the hitbox script)
constexpr uint32_t kThrownList = 0xFFE400, kThrownListSpace = 0x148;
// A byte: 0 a character's own colours, 1 the other set (player 2 in a match of the same two characters: 008CAC)
constexpr uint32_t kColourChoice = 0xAF;

// The stage, a byte: in a one-player game the computer's character's own (008CB4: from the list of opponents), in a
// two-player one taken at random from a table (008D60), or by the buttons held as the second player chooses (008CD8)
constexpr uint32_t kStage = 0xFF4893;
// The stages, each a character's own (in that character's order: a one-player game fights each opponent on its own;
// each seen in the stage viewer, and named for what it shows)
constexpr int kStages = 12;
inline constexpr const char *kStageNames[kStages] = {
    "The city street", "The shipwreck",   "The carnival", "Stark Enterprises", "The mountain lake",  "The neon city",
    "The burning field", "The skull canyon", "The harbour", "The space station", "The Infinity Gem chamber", "Thanos's throne"};
// The picture follows the stage's scroll layer that moves with the fighters: a record (by the stage, as the hitbox
// script has it: FF4980 on the mountain lake, FF4B00 on the burning field, FF4A00 elsewhere), with the camera's place at
// +0C across and +10 down, and its ends at +72/74 across and +76/78 down (083A46 keeps it between them; 082440 moves it
// after the players). The picture's left edge is the word across + 0x40, its top the word down.
inline uint32_t CameraRecord() {
    const uint8_t stage = cps2recomp::Ram8(kStage);
    return stage == 4 ? 0xFF4980 : stage == 6 ? 0xFF4B00 : 0xFF4A00;
}
constexpr uint32_t kCameraX = 0x0C, kCameraY = 0x10, kCameraLeft = 0x72, kCameraRight = 0x74, kCameraTop = 0x76, kCameraBottom = 0x78;
// The other places that keep a camera between its ends: the stages' and their planes' own routines each have the
// piece (move.w 0x72(a6),d0 ... move.w d0,0x10(a6), 0x30 bytes) that RegisterWidescreen's keepCamera (083A46) is one of,
// and the stage's code goes on after it (0: the piece ends its routine). Widescreen has to keep each 64 nearer the
// middle, or on those stages the camera goes to the board's ends while the players go 64 past them, out of the
// picture (issue #116). Found as every such piece in the listing (tools/lst.py -r).
struct KeptAt {
    uint32_t at, after;
};
inline constexpr KeptAt kCameraKept[] = {
    {cps2recomp::HookAt(0x07E620), 0x07E650}, {cps2recomp::HookAt(0x08340E), 0},
    {cps2recomp::HookAt(0x0836AA), 0x0836DA}, {cps2recomp::HookAt(0x084182), 0},
    {cps2recomp::HookAt(0x0846A2), 0x0846D2}, {cps2recomp::HookAt(0x084788), 0x0847B8},
    {cps2recomp::HookAt(0x084FF0), 0}, {cps2recomp::HookAt(0x085354), 0x085384},
    {cps2recomp::HookAt(0x085510), 0x085540}, {cps2recomp::HookAt(0x085716), 0x085746},
    {cps2recomp::HookAt(0x085B2C), 0}, {cps2recomp::HookAt(0x085D66), 0}, {cps2recomp::HookAt(0x086338), 0x086368},
    {cps2recomp::HookAt(0x0863C8), 0x0863F8}};
// The floor's height: the camera's lowest place + 0xD8 (0x1D8 on most stages; read off each stage's round start)
inline int Ground() {
    return static_cast<int16_t>(cps2recomp::Ram16(CameraRecord() + kCameraBottom)) + 0xD8;
}
// How the camera moves, a byte (082440 goes by it): 0 after both players, 4 after player 1 alone (0824F8: its middle
// 0x100 on from the camera's word); the game sets it every frame
constexpr uint32_t kCameraWay = 0xFF4B9C;
constexpr uint8_t kCameraFollowsOne = 4;
// The camera's ends across, by stage (each read from the record in a round on the stage)
inline void CameraEnds(int stage, int &left, int &right) {
    static constexpr int16_t kEnds[kStages][2] = {{0, 384}, {-64, 1088}, {0, 384}, {0, 384},    {-64, 320}, {-64, 320},
                                                  {0, 384}, {-704, 256}, {-64, 1088}, {0, 384}, {64, 448},  {-240, 880}};
    const int n = stage >= 0 && stage < kStages ? stage : 0;
    left = kEnds[n][0];
    right = kEnds[n][1];
}
constexpr int kPictureLeft = 0x40;
constexpr int kWall = 0x15; // how near the picture's edge a player can go (00DFF2: 0x55 to 0x1AB on from the camera word)
constexpr uint32_t kPlanes = 0x01A752; // longs by a character's byte +30: the camera's word of the plane it stands in

// Each player's choosing: a record of 0x20 bytes, player 1's at FF4920; its first byte 1 while choosing a character,
// 3 once it has chosen (and the speed after it), and the word +0C the character under its cursor (traced through
// choosing; 008F1C takes it)
constexpr uint32_t kChoosing = 0xFF4920, kChoosingSize = 0x20;
constexpr uint8_t kChoosingDone = 3;
constexpr uint32_t kCursor = 0x0C;

// The control panel as read this frame: a word a player, the stick (bit 0 right, 1 left, 2 down, 3 up) and the
// punches (bits 4-6) in its low byte, the kicks in bits 8-10 (X-Men vs. Street Fighter's, placed here by
// tools/portmap.py)
constexpr uint32_t kPanel[2] = {0xFF4BC6, 0xFF4BC8};

// The sound queue: a ring of sixteen-byte requests at FF4C2C, 0x400 long, the word at FF4C26 the place of the next
// (08D58E puts one in)
constexpr uint32_t kSoundQueue = 0xFF4C2C, kSoundQueueEnd = 0xFF4C26;
constexpr uint16_t kSoundQueueMask = 0x3F0;

// The round: FF4800 is 0A once it is decided (00CC0A), and the byte FF4804 who won it then: 0 player 1, 1 player 2, FF
// neither (00CB8C on: the players' health compared)
constexpr uint32_t kRoundOver = 0xFF4804;
constexpr uint8_t kDecided = 0x0A;

// --- the characters' colours ---

// A character's colours, in the ROM: a long a character (by its word kCharacter, twice) at kColourTable, its own set
// then the other kColourSetSize on, each kPalettesInSet palettes of 16 colours (Thanos's two). The round's start
// copies the player's set to its colour sets in the buffer the board shows from (007AEC: 914000, 0x20 bytes a colour
// set): player 1's at set 0C, player 2's at 10.
constexpr uint32_t kColourTable = 0x160D2E;
constexpr int kColourSetsEach = 2, kPalettesInSet = 3, kColoursInPalette = 16;
constexpr uint32_t kColourSetSize = 0x80;
constexpr uint32_t kPaletteBuffer = 0x914000;
constexpr unsigned kPlayer1Set = 0x0C, kPlayer2Set = 0x10, kShadowSet = 0x08;

// Objects by what they are (for Video::hideObject; from the object lists of rounds on every stage): colour sets 00-03
// are the panel, 08 the fighters' shadows, 0B the gems' marks, 0C-12 the fighters (player 1's three, then player 2's),
// 13 on the stage's own pieces. The panel's pieces drawn in other sets are tiles 16000-16FFF: the fighters' faces (in
// their own sets), the clock's mark (08) and the gems' marks.
constexpr unsigned kStageSets = 0x13;
inline bool NotTheStage(uint32_t, uint16_t attributes) {
    return (attributes & 0x1F) < kStageSets;
}
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    return (attributes & 0x1F) <= 0x03 || (tile >= 0x16000 && tile < 0x17000);
}

// --- for hooks ---

inline void SetLow16(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFF0000) | (value & 0xFFFF);
}

} // namespace msh
