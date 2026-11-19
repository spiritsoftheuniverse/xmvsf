// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo (ssf2xjr1) as a guest: its stages behind another game's fighters, each from its own
// stage code alone (fight/guest_stages.h). Its facts, found for its port (src/ssf2t.h, src/widescreen.cpp) and with the
// call log on its own code: A5 = FF8000; credits FF82E9; who is in FF82F0; the stage a word at FF8C4E (its number, a
// character's); the camera a record at FF8ECE (its place across +06, up +0A), stepped across at 0CC4E4 and up at
// 0CC630 (the step taken +30 and +32, which way +24 and +25); objects in colour sets 0B and 1A-1F are the stage's.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace ssf2t_guest {

inline constexpr std::array<const char *, 16> kStageNames{
    "Super Turbo: Ryu's (the castle)",
    "Super Turbo: E. Honda's (the bathhouse)",
    "Super Turbo: Blanka's (the river)",
    "Super Turbo: Guile's (the air base)",
    "Super Turbo: Ken's (the harbour)",
    "Super Turbo: Chun-Li's (the market)",
    "Super Turbo: Zangief's (the factory)",
    "Super Turbo: Dhalsim's (the palace)",
    "Super Turbo: M. Bison's (the temple)",
    "Super Turbo: Sagat's (the Buddha)",
    "Super Turbo: Balrog's (Las Vegas)",
    "Super Turbo: Vega's (the cage)",
    "Super Turbo: Cammy's (the castle ruins)",
    "Super Turbo: T. Hawk's (Mexico)",
    "Super Turbo: Fei Long's (the garden)",
    "Super Turbo: Dee Jay's (the beach)",
};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "ssf2t";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF82E9;
    d.playersIn = 0xFF82F0;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write16(0xFF8C4E, static_cast<uint16_t>(stage)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return !(set == 0x0B || set >= 0x1A);
    };
    d.cameraLeft = 0x100;
    d.cameraRight = 0x280;
    d.cameraAtX = 0xFF8ED4;
    d.cameraAtY = 0xFF8ED8;
    d.camera = Guest::Camera::Steps;
    d.across = 0x0CC4E4;
    d.up = 0x0CC630;
    d.placeX = 0x06;
    d.placeY = 0x0A;
    d.stepX = 0x30;
    d.stepY = 0x32;
    d.wayX = 0x24;
    d.wayY = 0x25;
    d.wayRight = 0x04;
    d.wayLeft = 0x0C;
    d.wayUp = 0x00;
    d.wayDown = 0x08;
    // Its stage alone. Its start-up's and attract mode's loop (008FA0: the modes called at 008FA8) is kept, and goes on
    // at 008FAC. A stage is built as the match's steps before the round build it (00D758 on): 003440, 003AAE; its
    // colours (00258C, 0026FC, 002886); 068E96; 0C84AC; then the stage's frame, 0CC3EE, whose records' first steps build
    // its scroll screens. Each frame 0CC3EE (camera, scroll screens), 0C8E1C (the stage's pieces), 0BE518.
    d.mainLoop = 0x008FA8;
    d.loopGoesOn = 0x008FAC;
    d.build = {0x003440, 0x003AAE, 0x00258C, 0x0026FC, 0x002886, 0x068E96, 0x0C84AC, 0x0CC3EE};
    // (the scroll screens switched on: bits 1-3 of the word the frame interrupt gives the video's layer control,
    // 4E(A5), as 000F54 does when a round's picture fades in; and the fade itself, at once: its colours, written dark,
    // a step brighter each time 00120A is called with D2 0x1000, fifteen times from nothing to full)
    d.afterBuild = [](cps2recomp::M68K &c) {
        c.bus.Write16(0xFF804E, static_cast<uint16_t>(c.bus.Read16(0xFF804E) | 0x0E));
        for (int step = 0; step < 15; step++) {
            c.d[2] = 0x1000;
            cps2recomp::Call(c, 0x00120A);
        }
    };
    // (and the pool of the stage's breakable pieces, A5+1DA2 (16 of 0xC0, run by 07794A): the crates, the barrel, the fish,
    // which 0C84AC puts there from the stage's table; not run, they stood unmade and undrawn)
    d.frame = {0x02822A, 0x07794A, 0x0C8E1C, 0x0CC3EE, 0x0BE518, 0x0C66CA};
    d.placing = 0x0CC3EE;
    d.soundQueue = 0xFF0A7E;
    d.soundQueueEnd = 0xFF0A7A;
    d.stageTunes = {0x02, 0x03, 0x05, 0x07, 0x01, 0x04, 0x06, 0x08,
                                                  0x0C, 0x0B, 0x09, 0x0A, 0x0E, 0x0F, 0x0D, 0x10};
    return d;
}

} // namespace ssf2t_guest
