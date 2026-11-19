// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Darkstalkers (dstlk) as a guest: its stages behind another game's fighters (fight/guest_stages.h). Its facts, found
// for its port (family.h here; games/nwarr/src/nwarr.h has them at Night Warriors' places): A5 = FF8000; the game's
// steps FF8000, FF8004, FF8008; credits FF81D0; who is in FF81F9; a two-player match's stage set at 0059C6 from
// A5+13B0; the clock FF9409 (BCD); the camera an object at FF9508, stepped across at 0EDAF4 and up at 0EDC1A (from its
// frame, 0EDA94: its place at +10 and +14, the step at +34 and +36, which way at +2C and +2D); objects in colour sets
// below 14 are the panel's, the players' and their shadows' and sparks'.
#pragma once

#include "guest_stages.h"
#include "../../runtime/lift.h"

#include <array>
#include <cstdlib>

namespace dstlk_guest {

// Its stages, each a character's, in its order
inline constexpr std::array<const char *, 12> kStageNames{
    "Darkstalkers: Demitri's",  "Darkstalkers: Jon Talbain's", "Darkstalkers: Victor's",   "Darkstalkers: Lord Raptor's",
    "Darkstalkers: Morrigan's", "Darkstalkers: Anakaris's",    "Darkstalkers: Felicia's",  "Darkstalkers: Bishamon's",
    "Darkstalkers: Rikuo's",    "Darkstalkers: Sasquatch's",   "Darkstalkers: Huitzil's", "Darkstalkers: Pyron's"};

inline cps2recomp::guest_stages::Guest Description() {
    using namespace cps2recomp::guest_stages;
    Guest d;
    d.id = "dstlk";
    d.stageNames.assign(kStageNames.begin(), kStageNames.end());
    // A game (FF8000 2): its character choice (FF8004 0), its match (FF8004 4), a round fought (FF8008 0A)
    d.choosing = {{0xFF8000, 2}, {0xFF8004, 0}};
    d.match = {{0xFF8000, 2}, {0xFF8004, 4}};
    d.fought = {{0xFF8000, 2}, {0xFF8004, 4}, {0xFF8008, 0x0A}};
    d.credits = 0xFF81D0;
    d.playersIn = 0xFF81F9;
    // 0059C6: a two-player match's stage, from A5+13B0 (the stage + 1)
    d.stageAt = 0x0059C6;
    d.setStage = [](cps2recomp::M68K &c, int stage) { c.ram[0x93B0] = static_cast<uint8_t>(stage + 1); };
    d.clock = 0xFF9409;
    d.clockFull = 0x99;
    d.notTheStage = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) < 0x14; };
    // Its camera's ends are this game's own: 256 and 640 (both walked to), the floor 0x28 up
    d.cameraLeft = 0x100;
    d.cameraRight = 0x280;
    d.cameraAtX = 0xFF9518;
    d.cameraAtY = 0xFF951C;
    d.camera = Guest::Camera::Steps;
    d.across = 0x0EDAF4;
    d.up = 0x0EDC1A;
    // Its stage alone. Its game runs as a task (its start-up, attract mode and a game each another), and the loop of a
    // game's own (02E1AA: the frame's wait, the modes by FF8000 at 02E1C0, 041C2) is there only once a game is begun:
    // its start-up is given a credit and Start (beginAfter), and the loop's first frame is at its character choice.
    // A stage is built as a match's last step and a round's first build it (02E308 on: the stage's number 0059C6, 7734,
    // the stage 5C2C, 3EEE, the round's own task started, 17CC with 0x804: its colours brought up, then 5E90, 80CC,
    // 56B2, 76D8, 56516); each frame the stage's part of a fight's frame (02EB8A): its camera and scroll screens
    // (0ED9EC), B925C, C05C8, EEECA, D310C (its pieces: the generator, the pipes, the tree), and the object list (0017F4).
    // Not D8C90 (its colours put over the stage's, blue), nor the players' (33858, 4EFC8).
    d.mainLoop = 0x02E1C0;
    d.loopGoesOn = 0x02E1C4;
    d.beginAfter = 300;
    // (first: the colour set every load of colours takes, 1390(A5), is the character choice's, 0C, where its start-up
    // was left; a match's is 0. Then every object put away, 00521A, as the match's step before its stage's does
    // (02E2FE): the character choice's would go on painting into the stage's maps. Not 56B2 alone, which takes an object for the round's own announcing, 02E350, and
    // leaves it half made. Not 7734 either: it asks the sound program to stop everything (FF00), and the frame interrupt,
    // sending that, empties the whole ring, the stage's tune asked for after it too; the tune puts the music in its place)
    d.build = {0, 0x00521A, 0x0059C6, 0x005C2C, 0x003EEE, 0, 0x005E90, 0x0080CC, 0x0076D8, 0x056516};
    d.buildStep = [](cps2recomp::M68K &c, int step) {
        if (step == 0) {
            c.bus.Write16(0xFF9390, 0);
            return;
        }
        c.d[0] = 0x804;
        c.d[1] = 1;
        cps2recomp::Call(c, 0x0017CC);
    };
    d.frame = {0x0D8C90, 0x0ED9EC, 0x0B925C, 0x0C05C8, 0x0EEECA, 0x0D8290, 0x0D310C, 0x0017F4};
    d.placing = 0x0ED9EC;
    // (its sound ring: Night Warriors' FF0AA0 and FF8026, here at FF0A9E: family.h)
    // (its object list, 0017F4: its draw lists in turn, the fighters' at 001988 (their records at FF8388 and FF8788);
    // the stage's pieces put in after them are in front of them)
    d.fightersDrawn = {0x00198C};
    d.listRegister = 0;
    d.soundQueue = 0xFF0A9E;
    d.soundQueueEnd = 0xFF8026;
    d.soundQueueMask = 0x3F0;
    d.stageTunes = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13};
    return d;
}

} // namespace dstlk_guest
