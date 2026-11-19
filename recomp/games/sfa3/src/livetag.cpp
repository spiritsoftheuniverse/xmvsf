// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: swapping a player's fighter in the middle of a fight, for the team battle's tag style
// (fight/battle.h), as Alpha 2's livetag.cpp does it. The game sets the player up again itself: its routine's step 0
// (027B66) clears +80 to +FF and +180 to +3FF and a few bytes more (the character +102, the colour choice +128 and the
// ism +132 are kept), sets the health, calls the character's own setting up (a routine by number, the table at 0DD192:
// 027C54) and, in a match's first round (+159 set), its entrance (027C72). So a swap is what the outgoing character
// made given back, the character's byte, its colours and the state put at 0; the game does the rest at its next step,
// with no entrance and where the player was (hooks). The colours are put in on the next frame.
//
// For tests, SFA3_SWAP=FRAME:PLAYER:CHARACTER,... (hex character) swaps outside the team battle, at that frame or, with
// SFA3_SWAP_FREE, as soon after as the player can act; SFA3_TAG_LOG writes the characters and the lying as they change.
#include "state.h"
#include "battle.h"
#include "machine.h"
#include "sfa3.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace cps2recomp {

uint32_t CharacterColourSet(int number, int set); // colours.cpp

namespace {
    using namespace sfa3;

    // The pools the game takes objects from (each a list of free ones from A5, made at 01C046 on; taken from at 01C1C8 on
    // and given back at 01C206 on): where the objects are, how many and how big, the tag at +8 a free one has, the list's
    // pointer and count, and what the giving back clears besides +00-+0F
    struct Pool {
        uint32_t first;
        int count;
        uint32_t size;
        uint8_t tag;
        uint32_t list, left;
        bool thrown;   // the things thrown: +68, +5B, +30, +26, +28-+2F, +BD, +D7, +CC, +EE, +EF, +3E; two lists, by +BF
        bool clears28; // +30, +26, +28-+2F
        bool clears26; // +30, +26
    };
    constexpr Pool kPools[] = {
        {0xFF9400, 12, 0x100, 0x08, 0xFFEE84, 0xFFEEB6, true, true, true},
        {0xFFA000, 12, 0x100, 0x08, 0xFFEE86, 0xFFEEB7, true, true, true},
        {0xFFAC00, 8, 0x80, 0x0C, 0xFFEE88, 0xFFEEB8, false, true, true},
        {0xFFB000, 32, 0x80, 0x10, 0xFFEE8A, 0xFFEEB9, false, true, true},
        {0xFFC000, 16, 0x80, 0x14, 0xFFEE8C, 0xFFEEBA, false, true, true},
        {0xFFC800, 16, 0x80, 0x18, 0xFFEE8E, 0xFFEEBB, false, false, true},
        {0xFFD000, 7, 0x200, 0x1C, 0xFFEE90, 0xFFEEBC, false, false, false},
    };
    constexpr uint32_t kOwner = 0x36;     // word: the record of whoever made the object (a player's or another object's)
    constexpr uint32_t kSecondList = 0xBF; // a thing thrown: not zero, it goes back to the second list
    // Words of a player's record that name objects of its own (cleared as a round starts: 01BF80)
    constexpr uint32_t kLinks[] = {0x28, 0x2A, 0x2C, 0x2E};

