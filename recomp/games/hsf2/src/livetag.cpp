// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: swapping a player's fighter in the middle of a fight, for the team battle's tag style
// (fight/battle.h), as Super Turbo's livetag.cpp does it (its story is there: these are its routines, moved). The game
// sets a player up again itself: its routine's step 0 (kFighting 0) takes the character's tables and makes the objects
// of its own (005416, 0053CA), and the computer's sets up its plan too. So a swap is the character's bytes, its colours
// and the steps put at 0; the game does the rest at its next step, where the player is. The objects the old
// character's setting up made are given back to their pool first, as the game gives one back (003BB2). The colours are
// put in on the next frame, as the round's start puts them (0C3DF4). The newcomer is of the version the player had.
//
// A hit that empties the bar of a fighter with a partner left is made a knock-down (a sweep's kind, 0E: cheats.cpp asks
// TagKnockDown where the damage is taken), and the one knocked out lies until the next comes in, not to be hit.
//
// For tests, HSF2_SWAP=FRAME:PLAYER:CHARACTER,... (hex character) swaps outside the team battle, at that frame or, with
// HSF2_SWAP_FREE, as soon after as the player can act.
#include "battle.h"
#include "machine.h"
#include "hsf2.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <cstdlib>
#include <cstring>

namespace cps2recomp {

namespace {
    using namespace hsf2;

    // The pool the characters' own objects are taken from (003B96) and given back to (003BB2): sixty records of 0xC0
    // from FFA754 (set up at 003900), the free ones a list of words from A5+1342, their count at A5+132B. An object
    // names its player at +26 (a word).
    constexpr uint32_t kPool = 0xFFA754, kPoolSize = 0xC0;
    constexpr int kPoolCount = 60;
    constexpr uint32_t kPoolList = 0xFF9342, kPoolLeft = 0xFF932B;
    constexpr uint32_t kOwner = 0x26, kKind = 0x20;
    // The kinds a player's setting up makes: 16 (0053DC, every version but World Warrior's) and 1A (005420, 00543C)
    constexpr uint8_t kMadeKinds[] = {0x16, 0x1A};

    constexpr uint32_t kSuperFlash = 0xFF8C0E; // word: a super's flash, the camera and the players held (Super Turbo's FF8DDC)
    constexpr uint32_t kLieTime = 0x1E;        // byte: frames still to lie, in the reeling's lying step
    constexpr uint32_t kNotHit = 0x1BF;        // byte: not to be hit, not to be thrown
    constexpr uint32_t kHitKind = 0x4B;        // byte: the kind of the hit coming
    constexpr uint8_t kKnockDown = 0x0E;       //   a sweep's: thrown up and lying
    constexpr uint8_t kLyingStep = 0x06;
    constexpr uint32_t kOpponentCharacter = 0x34A; // byte: the other player's character, kept by each (003CCE)
    constexpr uint32_t kRoundsWon = 0x349;         // byte: rounds the player has won (00D678)
    // What a swap clears of the player's record: all of it from +0E to +2FF, as a round's start clears it (0039E8, at
    // every round; the bytes from +300 are the player's for the whole match), but for what only the round's start puts
    // in (005276 on, as Super Turbo's 004F94), which is the player's as it stands:
    // its facing, its health (and before the last hit), its bar and the facing's copy, the place's copies, its gauge.
    // The rest the game makes again as it sets the newcomer up (step 0, and its first standing), as at a round's start.
    // (Less left a character's own state behind in Super Turbo: each character's reading of the controls for its moves
    // keeps its state in +80-+DF; the computer's plan keeps its step at +200.)
    struct Bytes {
        uint32_t from, to;
    };
    constexpr Bytes kKept[] = {{0x012, 0x013}, {0x02A, 0x02E}, {0x1BC, 0x1C0}, {0x1CC, 0x1D4}, {0x2B4, 0x2B5}};
    constexpr uint32_t kClearedFrom = 0x00E, kClearedTo = 0x300;
    constexpr uint32_t kHalfWidth = 0x1C2;         // word: half the player's width, by character (0052D6, the table at 0054B6)
    constexpr uint32_t kHalfWidths = 0x0054B6;

