// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: the players' own colours for the characters and the colour editor, which are the
// toolkit's (fight/colours.h). These are the game's facts for them: where its characters' colours are, the ROM file
// PalMod changes, and how the editor shows a set on player 1 in the model viewer (model.cpp).
//
// A character's colours are in the ROM in sets of five palettes of 16 colours (words, 4 bits each of red, green and
// blue; the game puts the brightness in as it copies them), each set a word of how many palettes and then the
// palettes. A table at 0BFAF2 names each character's list of its sets (a word from the table each, by the character's
// number; Akuma is 16), and that list a set for each choice (kColourChoice: 0-5 the button that chose, 6 Start, 7 a
// button held, 8 and 9 the old version's; Akuma's only in 0A and 0B). When a round starts the game copies each player's
// set to the palettes the board shows (0BFA78: player 1's to object palettes 1-5 at 900020, player 2's to 10-14 at
// 900200). Found from the palettes a fight showed, looked for in the ROM, and the routine that copies them.
#include "colours.h"
#include "machine.h"
#include "model.h"
#include "ssf2t.h"

#include <iterator>
#include <string>
#include <vector>

namespace cps2recomp {

void StartModelViewer(Machine &machine); // model.cpp
bool ModelViewer();

namespace {
    using namespace ssf2t;

    // A set's first palette (after its count): family.h
    uint32_t SetAddress(int number, int set) {
        int count = 0;
        return family::ColourSet(number, set, count);
    }
    std::vector<colours::Range> Ranges() {
        std::vector<colours::Range> ranges;
        for (const auto &range : family::kColourRanges) {
            ranges.push_back({range[0], range[1]});
        }
        return ranges;
    }

    // Player 1 in the set: its choice as the game keeps it; its palettes are in its own colour sets of the buffer
    uint32_t ShowSet(Machine &, int, int set, int &lines) {
        SetRam8(Player(0) + kColourChoice, static_cast<uint8_t>(set));
        const int who = Ram8(Player(0) + kCharacter);
        lines = family::kPalettesOf ? family::kPalettesOf(who) : family::kPalettesInSet;
        return family::ColourShown(0, who, set);
    }

    // The stages as short as the panel needs them: whose they are
    std::vector<const char *> Backgrounds() {
        static std::vector<std::string> names; // (the stage's name to its " (": "Ryu's")
        if (names.empty()) {
            for (const char *name : kStageNames) {
                const std::string whole = name;
                names.push_back(whole.substr(0, whole.find(" (")));
            }
        }
        std::vector<const char *> list;
        for (const std::string &name : names) {
            list.push_back(name.c_str());
        }
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    if (!family::kColours) {
        return;
    }
    colours::Description d;
    for (int n = 0; n < kCharacters; n++) {
        // (the names the game has in Japan, for files called by them: the boxer, the claw and the dictator go round)
        d.characters.push_back({n, kCharacterNames[n], n == 10 ? "M. Bison (Japan)" : n == 11 ? "Balrog (Japan)" : n == 8 ? "Vega (Japan)" : nullptr});
    }
    d.setNames.assign(std::begin(family::kColourSetNames), std::end(family::kColourSetNames));
    d.palettesInSet = family::kPalettesInSet;
    d.palettesOf = family::kPalettesOf;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The second of the set's program files has every character's colours; PalMod never changes its first part, which is
    // the game's code
    d.romFile = family::kColourFile;
    d.romFileAt = family::kColourFileAt;
    d.romFileSize = family::kColourFileSize;
    d.same = {family::kColourFileSameFrom, family::kColourFileSameTo};
    d.romFileWhose = family::kColourFileWhose;
    d.featureDescription = family::kColourFeatureWords;
    d.originalHelp = family::kColourHelp;
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
