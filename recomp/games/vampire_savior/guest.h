// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Vampire Savior (vsavj) as a guest: its stages behind another game's fighters (fight/guest_stages.h). Its facts, found
// for its port (src/vsav.h, src/training.cpp, src/machines.cpp, family.h): A5 = FF8000; the game's steps FF8000 (2 a
// game), FF8004 (2 the character choice, 4 the match), FF8008 (4 a round), FF800C (6 the round fought); credits FF8076;
// who is in FF808D; a two-player match's stage chosen at 00AE7C and put at FF8100 (the stage's number doubled) by
// 00AEC4, the routine's end at 00AEC8 (where the stage wanted goes in its place); the round's clock FF8109 (seconds, 99
// at most); the camera asks where the players are at 01E8F4 (their outer edges, D0 and D1) and 01E962 (their heights,
// D0 the lower, D1 the higher); objects in colour sets below 22, and the players' shadows, are not the stage's.
#pragma once

#include "guest_stages.h"

#include <array>

namespace vampire_savior_guest {

// Its stages, in the order of their numbers
inline constexpr std::array<const char *, 12> kStageNames{
    "Vampire Savior: Feast of the Damned", "Vampire Savior: Concrete Cave",   "Vampire Savior: Tower of Arrogance",
    "Vampire Savior: Red Thirst",          "Vampire Savior: Deserted Chateau", "Vampire Savior: Abaraya",
    "Vampire Savior: Vanity Paradise",     "Vampire Savior: War Agony",       "Vampire Savior: Forever Torment",
    "Vampire Savior: Green Scream",        "Vampire Savior: Iron Horse, Iron Terror", "Vampire Savior: Fetus of God"};

// The players' shadows: in colour set 22, the stage's first, by their tiles (family.h's kShadowTiles)
inline bool Shadow(uint32_t tile, uint16_t attributes) {
    return (attributes & 0x1F) == 22 && tile >= 0xF8E0 && tile < 0xF920;
}

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "vampire_savior";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.choosing = {{0xFF8000, 2}, {0xFF8004, 2}};
    d.match = {{0xFF8000, 2}, {0xFF8004, 4}};
    d.fought = {{0xFF8000, 2}, {0xFF8004, 4}, {0xFF8008, 4}, {0xFF800C, 6}};
    d.credits = 0xFF8076;
    d.playersIn = 0xFF808D;
    // (at the end of the routine that chose it: its choice, a word at FF8100, made the one wanted)
    d.stageAt = 0x00AEC8;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write16(0xFF8100, static_cast<uint16_t>(stage * 2)); };
    d.clock = 0xFF8109;
    d.clockFull = 99;
    d.notTheStage = [](uint32_t tile, uint16_t attributes) { return (attributes & 0x1F) < 22 || Shadow(tile, attributes); };
    // Its camera's ends: 256 and 640, the players from 280 to 1000, the floor 0x28 up (as Alpha 2's)
    d.cameraLeft = 256;
    d.cameraRight = 640;
    d.cameraAtX = 0xFF8290;
    d.cameraAtY = 0xFF8294;
    d.camera = Guest::Camera::Extents;
    d.across = 0x01E8F4;
    d.up = 0x01E962;
    d.middle = 0xC0;
    d.halfApart = 0x60;
    d.above = 0x60;
    // Its stage alone: its main loop's top 004E82 (the modes, by FF8000) and its wait for the frame 004E8E; a stage
    // built as a round's first step builds it (005A6E on: the A5 words, the stage's plan, colours and scroll screens'
    // records, 17144 putting them to work, then 4D60, 1C140, 2207E and the first placing, 1DA3C), the scroll screens
    // switched on (005B56: bits 1-3 of the video control's word, FF8032); each frame the scroll screens and the camera
    // (1DA3C), the stage's pieces (07D240, 084738) and the object list (01ABC8); its tunes by stage (family.h)
    d.mainLoop = 0x004E82;
    d.loopGoesOn = 0x004E8E;
    d.build = {0x016C4E, 0x00A4BA, 0x01C256, 0x01C2B0, 0x01C336, 0, 0x017144, 0x004D60, 0x01C140, 0x02207E, 0x01DA3C};
    // (005AE4: the three scroll screens' records put to work, the camera's among them; 005AF6: the round's words)
    d.buildStep = [](cps2recomp::M68K &c, int) {
        c.bus.Write8(0xFF8200, 1);
        c.bus.Write8(0xFF8280, 1);
        c.bus.Write8(0xFF8300, 1);
        c.bus.Write8(0xFF8102, 3);
        c.bus.Write8(0xFF8103, 0);
    };
    d.afterBuild = [](cps2recomp::M68K &c) {
        c.bus.Write16(0xFF8032, static_cast<uint16_t>(c.bus.Read16(0xFF8032) | 0x0E));
    };
    d.frame = {0x01DA3C, 0x07D240, 0x084738, 0x01ABC8};
    d.placing = 0x01DA3C;
    // (a round's own start: the three scroll screens' records back to their first step, +04, where the camera is put at
    // the round's place and each is painted there)
    d.roundStart = [](cps2recomp::M68K &c) {
        c.bus.Write8(0xFF8204, 0);
        c.bus.Write8(0xFF8284, 0);
        c.bus.Write8(0xFF8304, 0);
    };
    // (the camera's most in a frame, +5C across and +5E up, set each frame at 01DA82, 5 and 1 in a match: as far as
    // another game's camera goes, within a strip of the 8-pixel scroll screen, which is painted as it goes, 16)
    d.hooks = {{0x01DA9A, [](cps2recomp::M68K &c) {
                    c.bus.Write16(0xFF82DC, 16);
                    c.bus.Write16(0xFF82DE, 16);
                    return false;
                }},
               // (the train, Iron Horse, Iron Terror: its camera's own script, +05 of its record, is the train's arrival
               // at a round's start, the camera put off by +50 and +54 and brought back, 01E098 on. A stage taken into
               // another game's fight is one under way: past it, step 6, nothing off, the round begun, 103(A5); each time
               // its record starts again, a round's start)
               {0x01E098, [](cps2recomp::M68K &c) {
                    const uint32_t camera = c.a[6] & 0xFFFFFF;
                    c.bus.Write8(camera + 0x05, 6);
                    for (uint32_t at = 0x50; at < 0x58; at += 2) {
                        c.bus.Write16(camera + at, 0);
                    }
                    c.bus.Write8(0xFF8103, 1);
                    return false;
                }}};
    // (its object list, 01ABC8: its draw lists in turn, the fighters' at 01AC20 (FF8400 and FF8800); the stage's pieces
    // put in after them, Fetus of God's bulbs, are in front of them)
    // (the fight's frame counter, B4(A5), a step a frame as at 009408: the stage's pieces go by it)
    d.frameCounter = 0xFF80B4;
    d.fightersDrawn = {0x01AC24};
    d.soundQueue = 0xFF0E0E;
    d.soundQueueEnd = 0xFF1E0E;
    d.stageTunes = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};
    return d;
}

} // namespace vampire_savior_guest
