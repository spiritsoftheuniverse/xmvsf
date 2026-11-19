// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The game's ROM set: the files of an arcade ROM collection's set, read from a folder (or later a zip),
// checked against the checksums in game.cfg, and laid out the way the board's chips see them.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cps2recomp {

// Which board the game is on. A CPS-2 unless the game's game.cfg says `board = cps1` (with `pairs=N` after it when
// the program starts with N pairs of byte-wide files) (a CPS-1 with QSound, the
// "CPS-1.5": a 12 MHz 68000 with an unenciphered program, the objects in the video memory, the Z80's program
// enciphered with Kabuki). A CPS-1's CPS-B chip has its registers where the game's chip has them (game.cfg's
// `cps_b`, the byte offsets as MAME's table of the chips has them: layer control, the four priority masks, palette
// control, then the bits of the layer control that switch scroll screens 1-3 on; then, for a chip with one, its ID
// register's offset and value). The CPS-2's are the defaults.
// `board = cps1-classic` (`pairs=N` too) is a CPS-1 with no QSound (Street Fighter II's): its Z80's program is not enciphered, and
// its Z80 drives the YM2151 and OKI MSM6295 through machine/cps1_sound.cpp.
// A CPS-1's graphics mapper (the PAL that maps each kind of tile's numbers to the graphics ROMs: game.cfg's
// `gfx_map`, as MAME's tables have them): the banks' sizes, then ranges of a kind's tile numbers (in 64-byte units:
// an 8-pixel tile's number, a 16-pixel one's times 2, a 32-pixel one's times 8) and the bank they are in. With none,
// tile numbers are where the ROMs have them (Cadillacs and Dinosaurs).
struct GfxRange {
    enum : uint8_t { kObjects = 1, kScroll1 = 2, kScroll2 = 4, kScroll3 = 8 };
    uint8_t kinds = 0;
    uint32_t start = 0, end = 0;
    unsigned bank = 0;
};

struct Board {
    bool cps1 = false;
    bool qsound = true; // (false: classic CPS-1's YM2151/OKI sound)
    uint8_t layerControl = 0x26;
    uint8_t priority[4] = {0x28, 0x2A, 0x2C, 0x2E};
    uint8_t paletteControl = 0x30;
    uint8_t enable[3] = {0x02, 0x04, 0x08};
    // Pairs of files the program list starts with, each the even bytes then the odd ones of the same words (MAME's
    // ROM_LOAD16_BYTE: game.cfg's `board = cps1 pairs=N`); the rest of the list whole words as usual
    int programPairs = 0;
    // The chip's ID register, which some games check as they start (MAME's first two values for the chip: the
    // register's offset and what it reads; 0: none): two more values after cps_b's nine
    uint8_t idRegister = 0;
    uint16_t idValue = 0;
    uint32_t gfxBanks[4] = {};
    std::vector<GfxRange> gfxMap;
    // (a CPS-1) its 68000's clock in MHz, 10 or 12 (game.cfg's cpu_clock), and its three banks of DIP switches, A, B
    // and C (game.cfg's dip; a bit 1: the switch off)
    int cpuClock = 12;
    uint8_t dip[3] = {0xFF, 0xFF, 0xFF};
};
// The graphics mapper as a table, for drawing a tile at a time (the picture from the records, video/native_render.cpp,
// and the GPU's, video/shaders/layers.frag; video.cpp reads Board::gfxMap itself): for each kind (0 the objects, 1-3
// the scroll screens) and each kStep of its units, where the graphics have them: unit = base + (unit & mask), base -1
// not drawn. A map whose ranges are not whole steps makes none (`on` false; Gfx's ranges are 0x1000s and more).
struct GfxTable {
    static constexpr uint32_t kStep = 0x100, kEntries = 0x400; // (units 0-3FFFF)
    bool on = false;
    int32_t base[4][kEntries] = {};
    uint32_t mask[4][kEntries] = {};
    int64_t Map(int kind, uint32_t unit) const {
        const uint32_t entry = unit / kStep;
        if (entry >= kEntries || base[kind][entry] < 0) {
            return -1;
        }
        return static_cast<int64_t>(base[kind][entry]) + (unit & mask[kind][entry]);
    }
};
bool MakeGfxTable(const Board &board, GfxTable &table);

