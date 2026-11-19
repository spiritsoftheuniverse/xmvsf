// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter (xmvsf) as a guest: its stages behind another game's fighters, each from its own stage code
// alone (fight/guest_stages.h). Marvel Super Heroes' engine (games/msh/guest.h, whose description this is at this
// game's places: the two programs' listings set side by side, and src/xmvsf.h): A5 = FF8000; credits FF545F; who is in
// FF5101; the stage a byte at FF5113; its camera one of three records by stage (src/xmvsf.h's CameraRecord), its place
// at +0C across and +10 down, its ends at +72/74 and +76/78; objects in colour sets 13 and up are the stage's pieces;
// its sound ring at FF2648, the place of the next at FF2642.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace xmvsf_guest {

// Its stages, by its stage byte (src/xmvsf.h)
inline constexpr std::array<const char *, 11> kStageNames{
    "X-Men vs. Street Fighter: Apocalypse's lair", "X-Men vs. Street Fighter: the garden pond",
    "X-Men vs. Street Fighter: the cavern",        "X-Men vs. Street Fighter: the waterfall",
    "X-Men vs. Street Fighter: the snowy temple",  "X-Men vs. Street Fighter: the great tree",
    "X-Men vs. Street Fighter: Manhattan at night", "X-Men vs. Street Fighter: the refinery",
    "X-Men vs. Street Fighter: the space base",    "X-Men vs. Street Fighter: the TV studio",
    "X-Men vs. Street Fighter: the shopping mall"};

inline uint32_t CameraRecord(const uint8_t *ram) {
    static constexpr int16_t kOffsets[16] = {-0x2CC0, -0x2DC0, -0x2DC0, -0x2D40, -0x2DC0, -0x2DC0, -0x2CC0, -0x2DC0,
                                             -0x2D40, -0x2D40, -0x2DC0, -0x2DC0, -0x2DC0, -0x2DC0, -0x2DC0, -0x2DC0};
    return static_cast<uint32_t>(0xFF8000 + kOffsets[ram[0x5113] & 0x0F]);
}

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "xmvsf";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF545F;
    d.playersIn = 0xFF5101;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write8(0xFF5113, static_cast<uint8_t>(stage)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) < 0x13; };
    // Its camera, as Marvel Super Heroes': set where its routines move it (07025A across, 0704D0 up and down)
    d.camera = Guest::Camera::Place;
    d.cameraLeft = 0;
    d.cameraRight = 1024;
    d.across = 0x07025A;
    d.up = 0x0704D0;
    d.placeX = 0x0C;
    d.placeY = 0x10;
    d.endLeft = 0x72;
    d.endRight = 0x74;
    d.endTop = 0x76;
    d.endBottom = 0x78;
    d.placeDown = true;
    d.cameraRecord = CameraRecord;
    // Its stage alone, as Marvel Super Heroes': its game's loop (00A35A the game's step, carrying on at 00A35C) once a
    // game is begun; a match's first step (00A404 on) builds its stage: the lists of objects emptied (003C34), the pools
    // set out (004036, 003CCC), its colours (009D80), the round's words (00F116), the stage (06FF74), 004E98, the colours
    // brought up (009B10). Each frame, as a fight's (01EAC8): the lists of objects (01F066, 01F19E), its scroll screens
    // and camera (070162), its pieces' list (01F284), the object list (01F3BA) and the frame's end (01DB70)
    d.mainLoop = 0x00A35A;
    d.loopGoesOn = 0x00A35C;
    d.beginAfter = 600;
    d.build = {0x003C34, 0x004036, 0x003CCC, 0x00F116, 0x009D80, 0x06FF74, 0x004E98, 0x009B10};
    d.frame = {0x01F066, 0x01F19E, 0x070162, 0x01F284, 0x01F3BA, 0x01DB70};
    d.placing = 0x070162;
    // (its object list, made at the frame's end: the pieces on its four draw lists by layer, and those sorted by depth,
    // the fighters among them, done at 01DDC0; what is put in after them is in front of them)
    d.fightersDrawn = {0x01DDC0};
    d.listRegister = 4;
    d.soundQueue = 0xFF2648;
    d.soundQueueEnd = 0xFF2642;
    d.soundQueueMask = 0x3F0;
    // (its stages have no tunes of their own: a round plays a character's theme, 0B on. Each is given one: Apocalypse's
    // to his lair, Magneto's to the space base, and the others' to the rest)
    d.stageTunes = {0x13, 0x14, 0x0B, 0x0D, 0x17, 0x0E, 0x0F, 0x10, 0x12, 0x1C, 0x1B};
    return d;
}

} // namespace xmvsf_guest
