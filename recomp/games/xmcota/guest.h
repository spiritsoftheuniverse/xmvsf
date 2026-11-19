// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom (xmcota) as a guest: its stages behind another game's fighters, each from its own stage
// code alone (fight/guest_stages.h). The first of the Marvel engine's games (games/msh/guest.h, which grew from it): its
// facts from its port (src/xmcota.h) and its listing read beside Marvel Super Heroes': A5 = FF8000; credits FF4BDD; who
// is in FF4881; the stage a byte at FF488F (stage n character n's); its camera a record (FF4980 on stages 0 and 0B on,
// FF4A00 elsewhere), its place at +0C across and +10 down, its ends at +70/72 across and +74/76 down; objects in colour
// sets 14 and up are the stage's pieces; its sound ring at FF4C00, the place of the next at FF4BFA.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace xmcota_guest {

// Its stages, by its stage byte (src/xmcota.h)
inline constexpr std::array<const char *, 12> kStageNames{
    "X-Men: the Savage Land",   "X-Men: the pipeworks",    "X-Men: the city street",       "X-Men: the Danger Room",
    "X-Men: the sunset",        "X-Men: the frozen sea",   "X-Men: the bone garden",       "X-Men: the temple",
    "X-Men: the space station", "X-Men: the Sentinel factory", "X-Men: the building site", "X-Men: Avalon"};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "xmcota";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF4BDD;
    d.playersIn = 0xFF4881;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write8(0xFF488F, static_cast<uint8_t>(stage)); };
    d.notTheStage = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) < 0x14; };
    // Its camera, as Marvel Super Heroes': set where its routines move it (06EB84 across, 06ECAC up and down: after
    // their checks, where they go by how the camera moves)
    d.camera = Guest::Camera::Place;
    d.cameraLeft = 0;
    d.cameraRight = 1024;
    d.across = 0x06EB84;
    d.up = 0x06ECAC;
    d.placeX = 0x0C;
    d.placeY = 0x10;
    d.endLeft = 0x70;
    d.endRight = 0x72;
    d.endTop = 0x74;
    d.endBottom = 0x76;
    d.placeDown = true;
    d.cameraRecord = [](const uint8_t *ram) -> uint32_t {
        const uint8_t stage = ram[0x488F];
        return stage == 0 || stage >= 0x0B ? 0xFF4980 : 0xFF4A00;
    };
    // Its stage alone, as Marvel Super Heroes': its game's loop (003456 the game's step, carrying on at 003458) once a
    // game is begun; a match's first step (00350C on) builds its stage (not 000D66, which waits for
    // its sound program): 000F3C, its colours (003082, 003312), the round's words (0092F0), the lists of objects emptied (002CF4), the pools
    // set out (002E9E, 002D52), the stage (06E95A), 06C628, 06C016, 06C81A, the colours brought up (002F56). Each
    // frame, as a fight's (0146B8 and after): the lists of objects (014A6E, 014CBC) with its scroll screens and camera
    // between (06EB00), the object list (014F10) and the frame's end (0136FA)
    d.mainLoop = 0x003456;
    d.loopGoesOn = 0x003458;
    d.beginAfter = 600;
    d.build = {0x000F3C, 0x003082, 0x003312, 0x0092F0, 0x002CF4, 0x002E9E, 0x002D52,
               0x06E95A, 0x00A270, 0x06C628, 0x06C016, 0x06C81A, 0x002F56};
    // (Avalon's camera opens a round of its own: at the top until its round begins, then down to the floor (07399A on,
    // under +06 of its record), and only then follows the players. Nothing begins a round here: it goes to the floor at
    // once, as it does when that opening is passed over (0739B6), and the routine returns)
    // (the Danger Room shows a simulation once a round is fought: its record's +34 the one, its high byte set when to
    // change, by a piece that goes by the players' states (062686: 06E91C); with no players it would stay the bare room
    // (its first, 6, set as its record starts, 070DDC). It is given the jungle, 0, at once, as a round's start has it)
    d.hooks = {{0x070E82, [](cps2recomp::M68K &c) {
                    const uint32_t room = c.a[6] & 0xFFFFFF;
                    if (c.bus.Read16(room + 0x34) == 0x0006) {
                        c.bus.Write16(room + 0x34, 0xFF00);
                        c.bus.Write16(room + 0x38, 0);
                    }
                    return false;
                }},
               {0x07399A, [](cps2recomp::M68K &c) {
                    const uint32_t camera = c.a[6] & 0xFFFFFF;
                    c.bus.Write16(camera + 0x0C, 0x80);
                    c.bus.Write16(camera + 0x10, 0x100);
                    c.bus.Write16(camera + 0x06, static_cast<uint16_t>(c.bus.Read16(camera + 0x06) + 4));
                    c.pc = (static_cast<uint32_t>(c.bus.Read16(c.a[7])) << 16) | c.bus.Read16(c.a[7] + 2);
                    c.a[7] += 4;
                    return true;
                }}};
    d.frame = {0x014A6E, 0x06EB00, 0x014CBC, 0x014F10, 0x0136FA};
    d.placing = 0x06EB00;
    // (its object list, made at the frame's end: the pieces on its four draw lists by layer, and those sorted by depth,
    // the fighters among them, done at 0139AC; what is put in after them is in front of them)
    d.fightersDrawn = {0x0139AC};
    d.listRegister = 4;
    d.soundQueue = 0xFF4C00;
    d.soundQueueEnd = 0xFF4BFA;
    d.soundQueueMask = 0x3F0;
    // (stage n is character n's, and its tune is that character's theme, 07 on)
    d.stageTunes = {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12};
    return d;
}

} // namespace xmcota_guest
