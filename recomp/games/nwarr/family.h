// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors as the first of the games made from its program, whose sources Darkstalkers builds too
// (games/nwarr/src; places.h there): its places are where its sources say (an empty list).
#pragma once

#include <array>
#include <cstdint>

namespace nwarr::family {

// Its id, as another game's guest (fight/all_guests.h): its own stages are not shown as another game's
inline constexpr const char *kGuestId = "nwarr";

struct Place {
    uint32_t nwarr, here;
};
inline constexpr std::array<Place, 0> kPlaces = {};

// --- what differs between the games made from the program (each game's own family.h; the sources' nwarr.h) ---

// The players' records: how far apart, and the bytes at their end (the character chosen, the colours chosen as a word
// and its low byte, 1 for a person, 4 once the guard mode is chosen (0: none asked), 1 or 2 at the picture's edge)
inline constexpr uint32_t kPlayerSize = 0x500;
inline constexpr uint32_t kChoice = 0x4A1, kColoursWord = 0x4A6, kColoursChosen = 0x4A7, kPerson = 0x49F, kModeChosen = 0x4B9,
                          kAtEdge = 0x6F;
// A byte (from A5) of a bit for each player still choosing its character, which the choice clears (0: the sources
// tell it from the colours chosen and the guard mode); the step word FF8004 while the players choose
inline constexpr int32_t kChoosingBits = 0;
// A player's byte and its value while the player is in a fight (Night Warriors' +03 is 2; Darkstalkers' +03 stays 0,
// its +00 is 1)
inline constexpr uint32_t kInFight = 3;
inline constexpr uint8_t kInFightValue = 2;
// What a player handed to the computer mid-fight is given first (training.cpp): its scripts (enough here)
inline constexpr uint32_t kComputerStarts = 0x068CFC;
// Where the computer's play runs for a player (A6) apart from the player's step, which that check goes before too (0:
// none; Darkstalkers' 033A80 and 033A9A run it for each player)
inline constexpr uint32_t kComputerPlays = 0, kComputerPlays2 = 0;
inline constexpr uint16_t kGameChoosing = 0x0C;

// The routines that have an object drawn only while it is near the board's picture (Night Warriors' 00559C, 005600,
// 005638 and 005672): where each tests the object's place across, where they go on to have it drawn, and the table
// they go to it through (the last three leave its place in A0). src/widescreen.cpp widens them.
inline constexpr uint32_t kNearPicture1 = 0x0055A6, kNearPicture2 = 0x005608, kNearPicture3 = 0x005640, kNearPicture4 = 0x00567C;
inline constexpr uint32_t kNearPictureDrawn = 0x0055CC, kNearPictureTable = 0x0055DC;

// The show's loop (FF8000): its parts up to kShowUnseen are run unseen in attract mode, the introduction is
// kIntroFirst-kIntroLast (the title in it: kShowTitle with FF8004 at kShowTitleStep on), the fights kFightFirst-kFightLast
inline constexpr uint16_t kShowUnseen = 0x02, kIntroFirst = 0x04, kIntroLast = 0x06, kFightFirst = 0x0E, kFightLast = 0x14;
inline constexpr uint16_t kShowTitle = 0x06, kShowTitleStep = 0x14;

// The characters by number (0: none), and the stages by number, each the stage of the character one on
inline constexpr int kCharacters = 15;
inline constexpr const char *kCharacterNames[kCharacters] = {"?",          "Demitri",  "Jon Talbain", "Victor",   "Lord Raptor",
                                                             "Morrigan",   "Anakaris", "Felicia",     "Bishamon", "Rikuo",
                                                             "Sasquatch",  "Huitzil",  "Pyron",       "Hsien-Ko", "Donovan"};
inline constexpr int kStages = 14;
inline constexpr const char *kStageNames[kStages] = {"Demitri's", "Jon Talbain's", "Victor's", "Lord Raptor's", "Morrigan's",
                                                     "Anakaris's", "Felicia's", "Bishamon's", "Rikuo's", "Sasquatch's",
                                                     "Huitzil's", "Pyron's", "Hsien-Ko's", "Donovan's"};

// What the players throw: how many records of 0xC0 from the first (the sources' kThrown)
inline constexpr int kThrownRecords = 9;

// Objects by colour set: the panel's are up to kPanelSets, the stage's pieces' from kStageSets; the faces beside the
// bars are in the players' sets, tiles kFaces to kFacesEnd
inline constexpr unsigned kPanelSets = 0x04, kStageSets = 0x14;
inline constexpr uint32_t kFaces = 0x28440, kFacesEnd = 0x284C0;

// Each place the game takes health off a player (lst.py -r 'sub.w d4,0x42(aN)|subq.w #1,0x42(aN)'), in slots the
// sources hook (cheats.cpp; 0: none), and for each the register the player is in, 0x10 more for a subq of one
inline constexpr uint32_t kDamage00 = 0x02C184, kDamage01 = 0x02C738, kDamage02 = 0x02ED20, kDamage03 = 0x02F3F4,
                          kDamage04 = 0x03029A, kDamage05 = 0x0302C6, kDamage06 = 0x0303BC, kDamage07 = 0x03043A,
                          kDamage08 = 0x030626, kDamage09 = 0x030636, kDamage10 = 0x0306F8, kDamage11 = 0x0307BA,
                          kDamage12 = 0x03096E, kDamage13 = 0x030C38, kDamage14 = 0x030E86, kDamage15 = 0x031074,
                          kDamage16 = 0x03112A, kDamage17 = 0x03787A, kDamage18 = 0x03AD7C, kDamage19 = 0x03BD18,
                          kDamage20 = 0x0421A0, kDamage21 = 0x043C2C, kDamage22 = 0x04A2EC, kDamage23 = 0x04A4D4,
                          kDamage24 = 0x04A5FE, kDamage25 = 0x04C8E8, kDamage26 = 0x04D528, kDamage27 = 0x04D57C,
                          kDamage28 = 0x04F3A0, kDamage29 = 0x04F47E, kDamage30 = 0x052142, kDamage31 = 0x06811A,
                          kDamage32 = 0x0681B4, kDamage33 = 0x0C1384, kDamage34 = 0x0DA878, kDamage35 = 0,
                          kDamage36 = 0, kDamage37 = 0, kDamage38 = 0, kDamage39 = 0;
inline constexpr uint8_t kDamageRegs[40] = {0x16, 0x4, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x16, 0x16, 0x4, 0x16, 0, 0, 0, 0, 0};

// The board's settings (the Board page, board.cpp): each variable (from A5) and the test menu's names for its values,
// first to last; and the event and join-in variables (0: the game has none), as the test menu couples them. Each found
// by saving the test menu with that option alone changed and diffing the game's memory after the next start; the
// names read from the test menu, each option stepped through
struct BoardSetting {
    const char *name;
    int32_t variable;
    const char *title, *words;
    const char *choices[10];
};
inline constexpr BoardSetting kBoard[] = {
    {"board_difficulty", 0x1F1, "Difficulty", "How well the computer fights.",
     {"1 Easiest", "2 Very easy", "3 Easy", "4 Medium", "5 Medium hard", "6 Hard", "7 Very hard", "8 Hardest"}},
    {"board_damage", 0x263, "Damage level", "How much damage a hit does, 1 (least) to 9.",
     {"1 Minimum", "2", "3", "4", "5 Medium", "6", "7", "8", "9 Maximum"}},
    {"board_timer", 0x233, "Time count speed", "How fast the round's clock runs, 1 (slowest) to 8.",
     {"1 Slow", "2", "3", "4 Normal", "5", "6", "7", "8 Fast"}},
    {"board_turbo", 0x212, "Turbo", "The game's speed: normal, turbo, or the players' choice.", {"Off", "On", "Free select"}},
    {"board_rounds", 0x237, "Max round", "Rounds a match lasts at most.", {"1 round", "3 rounds", "5 rounds"}},
    {"board_mode_select", 0x203, "Mode select", "Whether players choose between normal and automatic guard.", {"Off", "On"}},
    {"board_join_in", 0x20A, "Join-in", "Whether a second player can join a game against the computer.", {"Off", "On"}},
    {"board_event", 0x209, "Event", "Off, or one match only (and no join-in).", {"Off", "On"}},
    {"board_continue", 0x1EB, "Continue", "Whether a game over can be continued.", {"Off", "On"}},
    {"board_demo_sound", 0x1F0, "Demo sound", "Whether the attract mode plays sound.", {"Off", "On"}},
};
inline constexpr int32_t kEventVariable = 0x209, kJoinInVariable = 0x20A;

// Each stage's tune (the stage viewer's music), and the songs of the sound program's table (0x9006, kSongCount of them)
// by name: found by playing (CPS2RECOMP_SOUNDTRACK_LOG): the ladder with Lord Raptor, each stage held with --poke for
// the four it did not reach, attract mode; `once` for the ones the board plays once
inline constexpr uint16_t kStageTunes[kStages] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x3EC, 0x3ED};
inline constexpr uint16_t kTuneSelect = 0x003, kTuneVersus = 0x002;
inline constexpr uint16_t kSongCount = 0x400;
struct Song {
    uint16_t number;
    const char *name, *title;
    bool once = false;
};
inline constexpr Song kSongs[] = {

    // Each stage's, the stage's own character's (08 on in the stages' order; Hsien-Ko's and Donovan's, new in this
    // game, are 03EC and 03ED)
    {0x008, "demitri", "Demitri's stage"},
    {0x009, "jon_talbain", "Jon Talbain's stage"},
    {0x00A, "victor", "Victor's stage"},
    {0x00B, "lord_raptor", "Lord Raptor's stage"},
    {0x00C, "morrigan", "Morrigan's stage"},
    {0x00D, "anakaris", "Anakaris's stage"},
    {0x00E, "felicia", "Felicia's stage"},
    {0x00F, "bishamon", "Bishamon's stage"},
    {0x010, "rikuo", "Rikuo's stage"},
    {0x011, "sasquatch", "Sasquatch's stage"},
    {0x012, "huitzil", "Huitzil's stage"},
    {0x013, "pyron", "Pyron's stage"},
    {0x3EC, "hsien_ko", "Hsien-Ko's stage"},
    {0x3ED, "donovan", "Donovan's stage"},
    // Around the matches
    {0x02B, "qsound", "The QSound notice", true},
    {0x001, "introduction", "The introduction and the title"},
    {0x005, "ranking", "The Best 5 ranking", true},
    {0x003, "select", "Character select"},
    {0x02F, "next_opponent", "The next opponent chosen", true},
    {0x002, "versus", "Versus", true},
    {0x017, "winner", "Winner", true},
    {0x023, "ending_start", "After the last fight, before the ending"},
    {0x3F5, "ending", "Ending (Lord Raptor's)"},
    {0x02A, "staff_roll", "Staff roll"},
};

