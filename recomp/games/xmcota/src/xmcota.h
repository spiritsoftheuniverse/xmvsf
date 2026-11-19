// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: names for what is known of the game's memory and routines, each found by experiment in
// this game (PROGRESS.md). It is the first game of the engine Marvel Super Heroes was made from (tools/portmap.py places
// much of that game's code here, and many of its variables at the same places), and its memory is laid out alike: A5
// is FF8000, the game's variables below it (A5-3800 on), the players' records at FF4000.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>

namespace xmcota {

using cps2recomp::M68K;

// --- where the game is ---

// A byte: 4 while a round is being fought (the damage routines test it before taking health: 00D35E, 00D8A0), 6 once
// it is decided (RAM traces through a one-player game)
constexpr uint32_t kFight = 0xFF4800;
enum : uint8_t { kFighting = 0x04, kDecided = 0x06 };

// A byte, set once a game is being played (079EDE, after Start), and the byte after it: who is in, bit 0 player 1,
// bit 1 player 2
constexpr uint32_t kPlaying = 0xFF4880, kPlayersIn = 0xFF4881;
// Words, each a step of the one before (RAM traces through a game from Start): the game's (4 while the players choose
// their characters, 8 in a match), and the match's
constexpr uint32_t kGameStep = 0xFF4884, kMatchStep = 0xFF4886;
enum : uint16_t { kGameChoosing = 4, kGameMatch = 8 };

// The round's clock: seconds left, a BCD byte (99 at a round's start, 00361E; a second off at 06CBC8, sbcd), and the
// frames left of the second after it (counted down at 06CBB4; set again from a table by the timer speed, 06CBD6)
constexpr uint32_t kRoundClock = 0xFF4808, kClockFrames = 0xFF4809;

// Credits paid for and not yet used, a byte (RAM dumps after one, two and three coins)
constexpr uint32_t kCredits = 0xFF4BDD;

// The game's own free play, one of the operator's coin modes (the test menu's System page, its first option; 12 free
// play). The board's start reads the settings record (kSettings, 0859D6) and keeps the mode in kCoinMode; free play
// sets kFreePlay (085A92), which the code that lets a player in tests in place of the credits. Set alone it is the
// whole of free play: the title says FREE PLAY (074118). (Marvel Super Heroes' kFreePlay, A5-33F6 there.)
constexpr uint32_t kCoinMode = 0xFF4BE7, kFreePlay = 0xFF4BDE;
constexpr uint8_t kCoinFreePlay = 0x12;

// The operator's settings as the settings memory holds them, read into the game's variables at the board's start
// (0859D6 on; called from 000C92, so the call returns to 000C98, where board.cpp and title.cpp hook). The offsets and
// where each goes are in board.cpp.
constexpr uint32_t kSettings = 0xFFEB8C;

// The show: its scenes one after another (073FBE): notices, the introduction's five scenes (075388 to 076084), then the
// title (074712, called at 07409A; frame 2076 from switching on, pictures every 20 frames), a fight and the rankings.
// Each introduction scene marks the record at FF2600 as it starts (st -0x5A00(a5)) and clears it as it ends.
constexpr uint32_t kShowScene = 0xFF2600;
constexpr uint32_t kShowTitle = 0x074712;

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
// Health, a word (0x8F full: 009E8A sets it at a round's start); a hit takes it at 00D36C and 00D8AE (sub.w d5,0x190(a6),
// the one hit in A6), and 00E28A takes what is lost to a move's own cost. The word after it is the bar's other end.
constexpr uint32_t kHealth = 0x190, kHealthTop = 0x192;
constexpr uint16_t kHealthFull = 0x8F;

// A byte: 1 while it is in use (both players' records, in a fight)
constexpr uint32_t kActive = 0x01;
// A byte: 1 a person plays the character, 0 the computer (one- and two-player fights' records compared)
constexpr uint32_t kHuman = 0x03;
// Where a player stands: a long, the word at +0C across (16 bits of fraction after it), and the word at +10 down (the
// floor's line 472 on most stages; smaller is higher: Ground). Longs, 16 bits of fraction: the speed across and
// down, and what is added to the speed down each step (traced through walking and a jump: Marvel Super Heroes' places)
constexpr uint32_t kX = 0x0C, kY = 0x10, kSpeedX = 0x14, kSpeedY = 0x1C, kPullY = 0x20;
// A byte: bit 0 set facing right, bit 1 turned upside down (the FBNeo training script's direction, and the hitbox
// script's flip)
constexpr uint32_t kFlip = 0x4D;
// Who the character is: a word, even (the FBNeo training script's list; each seen in a fight). Juggernaut and Magneto
// are the computer's last two opponents, and Akuma a hidden one: the choosing screen's table has a place for each
// (050AF0, its 11th to 13th), which the cursor never reaches.
constexpr uint32_t kCharacter = 0x50;
constexpr int kCharacters = 13;
inline constexpr const char *kCharacterNames[kCharacters] = {
    "Wolverine", "Psylocke", "Colossus", "Cyclops", "Storm", "Iceman", "Spiral", "Silver Samurai", "Omega Red", "Sentinel",
    "Juggernaut", "Magneto", "Akuma"};
constexpr int kOrdinary = 10; // (the ones the cursor reaches)
// A word: who is in charge of the character, 0 itself (the player); others reeling from a hit
constexpr uint32_t kControl = 0x06;
// A byte: what the character is doing (traced: 0 standing, 2 walking, 4 jumping, 8 crouching, 20 a standing attack;
// guarding 1C and 1E, as the hitbox script has them)
constexpr uint32_t kAction = 0xA0;
// A byte of the frame's own (001806 copies the frame's longs to +64 on): 40 standing, walking, jumping and crouching,
// with the top bit set through an attack, from its start to the end of its recovery (traced through jabs)
constexpr uint32_t kAttackPhase = 0x64;
constexpr uint8_t kAttacking = 0x80;
constexpr uint32_t kFrame = 0x34;   // long: the animation frame being shown (in the ROM; 001806)
constexpr uint32_t kHitStop = 0x82; // byte: steps both players stay frozen as a hit lands, counted down (10 for a jab)
// The character's boxes (the hitbox script names their places; the attack's and the hurt boxes' seen to change through
// a jab): a long, the table of its boxes, and words, each a box's number in it: two attacks, four places it can be hit
// and one it can be thrown in; a byte, its push box's number in kPushTable. Bytes: attacks hit only while kCanHit is not
// 0, and it is hit only while kCanBeHit is not 0.
constexpr uint32_t kBoxTable = 0x88, kAttackBoxes = 0x70, kHurtBoxes = 0x74, kThrowableBox = 0x7C, kPushBox = 0xA2;
constexpr uint32_t kCanHit = 0x83, kCanBeHit = 0x84;
constexpr uint32_t kPushTable = 0x0C1618;
// What they throw: for each player a list of the records ending at kThrownList (player 2's 0x1C8 on) (the hitbox script)
constexpr uint32_t kThrownList = 0xFFD6C4, kThrownListSpace = 0x1C8;
// A byte: 0 a character's own colours, 1 the other set (player 2 in a match of the same two characters; 00315A reads it)
constexpr uint32_t kColourChoice = 0xAF;

// The stage, a byte: in a one-player game the computer's character's own (0045CC: by the character, from two tables), in
// a two-player one the one a roulette stops on (00468A)
constexpr uint32_t kStage = 0xFF488F;
// The stages, each a character's own (stage n is the character n's: the tables at 00461E and 00462C; a one-player game
// fights each opponent on its own; each seen in the stage viewer, and named for what it shows). Akuma's is 0F.
constexpr int kStages = 12;
inline constexpr const char *kStageNames[kStages] = {
    "The Savage Land", "The pipeworks",   "The city street",   "The Danger Room",       "The sunset",        "The frozen sea",
    "The bone garden", "The temple",      "The space station", "The Sentinel factory", "The building site", "Avalon"};
// The picture follows the stage's scroll layer that moves with the fighters: a record (by the stage, as the hitbox
// script has it: FF4980 on stages 0 and 0B on, FF4A00 elsewhere), with the camera's place at +0C across and +10 down, and
// its ends at +70/72 across and +74/76 down (each stage's own routine keeps it between them: StageCameraKept; 06EB74
// moves it after the players). The picture's left edge is the word across + 0x40, its top the word down.
inline uint32_t CameraRecord() {
    const uint8_t stage = cps2recomp::Ram8(kStage);
    return stage == 0 || stage >= 0x0B ? 0xFF4980 : 0xFF4A00;
}
constexpr uint32_t kCameraX = 0x0C, kCameraY = 0x10, kCameraLeft = 0x70, kCameraRight = 0x72, kCameraTop = 0x74, kCameraBottom = 0x76;
// The floor's height: the camera's lowest place + 0xD8, + 0xCC on the pipeworks (468 there, 472 on the others with the
// camera's lowest at 256; read off each stage's round start in the stage viewer)
inline int Ground() {
    return static_cast<int16_t>(cps2recomp::Ram16(CameraRecord() + kCameraBottom)) + (cps2recomp::Ram8(kStage) == 1 ? 0xCC : 0xD8);
}
// Where each stage's routine keeps its camera between its ends (twelve routines, 06F7A8 to 0739EC, one a stage), and
// where the routine goes on after (0: it returns)
struct KeptAt {
    uint32_t at, after;
};
inline constexpr KeptAt kCameraKept[] = {
    {cps2recomp::HookAt(0x06F804), 0x06F834}, {cps2recomp::HookAt(0x06FEAA), 0}, {cps2recomp::HookAt(0x0708D8), 0},
    {cps2recomp::HookAt(0x070FBC), 0},        {cps2recomp::HookAt(0x07136E), 0}, {cps2recomp::HookAt(0x07160C), 0},
    {cps2recomp::HookAt(0x071BCA), 0},        {cps2recomp::HookAt(0x07237A), 0}, {cps2recomp::HookAt(0x072BF0), 0},
    {cps2recomp::HookAt(0x0734E4), 0},        {cps2recomp::HookAt(0x07388C), 0}, {cps2recomp::HookAt(0x073A2E), 0x073A5E}};
// How the camera moves, two bytes (06EB74 goes by the first across, 06EC7A by the second down): 0 after both players,
// 4 after player 1 alone (06EBF8, 06ED54); the game sets them as a fight goes
constexpr uint32_t kCameraWay = 0xFF4B9A, kCameraWayDown = 0xFF4B9B;
constexpr uint8_t kCameraFollowsOne = 4;
// The camera's ends across, by stage (each read from the record in a round on the stage)
inline void CameraEnds(int stage, int &left, int &right) {
    static constexpr int16_t kEnds[kStages][2] = {{-64, 320}, {0, 384}, {0, 384}, {0, 384}, {-64, 320}, {-64, 320},
                                                  {0, 384},   {0, 384}, {0, 384}, {0, 384},   {0, 384},   {-64, 320}};
    const int n = stage >= 0 && stage < kStages ? stage : 0;
    left = kEnds[n][0];
    right = kEnds[n][1];
}
// Avalon (stage 0B) is Magneto's: a round there waits for his entrance, which sets a byte (032664; 013120 sets it too
// when a challenger joins there), before it begins (009340) and before the players make their own entrances (006EE8).
// The game puts nobody else there; a mode's stage choice can (modes.cpp).
constexpr uint8_t kAvalon = 0x0B;
constexpr uint16_t kMagneto = 0x16;
constexpr uint32_t kMagnetoCame = 0xFF4854;
constexpr int kPictureLeft = 0x40;
constexpr int kWall = 0x15;            // how near the picture's edge a player can go (00A302: 0x55 to 0x1AB on from the camera word)
constexpr uint32_t kPlanes = 0x0143A4; // longs by a character's byte +30: the camera's word of the plane it stands in

// Choosing: a byte for each player, the place of its cursor on the choosing screen (FF4900 player 1's, FF4902 player
// 2's: 0 to 9, which the table at 050AF0 turns into a character), and a byte with a bit for each player still to choose
// (bit 0 player 1; 050660 clears it as a button takes the character). Traced through choosing.
constexpr uint32_t kCursor = 0xFF4900, kStillChoosing = 0xFF4912;

// The control panel as read this frame: a word a player, the stick (bit 0 right, 1 left, 2 down, 3 up) and the
// punches (bits 4-6) in its low byte, the kicks in bits 8-10 (X-Men vs. Street Fighter's, placed here by tools/portmap.py)
constexpr uint32_t kPanel[2] = {0xFF4BAA, 0xFF4BAC};

// The sound queue: a ring of sixteen-byte requests at FF4C00, 0x400 long, the word at FF4BFA the place of the next
// (07A530 puts one in)
constexpr uint32_t kSoundQueue = 0xFF4C00, kSoundQueueEnd = 0xFF4BFA;
constexpr uint16_t kSoundQueueMask = 0x3F0;

// The round: FF4800 is 6 once it is decided, and the byte FF4804 who won it then: 0 player 1, 1 player 2, FF neither
constexpr uint32_t kRoundOver = 0xFF4804;

// --- the characters' colours ---

// A character's colours, in the ROM: a long a character (by its word kCharacter, twice) at kColourTable, its own set
// then the other kColourSetSize on, each kPalettesInSet palettes of 16 colours. The round's start copies the player's
// set to its colour sets in the buffer the board shows from (003148: 914000, 0x20 bytes a colour set): player 1's at set
// 0C, player 2's at 10.
constexpr uint32_t kColourTable = 0x003362;
constexpr int kColourSetsEach = 2, kPalettesInSet = 4, kColoursInPalette = 16;
constexpr uint32_t kColourSetSize = 0x80;
constexpr uint32_t kPaletteBuffer = 0x914000;
constexpr unsigned kPlayer1Set = 0x0C, kPlayer2Set = 0x10, kShadowSet = 0x06;

// Objects by what they are (for Video::hideObject; from the object lists of rounds): colour sets 00-03 are the panel, 06
// the fighters' shadows, 0C-13 the fighters (player 1's four, then player 2's), 14 on the stage's own pieces. The panel's
// pieces drawn in other sets are tiles 16000-16FFF: the fighters' faces (in their own sets).
constexpr unsigned kStageSets = 0x14;
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

} // namespace xmcota
