// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Final Fight: the menu's "Alternative modes" (beat/beat.h): attract mode with selectable shows, score attack,
// stage viewer, and model viewer.
#include "beat.h"
#include "ffight.h"
#include "machine.h"

#include <cstdint>

namespace cps2recomp {

namespace {
    using namespace ffight;

    enum { kShowAll, kShowStory, kShowDemonstrations, kShowTitleOnly };
    constexpr uint32_t kBootFrames = 120;

    bool Wanted(uint16_t s, int show) {
        if (show == kShowStory) {
            return s == kShowStory1 || s == kShowStory2 || s == kShowStory3;
        }
        if (show == kShowDemonstrations) {
            return s == kShowDemo1 || s == kShowDemo2 || s == kShowDemo3;
        }
        if (show == kShowTitleOnly) {
            return s == kShowProfiles || s == kShowRanking || s == kShowTitle;
        }
        return true;
    }

    void AttractFrame() {
        const int show = beat::AttractShow();
        if (show == kShowAll || Ram16(kStep) != 0) {
            return;
        }
        const uint16_t cur = Ram16(kShow);
        if (cur == kShowBoot || cur == kShowEnd || Wanted(cur, show)) {
            return;
        }
        for (uint16_t next = static_cast<uint16_t>((cur + 2) % kShowLoopEnd); next != cur;
             next = static_cast<uint16_t>((next + 2) % kShowLoopEnd)) {
            if (Wanted(next, show)) {
                SetRam16(kShow, next);
                SetRam16(kShowSub, 0);
                return;
            }
        }
    }

    uint32_t FromBcd(uint32_t bcd) {
        uint32_t value = 0;
        for (int shift = 28; shift >= 0; shift -= 4) {
            value = value * 10 + ((bcd >> shift) & 0xF);
        }
        return value;
    }
} // namespace

bool ChoicesLeftOut() {
    return beat::AttractMode() || beat::ScoreAttack() || beat::Viewing();
}

void ModesBeforeFrame(Machine &machine) {
    beat::BeforeFrame(machine);
    if (beat::AttractMode()) {
        machine.dip[2] &= ~0x20; // Demo sound on
    }
    if (beat::ScoreAttack()) {
        machine.dip[2] |= 0x40; // Allow continue off
    }
}

void ModesFrame(Machine &machine) {
    beat::Frame(machine);
    if (beat::AttractMode() && !beat::Starting()) {
        AttractFrame();
    }
}

void ModesState(Archive &ar) {
    beat::State(ar);
}

void DescribeViewer(beat::Description &d); // viewer.cpp

void RegisterModes() {
    beat::Description d;
    DescribeViewer(d);
    d.shows = {"The story only", "The demonstrations only", "The title & rankings"};
    d.showsWords = "What attract mode plays: everything the board does, or only the story, only the demonstration fights, "
                   "or only the title and rankings, over and over.";
    d.showBegun = [](const Machine &machine) { return machine.frame > kBootFrames; };
    d.canStart = [](const Machine &machine) { return machine.frame > kBootFrames; };
    d.credit = [] { SetRam8(kCredits, 1); };
    d.credits = [] { return static_cast<int>(Ram8(kCredits)); };
    d.inGame = [] {
        const uint16_t step = Ram16(kStep);
        return step >= 2 && step <= 0xE && Ram8(kDemo) == 0 && Ram8(kContinueScreen) == 0;
    };
    d.score = [] { return FromBcd(Ram32(Player(0) + kScore)); };
    beat::Register(d);
}

} // namespace cps2recomp
