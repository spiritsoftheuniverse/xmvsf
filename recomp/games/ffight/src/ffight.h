// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Final Fight: the game's addresses in 68000 RAM and ROM, as the translator's listing (work/ffight/program.lst) and
// memory watches found them.
#pragma once

#include <cstdint>

namespace cps2recomp::ffight {

// Game state (A5 = 0xFF8000)
constexpr uint32_t kStep = 0xFF8000;            // word: 0 title/attract, 2 char select, 4 stage setup, 6 play, 8 area clear, A stage clear, C ending, E bonus
constexpr uint32_t kCredits = 0xFF804D;         // byte: low byte of 0x4C(a5) word, 0..9
constexpr uint32_t kFreePlay = 0xFF807E;        // byte: 1 = free play (from DIP C bit 2)
constexpr uint32_t kInitialLives = 0xFF8085;    // byte: starting lives from DIP C bits 0-1
constexpr uint32_t kDemoSoundOff = 0xFF8088;    // byte: 1 = demo sound off (from DIP C bit 5)
constexpr uint32_t kClock = 0xFF80AF;           // byte: stage timer (BCD)
constexpr uint32_t kClockTicks = 0xFF80B0;      // word: frames since last clock tick
constexpr uint32_t kStage = 0xFF80BE;           // byte: 0..5 main stages, 6..7 bonus stages
constexpr uint32_t kArea = 0xFF80BF;            // byte: area within stage
constexpr uint32_t kProgression = 0xFF80C1;     // byte: index into 0054FC progression table
constexpr uint32_t kBonusFlag = 0xFF8122;       // byte: 1 on bonus stages

// Scroll & camera
constexpr uint32_t kScroll1X = 0xFF8022;
constexpr uint32_t kScroll1Y = 0xFF8024;
constexpr uint32_t kScroll2X = 0xFF802A;
constexpr uint32_t kScroll2Y = 0xFF802C;
constexpr uint32_t kScroll3X = 0xFF8032;
constexpr uint32_t kScroll3Y = 0xFF8034;
constexpr uint32_t kCameraX = 0xFF8412;         // word: stage camera X
constexpr uint32_t kCameraY = 0xFF8416;         // word: stage camera Y

// Attract mode & screens
constexpr uint32_t kShow = 0xFF1288;            // word (-0x6D78(a5)): attract loop step 0x00..0x14
constexpr uint32_t kShowSub = 0xFF128A;         // word (-0x6D76(a5)): sub-step within show
constexpr uint32_t kContinueScreen = 0xFFD3A8;  // byte (0x53A8(a5)): 1 on continue / game over
constexpr uint32_t kDemo = 0xFFD6AC;            // byte (0x56AC(a5)): 1 during attract demo

// Players (2 slots, 0xC0 bytes each)
constexpr uint32_t kPlayer = 0xFF8568;
constexpr uint32_t kPlayerSize = 0xC0;
constexpr uint32_t kInPlay = 0x00;              // byte: 1 when active
constexpr uint32_t kObjState = 0x02;            // long: object state step
constexpr uint32_t kX = 0x06;                   // word
constexpr uint32_t kY = 0x0A;                   // word
constexpr uint32_t kZ = 0x0E;                   // word
constexpr uint32_t kCharacter = 0x14;           // byte: 0..2
constexpr uint32_t kHealth = 0x18;              // word: current health
constexpr uint32_t kRedHealth = 0x1A;           // word: trailing red bar health
constexpr uint32_t kMaxHealth = 0x1C;           // word: max health (0x0090)
constexpr uint32_t kAnim = 0x38;                // long: animation/frame table pointer
constexpr uint32_t kLives = 0x80;               // byte: remaining lives
constexpr uint32_t kSavedChar = 0x81;           // byte: saved character id
constexpr uint32_t kScore = 0x84;               // long: BCD score

constexpr uint32_t Player(int n) { return kPlayer + static_cast<uint32_t>(n) * kPlayerSize; }

// 68000 code hooks
constexpr uint32_t kGameBegins = 0x004C54;      // right after kStage/kArea/kProgression are cleared
constexpr uint32_t kEnemyPool = 0x0057B4;       // enemy object pool update
constexpr uint32_t kItemPool = 0x0059DC;        // item/weapon object pool update

// Attract show indices (0x170CC jump table, steps of 2)
enum Show : uint16_t {
    kShowBoot = 0x00,
    kShowStory1 = 0x02,
    kShowStory2 = 0x04,
    kShowStory3 = 0x06,
    kShowProfiles = 0x08,
    kShowTitle = 0x0A,
    kShowDemo1 = 0x0C,
    kShowDemo2 = 0x0E,
    kShowDemo3 = 0x10,
    kShowRanking = 0x12,
    kShowEnd = 0x14,
    kShowLoopEnd = 0x16,
};

} // namespace cps2recomp::ffight
