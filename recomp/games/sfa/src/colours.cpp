// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the players' own colours for the characters and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its characters' colours are, the ROM file PalMod
// changes, and how the editor shows a set on player 1 in the model viewer (model.cpp).
//
// A character's colours are in the ROM from kColourSets, two sets a character (by its number): the first when a punch
// chose it, the second a kick (the player's kColourChoice), each eight palettes of 16 colours (words, 4 bits each of
// red, green and blue; the game puts the brightness in as it copies them). When a round starts the game copies the
// player's set to the palette buffer the board shows from (011090: kColourSets + character * 0x200 + choice * 0x100;
// player 1's to its colour sets 0-7 at 914000, player 2's to 8-15 at 914100). Found from the palettes a fight showed,
// looked for in the ROM, and the routine that reads them.
#include "colours.h"
#include "machine.h"
#include "model.h"
#include "sfa.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

void StartModelViewer(Machine &machine); // model.cpp
bool ModelViewer();

namespace {
    using namespace sfa;

    constexpr uint32_t kColourSets = 0x1E7960;
    constexpr int kColourSetsEach = 2, kPalettesInSet = 8;
    constexpr uint32_t kColourSetSize = kPalettesInSet * 16 * 2;
    constexpr uint32_t kPaletteBuffer = 0x914000; // (the objects' palettes, 0x20 bytes a colour set)

    uint32_t SetAddress(int number, int set) {
        return kColourSets + static_cast<uint32_t>(number * kColourSetsEach + set) * kColourSetSize;
    }
    std::vector<colours::Range> Ranges() {
        return {{kColourSets, kColourSets + kCharacters * kColourSetsEach * kColourSetSize}};
    }

    // Player 1 in the set: its choice as the game keeps it; its palettes are in its own colour sets of the buffer
    uint32_t ShowSet(Machine &, int, int set, int &lines) {
        SetRam8(Player(0) + kColourChoice, static_cast<uint8_t>(set));
        lines = kPalettesInSet;
        return kPaletteBuffer + (Ram8(Player(0) + kColourSet) & 0x1F) * 0x20u;
    }

    // The stages as short as the panel needs them: whose they are
    std::vector<const char *> Backgrounds() {
        static const char *const names[kStages] = {"Ryu's",  "Ken's",  "Akuma's", "Charlie's", "Chun-Li's", "Adon's", "Sodom's",
                                                   "Guy's",  "Birdie's", "Rose's", "M. Bison's", "Sagat's",  "Dan's"};
        std::vector<const char *> list(std::begin(names), std::end(names));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    colours::Description d;
    for (int n = 0; n < kCharacters; n++) {
        // (the names the game has in Japan, for files called by them: Charlie is Nash there, M. Bison Vega)
        d.characters.push_back({n, kCharacterNames[n], n == 3 ? "Nash" : n == 10 ? "Vega" : n == 2 ? "Gouki" : nullptr});
    }
    d.setNames = {"P", "K"};
    d.palettesInSet = kPalettesInSet;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The last of the set's program files has every character's colours; PalMod never changes its first part, which is
    // the game's data
    d.romFile = "sfz.06";
    d.romFileAt = 0x180000;
    d.romFileSize = 0x80000;
    d.same = {0x180000, 0x190000};
    d.romFileWhose = "Street Fighter Alpha's, 950727";
    d.featureDescription = "Characters in the colours of the files in the \"palettes\" folder in the game's data folder: a "
                           "sfz.06 changed with PalMod, and text files, one a character (Sakura.txt), which go before it. "
                           "With no files the game's own colours. They are taken when a player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each set is\n"
                     "# chosen by the button that chose the character ([P]: a punch, [K]: a kick) and has eight lines of 16\n"
                     "# colours; the character is drawn in the first line's, the others are its effects. The 16th colour of\n"
                     "# a line is never seen. A colour is #RRGGBB or #RGB, rounded to the board's 16 levels of each; - keeps\n"
                     "# the game's. Sets and lines that are left out keep the game's colours.\n";
    d.showSet = ShowSet;
    d.startViewer = StartModelViewer;
    d.viewerOn = ModelViewer;
    d.viewerCharacter = model::CharacterNumber;
    d.characterNumber = model::Number;
    d.animationStep = model::AnimationStep;
    d.animationNames = model::AnimationNames;
    d.animationForLine = [](Machine &, int line) { return model::AnimationForLine(line); };
    d.backgrounds = Backgrounds();
    colours::Register(d);
}

} // namespace cps2recomp
