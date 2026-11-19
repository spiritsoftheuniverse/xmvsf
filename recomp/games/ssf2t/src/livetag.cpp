// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: swapping a player's fighter in the middle of a fight, for the team battle's tag style
// (fight/battle.h), as Street Fighter Alpha 2's livetag.cpp does it. The game sets a player up again itself: its
// routine's step 0 (kFighting 0: 078282 a person's, 07BB34 the computer's) takes the character's tables (005078: its
// boxes, its sizes, and an object of its own for E. Honda and two for Dee Jay) and makes its shadow (00503C), and the
// computer's sets up its plan too. So a swap is the character's bytes, its colours and the steps put at 0; the game
// does the rest at its next step, where the player is. The objects the old character's setting up made (its shadow,
// E. Honda's, Dee Jay's) are given back to their pool first, as the game gives one back (003794), or there would be two
// shadows and another character's own object left running. The colours are put in on the next frame, as the round's
// start puts them (0BFA78).
//
// A hit that empties the bar of a fighter with a partner left is made a knock-down (a sweep's kind, 0E: cheats.cpp asks
// TagKnockDown where the damage is taken), and the one knocked out lies until the next comes in, not to be hit.
//
// For tests, SSF2T_SWAP=FRAME:PLAYER:CHARACTER,... (hex character) swaps outside the team battle, at that frame or, with
// SSF2T_SWAP_FREE, as soon after as the player can act.
#include "battle.h"
#include "machine.h"
#include "ssf2t.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <cstdlib>
#include <cstring>

namespace cps2recomp {

namespace {
    using namespace ssf2t;

    // The pool the shadows and the characters' own objects are taken from (003778) and given back to (003794): sixty
    // records of 0xC0 from FFA9A2, the free ones a list of words from A5+1580, their count at A5+154C (read off the list
    // in a fight). An object names its player at +26 (a word; Dee Jay's a long, the word after: 005144).
    constexpr uint32_t kPool = At(0xFFA9A2), kPoolSize = 0xC0;
    constexpr int kPoolCount = 60;
    constexpr uint32_t kPoolList = At(0xFF9580), kPoolLeft = At(0xFF954C);
    constexpr uint32_t kOwner = 0x26, kKind = 0x20;
    // The kinds a player's setting up makes: its shadow (00503C), E. Honda's (005116), Dee Jay's (005134)
    constexpr uint8_t kMadeKinds[] = {0x8F, 0x4F, 0x69};

    constexpr uint32_t kSuperFlash = At(0xFF8DDC); // word: a super's flash, the camera and the players held (0CC640)
    constexpr uint32_t kLieTime = 0x1E;        // byte: frames still to lie, in the reeling's lying step (07948E)
    constexpr uint32_t kNotHit = 0x1BF;        // byte: not to be hit, not to be thrown (0BE58C, 0BEB40, 0046D4)
    constexpr uint32_t kHitKind = 0x4B;        // byte: the kind of the hit coming (07AD10 copies it to +55)
    constexpr uint8_t kKnockDown = 0x0E;       //   a sweep's: thrown up and lying (07932E on)
    constexpr uint8_t kLyingStep = 0x06;
    constexpr uint32_t kOpponentCharacter = family::kOpponentCharacter; // byte: the other player's character, kept by each (00386E)
    // What a swap clears of the player's record: all of it from +0E to +37F, as a round's start clears it (003616, at
    // every round), but for what only the round's start puts in (004F94, 003EB6), which is the player's as it stands:
    // its facing, its health (and before the last hit), its bar and the facing's copy, the place's copies, its gauge.
    // The rest the game makes again as it sets the newcomer up (step 0, and its first standing), as at a round's start.
    // (Less left a character's own state behind: each character's reading of the controls for its moves keeps its state
    // in +80-+DF, M. Bison's at 08C19A, and T. Hawk's went off into the ROM's data on what M. Bison's left there; the
    // computer's plan keeps its step at +200, 07BC22.)
    struct Bytes {
        uint32_t from, to;
    };
    constexpr Bytes kKept[] = {{0x012, 0x013}, {0x02A, 0x02E}, {0x1BC, 0x1C0}, {0x1CC, 0x1D4}, {0x2B4, 0x2B5}};
    constexpr uint32_t kClearedFrom = 0x00E, kClearedTo = family::kClearedTo;
    constexpr uint32_t kBar = 0x1BC;               // word: the health as the bar draws it (01006A steps it down to the health)
    constexpr uint32_t kHalfWidth = 0x1C2;         // word: half the player's width, by character (004F94, the table at 00502A)
    constexpr uint32_t kHalfWidths = At(0x00502A);

