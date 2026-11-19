// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: names for what is known of the game's memory and routines, each found by
// experiment in this game (PROGRESS.md). The game is X-Men vs. Street Fighter's program carried on (tools/portmap.py
// places more than half of that game's instructions here), its variables 0x800 lower (FF5000 there is FF4800 here) and
// its players' records at FF3800; each place below was found again here.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>

namespace mshvsf {

using cps2recomp::M68K;

// --- where the game is ---

// A byte: 8 while a round is being fought (the damage routines test it before taking health: 0184F8, 018BA0)
constexpr uint32_t kFight = 0xFF4800;
enum : uint8_t { kFighting = 0x08 };

// A byte, set once a game is being played, and the byte after it: who is in, bit 0 player 1, bit 1 player 2 (RAM
// dumps of one- and two-player games; the attract mode's fights set 3)
constexpr uint32_t kPlaying = 0xFF4900, kPlayersIn = 0xFF4901;
// Words, each a step of the one before (traced with --watch over a game from Start): the game's (4 while the players
// choose their characters, 00932E; 8 in a match, 00A5D0), and the match's
constexpr uint32_t kGameStep = 0xFF4904, kMatchStep = 0xFF4906;
enum : uint16_t { kGameChoosing = 4, kGameMatch = 8 };

// The round's clock: seconds left, a BCD byte (99 at a round's start; a second off at 010E54, sbcd), and the frames
// left of the second after it (counted down at 010E38). Found by --watch through a fight.
constexpr uint32_t kRoundClock = 0xFF4808, kClockFrames = 0xFF4809;

// Credits paid for and not yet used, a byte (RAM dumps after coins)
constexpr uint32_t kCredits = 0xFF4C75;

// The game's own free play, one of the operator's coin modes (the test menu's System page, its first option: 00-08
// one coin for 1-9 credits, ..., 12 free play). The board's start reads the settings record (kSettings, 0AD926) and
// keeps the mode in kCoinMode; free play sets kFreePlay (0AD9E2), which the code that lets a player in tests in place
// of the credits. Set alone it is the whole of free play: the title says FREE.
constexpr uint32_t kCoinMode = 0xFF4C81, kFreePlay = 0xFF4C77;
constexpr uint8_t kCoinFreePlay = 0x12;

// The operator's settings as the settings memory holds them, read into the game's variables at the board's start
// (0AD926, called from 001010, so the call returns to 001016, where board.cpp and title.cpp hook). The offsets and
// where each goes are in board.cpp.
constexpr uint32_t kSettings = 0xFFFBC0;
// The show's scenes are run by a task whose record is at FF2A78 (--watch on it through the show): its step byte at +4
// is 2 while a scene runs, 0 between scenes. The title's scene (0A7E38 on, from frame 2,400 of the board's start)
// places the title's picture at 0A80AE (the picture, object 0B, made at 09995A).
constexpr uint32_t kShowStep = 0xFF2A7C;

// --- the players ---

// Four records, 0x400 long: player 1's first character at FF3800, player 2's at FF3C00, then their second characters
// at FF4000 and FF4400 (the FBNeo training script for this set names them; confirmed here by the characters and health
// each shows, and the record's long +264, the partner's). Unlike X-Men vs. Street Fighter's, a tag does not swap the
// records' contents: each character keeps its record, and the game keeps a long a side, the record of the character on
// point (FF48C8 player 1's, FF48D0 player 2's; the long after each the partner's: RAM dumps before and after a tag, and
// the camera's step reads them, 0A1C64). A record's byte +00 is 1 while its character is on point, +261 1 while it
// rests.
constexpr uint32_t kPlayer1 = 0xFF3800, kPlayerSize = 0x400, kPartners = 0x800;
constexpr uint32_t kOnPoint = 0xFF48C8;
// The record of a side's character on point now (the first character's until the game has made the round's records)
inline uint32_t Player(int n) {
    const uint32_t at = cps2recomp::Ram32(kOnPoint + static_cast<uint32_t>(n) * 8) & 0xFFFFFF;
    return at == kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize || at == kPlayer1 + kPartners + static_cast<uint32_t>(n) * kPlayerSize
               ? at
               : kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}
// ... and of its partner
inline uint32_t PartnerOf(int n) {
    const uint32_t point = Player(n);
    return point < kPlayer1 + kPartners ? point + kPartners : point - kPartners;
}
// Which side a record is (0, 1), or -1 for none of the four
inline int SideOf(uint32_t record) {
    record &= 0xFFFFFF;
    if (record < kPlayer1 || record >= kPlayer1 + 4 * kPlayerSize || (record - kPlayer1) % kPlayerSize) {
        return -1;
    }
    return static_cast<int>((record - kPlayer1) / kPlayerSize) & 1;
}
// Health, a word (0x90 full); a hit takes it at 018536 and 018BB2 (sub.w d5,0x250(a6), the victim in A6)
constexpr uint32_t kHealth = 0x250;
constexpr uint16_t kHealthFull = 0x90;
constexpr uint32_t kRedHealth = 0x25A; // what comes back while the character rests (cleared with the health at 018548)
constexpr uint32_t kPartner = 0x264;   // long: the record of the same side's other character (0113EC on)
// The hyper combo gauge, the side's: a byte, its levels (three at most), and a word, the bar towards the next (0x90
// full) (the FBNeo training script names the levels; the bar is the word before, as in X-Men vs. Street Fighter)
constexpr uint32_t kGaugeLevel = 0x254, kGaugeBar = 0x252;
constexpr uint8_t kGaugeFull = 3;
constexpr uint16_t kGaugeBarFull = 0x90;

// Where a player stands: a long, the word at +0C across (with 16 bits of fraction after it) and the word at +10 down
// (on the floor at Ground(); smaller is higher)
constexpr uint32_t kX = 0x0C, kY = 0x10;
// Longs, 16 bits of fraction: the speed across, and the speed down and what is added to it each step
constexpr uint32_t kSpeedX = 0x14, kSpeedY = 0x1C, kPullY = 0x20;
// A byte: bit 0 set facing right, bit 1 turned upside down (the hitbox script for this engine names it; confirmed by
// the players' sides)
constexpr uint32_t kFlip = 0x4B;
// Who the character is: a word, even (each number put in player 1's picks, FF48B0, and seen in a fight with its name
// on the bar: 0 Apocalypse, the one-player game's last opponent (its bar unnamed); 22 Norimaro, the Japanese release's
// alone; 26 Cyber-Akuma, a last opponent the game has in place of Apocalypse in some games (00B210); 28-32 the hidden
// characters; 34 on are none, the board starts again)
constexpr uint32_t kCharacter = 0x52;
constexpr int kCharacters = 26;
inline constexpr const char *kCharacterNames[kCharacters] = {
    "Apocalypse", "Cyclops",   "Captain America", "Hulk",        "Wolverine",    "Shuma-Gorath", "Omega Red",
    "Spider-Man", "Blackheart", "Ryu",            "Ken",         "Chun-Li",      "Dhalsim",      "Zangief",
    "M. Bison",   "Akuma",      "Sakura",         "Norimaro",    "Dan",          "Cyber-Akuma",  "Mech-Zangief",
    "Dark Sakura", "Shadow",    "U.S. Agent",     "Mephisto",    "Armored Spider-Man"};
enum : uint16_t {
    kApocalypse = 0x00, kCyclops = 0x02, kCaptainAmerica = 0x04, kSpiderMan = 0x0E, kBlackheart = 0x10, kZangief = 0x1A,
    kAkuma = 0x1E, kSakura = 0x20, kNorimaro = 0x22, kDan = 0x24, kCyberAkuma = 0x26, kMechZangief = 0x28,
    kDarkSakura = 0x2A, kShadow = 0x2C, kUSAgent = 0x2E, kMephisto = 0x30, kArmoredSpiderMan = 0x32
};
// The ones a player chooses on the board, with or without a code (not the last opponents, nor another release's)
inline bool Choosable(int number) {
    return (number & 1) == 0 && number >= kCyclops && number <= kArmoredSpiderMan && number != kNorimaro &&
           number != kCyberAkuma;
}
// A word: who is in charge of the character, 0 itself (the player)
constexpr uint32_t kControl = 0x06;
// A byte: what the character is doing
constexpr uint32_t kAction = 0xA0;
enum : uint8_t {
    kActionStand = 0x00, kActionWalk = 0x02, kActionJump = 0x06, kActionCrouch = 0x0C, kActionGuard = 0x24,
    kActionCrouchGuard = 0x26, kActionAttack = 0x28, kActionHit = 0x36
};
constexpr uint32_t kAttackPhase = 0x64; // byte: 80 an attack going on, 40 its recovery
constexpr uint32_t kFrame = 0x34;       // long: the animation frame being shown (in the ROM)
constexpr uint32_t kHitStop = 0x85;     // byte: steps both players stay frozen as a hit lands, counted down
// The character's boxes (the hitbox script names their places, the same as X-Men vs. Street Fighter's): a long, the
// table of its boxes, and words, each a box's number in it: two attacks, four places it can be hit and one it can be
// thrown in; a byte, its push box's number in kPushTable. Bytes: attacks hit only while kCanHit is not 0, and it is hit
// only while kCanBeHit is not 0.
constexpr uint32_t kBoxTable = 0x6C, kAttackBoxes = 0x70, kHurtBoxes = 0x74, kThrowableBox = 0x7C, kPushBox = 0xA4;
constexpr uint32_t kCanHit = 0x82, kCanBeHit = 0x83;
constexpr uint32_t kPushTable = 0x137EE2;
// What they throw: for each player a list of the records (longs, the newest last) ending at kThrownList (player 2's
// 0x150 on)
constexpr uint32_t kThrownList = 0xFFE32E, kThrownListSpace = 0x150;

// The picture follows the stage's scroll layer that moves with the fighters: a record (one of three, by the stage, the
// byte kStage), with the camera's place at +0C across and +10 down. Its step (0A1C4E) works on a copy of them (+76 and
// +78, made as it starts: 0A1C54, 0A1EDA), keeps the copy between the ends at +68/+6A across and +6C/+6E down
// (0A55AC, 0A55D0) and puts it back (0A55C4, 0A55E8). The picture's left edge is the word across + 0x40, its top the
// word down.
constexpr uint32_t kStage = 0xFF4913;
// The stages, by the byte kStage (0-11; each seen with a training session's stage held at it; named for what they
// show). The one-player game's last fight is on 0 (00B220); the others come from a table by a count (0BD720).
constexpr int kStages = 12;
inline constexpr const char *kStageNames[kStages] = {
    "Apocalypse's lair", "The autumn pond", "The canyon",         "The jungle at night", "The temple",     "The great tree",
    "Manhattan at night", "The refinery",  "The desert base",    "The TV studio",       "The shopping mall", "The refinery in flames"};
inline uint32_t CameraRecord() {
    static constexpr int16_t kOffsets[16] = {-0x34C0, -0x35C0, -0x35C0, -0x3540, -0x35C0, -0x35C0, -0x34C0, -0x35C0,
                                             -0x3540, -0x3540, -0x35C0, -0x35C0, -0x35C0, -0x35C0, -0x35C0, -0x35C0};
    return static_cast<uint32_t>(0xFF8000 + kOffsets[cps2recomp::Ram8(kStage) & 0x0F]);
}
constexpr uint32_t kCameraX = 0x0C, kCameraY = 0x10, kCameraWorkX = 0x76, kCameraWorkY = 0x78;
// The other places that keep the camera's copy across between its ends: each stage's own routine has the piece
// (move.w 0x68(a6),d0 ... move.w d0,0x76(a6), 0x18 bytes) that RegisterWidescreen's keepCamera (0A55AC) is one of, and
// its keeping is over at the piece's end. Widescreen has to keep each 64 nearer the middle, or on those stages the
// camera goes to the board's ends while the players go 64 past them, out of the picture (issue #116). Found as every
// such piece in the listing (tools/lst.py -r).
struct KeptAt {
    uint32_t at, after;
};
inline constexpr KeptAt kCameraKept[] = {
    {cps2recomp::HookAt(0x0A2F46), 0x0A2F5E}, {cps2recomp::HookAt(0x0A35A0), 0x0A35B8},
    {cps2recomp::HookAt(0x0A36F0), 0x0A3708}, {cps2recomp::HookAt(0x0A3912), 0x0A392A},
    {cps2recomp::HookAt(0x0A3A0A), 0x0A3A22}, {cps2recomp::HookAt(0x0A3BB4), 0x0A3BCC},
    {cps2recomp::HookAt(0x0A40F6), 0x0A410E}, {cps2recomp::HookAt(0x0A44EE), 0x0A4506},
    {cps2recomp::HookAt(0x0A5790), 0x0A57A8}, {cps2recomp::HookAt(0x0A58EA), 0x0A5902},
    {cps2recomp::HookAt(0x0A5B64), 0x0A5B7C}, {cps2recomp::HookAt(0x0A5EDA), 0x0A5EF2}};
constexpr uint32_t kCameraLeft = 0x68, kCameraRight = 0x6A, kCameraTop = 0x6C;
// How the camera moves, a byte (0A1C94 goes by it, or by the one two after it when a point character's +207 says): 0
// after both players, 4 after player 1 alone; the game sets it every frame
constexpr uint32_t kCameraWay = 0xFF4BE8;
constexpr uint8_t kCameraFollowsOne = 4;
// The camera's ends across, by stage (each read from the record in a training round on the stage): Apocalypse's lair,
// the refinery and the refinery in flames have their own, the others the same
inline void CameraEnds(int stage, int &left, int &right) {
    left = stage == 0 ? 704 : stage == 7 || stage == 11 ? -256 : -64;
    right = left + 384;
}
// The floor's height: the camera's lowest place + 0xD8 (0x1D8 on most stages, 0x3E8 in Apocalypse's lair; read in a
// training round on each stage)
inline int Ground() {
    return static_cast<int16_t>(cps2recomp::Ram16(CameraRecord() + 0x6E)) + 0xD8;
}
constexpr int kPictureLeft = 0x40;
constexpr int kWall = 0x15; // how near the picture's edge a player can go (011BB8: 0x55 to 0x1AB on from the camera)

// Each player's choosing: a record of 0x20 bytes, player 1's at FF4980; its first byte steps up to 7 once the player
// has chosen both characters
constexpr uint32_t kChoosing = 0xFF4980, kChoosingSize = 0x20;
constexpr uint8_t kChoosingDone = 7;
// The players' picks: words, player 1's first and second character, then player 2's (a round's records are made from
// them, 0113D6 on: the record's byte +260 says which of the two it is); then a byte each, the colour each was chosen in
constexpr uint32_t kPicks = 0xFF48B0;

// The control panel as read this frame: a word a player, the stick (bit 0 right, 1 left, 2 down, 3 up) and the punches
// (bits 4-6) in its low byte, the kicks in bits 8-10
constexpr uint32_t kPanel[2] = {0xFF4C2A, 0xFF4C2C};

// The sound queue: a ring of sixteen-byte requests at FF2BE6, 0x400 long, the word at FF2BE0 the place of the next
// (0ABD78 puts one in)
constexpr uint32_t kSoundQueue = 0xFF2BE6, kSoundQueueEnd = 0xFF2BE0;
constexpr uint16_t kSoundQueueMask = 0x3F0;

// --- the characters' colours ---

// A character's colours, in the ROM: a long a character (by its word kCharacter) at kColourTable, the first of its sets,
// two of them (punch, kick) 0x60 apart, each three palettes of 16 colours (the round's start copies them, 0113FC on, as
// X-Men vs. Street Fighter's does). The record's byte kColourChoice picks the set; its long kPalette is where its
// palettes are in the buffer the board shows from: player 1's first character's at set 0C, player 2's at 0F, their
// second characters' at 12 and 15 (0113E6 on).
constexpr uint32_t kColourTable = 0x1F172E;
constexpr int kColourSetsEach = 2, kPalettesInSet = 3, kColoursInPalette = 16;
constexpr uint32_t kColourSetSize = 0x60;
constexpr uint32_t kColourChoice = 0x257, kPalette = 0x268;
constexpr uint32_t kPaletteBuffer = 0x914000;
constexpr unsigned kPlayer1Set = 0x0C, kPlayer2Set = 0x0F, kShadowSet = 0x08;

// Objects by what they are (for Video::hideObject), as in X-Men vs. Street Fighter (from the object lists of rounds; the
// widescreen pictures keep the panel in the middle): colour sets 00-04 the panel, 08 the shadows, 0C-17 the fighters,
// 1A on the stage's own pieces
inline bool NotTheStage(uint32_t, uint16_t attributes) {
    return (attributes & 0x1F) < 0x1A;
}
// The panel: its sets and the fighters' faces, and the round's words
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return set <= 0x04 || (tile >= 0x15400 && tile < 0x15D00) || ((set == 0x00 || set == 0x18) && tile >= 0x1C000 && tile < 0x1D200);
}

// --- for hooks ---

inline void SetLow16(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFF0000) | (value & 0xFFFF);
}

} // namespace mshvsf