// The hidden characters a switch gives (choosing.cpp): Start held as a player chooses the first gives the second
inline constexpr uint8_t kHidden[2][2] = {{3, 11}, {1, 12}}; // (Victor: Huitzil; Demitri: Pyron)
inline constexpr const char *kHiddenWords = "Huitzil and Pyron, the bosses, to play: hold Start as you choose Victor for Huitzil, or "
                                            "Demitri for Pyron (the button still chooses the colours).";

// The characters' colours (colours.cpp): 0x500 a character, eight sets (by the colour chosen), the sets' names, the
// names the game has in Japan (for files called by them), and the program file PalMod changes
inline constexpr uint32_t kCharacterColours = 0x500;
inline constexpr int kColourSetsEach = 8;
inline constexpr const char *kColourSetNames[kColourSetsEach] = {"HK", "Start", "LP", "MP", "HP", "LK", "MK", "7"};
inline constexpr const char *kJapanNames[kCharacters] = {nullptr, nullptr, "Gallon",  nullptr, "Zabel",   nullptr, nullptr, nullptr,
                                                         nullptr, "Aulbath", nullptr, nullptr, nullptr, "Lei-Lei", nullptr};
inline constexpr const char *kColourRomFile = "vphe.09b", *kColourRomWhose = "Night Warriors' Europe 950316";
inline constexpr uint32_t kColourRomAt = 0x300000;

} // namespace nwarr::family
