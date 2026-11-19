// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Gem Fighter (pocket_fighter) as a guest: its stages behind another game's fighters, each from its own stage code
// alone (fight/guest_stages.h). Vampire Savior's engine (games/vampire_savior/guest.h): its facts from its port
// (src/pocket.h, src/widescreen.cpp) and its listing read beside Vampire Savior's: A5 = FF8000; credits FF80F6; who is in
// FF810D; the stage a word at FF8180 (its number doubled); the camera at FF8290 and FF8294 (0 to 100 across: its stages
// are 640 wide), asking where the players are at 01FAF0 (their outer edges, D0 and D1) and 01FB66 (their heights);
// objects in colour sets 1A and up, but the players' shadows, are the stage's; its sound ring at FFE230, the place of
// the next at FFF230.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>

namespace pocket_fighter_guest {

// Its stages, by whose they are (src/pocket.h)
inline constexpr std::array<const char *, 8> kStageNames{
    "Gem Fighter: Felicia's (the shop)",       "Gem Fighter: Ibuki's (the big screen)",
    "Gem Fighter: Hsien-Ko's (the snowy village)", "Gem Fighter: Chun-Li's and Sakura's (the elephant)",
    "Gem Fighter: Chinatown",                  "Gem Fighter: Ryu's (the forge)",
    "Gem Fighter: Tessa's and Akuma's (the castle)", "Gem Fighter: Ken's and Zangief's (the hall)"};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "pocket_fighter";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    d.credits = 0xFF80F6;
    d.playersIn = 0xFF810D;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.bus.Write16(0xFF8180, static_cast<uint16_t>(stage * 2)); };
    d.notTheStage = [](uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return set < 0x1A || (set == 0x1F && tile >= 0x1D8E0 && tile < 0x1D900);
    };
    d.cameraLeft = 0;
    d.cameraRight = 0x100;
    d.cameraAtX = 0xFF8290;
    d.cameraAtY = 0xFF8294;
    d.camera = Guest::Camera::Extents;
    d.across = 0x01FAF0;
    d.up = 0x01FB66;
    d.middle = 0xC0;
    d.halfApart = 0x60;
    d.above = 0x60;
    // Its stage alone, as Vampire Savior's: its main loop's dispatch (004D66, the modes by FF8000) passed over to
    // 004D6A; a round's first two steps build its stage (005A0E on): 01B1CA, 005BB2, 009050, its colours (02790A,
    // 02794A, 02797A), 01E73C, 01B764, the scroll screens and the camera (01E890); 02783A, 00C2C8; then its scroll
    // screens switched on (bits 1-3 of 36(A5)) and their base (24(A5)) as the second step leaves them. Each frame, as a
    // fight's (007DDE on): the scroll screens and the camera (01E890), the pools of objects but the players' (01C100,
    // 01C13C, 01C302, 01C0BC, 01C448, 01C3D0) and the object list (01D362)
    d.mainLoop = 0x004D66;
    d.loopGoesOn = 0x004D6A;
    d.build = {0x01B1CA, 0x005BB2, 0x009050, 0x02790A, 0x02794A, 0x02797A, 0x01E73C, 0x01B764, 0x01E890,
               0x02783A, 0x00C2C8, 0x01E890};
    d.afterBuild = [](cps2recomp::M68K &c) {
        c.bus.Write16(0xFF8036, static_cast<uint16_t>(c.bus.Read16(0xFF8036) | 0x0E));
        c.bus.Write16(0xFF8024, 0x90C0);
    };
    d.frame = {0x01E890, 0x01C100, 0x01C13C, 0x01C302, 0x01C0BC, 0x01C448, 0x01C3D0, 0x01D362};
    d.placing = 0x01E890;
    // (its object list, 01D362: its draw lists in turn, the fighters' at 01D3B6 (FF8400 and FF8800); the stage's pieces
    // put in after them are in front of them)
    d.fightersDrawn = {0x01D3BA};
    d.soundQueue = 0xFFE230;
    d.soundQueueEnd = 0xFFF230;
    // (stage n's tune is n + 1)
    d.stageTunes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    return d;
}

} // namespace pocket_fighter_guest
