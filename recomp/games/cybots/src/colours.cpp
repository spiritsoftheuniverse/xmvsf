// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the players' own colours for the machines and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its machines' colours are (cybots.h: each of a set's
// seven palettes from a table of its own, so a line's place is given, not the set's), the ROM file PalMod changes, and
// how the editor shows a set on player 1 in the model viewer (model.cpp).
#include "colours.h"
#include "machine.h"
#include "model.h"
#include "cybots.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

void StartModelViewer(Machine &machine); // model.cpp
bool ModelViewer();

namespace {
    using namespace cybots;

    // Where line `line` of the machine's set is: its table's palette, of the number the machine's colours name
    uint32_t LineAddress(int number, int set, int line) {
        const uint8_t *table = Program(kColourTable);
        const uint32_t offset = static_cast<uint32_t>((table[number * 2] << 8) | table[number * 2 + 1]);
        const uint8_t palette = *Program(kColourTable + offset + static_cast<uint32_t>(set) * 8 + static_cast<uint32_t>(line));
        return kColourBanks[line] + palette * static_cast<uint32_t>(kPaletteBytes);
    }
    std::vector<colours::Range> Ranges() {
        return {{kColourBanks[0], kColoursEnd}};
    }

    // Player 1 in the set: its choice as the game keeps it; its palettes are the first seven of the board's (900000)
    uint32_t ShowSet(Machine &, int, int set, int &lines) {
        SetRam8(Player(0) + kColourChoice, static_cast<uint8_t>((Ram8(Player(0) + kColourChoice) & ~1u) | (set & 1)));
        lines = kPalettesInSet;
        return kPalettes;
    }

    // The stages as short as the panel needs them
    std::vector<const char *> Backgrounds() {
        static const char *const names[kStages] = {"City", "Desert", "Station", "Town 3", "Forest", "Fortress", "Ice cave",
                                                   "Jungle",  "Volcano", "Space", "Sunset", "Ice night", "Ruins", "Burning"};
        std::vector<const char *> list(std::begin(names), std::end(names));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    colours::Description d;
    for (int n = 0; n < kMachines; n++) {
        d.characters.push_back({n, kMachineNames[n], nullptr});
    }
    d.setNames = {"1", "2"};
    d.palettesInSet = kPalettesInSet;
    d.lineAddress = LineAddress;
    d.ranges = Ranges;
    // The second of the set's program files has every machine's colours; PalMod never changes its first part, which
    // is the game's code
    d.romFile = "cybe.04";
    d.romFileAt = 0x080000;
    d.romFileSize = 0x80000;
    d.same = {0x080000, 0x090000};
    d.romFileWhose = "Cyberbots' Europe 950424";
    d.featureDescription = "Machines in the colours of the files in the \"palettes\" folder in the game's data folder: a "
                           "cybe.04 changed with PalMod, and text files, one a machine (Blodia.txt), which go before it. "
                           "With no files the game's own colours. They are taken when a player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each machine has\n"
                     "# two sets, [1] and [2] (the colours a player chose with), of seven lines of 16 colours: the machine's\n"
                     "# body, arm and weapon and its effects. A palette can be shared by machines, and then a change to it\n"
                     "# changes them all. On the two ice stages the game shows its own colours, made for their light. The\n"
                     "# 16th colour of a line is never seen. A colour is #RRGGBB or #RGB, rounded to the board's 16 levels of\n"
                     "# each; - keeps the game's. Sets and lines that are left out keep the game's colours.\n";
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
