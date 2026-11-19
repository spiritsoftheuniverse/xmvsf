// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 2 (sfa2u) as a guest: its stages behind another game's fighters (fight/guest_stages.h). Its
// facts, found for its port (src/sfa2.h, src/modes.cpp, src/training.cpp, src/fight.cpp, src/machines.cpp,
// src/widescreen.cpp): A5 = FF8000; the game's steps FF8000 (2 a game), FF8004 (2 the character choice, 4 the match),
// FF8008 (4 a round), FF800C (2 the round fought); credits FF8076; who is in FF80A8; the match's stage chosen at 00797C
// (D0's low byte, from one of the characters); the round's clock FF8109 (seconds, 99 at most); the camera asks where
// the players are at 01BFE0 (their outer edges, D0 and D1: far apart, it puts their middle in the picture's, 0xC0 on
// from its place) and 01C1D0 (their heights: D1, the higher, followed when more than 0x60 above its place); objects in
// colour sets below 27, and the panel's by their tiles, are not the stage's.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace sfa2_guest {

// Its stages, each a character's, in its order
inline constexpr std::array<const char *, 18> kStageNames{
    "Alpha 2: Ryu's (moonlit snow)",      "Alpha 2: Ken's (the yacht)",          "Alpha 2: Akuma's (the cave)",
    "Alpha 2: Charlie's (the hangar)",    "Alpha 2: Chun-Li's (the market)",     "Alpha 2: Adon's (the temple)",
    "Alpha 2: Sodom's (the truck)",       "Alpha 2: Guy's (the city street)",    "Alpha 2: Birdie's (the back alley)",
    "Alpha 2: Rose's (the harbour)",      "Alpha 2: M. Bison's (the airfield)",  "Alpha 2: Sagat's (the reclining Buddha)",
    "Alpha 2: Dan's (the cafe)",          "Alpha 2: Sakura's (the shopping street)", "Alpha 2: Rolento's (the mural)",
    "Alpha 2: Dhalsim's (the elephants)", "Alpha 2: Zangief's (the lanterns)",   "Alpha 2: Gen's (the ruins)"};

// The panel's objects (the bars, the names, the clock and the faces, the gauges): by their tiles, in whatever colours
// (0A400-0A6FF only outside the players' colour sets, 11-25)
inline bool ThePanel(uint32_t tile, uint16_t attributes) {
    const unsigned set = attributes & 0x1F;
    return (tile >= 0x1A400 && tile < 0x1AB00) || (tile >= 0x26A00 && tile < 0x26B00) ||
           (tile >= 0x0A400 && tile < 0x0A700 && (set < 11 || set > 25));
}

