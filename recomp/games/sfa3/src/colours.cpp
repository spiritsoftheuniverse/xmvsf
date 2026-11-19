// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: the players' own colours for the characters and the colour editor, which are the toolkit's
// (fight/colours.h). These are the game's facts for them: where its characters' colours are, the ROM file PalMod
// changes, and how the editor shows a set on player 1 in the model viewer (model.cpp).
//
// A character's colours are in the ROM from the table at kColourTable: a word a character (by its number) of how far
// on from the table its colours are, six sets there, each five palettes of 16 colours (words, 4 bits each of red,
// green and blue; the game keeps the brightness the palette buffer had). The set is the player's ism and its colour
// choice: (ism + 1) * 2 + choice, the ism FF for X, 0 for A, 1 for V, the choice 0 when the ism was chosen with a punch
// and 1 with a kick. When a round starts the game works the set out (01F95C), keeps where it is at the player's long
// kColours and where its palettes go at kColoursTo (the buffer the board shows from: 90C000 for player 1, 90C0A0 for
// player 2, the colour sets the player's +0D names), and copies all five there. The effects' palettes (fireballs, the
// supers' trails) are single palettes from the tables at 3329D4 and 34A7B4 (030306, 03037A), put back from kColours as
// they end (0303BE). Found from the routine that sets kColours, with the ism byte held at each value in a fight.
#include "colours.h"
#include "machine.h"
#include "model.h"
#include "sfa3.h"

#include <iterator>
#include <vector>

namespace cps2recomp {

void StartModelViewer(Machine &machine); // model.cpp
bool ModelViewer();

namespace {
    using namespace sfa3;

    constexpr uint32_t kColourTable = 0x32C094;
    constexpr int kColourSetsEach = 6, kPalettesInSet = 5;
    constexpr uint32_t kColourSetSize = kPalettesInSet * 16 * 2;
    // Where the sets end (the effects' table follows), and the effects' palettes, to the stages' table (3383B4)
    constexpr uint32_t kColourSetsEnd = 0x3329D4, kEffectColours = 0x332A14, kEffectColoursEnd = 0x3383B4;
    constexpr uint32_t kIsm = 0x132;                       // a player's byte: FF X-ism, 0 A-ism, 1 V-ism
    constexpr uint32_t kColours = 0x14C, kColoursTo = 0x150; // longs (01F986)

    // (the table's words, by character: 0x3C0 apart, six sets of 0xA0, but where characters share colours, the ones
    // of no character of this version among them)
    constexpr uint16_t kSetOffsets[kCharacters] = {
        0x0040, 0x0400, 0x07C0, 0x0B80, 0x0F40, 0x1300, 0x16C0, 0x1A80, 0x1E40, 0x2200, 0x25C0, 0x2980, 0x2D40, 0x3100, 0x34C0, 0x3880,
        0x3C40, 0x4000, 0x43C0, 0x4000, 0x43C0, 0x43C0, 0x4780, 0x4B40, 0x4B40, 0x4F00, 0x52C0, 0x5680, 0x5A40, 0x5E00, 0x61C0, 0x6580};
    uint32_t SetAddress(int number, int set) {
        return kColourTable + kSetOffsets[number & (kCharacters - 1)] + static_cast<uint32_t>(set) * kColourSetSize;
    }
    std::vector<colours::Range> Ranges() {
        return {{SetAddress(0, 0), kColourSetsEnd}, {kEffectColours, kEffectColoursEnd}};
    }

    // Player 1 in the set: its ism and choice as the game would have them, and the set it keeps; its palettes are
    // where the game put them for the round
    uint32_t ShowSet(Machine &, int number, int set, int &lines) {
        SetRam8(Player(0) + kIsm, static_cast<uint8_t>(set / 2 - 1));
        SetRam8(Player(0) + kColourChoice, static_cast<uint8_t>(set % 2));
        SetRam32(Player(0) + kColours, SetAddress(number, set));
        lines = kPalettesInSet;
        const uint32_t to = Ram32(Player(0) + kColoursTo) & 0xFFFFFF;
        return to >= 0x900000 && to < 0x930000 ? to : 0;
    }

    // The stages as the panel names them (the menu's names are longer): whose stage it is
    std::vector<const char *> Backgrounds() {
        static const char *const names[kStages] = {"Ryu's",     "Ken's",    "Akuma's",    "Charlie's", "Chun-Li's", "Adon's",   "Sodom's",
                                                   "Guy's",     "Birdie's", "Rose's",     "The final", "Sagat's",   "Dan's",    "Sakura's",
                                                   "Rolento's", "Dhalsim's", "Zangief's", "Gen's",     "M. Bison's", "Cammy's", "E. Honda's",
                                                   "Blanka's",  "R. Mika's", "Cody's",    "Vega's",    "Karin's"};
        std::vector<const char *> list(std::begin(names), std::end(names));
        list.push_back("White");
        list.push_back("Black");
        return list;
    }
} // namespace

// Where a character's set of colours is in the ROM (set: (ism + 1) * 2 + choice), for the tag style's newcomer
// (livetag.cpp)
uint32_t CharacterColourSet(int number, int set) {
    return SetAddress(number, set);
}

void RegisterColours() {
    colours::Description d;
    for (const uint8_t n : kFighterNumbers) {
        // (the names the game has in Japan, for files called by them, where they are no other character's here)
        const char *abroad = n == 0x03 ? "Nash" : n == 0x02 ? "Gouki" : nullptr;
        d.characters.push_back({n, kCharacterNames[n], abroad});
    }
    d.setNames = {"XP", "XK", "AP", "AK", "VP", "VK"};
    d.palettesInSet = kPalettesInSet;
    d.setAddress = SetAddress;
    d.ranges = Ranges;
    // The seventh of the set's program files has every character's colours (and the stages' and other palettes); PalMod
    // never changes its first part, which is the game's data
    d.romFile = "sz3.09c";
    d.romFileAt = 0x300000;
    d.romFileSize = 0x80000;
    d.same = {0x300000, 0x320000};
    d.romFileWhose = "Street Fighter Alpha 3's, 980904";
    d.featureDescription = "Characters in the colours of the files in the \"palettes\" folder in the game's data folder: an "
                           "sz3.09c changed with PalMod, and text files, one a character (Sakura.txt), which go before it. With "
                           "no files the game's own colours. They are taken when a player is coloured: at the next round.";
    d.originalHelp = "# To change them, copy this file to the palettes folder and change the colours there. Each set is\n"
                     "# chosen by the ism and the button that chose it ([XP]: X-ism with a punch, [XK]: with a kick, then\n"
                     "# [AP] [AK] for A-ism and [VP] [VK] for V-ism) and has five lines of 16 colours; the character is\n"
                     "# drawn in the first line's, the others are its effects. The 16th colour of a line is never seen. A\n"
                     "# colour is #RRGGBB or #RGB, rounded to the board's 16 levels of each; - keeps the game's. Sets and\n"
                     "# lines that are left out keep the game's colours.\n";
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
