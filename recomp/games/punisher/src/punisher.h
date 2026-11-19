// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The Punisher (punisheru, USA 930422): names for what is known of the game's memory, with how each was found
// (PROGRESS.md has more). The game keeps its variables at A5 = FF8000, most of them below it.
#pragma once

#include "game.h"
#include "lift.h"
#include "machine.h"

namespace cps2recomp::punisher {

constexpr uint32_t kVars = 0xFF8000; // A5

// Credits (word, up to 9: a coin adds them at 09BF5A, Start takes them at 09C2B4; found from dumps after a coin)
constexpr uint32_t kCredits = kVars - 0x24EC;
// The game's own free play (byte): Start takes no credit while it is set (09C2A0). The coin set-up (006A4E) sets it
// when the test menu's coin mode (byte) is past 8 (9: FREE PLAY; 8 is "2 coins start, 1 continue")
constexpr uint32_t kFreePlay = kVars - 0x24C4, kCoinMode = kVars - 0x24AF;
constexpr uint8_t kCoinModeFreePlay = 9;
// The coin set-up: works the settings out into the values the game plays by (lives, its difficulty's level...) as
// the board starts and on the screen before a game
constexpr uint32_t kCoinSetUp = 0x006A4E;

// The board's settings: the record the settings memory keeps (A5-7CFA, FF0306; 006BEA reads it as the board starts
// and copies it into the variables below, 006C14), the bytes of the test menu's System Configuration page:
//   +0C coin mode (0-9)        +0D continue (0 off, 1 on; on)   +0E monitor flip
//   +0F demo sound (0 off, 1 on; off)                              +10 sound (QSound or mono)
//   +11 game difficulty (0 extra easy ... 3 normal ... 7 hardest; 3)
//   +12 number of players: lives, 0-3 for 1-4 (1)
//   +13 extend (0 at 800,000, 1 at 1,800,000, 2 at 2,800,000, 3 none; 1)
//   +14 player select (0 off, 1 on; on): the players' screen after Start (09BF90), which shows the Punisher for
//       player 1 and Nick Fury for player 2 (the players are not chosen: 020824 takes a player's animations by its number)
// Found by stepping each option of the page with Right and reading the test menu's copy (A5-7C05 on), then the
// routine that copies it back (006D10).
constexpr uint32_t kSettings = kVars - 0x7CFA;
constexpr uint32_t kSetContinue = kVars - 0x24BA, kSetDemoSound = kVars - 0x24BB, kSetDifficulty = kVars - 0x24AB,
                   kSetLives = kVars - 0x24AC, kSetExtend = kVars - 0x24AD, kSetPlayerSelect = kVars - 0x24A8;
// 006848: the settings are in their variables, and the coin set-up is next (the boot's; a hook)
constexpr uint32_t kSettingsRead = 0x006848;

// A credit's screen or a game is on (byte: 09C05A sets it as soon as a credit is in, before Start; 00AA0A clears it at
// the game over), and the players in the game (bits, set by Start: 09C02C; none but in a game)
constexpr uint32_t kGameOn = kVars - 0x24C0, kPlayersIn = kVars - 0x24C2;

// The game's own task (007A8A) and its step (word, from the table at 007B7C): 0 a game's start, 4 a stage's
// title, 8 a stage played, 0C a while between one part of a stage and the next (found by tracing it through stages), 10 a stage's end (its results). The show's
// demonstrations run it too. It is not put back to 0 at a game over: kGameOn tells a game from none.
constexpr uint32_t kStep = kVars - 0x2532;
constexpr uint16_t kStepTitle = 4, kStepPlay = 8, kStepEnd = 0x10;
// 007B90: a game's start (step 0), before it takes the lives and the difficulty's level
constexpr uint32_t kGameStarts = 0x007B90;
// 007C76: a game's start has set the stage (0 at a game's start; with the game's free play from a switch byte)
constexpr uint32_t kGameBegins = 0x007C76;

// The stage (byte): 0-5 the six stages (the number on each stage's title, less one), 7 the bonus round between
// stages 3 and 4 (breaking barrels). 007E66 takes the next from the table at 007EA2: 0, 1, 2, 7, 3, 4, 5 in play,
// then the ending. Found by starting a game at each.
constexpr uint32_t kStage = kVars - 0x2447;
constexpr int kStages = 6;

// The stage's clock (the TIME on the panel, from 30 at each life): its two digits as words of twice their value
// (A5-6752 tens, A5-6754 units), counted down each time the word at A5-6756 runs out from 0xBF (0AEB3C), and not
// while the byte at A5-1FEB is set (results, a boss's entrance). Found from the digits' drawing (FF18AD, the units
// times two, was the one byte that followed the clock in dumps).
constexpr uint32_t kClockFrames = kVars - 0x6756, kClockUnits = kVars - 0x6754, kClockTens = kVars - 0x6752;
constexpr uint16_t kClockTick = 0xBF;

// The players: two records of 0x100 bytes from FF8E68 (0E68(a5) and 0F68(a5) all through the code)
constexpr int kPlayers = 2;
constexpr uint32_t kPlayer = kVars + 0xE68, kPlayerSize = 0x100;
inline uint32_t Player(int n) {
    return kPlayer + kPlayerSize * static_cast<uint32_t>(n);
}
constexpr uint32_t kInPlay = 0x00;  // byte: 1 while the player is in the game
constexpr uint32_t kNumber = 0x02;  // byte: the player, 0 or 1
constexpr uint32_t kHealth = 0x36;  // word: health (found standing still while hit: 0x88 down to 0, and 0x88 again
                                    // with a life: 0173F4); +38 follows it down for the bar
constexpr uint16_t kHealthFull = 0x88;
// Lives left after this one (byte a player, A5-245C on: 01FE16 takes one; 007BDE sets them as a game begins)
constexpr uint32_t kLives = kVars - 0x245C;
// The players' scores (BCD longs, A5-2456 player 1's, A5-2452 player 2's: found from dumps while it went up)
constexpr uint32_t kScore = kVars - 0x2456;

} // namespace cps2recomp::punisher
