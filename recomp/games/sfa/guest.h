// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha (sfa) as a guest: its stages behind another game's fighters, each from its own stage code alone
// (fight/guest_stages.h). Its facts, found for its port (src/sfa.h, src/widescreen.cpp) and with the call log on its own
// code: A5 = FF8000; its sound ring at FF263E (A5-59C2), the place of the next at FF363E; the stage a word at FFAE00 (its number doubled); the camera at FF8290 and FF8294, asking where the
// players are at 01A9A8 (their outer edges, D0 and D1) and 01AA2A (their heights); its stage's objects in colour sets
// 18 and 19; the first scroll screen (8-pixel tiles) holds the panel's bars.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace sfa_guest {

// Its stages, each a character's, in its order (src/sfa.h)
inline constexpr std::array<const char *, 13> kStageNames{
    "Alpha: Ryu's (the shopping street)", "Alpha: Ken's (the night market)",    "Alpha: Akuma's (the Great Wall at dusk)",
    "Alpha: Charlie's (the crowd)",       "Alpha: Chun-Li's (the Great Wall)",  "Alpha: Adon's (the reclining Buddha)",
    "Alpha: Sodom's (the rail yard)",     "Alpha: Guy's (the street at dusk)",  "Alpha: Birdie's (the ruins at night)",
    "Alpha: Rose's (the ruins)",          "Alpha: M. Bison's (the rail yard at dusk)", "Alpha: Sagat's (the Buddha)",
    "Alpha: Dan's (the Buddha at sunset)"};

// A step of its camera (D2, a word) no more than another game's camera goes in a frame, then on to `next` (its own most
// is 5 across and 1 up and down: 01A5B8, 01A5D6, 01A60C, 01A62A)
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
    d.id = "sfa";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF8170;
    d.playersIn = 0xFF818D;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write16(0xFFAE00, static_cast<uint16_t>(stage * 2)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return set != 0x18 && set != 0x19;
    };
    d.layers = 0xF & ~2u; // (not the first scroll screen: the panel's bars)
    d.cameraLeft = 256;
    d.cameraRight = 640;
    d.cameraAtX = 0xFF8290;
    d.cameraAtY = 0xFF8294;
    d.camera = Guest::Camera::Extents;
    d.across = 0x01A9A8;
    d.up = 0x01AA2A;
    d.middle = 0xC0;
    d.halfApart = 0x60;
    d.above = 0x60;
    // Its stage alone. Its start-up's and attract mode's loop (001996: the modes called at 00199E) is kept, and goes on
    // at 0019A2. First, as a match does before its
    // character choice (0043A0), its memory of objects set out again (016170: the three pools' free lists, which attract
    // mode leaves used up, and the scroll screens' records cleared). A stage is built before the versus screen by a task that wipes the picture over (009CC6 with D1 0x0C,
    // 0099B4 a step a frame): the scroll screens (01A37C), 01A754, 011302, 0112D4, 019A6A, 019AE6, its colours (011324,
    // 011368, 0113A0); then, as the versus screen's first step (00449E), its objects (01BC9A: a table by stage, its
    // cats and its people); a round's start (004874) sets the words the frame interrupt gives the layer control. Each frame,
    // as a fight's (004904 on): the three pools of objects (01BBD6, 01BA48, 01BC08), the scroll screens and the camera (01A376), the sprite list
    // (011B3A).
    d.mainLoop = 0x00199E;
    d.loopGoesOn = 0x0019A2;
    d.build = {0x016170, 0, 0x01A37C, 0x01A754, 0x011302, 0x0112D4, 0x019A6A, 0x019AE6, 0x011324, 0x011368, 0x0113A0, 0x01BC9A};
    // (first the three scroll screens' records, FF8200, FF8280, FF8300, put to work at their first step, +04, which
    // builds each: attract mode left them at its own stage's)
    d.buildStep = [](cps2recomp::M68K &c, int) {
        for (const uint32_t record : {0xFF8200u, 0xFF8280u, 0xFF8300u}) {
            c.bus.Write8(record, 1);
            c.bus.Write8(record + 4, 0);
        }
    };
    d.afterBuild = [](cps2recomp::M68K &c) {
        c.bus.Write16(0xFF803A, 0x12CE);
        c.bus.Write16(0xFF804A, 0x2354);
    };
    d.frame = {0x01BBD6, 0x01BA48, 0x01BC08, 0x01A376, 0x011B3A};
    d.placing = 0x01A376;
    // (before the objects each frame: the fight's frame counter, 1B4(A5), a step as at 004900; and two players standing
    // either side of the picture's middle, their records' places, +10 across and +14 up: its cats walk about by where
    // they are, and with none, to the stage's left end and away)
    d.hooks = {{0x01A5B8, [](cps2recomp::M68K &c) { return Most(c, 0x01A5C0); }},
               {0x01A5D6, [](cps2recomp::M68K &c) { return Most(c, 0x01A5DE); }},
               {0x01A60C, [](cps2recomp::M68K &c) { return Most(c, 0x01A614); }},
               {0x01A62A, [](cps2recomp::M68K &c) { return Most(c, 0x01A632); }},
               {0x01BBD6, [](cps2recomp::M68K &c) {
                    c.bus.Write8(0xFF81B4, static_cast<uint8_t>(c.bus.Read8(0xFF81B4) + 1));
                    const uint16_t camera = c.bus.Read16(0xFF8290);
                    c.bus.Write16(0xFF8410, static_cast<uint16_t>(camera + 0xC0 - 0x60));
                    c.bus.Write16(0xFF8810, static_cast<uint16_t>(camera + 0xC0 + 0x60));
                    c.bus.Write16(0xFF8414, 0x28);
                    c.bus.Write16(0xFF8814, 0x28);
                    return false;
                }}};
    // (its object list, 011B3A: its draw lists in turn, the fighters' at 011B7A (FF8400 and FF8800); the stage's pieces
    // put in after them are in front of them)
    d.fightersDrawn = {0x011B7E};
    d.soundQueue = 0xFF263E;
    d.soundQueueEnd = 0xFF363E;
    d.soundQueueMask = 0xFF0;
    d.stageTunes = {0x01, 0x02, 0x0B, 0x09, 0x03, 0x06, 0x05, 0x08, 0x07, 0x0A, 0x0C, 0x04, 0x32};
    return d;
}

} // namespace sfa_guest
