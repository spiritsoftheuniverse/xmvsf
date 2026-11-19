// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// Capcom's video library as the games share it, written once (docs/NATIVE_VIDEO.md): the routines that hand the
// board the picture, rewritten (docs/READABLE.md) so that they also hand it to the native interface (native.h).
//
// A game says in its game.cfg where its copy of each part is. A routine rewritten whole: the address, then a CRC-32 of
// the routine's instruction words with their operands left out (`video_palette_commit = 00123E:XXXXXXXX`); its
// operands (where the game keeps its variables, which of the board's addresses it writes) are read from the game's
// own copy, so that one rewrite serves every game with that version. A copy that is not that version is not
// rewritten (its own code runs) and is reported. Elsewhere, the instructions that write the video's registers
// (`video_writes = ADDRESS ...`): a hook at each says what it is about to write, and the instruction then runs as it
// was.
#include "native.h"

#include "../runtime/game.h"
#include "../runtime/lift.h"
#include "../runtime/guest.h"
#include "game_info.h"

#include <zlib.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace cps2recomp {

namespace {
    inline uint16_t OpcodeWord(uint32_t address) {
        const uint8_t *p = ProgramOpcodes(address);
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }
    inline uint32_t OpcodeLong(uint32_t address) {
        return (static_cast<uint32_t>(OpcodeWord(address)) << 16) | OpcodeWord(address + 2);
    }
    inline uint32_t Variable(const M68K &c, int16_t offset) {
        return (c.a[5] + offset) & 0xFFFFFF;
    }

    // `address:crc` from game.cfg; false if the routine there is not that version (the CRC of the instruction words
    // at `words`, offsets from the address)
    bool Find(const char *key, const char *value, std::initializer_list<int> words, uint32_t &address) {
        char *end = nullptr;
        address = static_cast<uint32_t>(strtoul(value, &end, 16));
        const uint32_t want = *end == ':' ? static_cast<uint32_t>(strtoul(end + 1, nullptr, 16)) : 0;
        uint8_t shape[32];
        size_t size = 0;
        for (int at : words) {
            const uint16_t word = OpcodeWord(address + static_cast<uint32_t>(at));
            shape[size++] = static_cast<uint8_t>(word >> 8);
            shape[size++] = static_cast<uint8_t>(word);
        }
        const uint32_t got = static_cast<uint32_t>(crc32(0, shape, static_cast<uInt>(size)));
        if (got != want) {
            fprintf(stderr, "video: game.cfg's %s: the routine at %06X is not the version the toolkit has (its CRC %08X, not %08X); "
                            "the game's own code runs\n", key, address, got, want);
            return false;
        }
        return true;
    }

    // --- The palette's commit, once a frame in the vertical blank's interrupt (Alpha's kernel: Street Fighter Alpha
    // 1-3, Zero 2 Alpha, the Vampire Savior family, Pocket Fighter, Puzzle Fighter). Its three instructions:
    //
    //   move.w pages(a5),<CPS-B's palette control>   the pages the chip is to take
    //   move.w base(a5),<CPS-A's palette base>       where from: writing it makes the chip take them
    //   move.w next(a5),base(a5)                     next frame's buffer, as the game asked for it this frame
    //   rts
    struct PaletteCommit {
        int16_t pages, base, next;              // the variables (from A5)
        uint32_t pagesRegister, baseRegister;   // the board's addresses the game's copy writes
    } g_commit;

    void CommitPalette(M68K &c) {
        const uint16_t pages = Ram16(Variable(c, g_commit.pages));
        const uint16_t base = Ram16(Variable(c, g_commit.base));
        // (a rewrite's writes to the video's registers are told to the native interface as they are made: the upload's
        // record is made from the game's buffer and the pages it asks for)
        c.Write(g_commit.pagesRegister, 1, pages);
        c.Write(g_commit.baseRegister, 1, base);
        const uint16_t next = Ram16(Variable(c, g_commit.next));
        SetRam16(Variable(c, g_commit.base), next);
        // (the flags of the last move)
        c.flagN = next & 0x8000;
        c.flagZ = next == 0;
        c.flagV = c.flagC = false;
    }

