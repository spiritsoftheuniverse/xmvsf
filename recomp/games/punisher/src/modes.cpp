// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The Punisher: the menu's "Alternative modes" (the toolkit's beat/beat.h): what they need of the game.
//
// The show is a task of its own (00827C, started at 0068B0) that calls its parts one after another, round and round
// (from 0082D6): the best scores (00A22E), the story (008464: "Based upon characters", New York, the Castle family,
// "If you're guilty"), the title (008666), the demonstrations (00A0D4, three in each round), the players' profiles
// (00A4E4, between them) and the FBI's words (00A33E); the second round has another part in the title's place
// (008330). Attract mode leaves a part out by returning from its routine as it is called; the show goes on to the
// next. The show's sound is on whatever the board's demo sound says (board.cpp).
#include "beat.h"
#include "punisher.h"
#include "state.h"

namespace cps2recomp {

namespace {
    using namespace punisher;

    enum { kShowAll, kShowStory, kShowDemonstrations, kShowCharacters };

    // Where the show is (kept with the state): begun (its first part called) and its title reached since the
    // board's start
    struct Show {
        bool begun = false, title = false;
    } g_show;

    // A part's routine is called: left out if it is not wanted
    template <int kShow> bool Part(M68K &c) {
        g_show.begun = true;
        const int show = beat::AttractShow();
        if (show == kShowAll || show == kShow || !beat::AttractMode()) {
            return false;
        }
        Return(c);
        return true;
    }

    bool Title(M68K &c) {
        g_show.title = true;
        return Part<-1>(c);
    }

    uint32_t FromBcd(uint32_t bcd) {
        uint32_t value = 0;
        for (int shift = 28; shift >= 0; shift -= 4) {
            value = value * 10 + ((bcd >> shift) & 0xF);
        }
        return value;
    }
} // namespace

// Whether the player's own choices (free play, the cheats, where a game starts, the board's settings for the show's
// sound and continuing) are left out now
bool ChoicesLeftOut() {
    return beat::AttractMode() || beat::ScoreAttack() || beat::Viewing();
}

// The show has come to its title since the board's start
bool TitleReached() {
    return g_show.title;
}

void ModesBeforeFrame(Machine &machine) {
    if (machine.frame <= 1) {
        g_show = {};
    }
    beat::BeforeFrame(machine);
}

void ModesFrame(Machine &machine) {
    beat::Frame(machine);
}

void ModesState(Archive &ar) {
    beat::State(ar);
    ar.Mark(0x53484F57); // SHOW
    ar.Pod(g_show);
}

void DescribeViewer(beat::Description &d); // viewer.cpp

void RegisterModes() {
    beat::Description d;
    DescribeViewer(d);
    d.shows = {"The story only", "The demonstrations only", "The players' profiles only"};
    d.showsWords = "What attract mode plays: everything the board does, or only the story (New York, the Castle family), only "
                   "the demonstrations, or only the profiles of the Punisher and Nick Fury, over and over.";
    d.showBegun = [](const Machine &) { return g_show.begun; };
    d.canStart = [](const Machine &) { return g_show.begun; };
    d.credit = [] { SetRam16(kCredits, 1); };
    d.credits = [] { return static_cast<int>(Ram16(kCredits)); };
    d.inGame = [] { return Ram8(kPlayersIn) != 0; };
    d.score = [] { return FromBcd(Ram32(kScore)); };
    beat::Register(d);
    AddHook(HookAt(0x00A22E), "ShowScores", Part<-1>);
    AddHook(HookAt(0x008464), "ShowStory", Part<kShowStory>);
    AddHook(HookAt(0x008666), "ShowTitle", Title);
    AddHook(HookAt(0x008330), "ShowTitleSecond", Part<-1>);
    AddHook(HookAt(0x00A0D4), "ShowDemonstration", Part<kShowDemonstrations>);
    AddHook(HookAt(0x00A4E4), "ShowProfile", Part<kShowCharacters>);
    AddHook(HookAt(0x00A33E), "ShowWords", Part<-1>);
}

} // namespace cps2recomp
