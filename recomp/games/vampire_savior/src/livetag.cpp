// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Vampire Savior: swapping a player's fighter in the middle of a fight, for the team battle's tag style (fight/battle.h).
// The game sets the player up again itself: its routine's step 0 (0222C8) clears the record from +80 to +37F, sets the
// health, the armour, the palette slot and the first animation, calls the character's own setting up (a routine by
// number at BD0FA), and plays no entrance (the player's bit at A5+102 set). What the outgoing character made (objects
// from the game's pools that name its record at +30 or +32, and its links at +28-+2E) is given back first, as the game
// gives an object back, or it would run on with another character in the record (Q-Bee's and Lilith's crashed the
// game). The colours are put in on the next frame. Checked with every pair of the fifteen swapped back and forth under
// made-up play (PROGRESS.md).
//
// For tests, VSAV_SWAP=FRAME:PLAYER:CHARACTER,... (hex character) swaps outside the team battle, at that frame or, with
// VSAV_SWAP_FREE, as soon after as the player can act.
#include "state.h"
#include "machine.h"
#include "vsav.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace cps2recomp {

namespace {
    using namespace vsav;

    // The pools the game takes objects from (016C4E on: each a free list from A5, pushed to on release, 017028 on):
    // where the objects are, how many and how big, the tag at +8 a free one has, the list's pointer and count, and a
    // byte set on release (+3C FF, +0D 1; 0: none)
    struct Pool {
        uint32_t first;
        int count;
        uint32_t size;
        uint8_t tag;
        uint32_t list, left;
        uint32_t extra;
        uint8_t extraValue;
    };
    constexpr Pool kPools[] = {
        {At(0xFF9400), 32, 0x100, 0x04, At(0xFFF966), At(0xFFF9BE), 0x3C, 0xFF},
        {At(0xFFB400), 8, 0x80, 0x08, At(0xFFF96A), At(0xFFF9BF), 0, 0},
        {At(0xFFB800), 32, 0x80, 0x0C, At(0xFFF96E), At(0xFFF9C0), 0x3C, 0xFF},
        {At(0xFFC800), 24, 0x80, 0x10, At(0xFFF972), At(0xFFF9C1), 0x0D, 1},
        {At(0xFFD400), 32, 0x80, 0x14, At(0xFFF976), At(0xFFF9C2), 0, 0},
        {At(0xFFE400), 16, 0x40, 0x18, At(0xFFF97A), At(0xFFF9C3), 0, 0},
        {At(0xFFE800), 6, 0x200, 0x1C, At(0xFFF97E), At(0xFFF9C4), 0, 0},
    };
    constexpr uint32_t kOwner = 0x30;            // word: the record of whoever made the object
    constexpr uint32_t kPlayerOf = 0x32;         // word: the player's record it belongs to, for some (Q-Bee's, Anakaris's)
    constexpr uint32_t kDrawing = 0x3C;          // byte: how the object is drawn (01ADD6); some characters' setting up
                                                 // sets their own (Aulbath 4), the rest keep the 0 a match begins with
    constexpr uint32_t kLinks = 0x28;            // four words of a player's record: objects of its own (01ADF6)
    // Aulbath's: a bit by player (+381) while one of his moves is going on, and the player that set it (045D7C; his
    // setting up clears his own, and the second byte): cleared as his setting up clears them, for one swapped out
    constexpr uint32_t kAulbathMoves = At(0xFF50B0), kAulbathMover = At(0xFF50B1);
    constexpr uint32_t kSkipEntrance = At(0xFF8102); // a bit by player (+381): its setting up plays no entrance
    constexpr uint32_t kPlayerNumber = 0x381;

    // Every live object `owner` made (and everything those made), given back to its pool as the game gives one back;
    // the links to it of every player's record (the opponent's too) cleared
    int GiveBack(uint32_t owner) {
        int given = 0;
        for (const Pool &pool : kPools) {
            for (int n = 0; n < pool.count; n++) {
                const uint32_t object = pool.first + static_cast<uint32_t>(n) * pool.size;
                const uint16_t mine = static_cast<uint16_t>(owner);
                if (!Ram8(object + kActive) || object == owner || (Ram16(object + kOwner) != mine && Ram16(object + kPlayerOf) != mine)) {
                    continue;
                }
                for (uint32_t at = 0; at < 0x10; at += 4) {
                    SetRam32(object + at, 0);
                }
                SetRam8(object + 0x08, pool.tag);
                if (pool.extra) {
                    SetRam8(object + pool.extra, pool.extraValue);
                }
                const uint32_t list = Ram32(pool.list) - 2;
                SetRam16(list, static_cast<uint16_t>(object));
                SetRam32(pool.list, list);
                SetRam8(pool.left, static_cast<uint8_t>(Ram8(pool.left) + 1));
                for (int n = 0; n < kPlayerRecords; n++) {
                    for (uint32_t at = kLinks; at < kLinks + 8; at += 2) {
                        if (Ram16(Player(n) + at) == (object & 0xFFFF)) {
                            SetRam16(Player(n) + at, 0);
                        }
                    }
                }
                given += 1 + GiveBack(object);
            }
        }
        return given;
    }