    // (the routine's address, or 0 if it is not taken)
    uint32_t AddPaletteCommit(const char *value) {
        uint32_t at;
        if (!Find("video_palette_commit", value, {0, 8, 16, 22}, at)) {
            return 0;
        }
        g_commit.pages = static_cast<int16_t>(OpcodeWord(at + 2));
        g_commit.pagesRegister = OpcodeLong(at + 4);
        g_commit.base = static_cast<int16_t>(OpcodeWord(at + 10));
        g_commit.baseRegister = OpcodeLong(at + 12);
        g_commit.next = static_cast<int16_t>(OpcodeWord(at + 18));
        if (static_cast<int16_t>(OpcodeWord(at + 20)) != g_commit.base) {
            fprintf(stderr, "video: the palette's commit at %06X does not move the next buffer into the one it gave\n", at);
            return 0;
        }
        // (move.w d16(An),xxx.l twice: 24 cycles each; move.w d16(An),d16(An): 20; Return adds the rts)
        AddRewrite(at, "CommitPalette", CommitPalette, 0, 68);
        return at;
    }
    // --- The video's registers written elsewhere: every other family's commit, every game's start-up, the object
    // bank's swaps, the scrolls and the layers. Each is a `move` of a byte, a word or a long (from a data register, an
    // address register's place, a variable or a number), or a `clr`; to a fixed address or an address register's
    // place. A watch there works out, from the CPU as the instruction finds it, the value and the place, and tells the
    // native interface when the place is a video register.
    struct Operand {
        unsigned mode = 0, reg = 0;
        uint32_t constant = 0; // (resolved from its extension words: d16's displacement, an address, a number)
    };
    struct Move {
        unsigned size = 2; // (bytes: 1, 2 or 4)
        bool clear = false; // (clr: no source, 0 written)
        Operand from, to;
    };
    // The instructions watched in a program: the game's own, or a guest board's (guest.h; the CPU's watchContext)
    struct MoveTable {
        std::map<uint32_t, Move> moves;
    };
    MoveTable g_moves;
    std::map<std::string, MoveTable> &GuestMoves() {
        static auto &tables = *new std::map<std::string, MoveTable>; // (never freed: guest boards' spares may outlive us)
        return tables;
    }

    unsigned ExtensionWords(const Operand &o, unsigned size) {
        switch (o.mode) {
        case 5: return 1; // d16(An)
        case 7:
            switch (o.reg) {
            case 0: return 1; // xxx.w
            case 1: return 2; // xxx.l
            case 2: return 1; // d16(PC)
            case 4: return size == 4 ? 2 : 1; // #n (a byte's in the low half of a word)
            default: return 0;
            }
        default: return 0;
        }
    }
    bool Supported(const Operand &o, bool source) {
        if (o.mode == 6) {
            return false; // (an index: none of the games' writers so far)
        }
        if (o.mode == 7) {
            return o.reg == 0 || o.reg == 1 || (source && (o.reg == 2 || o.reg == 4));
        }
        return source || o.mode >= 2; // (a register is no destination here)
    }
    // An operand's constant, read from the program its extension words start at (`word`: a word of that program)
    template <typename Word>
    void Resolve(Operand &o, uint32_t extension, unsigned size, const Word &word) {
        auto at16 = [&](uint32_t a) { return static_cast<uint32_t>(static_cast<int16_t>(word(a))); };
        auto at32 = [&](uint32_t a) { return (static_cast<uint32_t>(word(a)) << 16) | word(a + 2); };
        switch (o.mode) {
        case 5: o.constant = at16(extension); break;
        case 7:
            switch (o.reg) {
            case 0: o.constant = at16(extension); break;
            case 1: o.constant = at32(extension); break;
            case 2: o.constant = extension + at16(extension); break;
            case 4: o.constant = size == 4 ? at32(extension) : word(extension) & (size == 1 ? 0xFF : 0xFFFF); break;
            default: break;
            }
            break;
        default: break;
        }
    }
    // Where an operand is in memory, as the CPU is before the instruction
    uint32_t Place(const M68K &c, const Operand &o, unsigned size) {
        switch (o.mode) {
        case 2: case 3: return c.a[o.reg];
        case 4: return c.a[o.reg] - ((size == 1 && o.reg == 7) ? 2 : size);
        case 5: return c.a[o.reg] + o.constant;
        case 7: return o.constant;
        default: return 0;
        }
    }
    uint32_t Value(M68K &c, const Operand &o, unsigned size) {
        const uint32_t mask = size == 1 ? 0xFF : size == 2 ? 0xFFFF : 0xFFFFFFFF;
        if (o.mode == 0) {
            return c.d[o.reg] & mask;
        }
        if (o.mode == 1) {
            return c.a[o.reg] & mask;
        }
        if (o.mode == 7 && o.reg == 4) {
            return o.constant & mask;
        }
        const uint32_t at = Place(c, o, size) & 0xFFFFFF;
        switch (size) {
        case 1: return c.bus.Read8(at);
        case 2: return c.bus.Read16(at & ~1u);
        default: return (static_cast<uint32_t>(c.bus.Read16(at & ~1u)) << 16) | c.bus.Read16((at + 2) & 0xFFFFFE);
        }
    }
    bool VideoWrite(M68K &c) {
        const MoveTable &table = c.watchContext ? *static_cast<const MoveTable *>(c.watchContext) : g_moves;
        const auto found = table.moves.find(c.pc);
        if (found == table.moves.end() || !c.nativeVideo) {
            return false;
        }
        const Move &move = found->second;
        const uint32_t to = Place(c, move.to, move.size) & 0xFFFFFF;
        if (!native_video::IsVideoRegister(to)) {
            return false;
        }
        const uint32_t value = move.clear ? 0 : Value(c, move.from, move.size);
        if (move.size == 4) {
            c.nativeVideo->GameWrites(c, to, static_cast<uint16_t>(value >> 16), false);
            c.nativeVideo->GameWrites(c, (to + 2) & 0xFFFFFF, static_cast<uint16_t>(value), false);
        } else {
            c.nativeVideo->GameWrites(c, to, static_cast<uint16_t>(value), move.size == 1);
        }
        return false;
    }

