// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: names for what is known of the game's memory and routines, each found by experiment in
// this game (PROGRESS.md). A5 is FF8000 as in Capcom's other CPS-2 games, but this game keeps its variables below it
// (A5-3000 on), and its players' records at FF4000.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>

namespace xmvsf {

using cps2recomp::M68K;

// --- where the game is ---

// A byte: 8 while a round is being fought (the damage routines test it before taking health: 015568, 015B58)
constexpr uint32_t kFight = 0xFF5000;
enum : uint8_t { kFighting = 0x08 };

// A byte, set once a game is being played (077FA2, after Start), and the byte after it: who is in, bit 0 player 1,
// bit 1 player 2 (078044 sets a player's bit as Start takes its credit; the attract mode's fights set 3)
constexpr uint32_t kPlaying = 0xFF5100, kPlayersIn = 0xFF5101;
// Words, each a step of the one before (traced with --watch over a game from Start): the game's (4 while the players
// choose their characters, 8 in a match: 00A3EA, 00B142), and the match's (2 then 4 as its first round begins)
constexpr uint32_t kGameStep = 0xFF5104, kMatchStep = 0xFF5106;
enum : uint16_t { kGameChoosing = 4, kGameMatch = 8 };

// The round's clock: seconds left, a BCD byte (99 at a round's start, 00A4AC; a second off at 00FB2C, sbcd), and the
// frames left of the second after it (counted down at 00FB14; set again from a table by the timer speed, 00FB3A).
// Found by RAM traces through a fight, then --watch.
constexpr uint32_t kRoundClock = 0xFF5008, kClockFrames = 0xFF5009;

// Credits paid for and not yet used, a byte (RAM dumps after one to four coins: 1-4; nine at most, 07811A)
constexpr uint32_t kCredits = 0xFF545F;

// The game's own free play, one of the operator's coin modes (the test menu's System page, its first option: 00-08
// one coin for 1-9 credits, ..., 12 free play). The board's start reads the settings record (kSettings, 079E7C) and
// keeps the mode in kCoinMode; free play sets kFreePlay (079F38), which the code that lets a player in tests in place
// of the credits (078016). Set alone it is the whole of free play: the title says FREE.
constexpr uint32_t kCoinMode = 0xFF5469, kFreePlay = 0xFF5460;
constexpr uint8_t kCoinFreePlay = 0x12;

// The operator's settings as the settings memory holds them, read into the game's variables at the board's start
// (079ECA on; called from 000E20, so the call returns to 000E26, where board.cpp and title.cpp hook). The offsets
// and where each goes are in board.cpp. The attract mode places the title's picture (its parts 4, 5 and 6, 06BA02)
// at 075BF2: the show has reached the title.
constexpr uint32_t kSettings = 0xFFFAA4;
// The show's scenes are run by a task whose record is at FF24DC (seen as the title is placed): its step byte at +4 is 2
// while a scene of the introduction runs, 0 in the rankings and the notices. The show's loop: the notices and the
// maker's logo; the introduction (pictures, words and short fights, the title last, 075BF2); a fight with the
// copyright over it (the game's step kGameMatch); the rankings.
constexpr uint32_t kShowStep = 0xFF24E0;

// --- the players ---

// Four records, 0x400 long: player 1's first character at FF4000, player 2's at FF4400, then their second characters
// at FF4800 and FF4C00 (named in the FBNeo training script for this set; confirmed here by the health each shows)
constexpr uint32_t kPlayer1 = 0xFF4000, kPlayerSize = 0x400, kPartners = 0x800;
inline uint32_t Player(int n) {
    return kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}
// Which side a record is (0, 1), or -1 for none of the four
inline int SideOf(uint32_t record) {
    record &= 0xFFFFFF;
    if (record < kPlayer1 || record >= kPlayer1 + 4 * kPlayerSize || (record - kPlayer1) % kPlayerSize) {
        return -1;
    }
    return static_cast<int>((record - kPlayer1) / kPlayerSize) & 1;
}
// Health, a word (0x90 full: 0101F0); a hit takes it at 0155A2 and 015B6A (sub.w d5,0x210(a6), the victim in A6)
constexpr uint32_t kHealth = 0x210;
constexpr uint16_t kHealthFull = 0x90;
constexpr uint32_t kRedHealth = 0x21A; // what comes back while the character rests (016934)
constexpr uint32_t kPartner = 0x224;   // long: the record of the same side's other character (0155C4)
// The hyper combo gauge, the side's (both characters show the same): a byte, its levels (three at most), and a word, the
// bar towards the next (0x90 full) (the FBNeo training script names them)
constexpr uint32_t kGaugeLevel = 0x214, kGaugeBar = 0x212;
constexpr uint8_t kGaugeFull = 3;
constexpr uint16_t kGaugeBarFull = 0x90;

// Where a player stands: a long, the word at +0C across (with 16 bits of fraction after it) and the word at +10 down
// (0x1D8 on the floor; smaller is higher). Players start a round at 0x100 and 0x200 on most stages, elsewhere on
// Apocalypse's lair and the refinery (RAM traces of rounds).
constexpr uint32_t kX = 0x0C, kY = 0x10;
// Longs, 16 bits of fraction: the speed across (a walk's 4 pixels a step), and the speed down and what is added to it
// each step (a jump: up 6.1 pixels a step as it leaves the floor, pulled down by 0.42 a step; RAM dumps through a jump)
constexpr uint32_t kSpeedX = 0x14, kSpeedY = 0x1C, kPullY = 0x20;
constexpr uint16_t kGround = 0x1D8;
// A byte: bit 0 set facing right (a box's offset across is kept as if facing left), bit 1 turned upside down (the
// hitbox script for this engine in fbneo-training-mode names it; confirmed by the players' sides)
constexpr uint32_t kFlip = 0x4B;
// Who the character is: a word, even (0 Wolverine to 24 Alpha Chun-Li: the FBNeo training script's table, each seen
// in the model viewer)
constexpr uint32_t kCharacter = 0x52;
constexpr int kCharacters = 19;
inline constexpr const char *kCharacterNames[kCharacters] = {
    "Wolverine", "Cyclops", "Storm", "Rogue", "Gambit", "Sabretooth", "Juggernaut", "Magneto", "Apocalypse", "Ryu",
    "Ken", "Chun-Li", "Dhalsim", "Zangief", "M. Bison", "Akuma", "Charlie", "Cammy", "Chun-Li (Alpha)"};
// A word: who is in charge of the character, 0 itself (the player); other values reeling from a hit (0C), thrown, and
// so on (RAM dumps through hits)
constexpr uint32_t kControl = 0x06;
// A byte: what the character is doing (a guarded hit 24, crouching 26; RAM dumps of standing, walking, jumping,
// crouching, attacking, guarding and being hit)
constexpr uint32_t kAction = 0xA0;
enum : uint8_t {
    kActionStand = 0x00, kActionWalk = 0x02, kActionJump = 0x06, kActionCrouch = 0x0C, kActionGuard = 0x24,
    kActionCrouchGuard = 0x26, kActionAttack = 0x28, kActionHit = 0x36
};
constexpr uint32_t kAttackPhase = 0x64; // byte: 80 an attack going on, 40 its recovery
constexpr uint32_t kFrame = 0x34;       // long: the animation frame being shown (in the ROM)
constexpr uint32_t kHitStop = 0x85;     // byte: steps both players stay frozen as a hit lands, counted down
// The character's boxes (the hitbox script names their places): a long, the table of its boxes (eight bytes each: the
// middle across from where it stands and half the width, then the same up and down, signed words), and words, each a
// box's number in it (the low 13 bits; 0 none): two attacks, four places it can be hit and one it can be thrown in; a
// byte, its push box's number in kPushTable (then the character's own of it, eight bytes each). An attack's high bit
// says a throw. Bytes: attacks hit only while kCanHit is not 0, and it is hit only while kCanBeHit is not 0.
constexpr uint32_t kBoxTable = 0x6C, kAttackBoxes = 0x70, kHurtBoxes = 0x74, kThrowableBox = 0x7C, kPushBox = 0xA4;
constexpr uint32_t kCanHit = 0x82, kCanBeHit = 0x83;
constexpr uint32_t kPushTable = 0x08B022;
// What they throw: for each player a list of the records (longs, the newest last) ending at kThrownList (player 2's
// 0x150 on), the list's end where a long is a player's record or less (the hitbox script)
constexpr uint32_t kThrownList = 0xFFE3D8, kThrownListSpace = 0x150;

// The picture follows the stage's scroll layer that moves with the fighters: a record (one of three, by the stage, the
// byte kStage: the hitbox script's table), with the camera's place at +0C across and +10 down, and its ends at +72/74
// across and +76/78 down (074488 keeps it between them; 0702D4 moves it after the players). The picture's left edge
// is the word across + 0x40, its top the word down; the floor's line in the picture is 0x1D8 less the word down less
// 0x0F.
constexpr uint32_t kStage = 0xFF5113;
// The stages, by the byte kStage (each seen with a training session's stage held at it; named for what they show). The
// music is the characters', not the stage's.
constexpr int kStages = 11;
inline constexpr const char *kStageNames[kStages] = {
    "Apocalypse's lair", "The garden pond", "The cavern",    "The waterfall",   "The snowy temple", "The great tree",
    "Manhattan at night", "The refinery",   "The space base", "The TV studio", "The shopping mall"};
inline uint32_t CameraRecord() {
    static constexpr int16_t kOffsets[16] = {-0x2CC0, -0x2DC0, -0x2DC0, -0x2D40, -0x2DC0, -0x2DC0, -0x2CC0, -0x2DC0,
                                             -0x2D40, -0x2D40, -0x2DC0, -0x2DC0, -0x2DC0, -0x2DC0, -0x2DC0, -0x2DC0};
    return static_cast<uint32_t>(0xFF8000 + kOffsets[cps2recomp::Ram8(kStage) & 0x0F]);
}
constexpr uint32_t kCameraX = 0x0C, kCameraY = 0x10, kCameraLeft = 0x72, kCameraRight = 0x74, kCameraTop = 0x76;
// The other places that keep a camera between its ends: the stages' and their planes' own routines each have the
// piece (move.w 0x72(a6),d0 ... move.w d0,0x10(a6), 0x30 bytes) that RegisterWidescreen's keepCamera (074488) is one of,
// and the stage's code goes on after it (0: the piece ends its routine). Widescreen has to keep each 64 nearer the
// middle, or on those stages the camera goes to the board's ends while the players go 64 past them, out of the
// picture (issue #116). Found as every such piece in the listing (tools/lst.py -r).
struct KeptAt {
    uint32_t at, after;
};
inline constexpr KeptAt kCameraKept[] = {
    {cps2recomp::HookAt(0x0712D2), 0x071302}, {cps2recomp::HookAt(0x07185C), 0}, {cps2recomp::HookAt(0x071994), 0},
    {cps2recomp::HookAt(0x071B9E), 0}, {cps2recomp::HookAt(0x071C7E), 0}, {cps2recomp::HookAt(0x071E54), 0},
    {cps2recomp::HookAt(0x071EDC), 0}, {cps2recomp::HookAt(0x072496), 0x0724C6}, {cps2recomp::HookAt(0x072B0A), 0},
    {cps2recomp::HookAt(0x072B90), 0}, {cps2recomp::HookAt(0x072C96), 0}, {cps2recomp::HookAt(0x072D26), 0},
    {cps2recomp::HookAt(0x072E16), 0}, {cps2recomp::HookAt(0x072E8E), 0}, {cps2recomp::HookAt(0x07460A), 0},
    {cps2recomp::HookAt(0x074752), 0}};
// How the camera moves, a byte (070232 goes by it, or by the one two after it when a player's +257 says): 0 after both
// players, 4 after player 1 alone (070320: its middle 0x100 on from the camera's word); the game sets it every frame
constexpr uint32_t kCameraWay = 0xFF53E8;
constexpr uint8_t kCameraFollowsOne = 4;
// The camera's ends across, by stage (each read from the record in a round on the stage): Apocalypse's lair and the
// refinery have their own, the others the same
inline void CameraEnds(int stage, int &left, int &right) {
    left = stage == 0 ? 704 : stage == 7 ? -256 : -64;
    right = left + 384;
}
constexpr int kPictureLeft = 0x40;
constexpr int kWall = 0x15; // how near the picture's edge a player can go (a player held against the right edge: 747 of 768)

// Each player's choosing: a record of 0x20 bytes, player 1's at FF5180; its first byte steps up to 7 once the player
// has chosen both characters, the mode and the speed (traced through choosing)
constexpr uint32_t kChoosing = 0xFF5180, kChoosingSize = 0x20;
constexpr uint8_t kChoosingDone = 7;
// The players' picks: words, player 1's first and second character, then player 2's (a round's records are made from
// them, 00FFC0: the record's byte +220 says which of the two it is)
constexpr uint32_t kPicks = 0xFF50B0;

// The control panel as read this frame (000BDE on): a word a player, the stick (bit 0 right, 1 left, 2 down, 3 up) and
// the punches (bits 4-6) in its low byte, the kicks in bits 8-10
constexpr uint32_t kPanel[2] = {0xFF5416, 0xFF5418};

// The sound queue: a ring of sixteen-byte requests at FF2648, 0x400 long, the word at FF2642 the place of the next
// (0787E0 puts one in; 000B04 sends the oldest to the sound CPU)
constexpr uint32_t kSoundQueue = 0xFF2648, kSoundQueueEnd = 0xFF2642;
constexpr uint16_t kSoundQueueMask = 0x3F0;

// --- the characters' colours ---

// A character's colours, in the ROM: a long a character (by its word kCharacter) at kColourTable, the first of its sets,
// kColourSetsEach of them kColourSetSize apart, each kPalettesInSet palettes of 16 colours (words, 4 bits each of red,
// green and blue; the game keeps the brightness the palette buffer had). The round's start copies the player's set to
// its colour sets in the buffer (009F64: kColourTable, then + 0x60 for the kick's; Rogue's has one palette). The
// player's byte kColourChoice picks the set: 0 a punch chose the character, 1 a kick. The record's long kPalette is
// where its palettes are in the buffer the board shows from (914000, 0x20 bytes a colour set): player 1's point
// character's at set 0C, player 2's at 0F, their partners' at 12 and 15 (00FFC0).
constexpr uint32_t kColourTable = 0x1145A8;
constexpr int kColourSetsEach = 2, kPalettesInSet = 3, kColoursInPalette = 16;
constexpr uint32_t kColourSetSize = 0x60;
constexpr uint32_t kColourChoice = 0x217, kPalette = 0x228;
constexpr uint32_t kPaletteBuffer = 0x914000;
constexpr unsigned kPlayer1Set = 0x0C, kPlayer2Set = 0x0F, kShadowSet = 0x08;

// Objects by what they are (for Video::hideObject; from the object lists of rounds on each stage): colour sets 00-04 are
// the panel, 08 the fighters' shadows, 0C-17 the fighters (their three sets each, kPalette; and their faces on the
// panel, tiles 15Bxx), 1A-1E the stage's own pieces
inline bool NotTheStage(uint32_t, uint16_t attributes) {
    return (attributes & 0x1F) < 0x1A;
}
// The panel: its sets and the fighters' faces, and the round's words ("READY" in set 00, "FIGHT!" in 18: tiles
// 1C000-1D1FF, from the object lists as each was shown)
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return set <= 0x04 || (tile >= 0x15400 && tile < 0x15D00) || ((set == 0x00 || set == 0x18) && tile >= 0x1C000 && tile < 0x1D200);
}

// --- for hooks ---

inline void SetLow16(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFF0000) | (value & 0xFFFF);
}

} // namespace xmvsf
