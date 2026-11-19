// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3 (sfa3u) as a guest: its stages behind another game's fighters, each from its own stage code
// alone (fight/guest_stages.h). Its facts, found for its port (src/sfa3.h, src/modes.cpp, src/widescreen.cpp) and with
// the call log on its own code: A5 = FF8000; credits FF8076; who is in FF80A8; the round's clock FF8109; the stage a word
// at FF8100, the stage's number doubled (from FF8174 as a round is set up); the camera asks where the players are at
// 018654 (their outer edges, D0 and D1) and 01877C (their heights); objects in colour sets 10-13 are the stage's.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace sfa3_guest {

// Its stages, in its port's order (src/sfa3.h), and each one's number in the game
inline constexpr std::array<const char *, 26> kStageNames{
    "Alpha 3: Ryu's (the castle at sunset)",
    "Alpha 3: Ken's (the resort)",
    "Alpha 3: Akuma's (the cave)",
    "Alpha 3: Charlie's (the hangar)",
    "Alpha 3: Chun-Li's (the market)",
    "Alpha 3: Adon's (the temple ruins)",
    "Alpha 3: Sodom's (the street at night)",
    "Alpha 3: Guy's (the arches)",
    "Alpha 3: Birdie's (the scrapyard)",
    "Alpha 3: Rose's (the stairway)",
    "Alpha 3: The final stage (the skulls)",
    "Alpha 3: Sagat's (the reclining Buddha)",
    "Alpha 3: Dan's (the playground)",
    "Alpha 3: Sakura's (the shopping street)",
    "Alpha 3: Rolento's (the tank depot)",
    "Alpha 3: Dhalsim's (the temple)",
    "Alpha 3: Zangief's (the furnace)",
    "Alpha 3: Gen's (the harbour)",
    "Alpha 3: M. Bison's (the rainy garden)",
    "Alpha 3: Cammy's (the windmills)",
    "Alpha 3: E. Honda's (the bath house)",
    "Alpha 3: Blanka's (the jungle river)",
    "Alpha 3: R. Mika's (the wrestling show)",
    "Alpha 3: Cody's (the jail)",
    "Alpha 3: Vega's (the cage)",
    "Alpha 3: Karin's (the shopping street at dusk)",
};
inline constexpr std::array<uint8_t, 26> kStageNumbers{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                                   0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x15, 0x16, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "sfa3";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF8076;
    d.playersIn = 0xFF80A8;
    d.setStage = [](cps2recomp::M68K &c, int stage) {
        const uint16_t word = static_cast<uint16_t>(kStageNumbers[static_cast<size_t>(stage)] * 2);
        c.bus.Write16(0xFF8174, word);
        c.bus.Write16(0xFF8100, word);
    };
    d.clock = 0xFF8109;
    d.clockFull = 99;
    d.notTheStage = [](uint32_t, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return set < 0x10 || set > 0x13;
    };
    d.cameraLeft = 256;
    d.cameraRight = 640;
    d.cameraAtX = 0xFF8290;
    d.cameraAtY = 0xFF8294;
    d.camera = Guest::Camera::Extents;
    d.across = 0x018654;
    d.up = 0x01877C;
    d.middle = 0xC0;
    d.halfApart = 0x60;
    d.above = 0x60;
    // Its stage alone. Its start-up's and attract mode's loop (003ED0: the modes called at 003ED8) is kept, and goes on
    // at 003EDC. A stage is built as the match's steps before the round build it (00945C on): 01BDD0, 01FF52, 01A5E8,
    // the first placing 017462, 01FAE2, 00322E, 03330E, 00B008; then what the round's start does in its own lines
    // (009B26: the scroll screens' records put to work). Each frame what a fight's does of the stage's (009508 on).
    d.mainLoop = 0x003ED8;
    d.loopGoesOn = 0x003EDC;
    d.build = {0x01BDD0, 0x01FF52, 0x01A5E8, 0x017462, 0x01FAE2, 0x00322E, 0x03330E, 0x00B008, 0};
    d.buildStep = [](cps2recomp::M68K &c, int) {
        c.bus.Write16(0xFF8032, static_cast<uint16_t>(c.bus.Read16(0xFF8032) | 0x0E));
        c.bus.Write16(0xFF8048, 0x9280);
        c.bus.Write8(0xFF8200, 1);
        c.bus.Write8(0xFF8280, 1);
        c.bus.Write8(0xFF8300, 1);
    };
    d.frame = {0x02040A, 0x033696, 0x00B718, 0x00B220, 0x00B34E, 0x017462, 0x033522, 0x0336D6, 0x0335E0, 0x020C4A};
    d.placing = 0x017462;
    // (its object list, 020C4A, in two forms by 83(A5): its draw lists in turn, the fighters' at 020CA2 or 020D4E; the
    // stage's pieces put in after them are in front of them)
    // (the fight's frame counter, B4(A5), a step a frame as at 00953A: the stage's pieces go by it)
    d.frameCounter = 0xFF80B4;
    d.fightersDrawn = {0x020CA6, 0x020D52};
    d.soundQueue = 0xFFDE80;
    d.soundQueueEnd = 0xFFEE80;
    d.stageTunes = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x20, 0x14, 0x15,
                                             0x16, 0x17, 0x18, 0x19, 0x1A, 0x13, 0x1F, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
    return d;
}

} // namespace sfa3_guest