    // Every live object `owner` made (and everything those made), given back to its pool as the game gives one back;
    // the links to it of both players' records cleared
    int GiveBack(uint32_t owner) {
        int given = 0;
        for (const Pool &pool : kPools) {
            for (int n = 0; n < pool.count; n++) {
                const uint32_t object = pool.first + static_cast<uint32_t>(n) * pool.size;
                if (!Ram8(object + kActive) || object == owner || Ram16(object + kOwner) != (owner & 0xFFFF)) {
                    continue;
                }
                const bool second = pool.thrown && Ram8(object + kSecondList) != 0;
                for (uint32_t at = 0; at < 0x10; at += 4) {
                    SetRam32(object + at, 0);
                }
                if (pool.thrown) {
                    for (const uint32_t at : {0x68u, 0x5Bu, 0xBDu, 0xD7u, 0xCCu, 0xEEu, 0xEFu, 0x3Eu}) {
                        SetRam8(object + at, 0);
                    }
                }
                if (pool.clears26) {
                    SetRam8(object + 0x30, 0);
                    SetRam16(object + 0x26, 0);
                }
                if (pool.clears28) {
                    SetRam32(object + 0x28, 0);
                    SetRam32(object + 0x2C, 0);
                }
                SetRam8(object + 0x08, pool.tag);
                const Pool &to = second ? kPools[1] : pool;
                const uint32_t list = 0xFF0000 | ((Ram16(to.list) - 2) & 0xFFFF);
                SetRam16(list, static_cast<uint16_t>(object));
                SetRam16(to.list, static_cast<uint16_t>(list));
                SetRam8(to.left, static_cast<uint8_t>(Ram8(to.left) + 1));
                for (int p = 0; p < 2; p++) {
                    for (uint32_t at : kLinks) {
                        if (Ram16(Player(p) + at) == (object & 0xFFFF)) {
                            SetRam16(Player(p) + at, 0);
                        }
                    }
                }
                given += 1 + GiveBack(object);
            }
        }
        return given;
    }

    // A swap's colours still to come (once the game has set the newcomer up), by player
    bool g_colours[2] = {false, false};
    // A newcomer of a swap: set up with no entrance, and put back where it was swapped in
    bool g_place[2] = {false, false};
    uint32_t g_placeX[2] = {0, 0}, g_placeY[2] = {0, 0};

    // Players off the stage: let out of the picture, the camera not following them (widescreen.cpp's KeepIn and camera)
    bool g_off[2] = {false, false};
    // A knocked-out player lying (the knock-down's step 8, counted down by +DF), kept lying until the next comes in; and
    // not hit meanwhile (+25D, the game's own time of not being hit, counted down at the player's step: 027CE2)
    constexpr uint32_t kLying = 0x02020208;
    constexpr uint32_t kLieSteps = 0xDF, kNotHit = 0x25D;
    constexpr uint32_t kEntrance = 0x159;   // byte: the player is to make its entrance (a match's first round)
    constexpr uint32_t kIsm = 0x132;        // byte: FF X-ism, 0 A-ism, 1 V-ism
    constexpr uint32_t kColours = 0x14C, kColoursTo = 0x150; // longs: the set of colours, and where it goes (01F95C)
    constexpr int kColourSetsEach = 6;
    constexpr uint32_t kColourSetSize = 5 * 16 * 2;

    // The panel (an object of the last pool, its kind +2 6) has the names under the bars and the faces beside them in its
    // record, made as a round begins from the characters (0B9442, 0B958C): a name four words a player from +56 (a tile
    // by character from 0B97EC, its attributes 0316, a word by character from 0B986C for player 1 and 0B98EC for player
    // 2, 0023; the tables 0x40 on outside Japan, kRegion), a face two longs a player from +46 (from 0B996C for player
    // 1, 0B9A7C for player 2, eight bytes a character, Chun-Li's in the X-ism the 0x20th; the first long with the
    // player's colour set in it). Put right for a newcomer of a swap as the round's beginning would have them (in the usual mode of play).
    constexpr uint32_t kPanelNameTiles = 0x0B97EC, kPanelNames[2] = {0x0B986C, 0x0B98EC}, kPanelFaces[2] = {0x0B996C, 0x0B9A7C};
    constexpr uint32_t kPlayMode = 0xFF8138;
    void Panel(M68K &c, int player) {
        if (Ram16(kPlayMode) != 0) {
            return;
        }
        const Pool &pool = kPools[6];
        for (int n = 0; n < pool.count; n++) {
            const uint32_t object = pool.first + static_cast<uint32_t>(n) * pool.size;
            if (!Ram8(object + kActive) || Ram8(object + 0x02) != 6) {
                continue;
            }
            const uint32_t record = Player(player);
            const uint8_t who = Ram8(record + kCharacter);
            const uint32_t tables = Ram16(kRegion) ? 0x40 : 0;
            const uint32_t name = object + 0x56 + static_cast<uint32_t>(player) * 8;
            SetRam16(name, Mem16(c, kPanelNameTiles + tables + who * 2u));
            SetRam16(name + 2, Ram8(record + 0xEB) ? 0x0317 : 0x0316);
            SetRam16(name + 4, Mem16(c, kPanelNames[player] + tables + who * 2u));
            SetRam16(name + 6, 0x0023);
            const int face = who == 4 && (Ram8(record + kIsm) & 0x80) ? 0x20 : who;
            const uint32_t from = kPanelFaces[player] + static_cast<uint32_t>(face) * 8;
            const uint32_t to = object + 0x46 + static_cast<uint32_t>(player) * 8;
            SetRam32(to, Mem32(c, from) | (Ram8(record + kColourSet) & 0x1F));
            SetRam32(to + 4, Mem32(c, from + 4));
        }
    }