// game.cfg's `gfx_map`: the banks' sizes (hex, up to four), "|", then kind:start-end:bank for each range, the kind
// one of objects, scroll1, scroll2, scroll3 (hex). "": none. False if it cannot be read.
bool ParseGfxMap(const char *text, Board &board);

struct RomSet {
    // The 68000's ROM, as bytes in the order the CPU sees them: as it is on the chips (what a data read
    // gives), and deciphered (what a program read gives)
    std::vector<uint8_t> program;
    std::vector<uint8_t> opcodes;
    // Graphics: 16x16 tiles of 128 bytes each (see video/tiles.h for the layout)
    std::vector<uint8_t> graphics;
    // The Z80's ROM: the files one after the other. The first 0x8000 bytes are always mapped;
    // bank n is at 0x8000 + n * 0x4000.
    std::vector<uint8_t> soundProgram;
    // The QSound chip's samples: signed bytes
    std::vector<uint8_t> samples;

    // Which set it is (RomList::set, or one of its others: its set, and the name ROM collections give it; "" for the
    // set's own)
    std::string set, name;

    uint32_t key[2] = {};     // the board's 64-bit key
    uint32_t encryptedEnd = 0; // program below this address is enciphered

    Board board;
    // (a CPS-1 with Kabuki: the sound program's first 0x8000 bytes as the Z80's opcode reads see them; soundProgram
    // has them as its other reads do. Empty: the program is not enciphered.)
    std::vector<uint8_t> soundOpcodes;
};

// One file of the set and its CRC32
struct RomFile {
    std::string name;
    uint32_t crc = 0;
    // (program) its 16-bit words are high byte first, as the 68000 reads them ("name:crc/be"; without it, low byte
    // first)
    bool highFirst = false;
    // Files whose bytes are interleaved, written "a:crc+b:crc...": in the program, a pair whose first gives the even
    // bytes and second the odd; in the graphics, eight files that each give one byte of the 64-bit words. Empty: one
    // file, this one.
    std::vector<RomFile> parts;
};
// The set's files by part, from game.cfg's lists ("name:crc name:crc ...")
struct RomList {
    std::vector<RomFile> program, graphics, soundProgram, samples, key;
    // The set's name in ROM collections, and its parent set's (collections often keep the files a set
    // shares with its parent only in the parent's zip)
    std::string set, parent;
    // Other sets of the same game the port plays as well (game.cfg's other_set: another region, whose program
    // is the same code, with the region in the header or in one instruction): their own program files and key,
    // the rest the set's. Read when the set's own program is not there.
    struct Other {
        std::string set, name;
        std::vector<RomFile> program, key;
    };
    std::vector<Other> others;
    // Where the first graphics file sits in the graphics space (a list that starts "@800000": boards with
    // few graphics ROMs have them in the last sockets, and the game's tile numbers count from the first)
    size_t graphicsBase = 0;
    Board board;
};
// (board, cpsB: game.cfg's `board` and `cps_b`, or ""; others: its other_set, "set | name | program | key" with
// " || " between two)
RomList ParseRomList(const char *program, const char *graphics, const char *soundProgram, const char *samples,
                     const char *key, const char *set = "", const char *parent = "", const char *board = "", const char *cpsB = "",
                     const char *others = "");

// Reads the set from `path`: a folder of its files, or a zip (<set>.zip; files not in it are looked for in
// the parent's and the set's zips beside it, and a folder's files can be in such zips inside it). Files in
// zips are found by checksum, whatever they are called. Nothing is written or changed. Where the set's own
// program is not there, each of its others' is tried (RomSet::set says which was read).
// False with a message if a file is missing or is not the one expected.
bool LoadRomSet(const std::string &path, const RomList &list, RomSet &set, std::string &error);

} // namespace cps2recomp
