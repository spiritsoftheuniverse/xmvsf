// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.

#include "native.h"

#include "../machine/m68k.h"
#include "../runtime/state.h"

#include <cstring>

namespace cps2recomp {

namespace native_video {

    namespace {
        inline uint16_t Word(const uint8_t *p) {
            return static_cast<uint16_t>((p[0] << 8) | p[1]);
        }
        constexpr int kPaletteBase = 5; // CPS-A's register (word index)
    } // namespace

    // The chip walks the pages from its base: a page taken is 0x400 bytes of the buffer; a page left out after the
    // first one taken still takes its room (Video::CopyPalette)
    PaletteUpload GamePalette(const uint8_t *videoRam, uint32_t source, uint8_t pages) {
        PaletteUpload upload;
        upload.pages = pages & 0x3F;
        upload.source = source;
        uint32_t at = (source - 0x900000) & 0x3FFFF;
        const uint32_t first = at;
        for (int page = 0; page < kPages; page++) {
            if (upload.pages & (1u << page)) {
                for (int n = 0; n < kPageColours; n++, at += 2) {
                    upload.colours[page][n] = Word(&videoRam[at & 0x3FFFF]);
                }
            } else if (at != first) {
                at += 0x400;
            }
        }
        return upload;
    }

    PaletteUpload DecodePalette(const uint8_t *videoRam, const uint16_t *cpsA, const uint16_t *cpsB, const Board &board) {
        const uint32_t base = (static_cast<uint32_t>(cpsA[kPaletteBase]) * 256) & ~0x3FFu & 0x3FFFF;
        return GamePalette(videoRam, 0x900000 + base, static_cast<uint8_t>(cpsB[board.paletteControl / 2]));
    }

    void PaletteUpload::Describe(const PaletteUpload &other, FILE *out) const {
        fprintf(out, "  pages %02X (the other: %02X), buffer %06X (the other: %06X)\n", pages, other.pages, source, other.source);
        int shown = 0;
        for (int page = 0; page < kPages; page++) {
            for (int n = 0; n < kPageColours; n++) {
                if (colours[page][n] != other.colours[page][n] && shown++ < 8) {
                    fprintf(out, "  page %d colour %03X: %04X (the other: %04X)\n", page, n, colours[page][n], other.colours[page][n]);
                }
            }
        }
    }

    ObjectList DecodeObjects(const uint8_t *bank, const uint16_t *objectRegs, const uint8_t *fromExtra) {
        ObjectList list;
        if (fromExtra) {
            memcpy(list.fromExtra, fromExtra, sizeof(list.fromExtra));
        }
        for (int n = 0; n < kObjectWords; n++) {
            list.words[n] = Word(&bank[n * 2]);
        }
        // (the registers Video reads: 400004 the priority control, 400008 and 40000A the offsets)
        list.priority = objectRegs[2];
        list.xOffset = objectRegs[4];
        list.yOffset = objectRegs[5];
        return list;
    }

    void ObjectList::Describe(const ObjectList &other, FILE *out) const {
        fprintf(out, "  priority %04X (the other: %04X), offsets %04X %04X (the other: %04X %04X)\n", priority, other.priority, xOffset,
                yOffset, other.xOffset, other.yOffset);
        int shown = 0;
        for (int n = 0; n < kObjectWords; n += 4) {
            if (memcmp(&words[n], &other.words[n], 8) != 0 && shown++ < 8) {
                fprintf(out, "  entry %d: %04X %04X %04X %04X (the other: %04X %04X %04X %04X)\n", n / 4, words[n], words[n + 1], words[n + 2],
                        words[n + 3], other.words[n], other.words[n + 1], other.words[n + 2], other.words[n + 3]);
            }
        }
    }

    Layers DecodeLayers(const uint16_t *cpsA, const uint16_t *cpsB, const uint16_t *objectRegs, uint8_t layerControl, const uint8_t *masks) {
        // (CPS-A's registers, by word: 1-3 the maps, 4 the row scroll's table, 6-11 the scrolls across and down,
        // 0x10 the row scroll's place, 0x11 the video control; video.cpp)
        Layers layers;
        for (int n = 0; n < 3; n++) {
            layers.maps[n] = cpsA[1 + n];
            layers.scroll[n][0] = cpsA[6 + 2 * n];
            layers.scroll[n][1] = cpsA[7 + 2 * n];
        }
        layers.rows = cpsA[4];
        layers.rowOffset = cpsA[0x10];
        layers.videoControl = cpsA[0x11];
        layers.layerControl = cpsB[layerControl / 2];
        layers.xOffset = objectRegs[4];
        layers.yOffset = objectRegs[5];
        for (int n = 0; masks && n < 4; n++) {
            layers.masks[n] = cpsB[masks[n] / 2];
        }
        return layers;
    }