    // A swap's colours still to come (once the game has set the newcomer up), by player; and players off the stage
    // (let out of the picture, the camera not following them: +134, which setting up clears)
    bool g_colours[2] = {false, false};
    bool g_off[2] = {false, false};
    // Where a newcomer is when swapped in, kept through its setting up (which puts it where a round begins: 552 across,
    // in the picture, for a frame), and put back where the setting up has done the rest (022354)
    bool g_place[2] = {false, false};
    uint32_t g_placeX[2] = {0, 0}, g_placeY[2] = {0, 0};
    // A knocked-out player that has come to lie (held there until the next comes in)
    bool g_lying[2] = {false, false};
    constexpr uint32_t kLying = 0x02020208;

} // namespace

// The player's fighter can be swapped now: a round is being fought, not paused, and it can act (not held, reeling or
// frozen)
bool CanTag(int player) {
    return Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchRound && Ram16(kRoundStep) == kRoundFought &&
           !Ram8(0xFF8000 + static_cast<uint32_t>(kRoundPaused)) && CanAct(player) && !Ram8(Player(player) + kHitStop);
}

// The game has set the player's newcomer up: its routine has gone on from step 0 (it is not run in every frame)
bool TaggedIn(int player) {
    return Ram8(Player(player) + kState) != 0;
}

// The team battle's machines, before each frame: both players begin a match ready, with no entrance (Bishamon's is
// long enough that the other player is free to move well before him)
void TagNoEntrances() {
    SetRam8(kSkipEntrance, static_cast<uint8_t>(Ram8(kSkipEntrance) | 3));
}

// ... and the computer's difficulty, 0-7: the board's setting, and the copy of it a game takes when it begins (008F56)
void TagDifficulty(int level) {
    SetRam8(At(0xFF80A0), static_cast<uint8_t>(level));
    SetRam8(At(0xFF80BE), static_cast<uint8_t>(level));
}

// The player's fighter becomes `character` in that set of its colours (-1: the player's), set up by the game anew
// at its next step
void TagIn(int player, int character, int colours) {
    const uint32_t record = Player(player);
    GiveBack(record);
    for (uint32_t at = kLinks; at < kLinks + 8; at += 2) {
        SetRam16(record + at, 0);
    }
    SetRam8(record + kDrawing, 0);
    const uint8_t number = Ram8(record + kPlayerNumber);
    if (Ram8(kAulbathMoves) & (1u << number)) {
        SetRam8(kAulbathMoves, static_cast<uint8_t>(Ram8(kAulbathMoves) & ~(1u << number)));
        SetRam8(kAulbathMover, 0);
    }
    SetRam8(record + kCharacter, static_cast<uint8_t>(character));
    SetRam8(record + kCharacterAgain, static_cast<uint8_t>(character));
    SetRam8(kSkipEntrance, static_cast<uint8_t>(Ram8(kSkipEntrance) | (1u << number)));
    // (the colours: the set asked for, or the one the player chose; a hidden form has two sets. Else, the other player
    // being the same character in the same set, the next one, as the game gives a second player of the same character.
    // kColourSet is above what is cleared.)
    bool hidden = false;
    for (const Character &c : family::kHidden) {
        hidden = hidden || c.number == character;
    }
    const uint32_t other = Player(1 - player);
    if (colours >= 0) {
        SetRam8(record + kColours, static_cast<uint8_t>(hidden ? colours & 1 : colours % kColourSetsEach));
    } else if (Ram8(other + kCharacter) == character && Ram8(other + kColours) == Ram8(record + kColours)) {
        SetRam8(record + kColours, static_cast<uint8_t>(Ram8(record + kColours) ^ 1));
    }
    const uint8_t *p = Program(kColourSets + 4u * static_cast<uint32_t>(character));
    const uint32_t sets = (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
    SetRam32(record + kColourSet, sets + static_cast<uint32_t>(Ram8(record + kColours)) * kColourSetSize);
    g_place[player] = true;
    g_placeX[player] = Ram32(record + kX);
    g_placeY[player] = Ram32(record + kY);
    SetRam32(record + kState, 0);
    g_colours[player] = true;
    g_lying[player] = false;
}

// The player knocked out, every frame it is down: not hit by anything for the next few frames (+147: the game's own
// time of not being hit, counted down each step; 02246E, and the hit test), and once it lies (the knock-down's step
// 02020208, eight frames of the game's own before it gets up) left lying: its steps are not run (TagStep)
void TagUntouchable(int player) {
    const uint32_t record = Player(player);
    SetRam8(record + kNotHit, 10);
    if (Ram32(record + kState) == kLying) {
        g_lying[player] = true;
    }
}

// At a player's step (frames.cpp's PlayerStep, A6): a knocked-out player that lies is left as it is (true: the step
// is made, by doing nothing)
bool TagStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    return (player == Player(0) && g_lying[0]) || (player == Player(1) && g_lying[1]);
}

