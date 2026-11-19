// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: the players' own colours for the characters and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its characters' colours are, the ROM file PalMod
// changes, and how the editor shows a set on player 1 in the model viewer (model.cpp). As Pocket Fighter's.
//
// A fighter is coloured at 00461C: five palettes of 0x20 bytes from 35DD0E on, 0x500 a character (by its number, one
// on) and 0xA0 a set, the set the colour the player chose (+4A6: 1 Start, 2-4 the punches, 5-6 light and medium kick,
// 0 heavy kick, 7 a button the panel's seventh bit is; 028F9C), copied into the player's sets of the palette the board
// shows from (91C120 player 1's, 0x20 a set: 00484A).
#include "colours.h"
#include "machine.h"
#include "model.h"
#include "nwarr.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

void StartModelViewer(Machine &machine); // model.cpp
bool ModelViewer();

namespace {
    using namespace nwarr;

    constexpr uint32_t kColourSets = At(0x35DD0E), kCharacterColours = family::kCharacterColours, kSetSize = 0xA0;
    constexpr int kPalettesInSet = 5;
    constexpr uint32_t kColourChoice = family::kColoursWord; // word: the set (above)

    uint32_t SetAddress(int number, int set) {
        return kColourSets + static_cast<uint32_t>(number - 1) * kCharacterColours + static_cast<uint32_t>(set) * kSetSize;
    }
    std::vector<colours::Range> Ranges() {
        return {{kColourSets, kColourSets + (kCharacters - 1) * kCharacterColours}};
    }

    // Player 1 in the set: its choice as the game keeps it; its palettes are its own sets of the buffer the board shows
    // from (91C000, a palette of 0x20 bytes a set)
    uint32_t ShowSet(Machine &, int, int set, int &lines) {
        SetRam16(Player(0) + kColourChoice, static_cast<uint16_t>(set));
        lines = kPalettesInSet;
        return 0x91C000 + (Ram8(Player(0) + kColourSet) & 0x1F) * 0x20u;
    }

    std::vector<const char *> Backgrounds() {
        std::vector<const char *> list(std::begin(kStageNames), std::end(kStageNames));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

void RegisterColours() {
    colours::Description d;
    for (int n = 1; n < kCharacters; n++) {
        d.characters.push_back({n, kCharacterNames[n], family::kJapanNames[n]}); // (the names the game has in Japan: family.h)
    }
    d.setNames.assign(std::begin(family::kColourSetNames), std::end(family::kColourSetNames));
    d.palettesInSet = kPalettesInSet;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The seventh of the set's program files holds every character's colours (and the faces', the stages')
    d.romFile = family::kColourRomFile;
    d.romFileAt = family::kColourRomAt;
    d.romFileSize = 0x80000;
    d.romFileWhose = family::kColourRomWhose;
    d.featureDescription = "Characters in the colours of the files in the \"palettes\" folder in the game's data folder: the "
                           "program file that holds the colours, changed with PalMod, and text files, one a character "
                           "(Felicia.txt), which go before it. With no files the game's own colours. They are taken when a "
                           "player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each set is\n"
                     "# chosen by the button that chose the character ([LP] to [HK], [Start]) and has five lines of 16\n"
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
