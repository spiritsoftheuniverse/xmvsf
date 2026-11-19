// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: Darkstalkers' Revenge: names for what is known of the game's memory and routines, each found by
// experiment in this game (PROGRESS.md). Its variables are reached from A5 = FF8000. Its program is its own: no more
// than 1.4% of any other game's in the collection lines up with it (tools/portmap.py), Vampire Savior's 0.3%.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"
#include "places.h"

#include <cstdint>

namespace nwarr {

using cps2recomp::M68K;

// --- where the game is ---

// Words from A5, each a step (in twos) of the one before. The first is the step of whatever the game is doing: the
// board's start steps it 2 to 10 (01F88C on, frames 72-441), then its show loops 2 to 1A (020BC0 on: 6 with the next
// word at 14 is the title, frames 2801-3304; 8 and on the fights it shows); a game started by Start runs its own
// routine (02527C, given at 0215F4), which steps it from 0 again. Traced with --watch.
constexpr uint32_t kMode = At(0xFF8000), kStep = At(0xFF8004), kStep2 = At(0xFF8008), kStep3 = At(0xFF800C);
enum : uint16_t { kBootLast = 0x10, kShowTitle = family::kShowTitle, kShowTitleStep = family::kShowTitleStep };

// Credits paid for and not yet used, a byte (RAM dumps after one, two and three coins: 1, 2, 3; 004D38 adds a coin's,
// 021454 takes one as Start is pressed). It is the sixth byte of the coin record at A5+1AC (004D26 on). The board
// has no free play of its own: the test menu's coin modes are 1 coin for 1-9 credits, 2-9 coins a credit, and 2 coins
// to start with 1 to continue (stepped through, eighteen in all).
constexpr uint32_t kCredits = At(0xFF81B2);

// The round's clock: seconds left, a BCD byte (99 at a round's start, 00876C; a second off at 0087AC, sbcd), and the
// frames left of the second (FF8E15, counted down at 008798 and set again from FF8E17, which the timer speed sets).
// Found with --watch through a fight (a second every 42 frames at the board's own speed).
constexpr uint32_t kRoundClock = At(0xFF8E09), kClockFrames = At(0xFF8E15);

// --- the players ---

// Each player's record, 0x500 long, the first at FF8388: the health word is at +42 (0x90 full, 00668A sets both),
// and the only writer of it in a fight is 0C1384 (sub.w d4,0x42(a4)), with --watch on it through a fight. The FBNeo
// training script for this set names the health byte FF83CB and FF88CB.
constexpr uint32_t kPlayer1 = At(0xFF8388), kPlayerSize = family::kPlayerSize;
inline uint32_t Player(int n) {
    return kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}
constexpr uint32_t kHealth = 0x42;
constexpr uint16_t kHealthFull = 0x90;
// The rest of the record, from dumps of player 1 standing, crouching, walking, jumping and attacking and of player 2
// hit and guarding (PROGRESS.md)
constexpr uint32_t kCharacter = 0x02; // who the player is (1-14: the stages' order, one on); set from kChoice at 006934
constexpr uint32_t kState = 0x04;     // 00 standing or walking, 02 crouching, 08 jumping, 0A guarding, 0C reeling
constexpr uint32_t kAttack = 0x05;    // 02 while attacking
constexpr uint32_t kFacesLeft = 0x09; // 00 facing right, 01 left
constexpr uint32_t kX = 0x10, kY = 0x14;
constexpr uint16_t kGround = 0x28;
constexpr uint32_t kFrame = 0x1C;     // long: the animation frame shown, in the ROM
constexpr uint32_t kHitStop = 0x4B;
constexpr uint32_t kChoice = family::kChoice;               // the character chosen (the cursor's, 028AA8)
constexpr uint32_t kColoursChosen = family::kColoursChosen; // 1-6, the button that chose (029012); 0 before
enum : uint8_t { kStateStand = 0x00, kStateCrouch = 0x02, kStateJump = 0x08, kStateGuard = 0x0A, kStateReel = 0x0C };
constexpr int kCharacters = family::kCharacters;
inline constexpr const char *const (&kCharacterNames)[kCharacters] = family::kCharacterNames;
constexpr uint32_t kColourSet = 0x0D; // byte: the first of the player's five colour sets (player 1 09, player 2 0E)
constexpr uint32_t kPerson = family::kPerson; // byte: 1 for a person, 0 for the computer (02A29E reads the panel only with it)
// The fight's step for each player (02A200 runs it for player 1, then player 2, A6 the player's record)
constexpr uint32_t kPlayerStep = At(0x02A220);

// Who is in: a byte, bit 0 player 1, bit 1 player 2 (RAM dumps before and after each Start: 0, 1, 3)
constexpr uint32_t kPlayersIn = At(0xFF81F9);

// A game's steps under kMode (2 in a game, run by its own routine 02527C): kStep 0C while the players choose, 04 the
// match; kStep2 06-08 a round's introduction, 0A while it is fought, 0C over, 0E-10 on to the next round, 12 the match
// won, 00-04 the next opponent's (watched through a ladder with one-hit knockouts)
enum : uint16_t { kGameRunning = 2, kGameChoosing = family::kGameChoosing, kGameMatch = 0x04, kRoundFought = 0x0A };

// The sound queue: a ring of sixteen-byte requests at FF0AA0, the word at FF8026 the place of the next (008058 sends the
// one at FF8028 to the sound CPU, and steps it by 0x10 in 0x3F0)
constexpr uint32_t kSoundQueue = At(0xFF0AA0), kSoundQueueEnd = At(0xFF8026);
constexpr uint16_t kSoundQueueMask = 0x3F0;

// The camera: an object at FF8F08, its X (the picture's left edge in the stage) at FF8F18 and Y at FF8F1C; scroll 2 is
// X less 0xC0 (0FF6A8). X runs 0x100-0x280, and the players from 0x118 to 0x3E8 (both walked to each end); they start
// a round at 0x228 and 0x2D8.
constexpr uint32_t kCameraX = At(0xFF8F18), kCameraY = At(0xFF8F1C);
constexpr int kCameraLeft = 0x100, kCameraRight = 0x280;
constexpr int kStageLeft = 0x118, kStageRight = 0x3E8;
constexpr uint16_t kStartX[2] = {0x228, 0x2D8};

// The stage: a word, its number, which is also the computer's next opponent (whose stage it is); 0064D6 sets it from
// the ladder's order (the word at FF8D8A its place). Each seen with the word held at it (14 on are no stage).
constexpr uint32_t kStage = At(0xFF8D88);
constexpr uint32_t kStageSet = At(0x0064D6);
constexpr int kStages = family::kStages;
inline constexpr const char *const (&kStageNames)[kStages] = family::kStageNames;

// The control panel as the game read it this frame (004942), a word a player: the low byte the stick (bits 0-3:
// right, left, down, up) and the punches (4-6), the high byte the kicks (0-2). The word after it is the frame before's.
// Found holding known buttons for each player.
constexpr uint32_t kControls[2] = {At(0xFF808E), At(0xFF8092)};

// A knockout: FF8E14 set (FF), FF8E13 the kind of finish (0C13B8); the game keeps no byte of who has won
constexpr uint32_t kKnockOut = At(0xFF8E14);

// What the players throw: records of a player's layout, 0xC0 long, nine from FFA86E (the hit test reads player 2's at
// A5+286E, 292E, 29EE and player 1's at A5+2CEE, 2DAE, 2E6E: 0C0DEE on); in use while the first byte is above 0
constexpr uint32_t kThrown = At(0xFFA86E), kThrownSize = 0xC0;
constexpr int kThrownRecords = family::kThrownRecords;

// Objects by their colour set (attributes' low five bits) and tile, from the object lists of fights on several stages:
// 00-04 the panel (bars, names, clock), 09-0D player 1's and 0E-12 player 2's (kColourSet), 13 their shadows and
// sparks, 14-1F the stage's pieces; the faces beside the bars are in the players' sets, tiles 28440-284BF
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return set <= family::kPanelSets || (set < family::kStageSets && tile >= family::kFaces && tile < family::kFacesEnd);
}
inline bool NotTheStage(uint32_t tile, uint16_t attributes) {
    (void)tile;
    return (attributes & 0x1F) < family::kStageSets;
}
inline bool NotTheFighters(uint32_t tile, uint16_t attributes) {
    return ThePanel(tile, attributes) || (attributes & 0x1F) >= family::kStageSets;
}

// --- the board's settings ---

// The settings record as read from the settings memory, at A5-6D12 (FF12EE); 01FC34 on copies it to the variables
// below and returns at 01FCDC. Each variable found by saving the test menu with that one option changed and diffing
// the game's memory after the next start.
constexpr uint32_t kSettingsCopied = At(0x01FCDC);

// --- for hooks ---

inline void SetLow16(uint32_t &reg, uint32_t value) {
    reg = (reg & 0xFFFF0000) | (value & 0xFFFF);
}

} // namespace nwarr

namespace cps2recomp {
// The wide picture (widescreen.cpp): the columns it adds on each side (0: none), how far right the stage is drawn in it,
// how far short of each end the camera stops (64, or 0: played as on the board), and a spare's frame drawn as wide as
// the port's (and the port's machine back)
int WideSide();
int WideStage();
int WideMargin();
void WideEnds(int &left, int &right);
void WideForSpare(Machine &machine);
void WideForPort(Machine &machine);
} // namespace cps2recomp
