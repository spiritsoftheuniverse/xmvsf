// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel vs. Capcom (mvsc) as a guest: its stages behind another game's fighters, each from its
// own stage code alone (fight/guest_stages.h). Marvel Super Heroes vs. Street Fighter's engine (games/mshvsf/guest.h, whose
// description this is at this game's places: the two programs' listings set side by side, and src/mvsc.h): A5 = FF8000; credits
// FF4499; who is in FF4101; the stage a byte at FF4113; its camera one of three records by stage (src/mvsc.h's
// CameraRecord), its place at +0C across and +10 down, which its step works on as a copy (+76, +78) kept between the
// ends at +68/6A and +6C/6E and puts back; objects in colour sets 16-1D are the stage's pieces; its sound ring at
// FF27D8, the place of the next at FF27D2.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace mvsc_guest {

// Its stages, by its stage byte (src/mvsc.h)
inline constexpr std::array<const char *, 10> kStageNames{
    "Marvel vs. Capcom: the bathhouse", "Marvel vs. Capcom: the haunted hall",   "Marvel vs. Capcom: the airship",
    "Marvel vs. Capcom: the laboratory", "Marvel vs. Capcom: the temple",        "Marvel vs. Capcom: the command centre",
    "Marvel vs. Capcom: the wreckage",  "Marvel vs. Capcom: the city street",   "Marvel vs. Capcom: the canyon",
    "Marvel vs. Capcom: Onslaught's city"};

inline uint32_t CameraRecord(const uint8_t *ram) {
    static constexpr int16_t kOffsets[16] = {-0x3DA0, -0x3DA0, -0x3DA0, -0x3D20, -0x3D20, -0x3DA0, -0x3DA0, -0x3E20,
                                             -0x3E20, -0x3CA0, -0x3DA0, -0x3DA0, -0x3DA0, -0x3DA0, -0x3DA0, -0x3DA0};
    return static_cast<uint32_t>(0xFF8000 + kOffsets[ram[0x4113] & 0x0F]);
}

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "mvsc";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF4499;
    d.playersIn = 0xFF4101;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write8(0xFF4113, static_cast<uint8_t>(stage)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return set < 0x16 || set >= 0x1E;
    };
    // Its camera, as Marvel Super Heroes vs. Street Fighter's, set in the copy its step works on (the stage's code keeps it between
    // the ends and puts it back): where its routines move it (0CB2E2 across, 0CB560 up and down)
    d.camera = Guest::Camera::Place;
    d.cameraLeft = 0;
    d.cameraRight = 1024;
    d.across = 0x0CB2E2;
    d.up = 0x0CB560;
    d.placeX = 0x76;
    d.placeY = 0x78;
    d.endLeft = 0x68;
    d.endRight = 0x6A;
    d.endTop = 0x6C;
    d.endBottom = 0x6E;
    d.placeDown = true;
    d.cameraRecord = CameraRecord;
    // Its stage alone, as Marvel Super Heroes vs. Street Fighter's: its game's loop (00A9C6 the game's step, carrying on at
    // 00A9C8) once a game is begun; a match's first step (00AA76 on) builds its stage: the lists of objects emptied
    // (0045FA), the pools set out (004A92, 0046AC), its colours (00A2B6), the round's words (011F16), the stage (0CAE0C),
    // 005D3C, the colours brought up (009C58). Each frame, as a fight's (0259CA): the lists of objects (025FFE, 0260A4,
    // 02623E), its scroll screens and camera (0CB168), its pieces' list (0263F4), the object list (026578) and the
    // frame's end (024970)
    d.mainLoop = 0x00A9C6;
    d.loopGoesOn = 0x00A9C8;
    d.beginAfter = 600;
    d.build = {0x0045FA, 0x004A92, 0x0046AC, 0x011F16, 0x00A2B6, 0x0CAE0C, 0x005D3C, 0x009C58};
    // (then the round's clock, A5-3FF8, full, after the round's words are cleared: the canyon turns to dusk and night as
    // it runs down (0CDD26, below 60 and 30), and nothing runs it here)
    d.afterBuild = [](cps2recomp::M68K &c) { c.bus.Write8(0xFF4008, 0x99); };
    d.frame = {0x025FFE, 0x0260A4, 0x02623E, 0x0CB168, 0x0263F4, 0x026578, 0x024970};
    d.placing = 0x0CB168;
    // (its object list, made at the frame's end: the pieces on its four draw lists by layer, and those sorted by depth,
    // the fighters among them, done at 024BF2; what is put in after them is in front of them)
    d.fightersDrawn = {0x024BF2};
    d.listRegister = 4;
    d.soundQueue = 0xFF27D8;
    d.soundQueueEnd = 0xFF27D2;
    d.soundQueueMask = 0x3F0;
    // (its stages have no tunes of their own: a round plays a character's theme, 0C on. Each is given one)
    d.stageTunes = {0x15, 0x1B, 0x1A, 0x1C, 0x17, 0x0D, 0x12, 0x13, 0x0E, 0x0C};
    return d;
}

} // namespace mvsc_guest