    int Side(uint32_t player) {
        player &= 0xFFFFFF;
        return player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
    }

    // The objects the player's setting up made, given back as 003BB2 gives one back
    void GiveBack(uint32_t player) {
        for (int n = 0; n < kPoolCount; n++) {
            const uint32_t object = kPool + static_cast<uint32_t>(n) * kPoolSize;
            const uint8_t kind = Ram8(object + kKind);
            const bool owned = Ram16(object + kOwner) == (player & 0xFFFF) || Ram32(object + kOwner) == (0xFFFF0000 | (player & 0xFFFF));
            if (!Ram8(object) || !owned || std::find(std::begin(kMadeKinds), std::end(kMadeKinds), kind) == std::end(kMadeKinds)) {
                continue;
            }
            SetRam32(object + 0x00, 0);
            SetRam16(object + 0x04, 0);
            SetRam16(object + 0x0E, 0);
            SetRam32(object + 0x10, 0);
            SetRam16(object + 0x20, 0);
            SetRam16(object + 0x24, 0);
            SetRam16(object + 0x5A, 0);
            SetRam16(object + 0x50, 0);
            SetRam8(object + 0x46, 0);
            const uint32_t list = 0xFF0000 | ((Ram16(kPoolList) - 2) & 0xFFFF);
            SetRam16(list, static_cast<uint16_t>(object));
            SetRam16(kPoolList, static_cast<uint16_t>(list));
            SetRam8(kPoolLeft, static_cast<uint8_t>(Ram8(kPoolLeft) + 1));
        }
    }

    // A swap's colours still to come (once the game has set the newcomer up), by player; the panel's faces to be made
    // again; players off the stage
    bool g_colours[2] = {false, false};
    bool g_faces = false;
    bool g_off[2] = {false, false};

