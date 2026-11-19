// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: the players' own colours for the characters and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its characters' colours are (msh.h), the ROM file
// PalMod changes, and how the editor shows a set on player 1 in the model viewer (fight/marvel.h).
#include "colours.h"
#include "machine.h"
#include "marvel.h"
#include "model.h"
#include "msh.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

namespace {
    using namespace msh;

    uint32_t Base(int number) {
        const uint8_t *p = Program(kColourTable + static_cast<uint32_t>(number) * 4);
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    }
    uint32_t SetAddress(int number, int set) {
        return Base(number) + static_cast<uint32_t>(set) * kColourSetSize;
    }
    // (every character's sets, one after another: from Spider-Man's to the end of Thanos's)
    std::vector<colours::Range> Ranges() {
        return {{Base(0), Base(kCharacters - 1) + kColourSetsEach * kColourSetSize}};
    }

    // Player 1 in the set: its choice as the game keeps it; its palettes are its own colour sets of the buffer the board
    // shows from
    uint32_t ShowSet(Machine &, int, int set, int &lines) {
        SetRam8(Player(0) + kColourChoice, static_cast<uint8_t>(set));
        lines = kPalettesInSet;
        return kPaletteBuffer + kPlayer1Set * 0x20u;
    }

    std::vector<const char *> Backgrounds() {
        static const char *const names[kStages] = {"Street", "Shipwreck", "Carnival", "Stark",  "Lake", "Neon city",
                                                   "Fire",   "Skull",     "Harbour",  "Space",  "Gems", "Throne"};
        std::vector<const char *> list(std::begin(names), std::end(names));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    colours::Description d;
    for (int n = 0; n < kCharacters; n++) {
        d.characters.push_back({n, kCharacterNames[n], nullptr});
    }
    d.setNames = {"1", "2"};
    d.palettesInSet = kPalettesInSet;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The third of the set's program files has every character's colours; PalMod never changes the table of where they
    // are
    d.romFile = "msh.05";
    d.romFileAt = 0x100000;
    d.romFileSize = 0x80000;
    d.same = {kColourTable, kColourTable + kCharacters * 4};
    d.romFileWhose = "Marvel Super Heroes', 951024";
    d.featureDescription = "Characters in the colours of the files in the \"palettes\" folder in the game's data folder: an "
                           "msh.05 changed with PalMod, and text files, one a character (Psylocke.txt), which go before it. "
                           "With no files the game's own colours. They are taken when a player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each character has\n"
                     "# two sets: [1] its own, and [2] the one player 2 has when both players chose the same character. A set\n"
                     "# has three lines of 16 colours; the character is drawn in the first line's, the others are its effects.\n"
                     "# The 16th colour of a line is never seen. A colour is #RRGGBB or #RGB, rounded to the board's 16 levels\n"
                     "# of each; - keeps the game's. Sets and lines that are left out keep the game's colours.\n";
    d.showSet = ShowSet;
    d.startViewer = marvel::StartModelViewer;
    d.viewerOn = marvel::ModelViewer;
    d.viewerCharacter = model::CharacterNumber;
    d.characterNumber = model::Number;
    d.animationStep = model::AnimationStep;
    d.animationNames = model::AnimationNames;
    d.animationForLine = [](Machine &, int line) { return model::AnimationForLine(line); };
    d.backgrounds = Backgrounds();
    colours::Register(d);
}

} // namespace cps2recomp
