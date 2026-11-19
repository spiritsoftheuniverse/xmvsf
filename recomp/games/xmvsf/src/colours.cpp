// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: the players' own colours for the characters and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its characters' colours are (xmvsf.h), the ROM file
// PalMod changes, and how the editor shows a set on player 1 in the model viewer (fight/marvel.h).
#include "colours.h"
#include "machine.h"
#include "marvel.h"
#include "model.h"
#include "xmvsf.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

namespace {
    using namespace xmvsf;

    uint32_t Base(int number) {
        const uint8_t *p = Program(kColourTable + static_cast<uint32_t>(number) * 4);
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    }
    uint32_t SetAddress(int number, int set) {
        return Base(number) + static_cast<uint32_t>(set) * kColourSetSize;
    }
    // (every character's sets, one after another: from Wolverine's to the end of Alpha Chun-Li's)
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
        static const char *const names[kStages] = {"Lair", "Pond", "Cavern", "Waterfall", "Temple", "Tree",
                                                   "Manhattan",    "Refinery", "Space base", "TV studio", "Mall"};
        std::vector<const char *> list(std::begin(names), std::end(names));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    colours::Description d;
    for (int n = 0; n < kCharacters; n++) {
        // (the names the game has in Japan, for files called by them: M. Bison is Vega there, Charlie is Nash)
        d.characters.push_back({n, kCharacterNames[n], n == 14 ? "Vega" : n == 16 ? "Nash" : nullptr});
    }
    d.setNames = {"P", "K"};
    d.palettesInSet = kPalettesInSet;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The third of the set's program files has every character's colours; PalMod never changes the table of where they
    // are
    d.romFile = "xvs.05a";
    d.romFileAt = 0x100000;
    d.romFileSize = 0x80000;
    d.same = {kColourTable, kColourTable + kCharacters * 4};
    d.romFileWhose = "X-Men vs. Street Fighter's, 961004";
    d.featureDescription = "Characters in the colours of the files in the \"palettes\" folder in the game's data folder: an "
                           "xvs.05a changed with PalMod, and text files, one a character (Cyclops.txt), which go before it. "
                           "With no files the game's own colours. They are taken when a player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each set is\n"
                     "# chosen by the button that chose the character ([P]: a punch, [K]: a kick) and has three lines of 16\n"
                     "# colours; the character is drawn in the first line's, the others are its effects. The 16th colour of\n"
                     "# a line is never seen. A colour is #RRGGBB or #RGB, rounded to the board's 16 levels of each; - keeps\n"
                     "# the game's. Sets and lines that are left out keep the game's colours.\n";
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
