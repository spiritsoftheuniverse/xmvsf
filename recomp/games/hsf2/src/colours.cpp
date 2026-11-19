// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: the players' own colours for the characters and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its characters' colours are, the ROM file PalMod
// changes, and how the editor shows a set on player 1 in the model viewer (model.cpp).
//
// A character's colours are in the ROM in sets of five palettes of 16 colours (words, 4 bits each of red, green and
// blue), each set a word of how many palettes and then the palettes, as in Super Turbo. A table at 0C3E70 names each
// version's table (a word a version, by kVersion), that a long for each character (Akuma the Super Turbo version's
// seventeenth), and that a list of the character's sets, a word a choice (kColourChoice). The Super Turbo version has
// eight sets a character (0-5 the button that chose, 6 Start, 7 a button held: 00C2A2), the old versions two, a punch's and a
// kick's. When a round starts the game copies each player's set to the palettes the board shows (0C3DF4: player 1's to
// object palettes 1-5 at 900020, player 2's to 6-10 at 9000C0). Found from the routine that copies them; the sets lie
// from 0C3F9E to 0CD036 (every version's). The editor works on the Super Turbo version's, the version the modes play.
#include "colours.h"
#include "machine.h"
#include "model.h"
#include "hsf2.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

void StartModelViewer(Machine &machine); // model.cpp
bool ModelViewer();

namespace {
    using namespace hsf2;

    constexpr uint32_t kColourTable = 0x0C3E70;
    constexpr int kPalettesInSet = 5;
    constexpr uint32_t kColoursFrom = 0x0C3F9E, kColoursTo = 0x0CD036; // (every version's sets)
    constexpr uint32_t kPaletteBuffer = 0x900000; // (the objects' palettes, 0x20 bytes a colour set)
    constexpr int kFirstPalette = 1;              // player 1's

    uint16_t Word(uint32_t address) {
        const uint8_t *at = Program(address); // (the data view: 0C3DFE reads it through A0)
        return static_cast<uint16_t>((at[0] << 8) | at[1]);
    }
    uint32_t Long(uint32_t address) {
        return (static_cast<uint32_t>(Word(address)) << 16) | Word(address + 2);
    }
    // A set's first palette (after its count)
    uint32_t SetAddress(int number, int set) {
        const uint32_t versions = kColourTable + Word(kColourTable + kSuperTurbo);
        const uint32_t list = Long(versions + static_cast<uint32_t>(number) * 4);
        return list + Word(list + static_cast<uint32_t>(set) * 2) + 2;
    }
    std::vector<colours::Range> Ranges() {
        return {{kColoursFrom, kColoursTo}};
    }

    // Player 1 in the set: its choice as the game keeps it; its palettes are in its own colour sets of the buffer
    uint32_t ShowSet(Machine &, int, int set, int &lines) {
        SetRam8(Player(0) + kColourChoice, static_cast<uint8_t>(set));
        lines = kPalettesInSet;
        return kPaletteBuffer + kFirstPalette * 0x20u;
    }

    // The stages as short as the panel needs them: whose they are
    std::vector<const char *> Backgrounds() {
        static const char *const names[kStages] = {"Ryu's",   "E. Honda's", "Blanka's", "Guile's",  "Ken's",    "Chun-Li's",
                                                   "Zangief's", "Dhalsim's", "M. Bison's", "Sagat's", "Balrog's", "Vega's",
                                                   "Cammy's", "T. Hawk's",  "Fei Long's", "Dee Jay's"};
        std::vector<const char *> list(std::begin(names), std::end(names));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    colours::Description d;
    for (int n = 0; n < kCharacters; n++) {
        // (the names the game has in Japan, for files called by them: the boxer, the claw and the dictator go round)
        d.characters.push_back({n, kCharacterNames[n], n == 10 ? "M. Bison (Japan)" : n == 11 ? "Balrog (Japan)" : n == 8 ? "Vega (Japan)" : nullptr});
    }
    d.setNames = {"LP", "MP", "HP", "LK", "MK", "HK", "Start", "Held"};
    d.palettesInSet = kPalettesInSet;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The second of the set's program files has every character's colours; PalMod never changes its first part, which is
    // the game's code
    d.romFile = "hs2u.04";
    d.romFileAt = 0x080000;
    d.romFileSize = 0x80000;
    d.same = {0x080000, 0x090000};
    d.romFileWhose = "Hyper Street Fighter II's, USA 040202";
    d.featureDescription = "Characters in the colours of the files in the \"palettes\" folder in the game's data folder: a "
                           "hs2u.04 changed with PalMod, and text files, one a character (Cammy.txt), which go before it. "
                           "With no files the game's own colours. They are taken when a player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each set is\n"
                     "# chosen by the button that chose the character ([LP] to [HK]), Start, or a button held ([Held]),\n"
                     "# each five lines of 16 colours, the Super Turbo version's; the character is drawn in\n"
                     "# them. The 16th colour of a line is never seen. A colour is #RRGGBB or #RGB, rounded to the board's\n"
                     "# 16 levels of each; - keeps the game's. Sets and lines that are left out keep the game's colours.\n";
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