    int Side(uint32_t player) {
        player &= 0xFFFFFF;
        return player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
    }

    // The objects the player's setting up made, given back as 003794 gives one back
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

    // The player's set of colours copied to the palettes the board shows, as 0BFA78 copies it (its count of palettes,
    // then the palettes), the brightness the game has put in each word kept
    void Colour(M68K &c, int player) {
        const uint32_t record = Player(player);
        const int who = IsAkuma(record) ? 0x10 : Ram8(record + kCharacter);
        int sets = 0;
        uint32_t from = family::ColourSet(who, Ram8(record + kColourChoice), sets);
        const uint32_t count = static_cast<uint32_t>(sets);
        uint32_t to = family::ColourShown(player, who, Ram8(record + kColourChoice));
        for (uint32_t n = 0; n < count * 0x10; n++, from += 2, to += 2) {
            const uint16_t bright = static_cast<uint16_t>(c.Read(to, 1) & 0xF000);
            c.Write(to, 1, static_cast<uint16_t>((Mem16(c, from) & 0x0FFF) | bright));
        }
    }
} // namespace

// The player's fighter can be swapped now: a round is being fought, no super flashing, and it can act (not held,
// reeling, frozen or in the air)
bool CanTag(int player) {
    return Ram16(kMode) == kModeGame && Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchFought && !Ram16(kSuperFlash) &&
           CanAct(Player(player)) && !Ram8(Player(player) + kHitStop) && !Ram8(Player(player) + kInAir);
}

namespace {

// A swap asked for and not yet made, by player: it is made where the player's routine begins (078030, TagFaces), not
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
    SetRam8(record + kCharacter, static_cast<uint8_t>(character));
    NotHidden(record);
    const uint32_t other = Player(1 - player);
    if (colours >= 0) {
        SetRam8(record + kColourChoice, static_cast<uint8_t>(colours % 6));
    } else if (Ram8(record + kColourChoice) > 5) {
        SetRam8(record + kColourChoice, 0);
    }
    if (Ram8(other + kCharacter) == character && !IsAkuma(other) && !IsOld(other) &&
        Ram8(other + kColourChoice) == Ram8(record + kColourChoice)) {
        SetRam8(record + kColourChoice, static_cast<uint8_t>((Ram8(record + kColourChoice) + 1) % 6));
    }
    // (each player keeps the other's character, copied as a round starts: 00386E; T. Hawk's moves against M. Bison read
    // it, and with the old one there went off into the ROM's data)
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
    SetRam8(At(0xFF82D3), static_cast<uint8_t>(level));
}
int TagDifficultyNow() {
    return Ram8(At(0xFF82D3));
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

// 00FDEE: a time over, the round's result about to be decided by the two players' health. In the tag style it is the
// teams' (the toolkit's), and one round decides the match: the winner's rounds won (+390) put one short first.
bool TagTimeOver(M68K &c) {
    const int result = TeamTimeOverResult();
    if (!result) {
        return false;
    }
    SetLow16(c.d[0], Ram16(Player(0) + kHealth));
    if (result == 1) {
        SetRam8(Player(0) + family::kTagged, 1);
        c.pc = At(0x00FCE2);
    } else if (result == 2) {
        SetRam8(Player(1) + family::kTagged, 1);
        c.pc = At(0x00FD62);
    } else {
        c.pc = At(0x00FE0C);
    }
    return true;
}
// 00FCE6 and 00FD66: a side has won the round. In the tag style that is the match (its last fighter knocked out).
bool TagRoundWon(M68K &c) {
    if (TeamBattle() && TeamTimeOverResult() != 0) {
        SetRam8(Player(c.pc == At(0x00FCE6) ? 0 : 1) + family::kTagged, 1);
    }
    return false;
}

// Before each frame: the colours of a swap, and the tests' swaps
void LiveTagBeforeFrame(Machine &machine) {
    if (!TeamBattle()) {
        g_off[0] = g_off[1] = false;
    }
    if (Ram16(kMode) != kModeGame || Ram16(kGameStep) != kGameMatch) {
        g_pending[0] = g_pending[1] = {}; // (none kept past the match)
    }
    for (int player = 0; player < 2; player++) {
        if (g_colours[player] && TaggedIn(player)) {
            g_colours[player] = false;
            Colour(machine.cpu, player);
            g_faces = true;
        }
        // The bar as drawn (kBar) only steps down to the health (00E33A/01006A: it is stepped by one until the two are
        // alike), so a newcomer's health put back above it would have it go on down past nothing, the bar drawn empty:
        // it is put at the health then, as a round's start puts it
        const uint32_t record = Player(player);
        const int16_t health = static_cast<int16_t>(Ram16(record + kHealth)), bar = static_cast<int16_t>(Ram16(record + kBar));
        if (TeamBattle() && Ram16(kMode) == kModeGame && Ram16(kGameStep) == kGameMatch && health >= 0 && health <= kHealthFull &&
            bar < health) {
            SetRam16(record + kBar, static_cast<uint16_t>(health));
        }
    }

    static const char *swap = getenv("SSF2T_SWAP");
    if (!swap) {
        return;
    }
    static const bool free = getenv("SSF2T_SWAP_FREE") != nullptr;
    static int frame = 0, done = 0; // (the entries done, a bit each)
    static int wanted[2] = {-1, -1}, since[2] = {0, 0};
    static bool playing = false, reported = false;
    frame++;
    // (what the tests look for: the board reset, a swap not set up in time, a player not the character swapped to)
    playing = playing || (Ram16(kMode) == kModeGame && Ram16(kGameStep) == kGameMatch);
    if (playing && Ram16(kMode) != kModeGame && !reported) {
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

// 078030: a player's routine (the game's own code running, not a frame's edge): a swap asked for made; the panel's faces and names of both
// players made again after a swap, as a round's start makes them (0104B2: the faces' palettes, and their objects in the
// panel's part of the object list, by each player's character; 0102F2 the names). The game is sent through them in its
// own code, as if called from here, and comes back to this routine: they change no register its callers keep (they
// keep A0 and A1 in memory, and only A6 is read after; neither routine uses A6). Not called from C++: the board's time
// they take is counted as the game's own code counts it, and the interpreted run of the same code is alike.
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
    c.Push32(At(0x078030)); // (back here)
    if constexpr (At(0x0104B2) != 0) {
        c.Push32(At(0x0102F2)); // (the names, after the faces)
        c.pc = At(0x0104B2);
    } else {
        c.pc = At(0x0102F2); // (a game whose faces are not made apart from its names: Super Street Fighter II's)
    }
    return true;
}

void RegisterLiveTag() {
    AddHook(HookAt(At(0x078030)), "TagFaces", TagFaces);
    AddHook(HookAt(At(0x00FDEE)), "TagTimeOver", TagTimeOver);
    AddHook(HookAt(At(0x00FCE6)), "TagRoundWon", TagRoundWon);
    AddHook(HookAt(At(0x00FD66)), "TagRoundWon", TagRoundWon);
}

} // namespace cps2recomp
