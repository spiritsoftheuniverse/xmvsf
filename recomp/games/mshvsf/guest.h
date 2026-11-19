// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter (mshvsf) as a guest: its stages behind another game's fighters, each from its
// own stage code alone (fight/guest_stages.h). X-Men vs. Street Fighter's engine (games/xmvsf/guest.h, whose description
// this is at this game's places: the two programs' listings set side by side, and src/mshvsf.h): A5 = FF8000; credits
// FF4C75; who is in FF4901; the stage a byte at FF4913; its camera one of three records by stage (src/mshvsf.h's
// CameraRecord), its place at +0C across and +10 down, which its step works on as a copy (+76, +78) kept between the
// ends at +68/6A and +6C/6E and puts back; objects in colour sets 1A and up are the stage's pieces; its sound ring at
// FF2BE6, the place of the next at FF2BE0.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace mshvsf_guest {

// Its stages, by its stage byte (src/mshvsf.h)
inline constexpr std::array<const char *, 12> kStageNames{
    "Marvel vs. Street Fighter: Apocalypse's lair", "Marvel vs. Street Fighter: the autumn pond",
    "Marvel vs. Street Fighter: the canyon",        "Marvel vs. Street Fighter: the jungle at night",
    "Marvel vs. Street Fighter: the temple",        "Marvel vs. Street Fighter: the great tree",
    "Marvel vs. Street Fighter: Manhattan at night", "Marvel vs. Street Fighter: the refinery",
    "Marvel vs. Street Fighter: the desert base",   "Marvel vs. Street Fighter: the TV studio",
    "Marvel vs. Street Fighter: the shopping mall", "Marvel vs. Street Fighter: the refinery in flames"};

inline uint32_t CameraRecord(const uint8_t *ram) {
    static constexpr int16_t kOffsets[16] = {-0x34C0, -0x35C0, -0x35C0, -0x3540, -0x35C0, -0x35C0, -0x34C0, -0x35C0,
                                             -0x3540, -0x3540, -0x35C0, -0x35C0, -0x35C0, -0x35C0, -0x35C0, -0x35C0};
    return static_cast<uint32_t>(0xFF8000 + kOffsets[ram[0x4913] & 0x0F]);
}

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "mshvsf";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF4C75;
    d.playersIn = 0xFF4901;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write8(0xFF4913, static_cast<uint8_t>(stage)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) < 0x1A; };
    // Its camera, as X-Men vs. Street Fighter's, but set in the copy its step works on (the stage's code keeps it between
    // the ends and puts it back): where its routines move it (0A1C94 across, 0A1F52 up and down)
    d.camera = Guest::Camera::Place;
    d.cameraLeft = 0;
    d.cameraRight = 1024;
    d.across = 0x0A1C94;
    d.up = 0x0A1F52;
    d.placeX = 0x76;
    d.placeY = 0x78;
    d.endLeft = 0x68;
    d.endRight = 0x6A;
    d.endTop = 0x6C;
    d.endBottom = 0x6E;
    d.placeDown = true;
    d.cameraRecord = CameraRecord;
    // Its stage alone, as X-Men vs. Street Fighter's: its game's loop (009292 the game's step, carrying on at 009294)
    // once a game is begun; a match's first step builds its stage: the lists of objects emptied (0041BC), the pools set
    // out (00463A, 004254), its colours (008BB0), the round's words (0100D2), the stage (0A197E), 005B32, the colours
    // brought up (008820). Each frame, as a fight's (0222DE): the lists of objects (0228C2, 022A0A), its scroll screens
    // and camera (0A1B3C), its pieces' list (022B2C), the object list (022C6A) and the frame's end (0212F0)
    d.mainLoop = 0x009292;
    d.loopGoesOn = 0x009294;
    d.beginAfter = 600;
    d.build = {0x0041BC, 0x00463A, 0x004254, 0x0100D2, 0x008BB0, 0x0A197E, 0x005B32, 0x008820};
    d.frame = {0x0228C2, 0x022A0A, 0x0A1B3C, 0x022B2C, 0x022C6A, 0x0212F0};
    d.placing = 0x0A1B3C;
    // (its object list, made at the frame's end: the pieces on its four draw lists by layer, and those sorted by depth,
    // the fighters among them, done at 021568; what is put in after them is in front of them)
    d.fightersDrawn = {0x021568};
    d.listRegister = 4;
    d.soundQueue = 0xFF2BE6;
    d.soundQueueEnd = 0xFF2BE0;
    d.soundQueueMask = 0x3F0;
    // (its stages have no tunes of their own: a round plays a character's theme, 0C on. Each is given one)
    d.stageTunes = {0x0C, 0x15, 0x0E, 0x10, 0x18, 0x11, 0x13, 0x19, 0x0F, 0x16, 0x17, 0x1F};
    return d;
}

} // namespace mshvsf_guest