    // The player's set of colours copied to the palettes the board shows, as 0C3DF4 copies it (the version's table, the
    // character's list, the set: its count of palettes, then the palettes), the brightness the game has put in each word
    // kept
    void Colour(M68K &c, int player) {
        const uint32_t record = Player(player);
        constexpr uint32_t kTable = 0x0C3E70;
        const uint32_t versions = kTable + Mem16(c, kTable + Ram8(record + kVersion));
        const uint32_t at = versions + Ram8(record + kCharacter) * 4u;
        const uint32_t list = (static_cast<uint32_t>(Mem16(c, at)) << 16) | Mem16(c, at + 2);
        uint32_t from = list + Mem16(c, list + Ram8(record + kColourChoice) * 2u);
        const uint32_t count = Mem16(c, from);
        from += 2;
        uint32_t to = player ? 0x9000C0 : 0x900020;
        for (uint32_t n = 0; n < count * 0x10; n++, from += 2, to += 2) {
            const uint16_t bright = static_cast<uint16_t>(c.Read(to, 1) & 0xF000);
            c.Write(to, 1, static_cast<uint16_t>((Mem16(c, from) & 0x0FFF) | bright));
        }
    }
} // namespace

// The player's fighter can be swapped now: a round is being fought, no super flashing, and it can act (not held,
// reeling, frozen or in the air)
bool CanTag(int player) {
    return Fought() && !Ram16(kSuperFlash) &&
           CanAct(Player(player)) && !Ram8(Player(player) + kHitStop) && !Ram8(Player(player) + kInAir);
}

namespace {

// A swap asked for and not yet made, by player: it is made where the player's routine begins (066964, TagFaces), not
// at a frame's edge. The game's frames fill the processor, so a frame's edge can come in the middle of its drawing of
// the player or of its routine, and those then went on with the record cleared under them (drawing through a frame
// pointer of 0 went round for ever).
struct Pending {
    bool asked = false;
    int character = 0, colours = -1;
};
Pending g_pending[2];

// The player's fighter becomes `character` in that set of its colours (-1: the player's), set up by the game anew as
// its routine goes on
void Swap(int player, int character, int colours) {
    const uint32_t record = Player(player);
    GiveBack(record);
    // (a character the player's version has not got is taken in the Super Turbo version, which has them all; the old
    // versions have two sets of colours, a punch's and a kick's)
    const uint8_t version = Ram8(record + kVersion);
    const int most = version == kSuperTurbo || version == kSuper ? 16 : version == kWorldWarrior ? 8 : 12;
    if (character >= most) {
        SetRam8(record + kVersion, kSuperTurbo);
    }
    const int sets = Ram8(record + kVersion) == kSuperTurbo ? 6 : 2;
    SetRam8(record + kCharacter, static_cast<uint8_t>(character));
    const uint32_t other = Player(1 - player);
    if (colours >= 0) {
        SetRam8(record + kColourChoice, static_cast<uint8_t>(colours % sets));
    } else if (Ram8(record + kColourChoice) >= sets) {
        SetRam8(record + kColourChoice, 0);
    }
    if (Ram8(other + kCharacter) == character && Ram8(other + kVersion) == Ram8(record + kVersion) &&
        Ram8(other + kColourChoice) == Ram8(record + kColourChoice)) {
        SetRam8(record + kColourChoice, static_cast<uint8_t>((Ram8(record + kColourChoice) + 1) % sets));
    }
    // (each player keeps the other's character, copied as a round starts: 003CCE; in Super Turbo T. Hawk's moves against
    // M. Bison read it)
    SetRam8(other + kOpponentCharacter, static_cast<uint8_t>(character));
    for (uint32_t at = kClearedFrom; at < kClearedTo; at++) {
        bool kept = false;
        for (const Bytes &bytes : kKept) {
            kept = kept || (at >= bytes.from && at < bytes.to);
        }
        if (!kept) {
            SetRam8(record + at, 0);
        }
    }
    SetRam16(record + kHalfWidth, static_cast<uint16_t>(static_cast<int8_t>(Program(kHalfWidths + static_cast<uint32_t>(character))[0])));
    (void)kRoundsWon;
    SetRam32(record + kFighting, 0); // (+02 to +05: the routine's step, and the fight's)
    SetRam8(record + kNotHit, 0);
    g_colours[player] = true;
}

} // namespace

// The game has set the player's newcomer up: its routine has gone on from step 0
bool TaggedIn(int player) {
    return !g_pending[player].asked && Ram8(Player(player) + kFighting) != 0;
}

// The player's fighter becomes `character` in that set of its colours (-1: the player's), as the player's routine next
// begins
void TagIn(int player, int character, int colours) {
    g_pending[player] = {true, character, colours};
}

// The team battle's computer difficulty, 0-7: the board's setting (the game reads it as it plays)
void TagDifficulty(int level) {
    SetRam8(0xFF82BC, static_cast<uint8_t>(level));
}
int TagDifficultyNow() {
    return Ram8(0xFF82BC);
}

// The player let out of the picture, the camera not following it, or not (widescreen.cpp's KeepIn and camera)
void TagOffStage(int player, bool off) {
    g_off[player] = off;
}
bool TagIsOff(uint32_t player) {
    const int n = Side(player);
    return n >= 0 && g_off[n];
}

// The player knocked out, every frame it is down: not hit, and once it lies, kept lying
void TagUntouchable(int player) {
    const uint32_t record = Player(player);
    SetRam8(record + kNotHit, 1);
    if (Ram8(record + kState) == kReeling && Ram8(record + kStateStep) == kLyingStep) {
        SetRam8(record + kLieTime, 8);
    }
}

// A hit about to take a fighter's health to nothing with a partner left (cheats.cpp): it lands as a knock-down
void TagKnockDown(uint32_t player) {
    SetRam8((player & 0xFFFFFF) + kHitKind, kKnockDown);
}

// 00D5F8: a time over, the round's result about to be decided by the two players' health (00D604 compares them, and
// 00D666 gives the round to the player at A6, A4 the other; 00D62E is a draw). In the tag style it is the teams' (the
// toolkit's), and one round decides the match: the winner's rounds won (+349) put one short first.
bool TagTimeOver(M68K &c) {
    const int result = TeamTimeOverResult();
    if (!result) {
        return false;
    }
    if (result == 1 || result == 2) {
        const int winner = result - 1;
        SetRam8(Player(winner) + kRoundsWon, 1);
        c.a[6] = 0xFF000000u | Player(winner);
        c.a[4] = 0xFF000000u | Player(1 - winner);
        c.pc = 0x00D666;
    } else {
        c.a[6] = 0xFF000000u | Player(0);
        c.a[4] = 0xFF000000u | Player(1);
        c.pc = 0x00D62E;
    }
    return true;
}
// 00D666: a side (A6) has won the round. In the tag style that is the match (its last fighter knocked out).
bool TagRoundWon(M68K &c) {
    if (TeamBattle() && TeamTimeOverResult() != 0) {
        SetRam8((c.a[6] & 0xFFFFFF) + kRoundsWon, 1);
    }
    return false;
}

// Before each frame: the colours of a swap, and the tests' swaps
void LiveTagBeforeFrame(Machine &machine) {
    if (!TeamBattle()) {
        g_off[0] = g_off[1] = false;
    }
    if (!InMatch()) {
        g_pending[0] = g_pending[1] = {}; // (none kept past the match)
    }
    for (int player = 0; player < 2; player++) {
        if (g_colours[player] && TaggedIn(player)) {
            g_colours[player] = false;
            Colour(machine.cpu, player);
            g_faces = true;
        }
    }

    static const char *swap = getenv("HSF2_SWAP");
    if (!swap) {
        return;
    }
    static const bool free = getenv("HSF2_SWAP_FREE") != nullptr;
    static int frame = 0, done = 0; // (the entries done, a bit each)
    static int wanted[2] = {-1, -1}, since[2] = {0, 0};
    static bool playing = false, reported = false;
    frame++;
    // (what the tests look for: the board reset, a swap not set up in time, a player not the character swapped to)
    playing = playing || InMatch();
    if (playing && !InGame() && !reported) {
        reported = true;
        fprintf(stderr, "swap test: reset at %d\n", frame);
    }
    for (int player = 0; player < 2; player++) {
        if (wanted[player] >= 0 && !g_pending[player].asked && Ram8(Player(player) + kCharacter) != wanted[player] && !reported) {
            reported = true;
            fprintf(stderr, "swap test: player %d is %02X at %d, not %02X\n", player, Ram8(Player(player) + kCharacter), frame, wanted[player]);
        }
        if (since[player] > 0 && ++since[player] > 60 && !TaggedIn(player) && !reported) {
            reported = true;
            fprintf(stderr, "swap test: player %d not set up at %d\n", player, frame);
        }
        if (TaggedIn(player)) {
            since[player] = 0;
        }
    }
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
            wanted[player] = character;
            since[player] = 1;
            break;
        }
    }
}

// 066964: a player's routine (the game's own code running, not a frame's edge): a swap asked for made; the panel made
// again after a swap, as a round's start makes it (011B48: the faces' palettes and objects, the names, by each
// player's character and version). The game is sent through it in its own code, as if called from here, and comes back
// to this routine. Not called from C++: the board's time it takes is counted as the game's own code counts it, and the
// interpreted run of the same code is alike.
bool TagFaces(M68K &c) {
    for (int player = 0; player < 2; player++) {
        if (g_pending[player].asked && (c.a[6] & 0xFFFFFF) == (Player(player) & 0xFFFFFF)) {
            g_pending[player].asked = false;
            Swap(player, g_pending[player].character, g_pending[player].colours);
        }
    }
    if (!g_faces) {
        return false;
    }
    g_faces = false;
    c.Push32(0x066964); // (back here)
    c.pc = 0x011B48;
    return true;
}

void RegisterLiveTag() {
    AddHook(HookAt(0x066964), "TagFaces", TagFaces);
    AddHook(HookAt(0x00D5F8), "TagTimeOver", TagTimeOver);
    AddHook(HookAt(0x00D666), "TagRoundWon", TagRoundWon);
}

} // namespace cps2recomp
