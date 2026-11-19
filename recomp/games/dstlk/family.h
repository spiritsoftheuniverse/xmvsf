// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Darkstalkers as one of the games made from Night Warriors' program, whose sources it builds (games/nwarr/src;
// places.h there): where the places those sources name are in this game.
#pragma once

#include <array>
#include <cstdint>

namespace nwarr::family {

// Its id, as another game's guest (fight/all_guests.h): its own stages are not shown as another game's
inline constexpr const char *kGuestId = "dstlk";

struct Place {
    uint32_t nwarr, here;
};
inline constexpr Place kPlaceList[] = {
    // places: begin (tools/places.py; a line ending "by hand" is kept)
    {0x0064D6, 0x005968}, // an instruction, 4 on alike
    {0x006502, 0x0059C6}, // an instruction, 4 on alike
    {0x00692E, 0x005F96}, // an instruction, 1 on alike
    {0x006B70, 0x0061C8}, // a place the code names so (1/1)
    {0x006BAC, 0x0061FC}, // an instruction, 2 on alike
    {0x008798, 0x00796C}, // an instruction, 10 on alike
    {0x01FCDC, 0x02AB92}, // an instruction, 1 on alike
    {0x0215F4, 0x02C4C4}, // a game begins: the game's routine (02E18E) given, as Start takes the credit by hand
    {0x029012, 0x02F3E8}, // the colours of a character chosen with a button, kept (+3A6) by hand
    {0x029092, 0x02F462}, // ... and of one chosen with Start by hand
    {0x02A220, 0x033FF4}, // an instruction, 15 on alike
    {0x068CFC, 0x05664E}, // an instruction, 6 on alike
    {0x0FF6D6, 0x0EDB06}, // the same entry of a jump table, 4 on alike
    {0x0FF6F6, 0x0EDB26}, // an instruction, 16 on alike
    {0x0FF780, 0x0EDBB0}, // an instruction, 10 on alike
    {0x0FF7BA, 0x0EDBEA}, // an instruction, 10 on alike
    {0x35DD0E, 0x30F3DC}, // a place the code names so (2/2)
    {0xFF0AA0, 0xFF0A9E}, // a variable the code names so (4/4)
    {0xFF8000, 0xFF8000}, // a variable the code names so (28/28)
    {0xFF8004, 0xFF8004}, // a variable the code names so (36/36)
    {0xFF8008, 0xFF8008}, // a variable the code names so (33/33)
    {0xFF800C, 0xFF800C}, // a variable the code names so (51/53)
    {0xFF8026, 0xFF8026}, // a variable the code names so (6/6)
    {0xFF808E, 0xFF808E}, // a variable the code names so (11/11)
    {0xFF8092, 0xFF8092}, // a variable the code names so (11/11)
    {0xFF81B2, 0xFF81D0}, // a variable the code names so (2/2)
    {0xFF81F9, 0xFF81F9}, // a variable the code names so (13/13)
    {0xFF8203, 0xFF81F3}, // a variable the code names so (1/1)
    {0xFF8308, 0xFF8308}, // a variable the code names so (7/7)
    {0xFF8388, 0xFF8388}, // a variable the code names so (47/48)
    {0xFF8D88, 0xFF9388}, // a variable the code names so (30/30)
    {0xFF8D98, 0xFF9398}, // a variable the code names so (3/3)
    {0xFF8D9A, 0xFF939A}, // a variable the code names so (3/3)
    {0xFF8E09, 0xFF9409}, // a variable the code names so (3/3)
    {0xFF8E14, 0xFF9414}, // a variable the code names so (1/1)
    {0xFF8E15, 0xFF9415}, // a variable the code names so (4/4)
    {0xFF8F08, 0xFF9508}, // a variable the code names so (3/3)
    {0xFF8F18, 0xFF9518}, // a variable the code names so (35/35)
    {0xFF8F1C, 0xFF951C}, // a variable the code names so (7/7)
    {0xFFA86E, 0xFFAA2E}, // a variable the code names so (13/13)
    // places: end
};
inline constexpr auto kPlaces = std::to_array(kPlaceList);

// --- what differs between the games made from the program (each game's own family.h; the sources' nwarr.h) ---

// The players' records: how far apart, and the bytes at their end (the character chosen, the colours chosen as a word
// and its low byte, 1 for a person, 4 once the guard mode is chosen (0: none asked), 1 or 2 at the picture's edge)
inline constexpr uint32_t kPlayerSize = 0x400;
inline constexpr uint32_t kChoice = 0x3A1, kColoursWord = 0x3A6, kColoursChosen = 0x3A7, kPerson = 0x39F, kModeChosen = 0,
                          kAtEdge = 0x6A;
// A byte (from A5) of a bit for each player still choosing its character, which the choice clears (0: the sources
// tell it from the colours chosen and the guard mode); the step word FF8004 while the players choose
inline constexpr int32_t kChoosingBits = 0x7417;
// A player's byte and its value while the player is in a fight (Night Warriors' +03 is 2; Darkstalkers' +03 stays 0,
// its +00 is 1)
inline constexpr uint32_t kInFight = 0;
inline constexpr uint8_t kInFightValue = 1;
// What a player handed to the computer mid-fight is given first (training.cpp): all of what a player made for the computer gets (029C2E's 06B252 in Night Warriors: a random draw, +2E9, +277, then the scripts)
inline constexpr uint32_t kComputerStarts = 0x058606;
// Where the computer's play runs for a player (A6) apart from the player's step, which that check goes before too (0:
// none; Darkstalkers' 033A80 and 033A9A run it for each player)
inline constexpr uint32_t kComputerPlays = 0x033A8C, kComputerPlays2 = 0x033AA6;
inline constexpr uint16_t kGameChoosing = 0x00;

// The routines that have an object drawn only while it is near the board's picture (Night Warriors' 00559C, 005600,
// 005638 and 005672): where each tests the object's place across, where they go on to have it drawn, and the table
// they go to it through (the last three leave its place in A0). src/widescreen.cpp widens them.
inline constexpr uint32_t kNearPicture1 = 0x004B98, kNearPicture2 = 0x004BFA, kNearPicture3 = 0x004C32, kNearPicture4 = 0x004C6E;
inline constexpr uint32_t kNearPictureDrawn = 0x004BBE, kNearPictureTable = 0x004BCE;

// The show's loop (FF8000): its parts up to kShowUnseen are run unseen in attract mode, the introduction is
// kIntroFirst-kIntroLast (the title in it: kShowTitle with FF8004 at kShowTitleStep on), the fights kFightFirst-kFightLast
inline constexpr uint16_t kShowUnseen = 0x00, kIntroFirst = 0x02, kIntroLast = 0x02, kFightFirst = 0x0E, kFightLast = 0x0E;
inline constexpr uint16_t kShowTitle = 0x02, kShowTitleStep = 0x08;

// The characters by number (0: none), and the stages by number, each the stage of the character one on
inline constexpr int kCharacters = 13;
inline constexpr const char *kCharacterNames[kCharacters] = {"?",        "Demitri", "Jon Talbain", "Victor",    "Lord Raptor",
                                                             "Morrigan", "Anakaris", "Felicia",    "Bishamon",  "Rikuo",
                                                             "Sasquatch", "Huitzil", "Pyron"};
inline constexpr int kStages = 12;
inline constexpr const char *kStageNames[kStages] = {"Demitri's", "Jon Talbain's", "Victor's", "Lord Raptor's", "Morrigan's", "Anakaris's",
                                                     "Felicia's", "Bishamon's", "Rikuo's", "Sasquatch's", "Huitzil's", "Pyron's"};

// What the players throw: how many records of 0xC0 from the first (the sources' kThrown)
inline constexpr int kThrownRecords = 4;

// Objects by colour set: the panel's are up to kPanelSets, the stage's pieces' from kStageSets; the faces beside the
// bars are in the players' sets, tiles kFaces to kFacesEnd
inline constexpr unsigned kPanelSets = 0x04, kStageSets = 0x14;
inline constexpr uint32_t kFaces = 0x002C0, kFacesEnd = 0x00400;

// Each place the game takes health off a player (21 here, 4 of them with no counterpart in Night Warriors: lst.py -r 'sub.w d4,0x42(aN)|subq.w #1,0x42(aN)'), in slots the
// sources hook (cheats.cpp; 0: none), and for each the register the player is in, 0x10 more for a subq of one
inline constexpr uint32_t kDamage00 = 0x036304, kDamage01 = 0x037C78, kDamage02 = 0x038A1A, kDamage03 = 0x038A4E,
                          kDamage04 = 0x038B42, kDamage05 = 0x038BB0, kDamage06 = 0x038D52, kDamage07 = 0x038D62,
                          kDamage08 = 0x038E1E, kDamage09 = 0x039154, kDamage10 = 0x03C442, kDamage11 = 0x03F700,
                          kDamage12 = 0x040346, kDamage13 = 0x0438CE, kDamage14 = 0x0458B6, kDamage15 = 0x04AC56,
                          kDamage16 = 0x04AE3E, kDamage17 = 0x04AEE2, kDamage18 = 0x04C690, kDamage19 = 0x0B6F7E,
                          kDamage20 = 0x0CE502, kDamage21 = 0, kDamage22 = 0, kDamage23 = 0,
                          kDamage24 = 0, kDamage25 = 0, kDamage26 = 0, kDamage27 = 0,
                          kDamage28 = 0, kDamage29 = 0, kDamage30 = 0, kDamage31 = 0,
                          kDamage32 = 0, kDamage33 = 0, kDamage34 = 0, kDamage35 = 0,
                          kDamage36 = 0, kDamage37 = 0, kDamage38 = 0, kDamage39 = 0;
inline constexpr uint8_t kDamageRegs[40] = {0x4, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// The board's settings (the Board page, board.cpp): each variable (from A5) and the test menu's names for its values,
// first to last; and the event and join-in variables (0: the game has none), as the test menu couples them. Each found
// by its copy from the settings record (02AB06 on, as Night Warriors' 01FC34); the names read from the test menu's
// one page, each option stepped through (time 0 slow, 1 normal, 2 fast; speed 0-2 turbo 1-3, 3 free select)
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
    {"board_timer", 0x233, "Time count speed", "How fast the round's clock runs.", {"Slow", "Normal", "Fast"}},
    {"board_turbo", 0x212, "Game speed", "The game's speed: one of three turbo speeds, or the players' choice.",
     {"Turbo 1", "Turbo 2", "Turbo 3", "Free select"}},
    {"board_rounds", 0x237, "Max round", "Rounds a match lasts at most.", {"1 round", "3 rounds", "5 rounds"}},
    {"board_continue", 0x1EB, "Continue", "Whether a game over can be continued.", {"Off", "On"}},
    {"board_demo_sound", 0x1F0, "Demo sound", "Whether the attract mode plays sound.", {"Off", "On"}},
};
inline constexpr int32_t kEventVariable = 0, kJoinInVariable = 0;

// Each stage's tune (the stage viewer's music), and the songs of the sound program's table (0x9006, kSongCount of them)
// by name: found by playing (CPS2RECOMP_SOUNDTRACK_LOG): the ladder with Jon Talbain and attract mode with demo sound
// on; `once` for the ones the board plays once
inline constexpr uint16_t kStageTunes[kStages] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13};
inline constexpr uint16_t kTuneSelect = 0x001, kTuneVersus = 0x002;
inline constexpr uint16_t kSongCount = 0x2B2;
struct Song {
    uint16_t number;
    const char *name, *title;
    bool once = false;
};
inline constexpr Song kSongs[] = {
    // Each stage's, the stage's own character's (08 on in the stages' order)
    {0x08, "demitri", "Demitri's stage"},
    {0x09, "jon_talbain", "Jon Talbain's stage"},
    {0x0A, "victor", "Victor's stage"},
    {0x0B, "lord_raptor", "Lord Raptor's stage"},
    {0x0C, "morrigan", "Morrigan's stage"},
    {0x0D, "anakaris", "Anakaris's stage"},
    {0x0E, "felicia", "Felicia's stage"},
    {0x0F, "bishamon", "Bishamon's stage"},
    {0x10, "rikuo", "Rikuo's stage"},
    {0x11, "sasquatch", "Sasquatch's stage"},
    {0x12, "huitzil", "Huitzil's stage"},
    {0x13, "pyron", "Pyron's stage"},
    // Around the matches (the show with demo sound on, the ladder with Jon Talbain)
    {0x00, "introduction", "The introduction and the title"},
    {0x2B, "qsound", "The QSound notice", true},
    {0x01, "select", "Character select"},
    {0x03, "map", "The map, before the next opponent", true},
    {0x02, "versus", "Versus", true},
    {0x15, "winner", "Winner", true},
    {0x05, "ranking", "The ranking", true},
    {0x21, "ending_start", "After the last fight, before the ending"},
    {0x2A, "ending", "Ending (Jon Talbain's)"},
    {0x07, "after_ending", "After the ending"},
};