    ObjectList DecodeObjectsCps1(const uint8_t *videoRam, const uint16_t *cpsA) {
        ObjectList list;
        const uint32_t base = (static_cast<uint32_t>(cpsA[0]) * 256) & ~0x7FFu & 0x3FFFF;
        for (int n = 0; n < 0x400 && base + n * 2 + 1 < 0x30000; n++) {
            list.words[n] = Word(&videoRam[base + n * 2]);
        }
        return list;
    }

    void Layers::Describe(const Layers &other, FILE *out) const {
        fprintf(out, "  maps %04X %04X %04X (the other: %04X %04X %04X), rows %04X (%04X) at %04X (%04X), video control %04X (%04X)\n",
                maps[0], maps[1], maps[2], other.maps[0], other.maps[1], other.maps[2], rows, other.rows, rowOffset, other.rowOffset,
                videoControl, other.videoControl);
        fprintf(out, "  scrolls %04X,%04X %04X,%04X %04X,%04X (the other: %04X,%04X %04X,%04X %04X,%04X), layer control %04X (%04X), "
                     "object offsets %04X,%04X (%04X,%04X)\n",
                scroll[0][0], scroll[0][1], scroll[1][0], scroll[1][1], scroll[2][0], scroll[2][1], other.scroll[0][0], other.scroll[0][1],
                other.scroll[1][0], other.scroll[1][1], other.scroll[2][0], other.scroll[2][1], layerControl, other.layerControl, xOffset,
                yOffset, other.xOffset, other.yOffset);
    }