// A character's call as it tags out: a sound of its own from its entrance (a voice where it has one, 0x200 on), the
// first that no other character's entrance has (sounds queued, traced: the ring at FF0E0E), the same numbers in
// Vampire Savior 2 and Vampire Hunter 2 for the characters they share. Not listened to. A hidden form takes its own
// character's; Phobos (the two games differ) and Donovan (none of his own) have none.
uint16_t TagCall(int character) {
    switch (character) {
    case 0x00: return 0x021B; // Bulleta
    case 0x01: return 0x0208; // Demitri
    case 0x02: case 0x12: return 0x01C9; // Gallon (Dark Gallon)
    case 0x03: return 0x0332; // Victor
    case 0x04: return 0x0273; // Zabel
    case 0x05: return 0x0228; // Morrigan
    case 0x06: return 0x0349; // Anakaris
    case 0x07: return 0x028A; // Felicia
    case 0x08: case 0x18: return 0x02B5; // Bishamon (Oboro Bishamon)
    case 0x09: return 0x01B8; // Aulbath
    case 0x0A: return 0x01A4; // Sasquatch
    case 0x0C: return 0x0308; // Q-Bee
    case 0x0D: return 0x02CE; // Lei-Lei
    case 0x0E: return 0x023C; // Lilith
    case 0x0F: return 0x02E8; // Jedah
    case 0x11: return 0x031B; // Pyron
    default: return 0;
    }
}

// 022354: a player's setting up has done the rest (the character's own setting up): a newcomer of a swap put back
// where it was swapped in, and out of the picture's edges again if it is off the stage
bool TagSetUp(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const int n = player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
    if (n >= 0 && g_place[n]) {
        g_place[n] = false;
        SetRam32(player + kX, g_placeX[n]);
        SetRam32(player + kY, g_placeY[n]);
        if (g_off[n]) {
            SetRam8(player + kNotFollowed, 0x80);
        }
    }
    return false;
}

// The player let out of the picture, the camera not following it, or not
void TagOffStage(int player, bool off) {
    g_off[player] = off;
    SetRam8(Player(player) + kNotFollowed, off ? 0x80 : 0);
}

// Before each frame: the colours of a swap, the players off the stage, and the tests' swaps
void LiveTagBeforeFrame(Machine &machine) {
    for (int player = 0; player < 2; player++) {
        if (g_off[player]) {
            SetRam8(Player(player) + kNotFollowed, 0x80);
        }
    }
    for (int player = 0; player < 2; player++) {
        if (!g_colours[player] || !TaggedIn(player)) {
            continue;
        }
        // (as 02ADAC does: the set's first palette, and its other three unless the character's bit says not, into the
        // buffer the board shows from at the player's slot, each colour keeping the brightness the buffer had; the
        // game's routine is not called from here, with the CPU stopped inside another)
        g_colours[player] = false;
        const uint32_t record = Player(player);
        M68K &c = machine.cpu;
        const int lines = (kFirstLineOnly >> (Ram8(record + kCharacter) & 31)) & 1 ? 1 : kPalettesInSet;
        uint32_t from = Ram32(record + kColourSet);
        uint32_t to = Ram32(kPaletteBuffer) + static_cast<uint32_t>(Ram8(record + kPaletteSlot)) * kColoursInPalette * 2;
        for (int n = 0; n < lines * kColoursInPalette; n++, from += 2, to += 2) {
            const uint16_t bright = static_cast<uint16_t>(c.Read(to, 1) & 0xF000);
            c.Write(to, 1, static_cast<uint16_t>(Mem16(c, from) | bright));
        }
    }

    static const char *swap = getenv("VSAV_SWAP");
    if (!swap) {
        return;
    }
    static const bool free = getenv("VSAV_SWAP_FREE") != nullptr;
    static int frame = 0, done = 0; // (the entries done, a bit each)
    frame++;
    int index = 0;
    for (const char *entry = swap; entry; entry = strchr(entry, ',') ? strchr(entry, ',') + 1 : nullptr, index++) {
        int at = 0, player = 0, character = 0;
        if (sscanf(entry, "%d:%d:%x", &at, &player, &character) < 3 || (done >> index) & 1) {
            continue;
        }
        if (free ? frame >= at && CanTag(player) : frame == at) {
            done |= 1 << index;
            fprintf(stderr, "swap at %d: player %d %02X -> %02X\n", frame, player, Ram8(Player(player) + kCharacter), character);
            TagIn(player, character, -1);
            break;
        }
    }
}

// The swaps under way, in a save state (online play's rollback puts frames back in the middle of one)
void LiveTagState(Archive &ar) {
    if (ar.version != 0 && ar.version < 11) { // (a save state from before has none)
        return;
    }
    ar.Pod(g_colours);
    ar.Pod(g_place);
    ar.Pod(g_placeX);
    ar.Pod(g_placeY);
    ar.Pod(g_off);
    ar.Pod(g_lying);
}

} // namespace cps2recomp

