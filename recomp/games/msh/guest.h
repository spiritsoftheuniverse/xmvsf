// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes (msh) as a guest: its stages behind another game's fighters, each from its own stage code alone
// (fight/guest_stages.h). Its facts, found for its port (src/msh.h) and with the call log on its own code: A5 = FF8000;
// credits FF4C09; who is in FF4881; the stage a byte at FF4893; its camera a record by stage (FF4980 on the mountain
// lake, FF4B00 on the burning field, FF4A00 elsewhere), its place at +0C across and +10 down (the picture's top), its
// ends at +72/74 across and +76/78 down; objects in colour sets 13 and up are the stage's pieces; its sound ring at
// FF4C2C, the place of the next at FF4C26; its stages' tunes 09-14 (08D6D0).
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace msh_guest {

// Its stages, each a character's, in its order (src/msh.h)
inline constexpr std::array<const char *, 12> kStageNames{
    "Marvel Super Heroes: the city street",   "Marvel Super Heroes: the shipwreck",
    "Marvel Super Heroes: the carnival",      "Marvel Super Heroes: Stark Enterprises",
    "Marvel Super Heroes: the mountain lake", "Marvel Super Heroes: the neon city",
    "Marvel Super Heroes: the burning field", "Marvel Super Heroes: the skull canyon",
    "Marvel Super Heroes: the harbour",       "Marvel Super Heroes: the space station",
    "Marvel Super Heroes: the Infinity Gem chamber", "Marvel Super Heroes: Thanos's throne"};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "msh";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF4C09;
    d.playersIn = 0xFF4881;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write8(0xFF4893, static_cast<uint8_t>(stage)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) < 0x13; };
    // Its camera: put where wanted in its record at the two places its routines move it (082440 across, 0826AA up and
    // down: after their checks, where they go by how the camera moves), between the record's own ends (the game's
    // camera's place, from its ends' 0 to 1024 here, put as far between them)
    d.camera = Guest::Camera::Place;
    d.cameraLeft = 0;
    d.cameraRight = 1024;
    d.across = 0x082440;
    d.up = 0x0826AA;
    d.placeX = 0x0C;
    d.placeY = 0x10;
    d.endLeft = 0x72;
    d.endRight = 0x74;
    d.endTop = 0x76;
    d.endBottom = 0x78;
    d.placeDown = true;
    d.cameraRecord = [](const uint8_t *ram) -> uint32_t {
        const uint8_t stage = ram[0x4893];
        return stage == 4 ? 0xFF4980 : stage == 6 ? 0xFF4B00 : 0xFF4A00;
    };
    // Its stage alone. Its game is a task, and its loop (007EB0: the frame's wait, then the game's step at 007F06) is
    // there once a game is begun: its start-up is given a credit and Start (beginAfter). A match's first step builds
    // its stage (007F9C on): the lists of objects emptied (0035BE: the character choice's would go on running), the
    // object pools set out (003844, 003656), the round's words cleared (00C7E4), its colours
    // (007A1A), the stage (082192), 004A3A, and the colours brought up (007890). Each frame, as a fight's (01AA2E): the
    // pools (01AEC8, 01AF96), the stage's scroll screens and camera (082346), its pieces' list (01B0A4: its people, its signs and posts), the
    // object list (01B34E), and the frame's end (019B02: the lists of objects to draw, which each piece puts itself on,
    // made empty again; left full they grow down over the lists of objects)
    d.mainLoop = 0x007F06;
    d.loopGoesOn = 0x007F08;
    d.beginAfter = 600;
    d.build = {0x0035BE, 0x003844, 0x003656, 0x00C7E4, 0x007A1A, 0x082192, 0x004A3A, 0x007890};
    // (Stark Enterprises' camera opens a round of its own: from the sky it slides down and across (083EE8's steps under
    // +06, moved on by the pieces it makes), making its machinery as it comes (084048), and only at its last step, 0C,
    // follows the players. Its frames played through at once, the stage is as a round has it once begun)
    d.afterBuild = [](cps2recomp::M68K &c) {
        if (c.bus.Read8(0xFF4893) != 3) {
            return;
        }
        // (... and its pieces after it, the craft come in and the wall behind turned green: some 700 frames more)
        int after = 700;
        for (int n = 0; n < 2000 && after > 0; n++) {
            for (const uint32_t at : {0x01AEC8u, 0x01AF96u, 0x082346u, 0x01B0A4u, 0x019B02u}) {
                cps2recomp::Call(c, at);
            }
            if (c.bus.Read16(0xFF4A06) >= 0x0C) {
                after--;
            }
        }
    };
    d.frame = {0x01AEC8, 0x01AF96, 0x082346, 0x01B0A4, 0x01B34E, 0x019B02};
    d.placing = 0x082346;
    // (its object list, made at the frame's end: the pieces on its four draw lists by layer, and those sorted by depth,
    // the fighters among them, done at 019D7A; what is put in after them is in front of them)
    d.fightersDrawn = {0x019D7A};
    d.listRegister = 4;
    d.soundQueue = 0xFF4C2C;
    d.soundQueueEnd = 0xFF4C26;
    d.soundQueueMask = 0x3F0;
    d.stageTunes = {0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};
    return d;
}

} // namespace msh_guest