    bool IsVideoRegister(uint32_t address) {
        address &= 0xFFFFFF;
        const uint32_t chip = address & ~0x4000u;
        // (and the object bank shown, at 700000: what is written there after it was handed over is in the list taken)
        return (address >= 0x400000 && address < 0x40000C) || address == 0x8040E0 || address == 0x8040E1 ||
               (chip >= 0x800100 && chip < 0x800180) || (address >= 0x700000 && address < 0x702000);
    }

} // namespace native_video

void NativeVideo::Serialize(Archive &ar) {
    ar.Pod(m_objectRegs);
    ar.Pod(m_cpsA);
    ar.Pod(m_cpsB);
    ar.Pod(m_bank);
    ar.Pod(m_list);
    ar.Pod(m_listExtra);
    m_raster.Serialize(ar);
    if (!ar.Saving()) {
        m_inFrame = false; // (a frame begun before the state was put back is not one)
        m_lastWhole = m_lastWhole && keepLastFrame;
        m_hasPending = false;
    }
}

void NativeVideo::Reset(const uint16_t *cpsA, const uint16_t *cpsB, const uint16_t *objectRegs) {
    memcpy(m_cpsA, cpsA, sizeof(m_cpsA));
    memcpy(m_cpsB, cpsB, sizeof(m_cpsB));
    memcpy(m_objectRegs, objectRegs, sizeof(m_objectRegs));
    m_bank = -1;
    m_hasPending = false;
    // (the board's palette and object list start empty too: a machine reset over another game's state, as the fighting
    // modes' spare machines are, keeps none of its picture)
    memset(m_list, 0, sizeof(m_list));
    memset(m_listExtra, 0, sizeof(m_listExtra));
    m_raster.Reset();
}

void NativeVideo::GameWrites(M68K &c, uint32_t address, uint16_t value, bool byte) {
    address &= 0xFFFFFF;
    // (a byte is half of its word: the board's, Machine::Write8)
    auto put = [&](uint16_t &word) {
        if (!byte) {
            word = value;
        } else if (address & 1) {
            word = static_cast<uint16_t>((word & 0xFF00) | (value & 0xFF));
        } else {
            word = static_cast<uint16_t>((word & 0x00FF) | (value << 8));
        }
        return word;
    };
    if (address >= 0x400000 && address < 0x40000C) {
        put(m_objectRegs[(address - 0x400000) / 2]);
        return;
    }
    if (address >= 0x700000 && address < 0x702000) {
        // The bank shown, written after it was handed over: the board takes it with the write in it, so the list does
        // too (Super Turbo's health bars write their empty pieces there, games/ssf2t/game.cfg)
        if (m_bank >= 0) {
            put(m_list[(address & 0x1FFE) / 2]);
        }
        return;
    }
    if (address == 0x8040E0 || address == 0x8040E1) {
        if (byte && address == 0x8040E0) {
            return; // (the bank is the low byte's)
        }
        // The game hands over the list it has filled (in the bank it reaches at 708000) when it names the other bank;
        // naming the bank shown, it keeps the list it handed over last
        const int bank = value & 1;
        if (bank != m_bank) { // (kept whether or not anything records: a spare board's state is another's after)
            for (int n = 0; n < native_video::kObjectWords; n++) {
                m_list[n] = c.bus.Read16(0x708000 + static_cast<uint32_t>(n) * 2);
            }
            // (and which of its entries are drawn from another set's graphics: those of the bank it names)
            if (objectsFromExtra) {
                memcpy(m_listExtra, objectsFromExtra[bank], sizeof(m_listExtra));
            }
        }
        m_bank = bank;
        return;
    }
    const uint32_t chip = address & ~0x4000u;
    if (chip >= 0x800100 && chip < 0x800140) {
        const unsigned index = (chip & 0x3E) / 2;
        const uint16_t now = put(m_cpsA[index]);
        if (index == 0x0A / 2) {
            // The palette's base: the chip takes the palette from there, the pages the game last asked for
            Palette(native_video::GamePalette(videoRam, 0x900000 + ((static_cast<uint32_t>(now) << 8) & 0x3FC00),
                                              static_cast<uint8_t>(m_cpsB[paletteControl / 2])));
        }
        return;
    }
    if (chip >= 0x800140 && chip < 0x800180) {
        put(m_cpsB[(chip & 0x3E) / 2]);
    }
}

void NativeVideo::BoardObjects(const native_video::ObjectList &list, const native_video::PictureOptions &options) {
    if (!Active()) {
        return;
    }
    if (m_bank < 0) {
        m_listsBefore++; // (the board takes a list each frame from the start; the game's first comes later)
        return;
    }
    native_video::ObjectList mine;
    memcpy(mine.words, m_list, sizeof(mine.words));
    memcpy(mine.fromExtra, m_listExtra, sizeof(mine.fromExtra));
    mine.priority = m_objectRegs[2];
    mine.xOffset = m_objectRegs[4];
    mine.yOffset = m_objectRegs[5];
    m_raster.Objects(mine, options); // (the frame's objects, the game's side's)
    for (uint8_t e : mine.fromExtra) {
        if (e) {
            m_listsExtra++; // (a list with entries drawn from another set's graphics)
            break;
        }
    }
    if (!checking) {
        return;
    }
    if (mine == list) {
        m_listsSame++;
        return;
    }
    if (m_listsDiffer++ < 10) {
        fprintf(stderr, "%s: frame %llu: the object list differs; the game's side:\n", checkName.c_str(), static_cast<unsigned long long>(frame));
        mine.Describe(list, stderr);
    }
}

void NativeVideo::BoardObjectsCps1(const native_video::ObjectList &list, const native_video::PictureOptions &options) {
    if (!Active()) {
        return;
    }
    // (the game's list: in video RAM where the game said, as the board takes it)
    const native_video::ObjectList mine = native_video::DecodeObjectsCps1(videoRam, m_cpsA);
    m_raster.Objects(mine, options);
    m_bank = 0; // (a list given: a CPS-1 has no bank to name)
    if (!checking) {
        return;
    }
    if (mine == list) {
        m_listsSame++;
        return;
    }
    if (m_listsDiffer++ < 10) {
        fprintf(stderr, "%s: frame %llu: the object list differs (a CPS-1's, where the game said it is: %04X; the board's: from "
                        "its register)\n", checkName.c_str(),
                static_cast<unsigned long long>(frame), m_cpsA[0]);
    }
}

void NativeVideo::BoardLayers(const native_video::Layers &layers, int line) {
    if (!checking) {
        return;
    }
    const native_video::Layers mine = native_video::DecodeLayers(m_cpsA, m_cpsB, m_objectRegs, layerControl, cps1Masks);
    if (mine == layers) {
        m_linesSame++;
        return;
    }
    if (m_linesDiffer++ < 10) {
        fprintf(stderr, "%s: frame %llu, line %d: the layers' registers differ; the game's side:\n", checkName.c_str(), static_cast<unsigned long long>(frame),
                line);
        mine.Describe(layers, stderr);
    }
}

void NativeVideo::VideoWritten(uint32_t address) {
    if (m_inFrame) {
        const uint32_t at = (address - 0x900000) & 0x3FFFE;
        m_frameEvents.push_back({static_cast<int16_t>(m_frameRow), at, static_cast<uint16_t>((videoRam[at] << 8) | videoRam[at + 1]),
                                 static_cast<uint8_t>(scrollFromExtra ? scrollFromExtra[at / 4] : 0)});
    }
}

void NativeVideo::FrameStart(const native_video::PictureOptions &options) {
    m_inFrame = true;
    m_frameRow = 0;
    m_frameVideo.assign(videoRam, videoRam + 0x30000);
    if (scrollFromExtra) {
        m_frameScrollExtra.assign(scrollFromExtra, scrollFromExtra + 0x30000 / 4);
        for (uint8_t e : m_frameScrollExtra) {
            if (e) {
                m_framesExtra++; // (a frame whose maps have tiles of another set's graphics)
                break;
            }
        }
    }
    m_frameRaster = m_raster; // (the palette as it is, and the frame's objects)
    m_frameOptions = options;
    m_frameEvents.clear();
    m_frameUploads.clear();
    if (checking) {
        m_frameBoard.assign(static_cast<size_t>(native_video::Raster::kMaxWidth) * 224, 0);
    }
}

NativeVideo::FrameData NativeVideo::FrameRecords() const {
    FrameData data;
    data.frame = frame;
    data.video = m_frameVideo.data();
    data.raster = &m_frameRaster;
    data.events = &m_frameEvents;
    data.uploads = &m_frameUploads;
    data.layers = m_frameLayers;
    data.scrollExtra = scrollFromExtra ? m_frameScrollExtra.data() : nullptr;
    data.overrides = m_frameOverrides;
    data.options = m_frameOptions;
    data.board = checking ? m_frameBoard.data() : nullptr;
    data.whole = m_frameComparable;
    return data;
}

bool NativeVideo::LastFrame(FrameData &data) const {
    if (!m_lastWhole) {
        return false;
    }
    data = FrameRecords();
    return true;
}

void NativeVideo::DrawFrame(const FrameData &frame, uint32_t *picture, int pitch) {
    // (on copies: the frame's video RAM, palette and maps' marks as at its first line, each line drawn with what was
    // written before it, as the GPU's bands draw it)
    static thread_local std::vector<uint8_t> video, scrollExtra;
    video.assign(frame.video, frame.video + 0x30000);
    if (frame.scrollExtra) {
        scrollExtra.assign(frame.scrollExtra, frame.scrollExtra + 0x30000 / 4);
    }
    native_video::Raster raster = *frame.raster;
    size_t next = 0;
    for (int row = 0; row < 224; row++) {
        for (; next < frame.events->size() && (*frame.events)[next].row <= row; next++) {
            const FrameEvent &e = (*frame.events)[next];
            if (e.address & 0x80000000u) {
                raster.Upload((*frame.uploads)[e.address & 0x7FFFFFFF]);
            } else {
                video[e.address] = static_cast<uint8_t>(e.value >> 8);
                video[e.address + 1] = static_cast<uint8_t>(e.value);
                if (frame.scrollExtra) {
                    scrollExtra[e.address / 4] = e.extra;
                }
            }
        }
        raster.Line(row, frame.layers[row], video.data(), frame.options, &picture[static_cast<size_t>(row) * pitch],
                    frame.scrollExtra ? scrollExtra.data() : nullptr);
    }
}

void NativeVideo::FrameEnd() {
    m_inFrame = false;
    m_lastWhole = m_frameComparable;
    if (frameDrawn) {
        frameDrawn(FrameRecords(), frameUser);
    }
    if (!m_frameComparable) {
        return;
    }
    if (!checking) {
        return;
    }
    // (how many bands the GPU will draw it in: one, and one more at each line a write comes before)
    // (only writes the drawing reads: into a map or the row scroll's table in use at that line, and palette uploads)
    int bands = 1, last = 0;
    for (const FrameEvent &e : m_frameEvents) {
        if (e.row <= last || e.row >= 224) {
            continue;
        }
        bool read = (e.address & 0x80000000u) != 0;
        const native_video::Layers &l = m_frameLayers[e.row];
        for (int n = 0; n < 3 && !read; n++) {
            const uint32_t map = (static_cast<uint32_t>(l.maps[n]) * 256) & ~0x3FFFu & 0x3FFFF;
            read = e.address - map < 0x4000;
        }
        const uint32_t rows = (static_cast<uint32_t>(l.rows) * 256) & ~0x7FFu & 0x3FFFF;
        read = read || ((l.videoControl & 1) && e.address - rows < 0x800);
        if (read) {
            bands++;
            last = e.row;
        }
    }
    m_bands += static_cast<uint64_t>(bands);
    m_bandFrames++;
    m_mostBands = bands > m_mostBands ? bands : m_mostBands;
    // Each line drawn with what was written before it: the GPU's bands, a line at a time (DrawFrame, on copies: the
    // records stay as they were, LastFrame's)
    const int width = native_video::Raster::kWidth + 2 * m_frameOptions.side;
    static std::vector<uint32_t> mine;
    mine.assign(static_cast<size_t>(native_video::Raster::kMaxWidth) * 224, 0);
    DrawFrame(FrameRecords(), mine.data(), native_video::Raster::kMaxWidth);
    int differ = 0;
    for (int row = 0; row < 224; row++) {
        if (memcmp(&mine[static_cast<size_t>(row) * native_video::Raster::kMaxWidth],
                   &m_frameBoard[static_cast<size_t>(row) * native_video::Raster::kMaxWidth], width * sizeof(uint32_t)) != 0) {
            differ++;
        }
    }
    if (differ) {
        m_frameLinesDiffer += differ;
        if (m_framesDiffer++ < 10) {
            fprintf(stderr, "%s: frame %llu: drawn at once, %d lines differ from the board's\n", checkName.c_str(), static_cast<unsigned long long>(frame),
                    differ);
        }
    } else {
        m_framesSame++;
    }
}

bool NativeVideo::DrawLine(int line, uint32_t *out, const native_video::PictureOptions &options) const {
    if (m_bank < 0) {
        return false;
    }
    m_raster.Line(line, native_video::DecodeLayers(m_cpsA, m_cpsB, m_objectRegs, layerControl, cps1Masks), videoRam, options, out,
                  scrollFromExtra);
    return true;
}

void NativeVideo::BoardLine(int line, const uint32_t *pixels, const native_video::PictureOptions &options, bool comparable) {
    if (!Active()) {
        return;
    }
    // (the frame drawn at once: its start, each line's layers and the board's line, its end)
    if (line == 0) {
        FrameStart(options);
        m_frameComparable = comparable && m_bank >= 0;
    }
    if (m_inFrame) {
        m_frameLayers[line] = native_video::DecodeLayers(m_cpsA, m_cpsB, m_objectRegs, layerControl, cps1Masks);
        // (the tiles the game's sources give beyond the board's columns, asked now, as the board's line asks them)
        m_raster.Overrides(line, m_frameLayers[line], videoRam, options, m_frameOverrides[line]);
        if (checking) {
            memcpy(&m_frameBoard[static_cast<size_t>(line) * native_video::Raster::kMaxWidth], pixels,
                   (native_video::Raster::kWidth + 2 * options.side) * sizeof(uint32_t));
        }
        m_frameRow = line + 1;
        if (line == 223) {
            FrameEnd();
        }
    }
    if (!checking) {
        return; // (recording only: the record kept, the CPU's picture not drawn)
    }
    if (!comparable || m_bank < 0) {
        m_picturesNot++; // (another set's graphics, or before the game's first list: not compared)
        return;
    }
    uint32_t mine[native_video::Raster::kMaxWidth];
    m_raster.Line(line, native_video::DecodeLayers(m_cpsA, m_cpsB, m_objectRegs, layerControl, cps1Masks), videoRam, options, mine, scrollFromExtra);
    const int width = native_video::Raster::kWidth + 2 * options.side;
    int x = 0;
    while (x < width && mine[x] == pixels[x]) {
        x++;
    }
    if (x == width) {
        m_picturesSame++;
        return;
    }
    if (m_picturesDiffer++ < 10) {
        fprintf(stderr, "%s: frame %llu, line %d: the picture drawn from the game's side differs from column %d: %08X (the "
                        "board's: %08X)\n", checkName.c_str(),
                static_cast<unsigned long long>(frame), line, x, mine[x], pixels[x]);
    }
}

void NativeVideo::Palette(const native_video::PaletteUpload &upload) {
    m_raster.Upload(upload); // (the game's side's palette: its uploads'; kept always, as the list is)
    if (!Active()) {
        return;
    }
    if (m_inFrame) {
        m_frameEvents.push_back({static_cast<int16_t>(m_frameRow), 0x80000000u | static_cast<uint32_t>(m_frameUploads.size()), 0});
        m_frameUploads.push_back(upload);
    }
    if (!checking) {
        return;
    }
    if (m_hasPending) {
        m_gameOnly++;
        fprintf(stderr, "%s: frame %llu: a palette upload the game's side gave, the board did not make\n", checkName.c_str(),
                static_cast<unsigned long long>(frame));
    }
    m_pending = upload;
    m_hasPending = true;
}

void NativeVideo::BoardPalette(const native_video::PaletteUpload &upload) {
    if (!checking) {
        return;
    }
    if (!m_hasPending) {
        // (code that is not native yet made it: counted, the first frame kept)
        if (m_boardOnly++ == 0) {
            m_firstBoardOnly = frame;
        }
        return;
    }
    m_hasPending = false;
    if (upload == m_pending) {
        m_same++;
        return;
    }
    if (m_differ++ < 10) {
        fprintf(stderr, "%s: frame %llu: the palette's upload differs; the game's side:\n", checkName.c_str(), static_cast<unsigned long long>(frame));
        m_pending.Describe(upload, stderr);
    }
}

bool NativeVideo::Report(FILE *out) const {
    fprintf(out, "%s: palette uploads: %llu the same from the game's side and the board's, %llu differ, %llu only the game's side gave\n", checkName.c_str(),
            static_cast<unsigned long long>(m_same), static_cast<unsigned long long>(m_differ), static_cast<unsigned long long>(m_gameOnly));
    if (m_boardOnly) {
        fprintf(out, "%s: %llu palette uploads made by the game's code that is not native yet (the first in frame %llu)\n", checkName.c_str(),
                static_cast<unsigned long long>(m_boardOnly), static_cast<unsigned long long>(m_firstBoardOnly));
    }
    fprintf(out, "%s: object lists: %llu the same from the game's side and the board's, %llu differ (and %llu taken before the "
                 "game gave its first)\n", checkName.c_str(),
            static_cast<unsigned long long>(m_listsSame), static_cast<unsigned long long>(m_listsDiffer),
            static_cast<unsigned long long>(m_listsBefore));
    if (objectsFromExtra) {
        fprintf(out, "%s: another set's graphics: %llu object lists with entries drawn from them, %llu frames with tiles\n", checkName.c_str(),
                static_cast<unsigned long long>(m_listsExtra), static_cast<unsigned long long>(m_framesExtra));
    }
    fprintf(out, "%s: lines: %llu drawn with the same layers' registers as the game's side has, %llu not\n", checkName.c_str(),
            static_cast<unsigned long long>(m_linesSame), static_cast<unsigned long long>(m_linesDiffer));
    fprintf(out, "%s: picture: %llu lines drawn from the game's side's records the same as the board's, %llu not (and %llu "
                 "not compared)\n", checkName.c_str(),
            static_cast<unsigned long long>(m_picturesSame), static_cast<unsigned long long>(m_picturesDiffer),
            static_cast<unsigned long long>(m_picturesNot));
    fprintf(out, "%s: frames drawn at once (as the GPU will): %llu the same as the board's, %llu not (%llu lines)\n", checkName.c_str(),
            static_cast<unsigned long long>(m_framesSame), static_cast<unsigned long long>(m_framesDiffer),
            static_cast<unsigned long long>(m_frameLinesDiffer));
    if (m_bandFrames) {
        fprintf(out, "%s: bands a frame (lines where writes come in): %.1f on average, %d at most\n", checkName.c_str(),
                static_cast<double>(m_bands) / static_cast<double>(m_bandFrames), m_mostBands);
    }
    return m_differ == 0 && m_gameOnly == 0 && m_boardOnly == 0 && m_listsDiffer == 0 && m_linesDiffer == 0 && m_picturesDiffer == 0 &&
           m_framesDiffer == 0;
}

} // namespace cps2recomp