    int Side(uint32_t player) {
        player &= 0xFFFFFF;
        return player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
    }
} // namespace

// The player's fighter can be swapped now: a round is being fought and it can act (not held, reeling or frozen), on the
// floor
bool CanTag(int player) {
    return Ram16(kGameStep) == kGameMatch && Ram16(kMatchStep) == kMatchRound && Ram16(kRoundStep) == kRoundFought &&
           CanAct(Player(player)) && !Ram8(Player(player) + kHitStop) && !Ram8(Player(player) + kInAir) &&
           !Ram8(Player(1 - player) + kHitStop);
}

// The game has set the player's newcomer up: its routine has gone on from step 0
bool TaggedIn(int player) {
    return Ram8(Player(player) + kState) != 0;
}

// The player's fighter becomes `character` in that set of its colours (-1: the player's), set up by the game anew at its
// next step. (The colours' choice is the punch or the kick; the ism, which is how the fighter plays, stays the side's.)
void TagIn(int player, int character, int colours) {
    const uint32_t record = Player(player);
    GiveBack(record);
    SetRam8(record + kCharacter, static_cast<uint8_t>(character));
    if (colours >= 0) {
        SetRam8(record + kColourChoice, static_cast<uint8_t>(colours % 2));
    } else {
        const uint32_t other = Player(1 - player);
        if (Ram8(other + kCharacter) == character && Ram8(other + kColourChoice) == Ram8(record + kColourChoice) &&
            Ram8(other + kIsm) == Ram8(record + kIsm)) {
            SetRam8(record + kColourChoice, static_cast<uint8_t>(Ram8(record + kColourChoice) ^ 1));
        }
    }
    g_place[player] = true;
    g_placeX[player] = Ram32(record + kX);
    g_placeY[player] = Ram32(record + kY);
    SetRam32(record + kState, 0);
    g_colours[player] = true;
    SetRam8(record + kLieSteps, 0);
}

// The team battle's computer difficulty, 0-7: the board's setting, and the copy of it a game takes when it begins
// (008D70)
void TagDifficulty(int level) {
    SetRam8(0xFF80A0, static_cast<uint8_t>(level));
    SetRam8(0xFF80BE, static_cast<uint8_t>(level));
}

// The player let out of the picture, the camera not following it, or not
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
    SetRam8(record + kNotHit, 10);
    if (Ram32(record + kState) == kLying) {
        SetRam8(record + kLieSteps, 8); // (more than one: a turbo speed's frame may make two steps)
    }
}

// A character's call as it tags out (none found yet for this game)
uint16_t TagCall(int) {
    return 0;
}

namespace {
    // 027C72: the setting up of a player, which in a match's first round goes on to the player's entrance: a newcomer of
    // a swap, and every fighter of the team battle, begins ready instead (as in the later rounds: 027C3C)
    bool TagEntrance(M68K &c) {
        const int n = Side(c.a[6]);
        if (n < 0 || (!g_place[n] && !TeamBattle()) || !Ram8((c.a[6] & 0xFFFFFF) + kEntrance)) {
            return false;
        }
        c.pc = 0x027C3C;
        return true;
    }

    // 027C68: the setting up has done the rest (the character's own): a newcomer of a swap put back where it was swapped in
    bool TagSetUp(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = Side(player);
        if (n >= 0 && g_place[n]) {
            g_place[n] = false;
            SetRam32(player + kX, g_placeX[n]);
            SetRam32(player + kY, g_placeY[n]);
        }
        return false;
    }
} // namespace

