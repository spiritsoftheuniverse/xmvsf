// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots (cybots) as a guest: its stages behind another game's fighters, each from its own stage code alone
// (fight/guest_stages.h). Its facts from its port (src/cybots.h, PROGRESS.md): A5 = FF8000; credits FF80B6; who is in
// FF80E5; the stage a byte at FFEA2D (copied as a word from FFEA2C to FFEA1A as a round is made, 001FCA); its camera an
// object that A5+6B1C points to (the stage's own: FFEC1A, FFECDA, FFED9A...), its place at +1A across and +1E up, the
// step it took at +50 and +52 and which way at +44 (4 right, 0 left) and +45 (0C up, 08 down), which the far planes'
// objects follow (01803E on, 018280 on); its ends across 0 and 0x280; objects in colour sets 18-1A are the stage's
// pieces; its sound ring at FF9438, the place of the next at FF9434.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace cybots_guest {

// Its stages, by the order of its port's lists (src/cybots.h; kStageNumbers gives the stage byte of each)
inline constexpr std::array<const char *, 14> kStageNames{
    "Cyberbots: the city",           "Cyberbots: the desert",        "Cyberbots: the space station",
    "Cyberbots: Town 3",             "Cyberbots: the forest",        "Cyberbots: the fortress",
    "Cyberbots: the ice cave",       "Cyberbots: the jungle base",   "Cyberbots: the volcano",
    "Cyberbots: space",              "Cyberbots: the desert at sunset", "Cyberbots: the ice cave at night",
    "Cyberbots: the burning ruins",  "Cyberbots: the burning city"};
inline constexpr std::array<uint8_t, 14> kStageNumbers{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                                      0x07, 0x08, 0x09, 0x0C, 0x0D, 0x0E, 0x0F};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "cybots";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF80B6;
    d.playersIn = 0xFF80E5;
    // (the byte the round's start copies, and the word its stage's code goes by, both)
    d.setStage = [](cps2recomp::M68K &c, int stage) {
        const uint8_t number = kStageNumbers[static_cast<size_t>(stage) % kStageNumbers.size()];
        c.bus.Write16(0xFFEA2C, number);
        c.bus.Write16(0xFFEA1A, number);
    };
    d.notTheStage = [](uint32_t, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return set < 0x18 || set >= 0x1B;
    };
    // Its camera, stepped here: 017F3A steps it across toward the players, 0181A6 up and down; the far planes follow the
    // steps it took
    d.camera = Guest::Camera::Steps;
    d.cameraLeft = 0;
    d.cameraRight = 0x280;
    d.across = 0x017F3A;
    d.up = 0x0181A6;
    d.placeX = 0x1A;
    d.placeY = 0x1E;
    d.stepX = 0x50;
    d.stepY = 0x52;
    d.wayX = 0x44;
    d.wayY = 0x45;
    d.wayRight = 4;
    d.wayLeft = 0;
    d.wayUp = 0x0C;
    d.wayDown = 0x08;
    d.cameraRecord = [](const uint8_t *ram) -> uint32_t {
        return 0xFF0000u | static_cast<uint32_t>((ram[0xEB1C] << 8) | ram[0xEB1D]);
    };
    // Its stage alone: its main loop is a game's own (00C3A6, begun by Start: a credit and Start once its start-up is
    // over), whose step (00C3EE, the game's modes by FF8000) is passed over to 00C3F2. A round's first step builds its
    // stage (00C642 on): 001B9A (the pools emptied), its stage's flags (A5+6A24 cleared, bit 0 of A5+6B9B set, as
    // 00C664 does), 001FBA (the round's words, the stage word copied), 014586 (the stage's colours), 0142B8 (their
    // brightness), 015610 (the stage's own code: its planes and its camera; ready at its first call) and 04D6A6 (its
    // pieces, by the stage). The players' own (019696 on) are left out. Each frame, as a fight's (00C9C6 on): the stage's
    // own code (015610, which steps the camera) and the pools of objects (04E01E)
    d.mainLoop = 0x00C3EE;
    d.loopGoesOn = 0x00C3F2;
    d.beginAfter = 600;
    d.build = {0x001B9A, 0, 0x001FBA, 0x014586, 0x0142B8, 0x015610, 0x04D6A6};
    d.buildStep = [](cps2recomp::M68K &c, int) {
        c.bus.Write8(0xFFEA24, 0);
        c.bus.Write8(0xFFEB9B, static_cast<uint8_t>(c.bus.Read8(0xFFEB9B) | 1));
    };
    // (the palettes' base as a fought round has it: A5+28 9000, the palettes at 900000 (00C706 sets it as the round is
    // fought; the character choice leaves it at 9040). And the round's fade from black is not run: the palettes the
    // build put out are brought to full brightness, the top four bits of each colour, as the fade leaves them)
    d.afterBuild = [](cps2recomp::M68K &c) {
        c.bus.Write16(0xFF8028, 0x9000);
        for (uint32_t at = 0x900000; at < 0x902000; at += 2) {
            c.bus.Write16(at, static_cast<uint16_t>(c.bus.Read16(at) | 0xF000));
        }
    };
    d.frame = {0x015610, 0x04E01E};
    d.placing = 0x015610;
    d.soundQueue = 0xFF9438;
    d.soundQueueEnd = 0xFF9434;
    d.soundQueueMask = 0xFF0;
    // (each stage's tune, as a two-player fight on it has it: src/music.cpp)
    d.stageTunes = {0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1A, 0x1B, 0x1C, 0x13, 0x13, 0x1A, 0x12};
    return d;
}

} // namespace cybots_guest
