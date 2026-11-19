// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors (nwarr) as a guest: its stages behind another game's fighters, each from its own stage code alone
// (fight/guest_stages.h). It is Darkstalkers' program grown (games/dstlk/guest.h, whose description this is at Night
// Warriors' places: games/dstlk/family.h and the two programs' listings set side by side): A5 = FF8000; credits FF81B2;
// who is in FF81F9; a two-player match's stage set at 006502 from A5+0DB0 (the stage + 1); the clock FF8E09 (BCD);
// the camera an object at FF8F08 (its place at FF8F18 and FF8F1C), stepped across at 0FF6C4 and up at 0FF7EA; objects
// in colour sets below 14 are the panel's, the players' and their shadows' and sparks' (src/nwarr.h). Its stages are
// Darkstalkers' twelve in other colours, and Hsien-Ko's and Donovan's.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace nwarr_guest {

// Its stages, each a character's, in its order (family.h)
inline constexpr std::array<const char *, 14> kStageNames{
    "Night Warriors: Demitri's",  "Night Warriors: Jon Talbain's", "Night Warriors: Victor's",
    "Night Warriors: Lord Raptor's", "Night Warriors: Morrigan's", "Night Warriors: Anakaris's",
    "Night Warriors: Felicia's",  "Night Warriors: Bishamon's",   "Night Warriors: Rikuo's",
    "Night Warriors: Sasquatch's", "Night Warriors: Huitzil's",   "Night Warriors: Pyron's",
    "Night Warriors: Hsien-Ko's", "Night Warriors: Donovan's"};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "nwarr";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.choosing = {{0xFF8000, 2}, {0xFF8004, 0}};
    d.match = {{0xFF8000, 2}, {0xFF8004, 4}};
    d.fought = {{0xFF8000, 2}, {0xFF8004, 4}, {0xFF8008, 0x0A}};
    d.credits = 0xFF81B2;
    d.playersIn = 0xFF81F9;
    d.stageAt = 0x006502;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.ram[0x8DB0] = static_cast<uint8_t>(stage + 1); };
    d.clock = 0xFF8E09;
    d.clockFull = 0x99;
    d.notTheStage = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) < 0x14; };
    d.cameraLeft = 0x100;
    d.cameraRight = 0x280;
    d.cameraAtX = 0xFF8F18;
    d.cameraAtY = 0xFF8F1C;
    d.camera = Guest::Camera::Steps;
    d.across = 0x0FF6C4;
    d.up = 0x0FF7EA;
    // Its stage alone, as Darkstalkers' (games/dstlk/guest.h says why each): a game's own loop (0252AE, carrying on at
    // 0252B2) reached by a credit and Start; the colour set every load of colours takes (A5+0D90) a match's; every
    // object put away (005D48); the stage (006502, 006562, 0046B4); the round's own task (001C1E with 0x804); 0067F4,
    // 008F94, 0083E0, 068268. Each frame: 0E8796, the camera and scroll screens (0FF5BC), 0C2B18, 0CE3D6, 100B28,
    // 0E7D8E, 0E2AD4 and the object list (001C46).
    d.mainLoop = 0x0252AE;
    d.loopGoesOn = 0x0252B2;
    d.beginAfter = 300;
    d.build = {0, 0x005D48, 0x006502, 0x006562, 0x0046B4, 0, 0x0067F4, 0x008F94, 0x0083E0, 0x068268};
    d.buildStep = [](cps2recomp::M68K &c, int step) {
        if (step == 0) {
            c.bus.Write16(0xFF8D90, 0);
            return;
        }
        c.d[0] = 0x804;
        c.d[1] = 1;
        cps2recomp::Call(c, 0x001C1E);
    };
    d.frame = {0x0E8796, 0x0FF5BC, 0x0C2B18, 0x0CE3D6, 0x100B28, 0x0E7D8E, 0x0E2AD4, 0x001C46};
    d.placing = 0x0FF5BC;
    // (its object list, 001C46: its draw lists in turn, the fighters' at 001DE6; the stage's pieces put in after them are
    // in front of them)
    d.fightersDrawn = {0x001DEA};
    d.listRegister = 0;
    d.soundQueue = 0xFF0AA0;
    d.soundQueueEnd = 0xFF8026;
    d.soundQueueMask = 0x3F0;
    d.stageTunes = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x3EC, 0x3ED};
    return d;
}

} // namespace nwarr_guest