// The hidden characters a switch gives (choosing.cpp): Start held as a player chooses the first gives the second
inline constexpr uint8_t kHidden[2][2] = {{3, 11}, {1, 12}}; // (Victor: Huitzil; Demitri: Pyron)
inline constexpr const char *kHiddenWords = "Huitzil and Pyron, the bosses, to play: hold Start as you choose Victor for Huitzil, or "
                                            "Demitri for Pyron (the button still chooses the colours).";

// The characters' colours (colours.cpp): 0x500 a character, eight sets (by the colour chosen), the sets' names, the
// names the game has in Japan (for files called by them), and the program file PalMod changes
inline constexpr uint32_t kCharacterColours = 0x140;
inline constexpr int kColourSetsEach = 2;
inline constexpr const char *kColourSetNames[kColourSetsEach] = {"Button", "Start"};
inline constexpr const char *kJapanNames[kCharacters] = {nullptr, nullptr, "Gallon", nullptr, "Zabel", nullptr, nullptr,
                                                         nullptr, nullptr, "Aulbath", nullptr, "Phobos", nullptr};
inline constexpr const char *kColourRomFile = "vame.09a", *kColourRomWhose = "Darkstalkers' Europe 940705";
inline constexpr uint32_t kColourRomAt = 0x300000;


} // namespace nwarr::family