    // The instructions in `list` (hex, spaces between) read from a program (`word`), into `table`; each added to `watched`
    template <typename Word, typename Watch>
    void AddWrites(const char *key, const char *list, MoveTable &table, const Word &word, const Watch &watched) {
        for (const char *p = list; *p;) {
            char *end = nullptr;
            const uint32_t at = static_cast<uint32_t>(strtoul(p, &end, 16));
            if (end == p) {
                p++; // (spaces, and the "||" a key given twice is joined with)
                continue;
            }
            p = end;
            const uint16_t op = word(at);
            Move move;
            bool known = false;
            if ((op & 0xFF00) == 0x4200 && ((op >> 6) & 3) != 3) { // clr
                known = true;
                move.clear = true;
                move.size = 1u << ((op >> 6) & 3);
                move.to = {static_cast<unsigned>((op >> 3) & 7), static_cast<unsigned>(op & 7), 0};
                Resolve(move.to, at + 2, move.size, word);
            } else if ((op >> 12) == 1 || (op >> 12) == 2 || (op >> 12) == 3) { // move.b, move.l, move.w
                move.size = (op >> 12) == 1 ? 1 : (op >> 12) == 3 ? 2 : 4;
                move.from = {static_cast<unsigned>((op >> 3) & 7), static_cast<unsigned>(op & 7), 0};
                move.to = {static_cast<unsigned>((op >> 6) & 7), static_cast<unsigned>((op >> 9) & 7), 0};
                Resolve(move.from, at + 2, move.size, word);
                Resolve(move.to, at + 2 + 2 * ExtensionWords(move.from, move.size), move.size, word);
                known = Supported(move.from, true);
            }
            if (!known || !Supported(move.to, false) || (move.to.mode == 7 && !native_video::IsVideoRegister(move.to.constant))) {
                fprintf(stderr, "video: %s: the instruction at %06X is not a move or clr to the video's registers the toolkit reads; "
                                "not native\n", key, at);
                continue;
            }
            table.moves[at] = move;
            watched(at);
        }
    }
} // namespace

// (a game.cfg without one of the keys: none of that part)
#ifndef GAME_VIDEO_PALETTE_COMMIT
#define GAME_VIDEO_PALETTE_COMMIT ""
#endif
#ifndef GAME_VIDEO_WRITES
#define GAME_VIDEO_WRITES ""
#endif

void RegisterGuestVideoLifts(const std::string &id, const char *writes, const uint8_t *opcodes, size_t size) {
    MoveTable &table = GuestMoves()[id];
    auto word = [&](uint32_t at) -> uint16_t { return at + 1 < size ? static_cast<uint16_t>((opcodes[at] << 8) | opcodes[at + 1]) : 0; };
    std::vector<uint32_t> watched;
    AddWrites(("the guest " + id + "'s video_writes").c_str(), writes, table, word, [&](uint32_t at) { watched.push_back(at); });
    AddGuestWatches(id, watched, VideoWrite, &table);
}

void RegisterVideoLifts() {
    if (*GAME_VIDEO_PALETTE_COMMIT) {
        if (const uint32_t at = AddPaletteCommit(GAME_VIDEO_PALETTE_COMMIT)) {
            // (its two moves to the registers watched as well, for when the rewrite stands aside and the game's own code
            // runs it: the board's timing with no feature that needs rewrites; build.mk gives the translator them too)
            char moves[32];
            snprintf(moves, sizeof(moves), "%06X %06X", at, at + 8);
            AddWrites("video_palette_commit", moves, g_moves, OpcodeWord, [](uint32_t a) { AddWatch(a, "VideoWrite", VideoWrite); });
        }
    }
    AddWrites("game.cfg's video_writes", GAME_VIDEO_WRITES, g_moves, OpcodeWord,
              [](uint32_t at) { AddWatch(at, "VideoWrite", VideoWrite); });
}

} // namespace cps2recomp