// A step of its camera (D2, a word) no more than another game's camera goes in a frame, then on to `next`
inline bool Most(cps2recomp::M68K &c, uint32_t next) {
    constexpr uint16_t kMost = 16;
    if ((c.d[2] & 0xFFFF) > kMost) {
        c.d[2] = (c.d[2] & 0xFFFF0000) | kMost;
    }
    c.pc = next;
    return true;
}

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "sfa2";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.choosing = {{0xFF8000, 2}, {0xFF8004, 2}};
    d.match = {{0xFF8000, 2}, {0xFF8004, 4}};
    d.fought = {{0xFF8000, 2}, {0xFF8004, 4}, {0xFF8008, 4}, {0xFF800C, 2}};
    d.credits = 0xFF8076;
    d.playersIn = 0xFF80A8;
    d.stageAt = 0x00797C;
    // (D0 where the match's stage is chosen; and the stage's word itself, FF8100, the stage's number doubled, for a stage
    // built alone)
    d.setStage = [](cps2recomp::M68K &c, int stage) {
        c.d[0] = (c.d[0] & 0xFFFFFF00) | static_cast<uint8_t>(stage);
        c.bus.Write16(0xFF8100, static_cast<uint16_t>(stage * 2));
    };
    d.clock = 0xFF8109;
    d.clockFull = 99;
    d.notTheStage = [](uint32_t tile, uint16_t attributes) { return (attributes & 0x1F) < 27 || ThePanel(tile, attributes); };
    // Its camera's ends: 256 and 640, the players from 280 to 1000, the floor 0x28 up (as Vampire Savior's)
    d.cameraLeft = 256;
    d.cameraRight = 640;
    d.cameraAtX = 0xFF8290;
    d.cameraAtY = 0xFF8294;
    d.camera = Guest::Camera::Extents;
    d.across = 0x01BFE0;
    d.up = 0x01C1D0;
    d.middle = 0xC0;
    d.halfApart = 0x60;
    d.above = 0x60;
    // Its stage alone. Its start-up's and attract mode's loop (0017F6: the modes by FF8000, called at 0017FE) is kept,
    // and goes on at 001802 (the frame's end, its wait for the frame). A stage is built as a match's steps before the
    // versus screen build it, a step a frame there (by A5+10, 0051B8): the scroll screens switched on (0051DA) and
    // 01A88C; 01B1DE; 01C31E, 01B7FC; the stage's colours (012608 with A0 29C180, A1 90C180, D7 0; 012510, 012534,
    // 012570, 0125A8, 0125D8); 0188B8; then 019774 and 01EB00 (the stage's objects); then what a round's start does in
    // its own lines (002446: the scroll screens' records put to work). Each frame what a fight's does of
    // the stage's: the objects (01E308, 01E582, 01E640), the scroll screens and camera (01A7F6), the object list
    // (01E6AE) and the colours given over (0127A4).
    d.mainLoop = 0x0017FE;
    d.loopGoesOn = 0x001802;
    d.build = {0, 0x01A88C, 0x01B1DE, 0x01C31E, 0x01B7FC, 0, 0x012510, 0x012534, 0x012570, 0x0125A8, 0x0125D8,
               0x0188B8, 0x019774, 0x01EB00, 0};
    d.buildStep = [](cps2recomp::M68K &c, int step) {
        if (step == 0) {
            c.bus.Write16(0xFF8032, static_cast<uint16_t>(c.bus.Read16(0xFF8032) | 0x0E));
            return;
        }
        if (step == 2) {
            // (a round's own lines, 002446: the three scroll screens' records put to work, the camera's among them)
            c.bus.Write8(0xFF8200, 1);
            c.bus.Write8(0xFF8280, 1);
            c.bus.Write8(0xFF8300, 1);
            c.bus.Write16(0xFF8102, 1);
            return;
        }
        c.a[0] = 0x29C180;
        c.a[1] = 0x90C180;
        c.d[7] = 0;
        cps2recomp::Call(c, 0x012608);
    };
    d.frame = {0x01E308, 0x01E582, 0x01E640, 0x01A7F6, 0x01E6AE, 0x0127A4};
    d.placing = 0x01A7F6;
    // (its camera goes 5 pixels a frame at most across and 1 up, 01B6A0 on, as constants: for a stage alone, as far as
    // another game's camera goes, within a strip of a scroll screen painted as it goes, 16; each test of D2 against the
    // most done here, and on to where the step is added)
    d.hooks = {
        {0x01B6A0, [](cps2recomp::M68K &c) { return Most(c, 0x01B6A8); }},
        {0x01B6BE, [](cps2recomp::M68K &c) { return Most(c, 0x01B6C6); }},
        {0x01B6F4, [](cps2recomp::M68K &c) { return Most(c, 0x01B6FC); }},
        {0x01B712, [](cps2recomp::M68K &c) { return Most(c, 0x01B71A); }},
    };
    // (its object list, 0127A4: its draw lists in turn, the fighters' at 0127F2 (as many as there are, 138(A5)); the
    // stage's pieces put in after them are in front of them)
    // (the fight's frame counter, B4(A5), a step a frame as at 006554: the stage's pieces go by it)
    d.frameCounter = 0xFF80B4;
    d.fightersDrawn = {0x0127F6};
    d.soundQueue = 0xFFCF60;
    d.soundQueueEnd = 0xFFDF60;
    d.stageTunes = {0x01, 0x02, 0x0B, 0x09, 0x03, 0x06, 0x05, 0x08, 0x07, 0x0A, 0x0C, 0x04, 0x0D, 0x12, 0x11, 0x0E, 0x0F, 0x10};
    return d;
}

} // namespace sfa2_guest