// Before each frame: the colours of a swap, and the tests' swaps
void LiveTagBeforeFrame(Machine &machine) {
    if (!TeamBattle()) {
        g_off[0] = g_off[1] = false;
    } else if (TeamTimeOverResult()) {
        // (the tag style: one round decides the match; the game's copy of the board's max round, taken as a game begins:
        // 008D7C)
        SetRam8(0xFF8108, 1);
    }
    for (int player = 0; player < 2; player++) {
        if (!g_colours[player] || !TaggedIn(player)) {
            continue;
        }
        // (as the round's start does, 01F95C: the character's set by the player's ism and choice kept at kColours, and
        // its five palettes copied to where the player's go, kColoursTo; each colour keeping the buffer's brightness)
        g_colours[player] = false;
        const uint32_t record = Player(player);
        M68K &c = machine.cpu;
        Panel(c, player);
        const int set = (static_cast<int8_t>(Ram8(record + kIsm)) + 1) * 2 + (Ram8(record + kColourChoice) & 1);
        uint32_t from = CharacterColourSet(Ram8(record + kCharacter), set % kColourSetsEach);
        SetRam32(record + kColours, from);
        uint32_t to = Ram32(record + kColoursTo) & 0xFFFFFF;
        if (to < 0x900000 || to >= 0x930000) {
            continue;
        }
        for (uint32_t n = 0; n < kColourSetSize; n += 2, from += 2, to += 2) {
            const uint16_t bright = static_cast<uint16_t>(c.Read(to, 1) & 0xF000);
            c.Write(to, 1, static_cast<uint16_t>((Mem16(c, from) & 0x0FFF) | bright));
        }
    }

    static const bool tagLog = getenv("SFA3_TAG_LOG") != nullptr;
    if (tagLog) {
        static int logFrame = 0, mode = -1, character[2] = {-1, -1};
        static bool lying[2] = {false, false};
        logFrame++;
        if (Ram16(kMode) != mode) {
            mode = Ram16(kMode);
            fprintf(stderr, "tag log %d: mode %d\n", logFrame, mode);
        }
        for (int n = 0; n < 2; n++) {
            if (Ram8(Player(n) + kCharacter) != character[n]) {
                character[n] = Ram8(Player(n) + kCharacter);
                fprintf(stderr, "tag log %d: player %d is %02X (health %d)\n", logFrame, n, character[n],
                        static_cast<int16_t>(Ram16(Player(n) + kHealth)));
            }
            if ((Ram32(Player(n) + kState) == kLying) != lying[n]) {
                lying[n] = !lying[n];
                fprintf(stderr, "tag log %d: player %d %s\n", logFrame, n, lying[n] ? "lies" : "up");
            }
        }
    }
    static const char *swap = getenv("SFA3_SWAP");
    if (!swap) {
        return;
    }
    static const bool free = getenv("SFA3_SWAP_FREE") != nullptr;
    static int frame = 0, done = 0; // (the entries done, a bit each)
    static int wanted[2] = {-1, -1}, since[2] = {0, 0};
    static bool playing = false, reported = false;
    frame++;
    // (what the tests look for: the board reset, a swap not set up in time, a player not the character swapped to)
    playing = playing || (Ram16(kMode) == 2 && Ram16(kGameStep) == kGameMatch);
    if (playing && Ram16(kMode) != 2 && !reported) {
        reported = true;
        fprintf(stderr, "swap test: reset at %d\n", frame);
    }
    for (int player = 0; player < 2; player++) {
        // (Gen's other stance is a character of its own, 0x13, which the game changes him to and back)
        auto stance = [](int character) { return character == 0x13 ? 0x11 : character; };
        if (wanted[player] >= 0 && stance(Ram8(Player(player) + kCharacter)) != stance(wanted[player]) && !reported) {
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

void RegisterLiveTag() {
    AddHook(HookAt(0x027C72), "TagEntrance", TagEntrance);
    AddHook(HookAt(0x027C68), "TagSetUp", TagSetUp);
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
}

} // namespace cps2recomp
