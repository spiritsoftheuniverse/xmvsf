// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.

#include "video.h"
#include "../port/xmvsf_source_identity.h" // 87.490 native OAM wall exemption
#include "../port/xmvsf_vs_handoff.h" // 87.482: original FBNeo 120+30 HUD shock

#include "../runtime/state.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace cps2recomp {

namespace {
    // CPS-A registers (word index)
    enum { kScroll1Base = 1, kScroll2Base = 2, kScroll3Base = 3, kOtherBase = 4, kPaletteBase = 5 };
    enum { kScroll1X = 6, kRowScrollOffset = 16, kVideoControl = 17 };
    // (CPS-B registers: where the board has them, Board)
    // Object registers at 0x400000 (word index)
    enum { kObjPriority = 2, kObjXOffset = 4, kObjYOffset = 5 };

    inline uint16_t Word(const uint8_t *p) {
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }

    // Where a CPS-1's PAL puts a kind's tile, in 64-byte units (machine/romset.h, GfxRange); -1: in no range, not
    // drawn
    int64_t MapTile(const Board &board, uint8_t kind, uint32_t unit) {
        for (const GfxRange &range : board.gfxMap) {
            if ((range.kinds & kind) && unit >= range.start && unit <= range.end) {
                uint32_t base = 0;
                for (unsigned b = 0; b < range.bank; b++) {
                    base += board.gfxBanks[b];
                }
                return base + (unit & (board.gfxBanks[range.bank] - 1));
            }
        }
        return -1;
    }

    // Eight pixels of a tile row from its four bytes: one bit plane per byte, leftmost pixel in bit 7
    inline void Unpack(const uint8_t *row, uint8_t *pixels) {
        const uint8_t b0 = row[0], b1 = row[1], b2 = row[2], b3 = row[3];
        for (int k = 0; k < 8; k++) {
            const int s = 7 - k;
            pixels[k] = static_cast<uint8_t>(((b0 >> s) & 1) | (((b1 >> s) & 1) << 1) | (((b2 >> s) & 1) << 2) |
                                             (((b3 >> s) & 1) << 3));
        }
    }
} // namespace

void Video::Reset() {
    memset(m_palette, 0, sizeof(m_palette));
    memset(m_objects, 0, sizeof(m_objects));
    m_priorityControl = 0;
    m_lastObject = -4;
    latchedObjectBank = 0;
    m_inkReady = false;
    m_inkInspection = false;
    m_pixelOwner.clear();
    m_objectInk.fill({});
}

// 87.492: prepare the native source-ink probe exactly once per emulator
// frame, BEFORE any board/native rasterizer draws a line. Leave graphics
// ROM, object RAM, CPU state and actual pixel output untouched.
void Video::BeginInkFrame() {
    m_inkReady = m_inkInspection;
    if (!m_inkReady) return;
    m_objectInk.fill({});
    m_pixelOwner.assign(static_cast<size_t>(kMaxWidth) * kHeight, -1);
}

void Video::RecordInk(unsigned slot, int x, int y) {
    if (!m_inkReady || slot >= m_objectInk.size() || x < 0 ||
        x >= Width() || x >= kMaxWidth || y < 0 || y >= kHeight) return;
    const auto at = static_cast<size_t>(y) * kMaxWidth +
                    static_cast<size_t>(x);
    if (at >= m_pixelOwner.size() || m_pixelOwner[at] >= 0) return;
    m_pixelOwner[at] = static_cast<std::int16_t>(slot);
    InkEvidence &ink = m_objectInk[slot];
    if (ink.pixels++ == 0) {
        ink.left = ink.right = x;
        ink.top = ink.bottom = y;
    } else {
        ink.left = std::min(ink.left, x);
        ink.top = std::min(ink.top, y);
        ink.right = std::max(ink.right, x);
        ink.bottom = std::max(ink.bottom, y);
    }
}

// 87.489 F9: decode genuine native object and scroll map records into
// labelled presentation candidates. NO screen pixel heuristics, filtering,
// writes to RAM, changes to OAM, or shader interception.
Video::GraphicsInspection Video::InspectGraphics() const {
    GraphicsInspection found;
    found.gameStep = hudShockGameStep;
    found.fightStatus = static_cast<unsigned>(hudShockPhase) & 0xFFu;
    found.objectBank = latchedObjectBank;
    if (!m_src.videoRam || !m_src.cpsA || !m_src.cpsB ||
        !m_src.objectRegs || !m_src.graphics) return found;
    const auto *a = m_src.cpsA;
    const auto *regs = m_src.objectRegs;
    found.layerControl = m_src.cpsB[m_src.board.layerControl / 2u];
    found.videoControl = a[kVideoControl];

    // Native frame-latched CPS2 OAM: each 8-byte entry has a true slot
    // identity and code. The bank is recorded separately because the
    // physical OAM banks alternate. Never pass these IDs into hiding code.
    for (int i = 0; i <= m_lastObject; i += 4) {
        const unsigned xw = m_objects[i], yw = m_objects[i + 1];
        const unsigned attr = m_objects[i + 3];
        const unsigned code = m_objects[i + 2] + ((yw & 0x6000u) << 3);
        const int across = static_cast<int>(((attr >> 8) & 0x0Fu) + 1u);
        const int down = static_cast<int>(((attr >> 12) & 0x0Fu) + 1u);
        const bool relative = (attr & 0x80u) != 0u;
        const int ox = static_cast<int>(regs[kObjXOffset]);
        const int oy = static_cast<int>(regs[kObjYOffset]);
        const int px = ((static_cast<int>(xw) +
                         (relative ? ox : 0) + 64 - ox) & 0x3FF) - kFirstColumn;
        const int py = ((static_cast<int>(yw) +
                         (relative ? oy : 0) + 16 - oy) & 0x3FF) - kFirstLine;
        if (px >= Width() || px + across * 16 <= 0 ||
            py >= kHeight || py + down * 16 <= 0) continue;
        GraphicRecord item{};
        item.kind = 0;
        item.index = static_cast<unsigned>(i / 4);
        item.sourceAddress = static_cast<unsigned>(i * 2); // bank-local byte offset
        item.tile = code;
        item.gfxOffset = code * 128u; // CPS2 object ROM byte offset
        item.attributes = attr;
        item.priority = (xw >> 13) & 7u;
        item.palette = attr & 31u;
        item.bank = latchedObjectBank;
        item.x = px + wide.side;
        item.y = py;
        item.w = across * 16;
        item.h = down * 16;
        item.enabled = (m_extraGraphics && m_objectsFromExtra[i / 4])
            ? item.gfxOffset < m_extraGraphicsSize
            : item.gfxOffset < m_src.graphicsSize;
        if (m_inkReady) {
            const InkEvidence &ink = m_objectInk[static_cast<size_t>(i / 4)];
            item.inkSampled = true;
            item.visibleInkPixels = ink.pixels;
            if (ink.pixels) {
                item.inkLeft = ink.left;
                item.inkTop = ink.top;
                item.inkRight = ink.right;
                item.inkBottom = ink.bottom;
            }
        }
        found.records.push_back(item);
    }

    // Native scroll-map entries, with actual VRAM addresses computed
    // from CPS-A map base/scroll and CPS-B layer control.
    for (int which = 0; which < 3; ++which) {
        const unsigned shift = static_cast<unsigned>(3 + which);
        const unsigned tileSize = 1u << shift;
        const unsigned mapMask = (64u << shift) - 1u;
        const unsigned base =
            (static_cast<unsigned>(a[kScroll1Base + which]) * 256u) &
            ~0x3FFFu & 0x3FFFFu;
        const unsigned scrollY = a[kScroll1X + which * 2 + 1];
        const bool rowOn = which == 1 && (found.videoControl & 1u);
        const bool enabled = (layerMask & (1u << (which + 1))) &&
            (found.layerControl & (1u << (which + 1))) &&
            (which == 0 || (found.videoControl & (1u << (which + 1))));
        const int yFirst = -static_cast<int>((kFirstLine + scrollY) & (tileSize - 1u));
        for (int y = yFirst; y < kHeight; y += static_cast<int>(tileSize)) {
            const unsigned rasterY = static_cast<unsigned>(y + kFirstLine);
            const unsigned row = ((rasterY + scrollY) & mapMask) >> shift;
            const unsigned rowOffset = (rasterY + a[kRowScrollOffset]) & 0x3FFu;
            const unsigned otherBase =
                (static_cast<unsigned>(a[kOtherBase]) * 256u) &
                ~0x7FFu & 0x3FFFFu;
            const unsigned rowScroll = rowOn ?
                Word(&m_src.videoRam[otherBase + rowOffset * 2u]) : 0u;
            const int ownLeft = wide.side + wide.scroll[which] +
                (rowOn && wide.rows ? wide.rows[rowOffset] : 0);
            const unsigned scrollX = static_cast<unsigned>(
                a[kScroll1X + which * 2]) + rowScroll;
            const unsigned firstX =
                static_cast<unsigned>(kFirstColumn - ownLeft +
                                      static_cast<int>(scrollX)) & mapMask;
            const int xFirst = -static_cast<int>(firstX & (tileSize - 1u));
            for (int x = xFirst; x < Width(); x += static_cast<int>(tileSize)) {
                const unsigned col =
                    ((firstX + static_cast<unsigned>(x)) & mapMask) >> shift;
                unsigned index;
                if (which == 0)
                    index = (row & 31u) + (col << 5) + ((row & 32u) << 6);
                else if (which == 1)
                    index = (row & 15u) + (col << 4) + ((row & 48u) << 6);
                else
                    index = (row & 7u) + (col << 3) + ((row & 56u) << 6);
                const unsigned at = base + index * 4u;
                const unsigned code = Word(&m_src.videoRam[at]);
                const unsigned attr = Word(&m_src.videoRam[at + 2u]);
                GraphicRecord item{};
                item.kind = static_cast<unsigned>(which + 1);
                item.index = index;
                item.sourceAddress = 0x900000u + at; // actual CPS2 VRAM byte address
                item.tile = code;
                item.gfxOffset = 0x800000u +
                    (which == 0 ? code * 64u + ((col & 1u) ? 4u : 0u) :
                     which == 1 ? code * 128u : (code & 0x3FFFu) * 512u);
                item.attributes = attr;
                item.palette = attr & 31u;
                item.x = x;
                item.y = y;
                item.w = static_cast<int>(tileSize);
                item.h = static_cast<int>(tileSize);
                item.enabled = enabled &&
                    item.gfxOffset < m_src.graphicsSize;
                found.records.push_back(item);
            }
        }
    }
    return found;
}

// 87.488: inspect the native graphic SOURCES of a captured game frame.
// It is deliberately independent of XM87392's HUD masks and never edits
// ROM/RAM, object ownership, tile IDs, the renderer, or the visual clock.
bool Video::ExportGraphicsSources(const char *path, std::uint64_t frame) const {
    if (!path || !*path || !m_src.videoRam || !m_src.cpsA ||
        !m_src.cpsB || !m_src.objectRegs) return false;
    std::FILE *out = std::fopen(path, "wb");
    if (!out) return false;
    const unsigned control = m_src.cpsB[m_src.board.layerControl / 2];
    const unsigned vcontrol = m_src.cpsA[kVideoControl];
    std::fprintf(out, "# 87.488 F6 source audit. Current native RAM and LATCHED OAM, no filter.\n");
    std::fprintf(out, "# Raster register changes may differ across scanlines; maps are potential sources, not proof of visible ink.\n");
    std::fprintf(out, "registers\tframe\tlayer_control\tvideo_control\tpriority\tobj_x_offset\tobj_y_offset\tgame_step\tfight_status\n");
    std::fprintf(out, "registers\t%llu\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t%02X\n",
        static_cast<unsigned long long>(frame), control, vcontrol, m_priorityControl,
        m_src.objectRegs[kObjXOffset], m_src.objectRegs[kObjYOffset],
        hudShockGameStep, hudShockPhase & 0xFF);
    std::fprintf(out, "scroll\tframe\tlayer\tenabled\tx\ty\tmap_base\tmap_offset\ttile_px\tcol\trow\tcode\tattr\tpal\tscroll_x\tscroll_y\trow_scroll\n");
    // Read every native 8-pixel sample from all three register-specified
    // scroll maps. These are REAL source entries, not inferred image colours.
    for (int which = 0; which < 3; ++which) {
        const unsigned base = (static_cast<unsigned>(m_src.cpsA[kScroll1Base + which]) * 256u) & ~0x3FFFu & 0x3FFFFu;
        const unsigned shift = 3u + static_cast<unsigned>(which);
        const unsigned tilePx = 1u << shift;
        const unsigned mapMask = 64u * tilePx - 1u;
        const unsigned scrollY = m_src.cpsA[kScroll1X + 2 * which + 1];
        const bool rowOn = which == 1 && (vcontrol & 1u);
        const bool enabled = (layerMask & (1u << (which + 1))) &&
            (control & (1u << (which + 1))) &&
            (which == 0 || (vcontrol & (1u << (which + 1))));
        for (int y = 0; y < kHeight; y += 8) {
            const unsigned rasterY = static_cast<unsigned>(y + kFirstLine);
            const unsigned row = ((rasterY + scrollY) & mapMask) >> shift;
            const unsigned rowScroll = rowOn ? Word(&m_src.videoRam[
                ((static_cast<unsigned>(m_src.cpsA[kOtherBase]) * 256u) & ~0x7FFu & 0x3FFFFu) +
                (((rasterY + m_src.cpsA[kRowScrollOffset]) & 0x3FFu) * 2u)]) : 0u;
            const int ownLeft = wide.side + wide.scroll[which] +
                (rowOn && wide.rows ? wide.rows[(rasterY + m_src.cpsA[kRowScrollOffset]) & 0x3FFu] : 0);
            const unsigned scrollX = static_cast<unsigned>(m_src.cpsA[kScroll1X + 2 * which]) + rowScroll;
            const unsigned firstX = static_cast<unsigned>(kFirstColumn - ownLeft + static_cast<int>(scrollX)) & mapMask;
            for (int x = 0; x < Width(); x += 8) {
                const unsigned col = ((firstX + static_cast<unsigned>(x)) & mapMask) >> shift;
                unsigned index = 0u;
                if (which == 0) index = (row & 0x1Fu) + (col << 5) + ((row & 0x20u) << 6);
                else if (which == 1) index = (row & 0x0Fu) + (col << 4) + ((row & 0x30u) << 6);
                else index = (row & 0x07u) + (col << 3) + ((row & 0x38u) << 6);
                const unsigned at = base + index * 4u;
                const unsigned code = Word(&m_src.videoRam[at]);
                const unsigned attr = Word(&m_src.videoRam[at + 2u]);
                std::fprintf(out, "scroll\t%llu\t%d\t%d\t%d\t%d\t%05X\t%05X\t%u\t%u\t%u\t%04X\t%04X\t%02X\t%u\t%u\t%u\n",
                    static_cast<unsigned long long>(frame), which + 1, int(enabled),
                    x, y, base, at, tilePx, col, row, code, attr, attr & 31u,
                    scrollX, scrollY, rowScroll);
            }
        }
    }
    std::fprintf(out, "oam\tframe\tslot\tx_word\ty_word\tcode\tattr\tpriority\tpalette\tscreen_x\tscreen_y\twide\ttall\trelative\textra\n");
    for (int i = 0; i <= m_lastObject; i += 4) {
        const unsigned xw = m_objects[i], yw = m_objects[i + 1];
        const unsigned code = m_objects[i + 2] + ((yw & 0x6000u) << 3);
        const unsigned attr = m_objects[i + 3];
        const bool relative = (attr & 0x80u) != 0;
        const unsigned sx = (xw + (relative ? m_src.objectRegs[kObjXOffset] : 0u) +
                             kFirstColumn - m_src.objectRegs[kObjXOffset]) & 0x3FFu;
        const unsigned sy = (yw + (relative ? m_src.objectRegs[kObjYOffset] : 0u) +
                             kFirstLine - m_src.objectRegs[kObjYOffset]) & 0x3FFu;
        std::fprintf(out, "oam\t%llu\t%d\t%04X\t%04X\t%05X\t%04X\t%u\t%u\t%d\t%d\t%u\t%u\t%d\t%d\n",
            static_cast<unsigned long long>(frame), i / 4, xw, yw, code, attr,
            (xw >> 13) & 7u, attr & 31u,
            static_cast<int>(sx) - kFirstColumn, static_cast<int>(sy) - kFirstLine,
            ((attr >> 8) & 15u) + 1u, ((attr >> 12) & 15u) + 1u,
            int(relative), int(m_objectsFromExtra[i / 4] != 0));
    }
    // A direct machine-readable companion to the F9 on-screen ledger.
    // Source records and graphics ROM offsets are NOT semantic HUD names
    // and do not by themselves prove that pixels survived occlusion.
    const GraphicsInspection inspector = InspectGraphics();
    std::fprintf(out,
        "f9_source\tframe\tkind\tindex\tbank\tsource_address_or_bank_offset\t"
        "gfx_tile\tgfx_rom_offset\tattr\tpriority\tpalette\t"
        "screen_x\tscreen_y\tw\th\tenabled\n");
    for (const GraphicRecord &item : inspector.records) {
        std::fprintf(out,
            "f9_source\t%llu\t%u\t%u\t%u\t%06X\t"
            "%05X\t%07X\t%04X\t%u\t%u\t"
            "%d\t%d\t%d\t%d\t%d\n",
            static_cast<unsigned long long>(frame), item.kind, item.index,
            item.bank, item.sourceAddress, item.tile, item.gfxOffset,
            item.attributes, item.priority, item.palette,
            item.x, item.y, item.w, item.h, int(item.enabled));
    }
    return std::fclose(out) == 0;
}

// The palette in video RAM is up to six pages of 512 colours (objects, scroll 1-3, two for stars);
// the control register says which pages are there. A colour is 4 bits each of brightness, red, green, blue.
void Video::CopyPalette() {
    const uint32_t base = (static_cast<uint32_t>(m_src.cpsA[kPaletteBase]) * 256) & ~0x3FFu & 0x3FFFF;
    const uint16_t control = m_src.cpsB[m_src.board.paletteControl / 2];
    uint32_t at = base;
    for (int page = 0; page < 6; page++) {
        if (control & (1u << page)) {
            for (int n = 0; n < 0x200; n++) {
                const uint16_t colour = Word(&m_src.videoRam[at & 0x3FFFF]);
                at += 2;
                const unsigned bright = 0x0F + ((colour >> 12) << 1);
                const unsigned r = ((colour >> 8) & 0x0F) * 0x11 * bright / 0x2D;
                const unsigned g = ((colour >> 4) & 0x0F) * 0x11 * bright / 0x2D;
                const unsigned b = (colour & 0x0F) * 0x11 * bright / 0x2D;
                m_palette[0x200 * page + n] = r | (g << 8) | (b << 16);
            }
        } else if (at != base) {
            at += 0x400; // (a page left out after the first still takes its room)
        }
    }
}

void Video::LatchObjects(const uint8_t *objectRam, const uint8_t *fromExtra) {
    m_priorityControl = m_src.objectRegs[kObjPriority];
    for (int n = 0; n < 0x1000; n++) {
        m_objects[n] = Word(&objectRam[n * 2]);
    }
    if (fromExtra) {
        memcpy(m_objectsFromExtra, fromExtra, sizeof(m_objectsFromExtra));
    }
    // The list ends at the first entry marked so
    m_lastObject = 0x1000 - 4;
    for (int n = 0; n < 0x1000; n += 4) {
        if (m_objects[n + 1] >= 0x8000 || m_objects[n + 3] >= 0xFF00) {
            m_lastObject = n - 4;
            break;
        }
    }
    if (objectsTaken) {
        objectsTaken(m_objects, m_lastObject);
    }
}

// One scroll screen's pixels on a raster line. which: 0 (8-pixel tiles), 1 (16), 2 (32).
template <bool kWide>
void Video::DrawScroll(int which, int rasterLine, uint16_t *pens, uint8_t *priority, uint8_t mark, uint8_t *high) {
    const uint16_t *a = m_src.cpsA;
    const uint32_t base = (static_cast<uint32_t>(a[kScroll1Base + which]) * 256) & ~0x3FFFu & 0x3FFFF;
    const uint8_t *map = &m_src.videoRam[base];
    const int shift = 3 + which;              // tile size as a power of two
    const int size = 1 << shift;
    const uint32_t mapMask = (64u << shift) - 1; // the map is 64 x 64 tiles
    uint32_t scrollX = a[kScroll1X + which * 2];
    const uint32_t scrollY = a[kScroll1X + which * 2 + 1];
    if (which == 1 && (a[kVideoControl] & 1)) {
        // Scroll 2 can have its own horizontal position on every line
        const uint32_t other = (static_cast<uint32_t>(a[kOtherBase]) * 256) & ~0x7FFu & 0x3FFFF;
        scrollX += Word(&m_src.videoRam[other + (((rasterLine + a[kRowScrollOffset]) & 0x3FF) * 2)]);
    }
    const uint32_t y = (rasterLine + scrollY) & mapMask;
    const uint32_t row = y >> shift;
    const int width = kWide ? Width() : +kWidth;
    const bool rowScroll = which == 1 && (a[kVideoControl] & 1);
    const int ownLeft = kWide ? wide.side + wide.scroll[which] + (rowScroll && wide.rows ? wide.rows[(rasterLine + a[kRowScrollOffset]) & 0x3FF] : 0) : 0;
    const int ownRight = ownLeft + kWidth; // the board's own columns
    const uint32_t firstX = static_cast<uint32_t>(kFirstColumn - ownLeft + static_cast<int>(scrollX)) & mapMask;
    if (layerProbe) {
        LayerLine &probe = layerProbe[which][rasterLine - kFirstLine];
        probe.row = static_cast<int32_t>(rasterLine + scrollY);
        probe.left = kFirstColumn - ownLeft + static_cast<int>(scrollX);
    }
    const uint16_t colourBase = static_cast<uint16_t>(0x20 * (which + 1));

    int out = -static_cast<int>(firstX & (size - 1)); // where the first (partly hidden) tile starts
    uint32_t column = firstX >> shift;
    for (; out < width; out += size, column = (column + 1) & 63) {
        uint32_t index;
        switch (which) {
        case 0: index = (row & 0x1F) + (column << 5) + ((row & 0x20) << 6); break;
        case 1: index = (row & 0x0F) + (column << 4) + ((row & 0x30) << 6); break;
        default: index = (row & 0x07) + (column << 3) + ((row & 0x38) << 6); break;
        }
        uint32_t code = Word(&map[index * 4]);
        uint16_t attr = Word(&map[index * 4 + 2]);
        if (kWide && wide.tile && (out < ownLeft || out + size > ownRight)) {
            // (beyond the board's own columns the game's sources may know better than the map: video.h)
            uint16_t itsCode = 0, itsAttr = 0;
            if (wide.tile(which, static_cast<int>(column), static_cast<int>(row), out, itsCode, itsAttr)) {
                code = itsCode;
                attr = itsAttr;
            }
        }
        // Where the tile is in the graphics ROMs: a CPS-2's scroll screens' tiles are in the second 8 MB; a CPS-1's
        // are where the objects' are (its board's PAL maps tile numbers to ROMs; the games here have them in order)
        const uint32_t tiles = m_src.board.cps1 ? 0 : 0x800000;
        uint32_t at;
        int rowBytes;
        switch (which) {
        case 0:
            at = tiles + code * 64 + ((column & 1) ? 4 : 0); // (odd columns use the right half of a 16-pixel strip)
            rowBytes = 8;
            break;
        case 1:
            at = tiles + code * 128;
            rowBytes = 8;
            break;
        default:
            code &= 0x3FFF;
            at = tiles + code * 512;
            rowBytes = 16;
            break;
        }
        if (!m_src.board.gfxMap.empty()) {
            // (a CPS-1 whose PAL does not keep the tiles in order: Street Fighter II's)
            static const uint8_t kKinds[3] = {GfxRange::kScroll1, GfxRange::kScroll2, GfxRange::kScroll3};
            const int64_t mapped = MapTile(m_src.board, kKinds[which], code << (which == 0 ? 0 : which == 1 ? 1 : 3));
            if (mapped < 0) {
                continue;
            }
            at = static_cast<uint32_t>(mapped) * 64 + (which == 0 && (column & 1) ? 4 : 0);
        }
        const int line = static_cast<int>(y & (size - 1));
        const bool flipX = attr & 0x20, flipY = attr & 0x40;
        at += static_cast<uint32_t>(flipY ? size - 1 - line : line) * rowBytes;
        // (a tile of the extra set's: in its graphics)
        const bool extra = m_extraGraphics && m_scrollFromExtra && m_scrollFromExtra[(base + index * 4) / 4];
        const uint8_t *graphics = extra ? m_extraGraphics : m_src.graphics;
        if (at + rowBytes > (extra ? m_extraGraphicsSize : m_src.graphicsSize)) {
            continue;
        }
        const uint16_t colour = static_cast<uint16_t>(((attr & 0x1F) + colourBase) * 16);
        uint8_t pixels[32];
        for (int part = 0; part < size / 8; part++) {
            Unpack(&graphics[at + part * 4], &pixels[part * 8]);
        }
        for (int k = 0; k < size; k++) {
            const int x = out + k;
            if (x < 0 || x >= width) {
                continue;
            }
            const uint8_t pen = pixels[flipX ? size - 1 - k : k];
            if (pen != 15) {
                pens[x] = static_cast<uint16_t>(colour + pen);
                priority[x] |= mark;
                if (high) {
                    // (a CPS-1's: the tile's group picks a priority mask, whose pens are in front of the objects)
                    high[x] = (m_src.cpsB[m_src.board.priority[(attr >> 7) & 3] / 2] >> pen) & 1;
                }
            }
        }
    }
}

// The objects on a raster line. Each entry: x, y, tile, attributes; an entry can be a block of up to
// 16 x 16 tiles. An object's priority (0-7) against the scroll screens is in its x word; among objects,
// later entries of the list are in front. Drawn back to front here would need the scroll screens' marks
// kept apart, so (as MAME does) they are drawn front to back and a pixel once claimed stays claimed.
template <bool kWide>
void Video::DrawObjects(int rasterLine, uint16_t *pens, uint8_t *priority, const uint32_t *masks,
                        uint8_t *marks, uint8_t *hudInk) {
    const int xOffset = 64 - m_src.objectRegs[kObjXOffset];
    const int yOffset = 16 - m_src.objectRegs[kObjYOffset];
    const int width = kWide ? Width() : +kWidth;
    for (int i = m_lastObject; i >= 0; i -= 4) {
        int x = m_objects[i];
        int y = m_objects[i + 1];
        const unsigned level = (x >> 13) & 7;
        const uint32_t code = m_objects[i + 2] + ((y & 0x6000u) << 3);
        const uint16_t attr = m_objects[i + 3];
        if (hideObject && hideObject(code, attr)) {
            continue;
        }
        if (attr & 0x80) {
            x += m_src.objectRegs[kObjXOffset];
            y += m_src.objectRegs[kObjYOffset];
        }
        const int across = ((attr >> 8) & 0x0F) + 1, down = ((attr >> 12) & 0x0F) + 1;
        const bool flipX = attr & 0x20, flipY = attr & 0x40;
        const uint16_t colour = static_cast<uint16_t>((attr & 0x1F) * 16);
        // The same native OAM classifier and first-source guard as FBNeo
        // 87.399. These are screen-space coordinates, not world coordinates.
        const int sourceX = ((x + xOffset) & 0x3FF) - kFirstColumn;
        const int sourceY = ((y + yOffset) & 0x3FF) - kFirstLine;
        const int nativeHudPhase = xmvsf_vs::HudObjectPhase(
            hudShockPhase, hudShockGameStep);
        // Preserve the complete 4x3 native wall objects identified by
        // F9/F6 OAM source records; they are never HUD shock candidates.
        const bool wallSource = xmvsf_sources::IsWallBlock({
            static_cast<unsigned>(i / 4), code,
            static_cast<unsigned>(attr), level,
            static_cast<unsigned>(across), static_cast<unsigned>(down)});
        const bool hudSource = hudShock && !wallSource &&
            XM87392::objectForPhase(nativeHudPhase, level, attr & 0x1Fu, code,
                sourceX, sourceY, static_cast<unsigned>(across),
                static_cast<unsigned>(down), false, false);
        if (hudSource && XM87392::verifiedTransitionHud(level, attr & 0x1Fu,
                 code, sourceX, sourceY, static_cast<unsigned>(across),
                 static_cast<unsigned>(down))) {
            XM87392::timeline().noteHudSource(hudShockFrame);
        }
        const unsigned hudStep = hudShock
            ? xmvsf_vs::HudStep(static_cast<unsigned>(nativeHudPhase), hudShockFrame) : XM87392::RevealFrames;
        // The uploaded F10 frame 1451 has the real P1/P2 HP strips:
        // priority 7, palette 2/4, native 0x1598F..0x159B6 tile family.
        // Do not let unrelated timer/VS-screen art start the live shock.
        const bool liveHp = hudSource && level == 7u &&
            XM87392::verifiedTransitionHud(level, attr & 0x1Fu, code,
                sourceX, sourceY, static_cast<unsigned>(across), static_cast<unsigned>(down));
        const bool shockTile = hudSource && hudStep < XM87392::RevealFrames;
        const uint32_t mask = masks[level] | 0x80000000u;
        // (what its pixels are marked as, where it is the front object: the panel's, or in front of the fighters)
        const uint8_t mark = !marks ? 0 : static_cast<uint8_t>((panelObject && panelObject(code, attr) ? kPanelPixel : 0) |
                                                               (markFront && (m_objects[i] & 0x1000) ? kFrontPixel : 0));
        // (an object of the extra set's: its tiles are in its graphics)
        const bool extra = m_extraGraphics && m_objectsFromExtra[i / 4];
        const uint8_t *graphics = extra ? m_extraGraphics : m_src.graphics;
        const size_t graphicsSize = extra ? m_extraGraphicsSize : m_src.graphicsSize;
        // (where the first column drawn is in the object's own count across, which wraps at 1024)
        const int first = kWide ? kFirstColumn - wide.side - (wide.object ? wide.object(code, attr, i / 4) : 0) - 16 : 0;
        for (int ty = 0; ty < down; ty++) {
            const int sy = (y + ty * 16 + yOffset) & 0x3FF;
            if (rasterLine < sy || rasterLine >= sy + 16) {
                continue;
            }
            const int line = rasterLine - sy;
            for (int tx = 0; tx < across; tx++) {
                const int sx = kWide ? ((x + tx * 16 + xOffset - first) & 0x3FF) - 16 : ((x + tx * 16 + xOffset) & 0x3FF) - kFirstColumn;
                if (sx <= -16 || sx >= width) {
                    continue;
                }
                uint32_t tile;
                if (across == 1 && down == 1) {
                    tile = code;
                } else {
                    const int cx = flipX ? across - 1 - tx : tx;
                    const int cy = flipY ? down - 1 - ty : ty;
                    // (a plain block wraps within its row of sixteen tiles; flipped ones do not)
                    tile = (!flipX && !flipY) ? (code & ~0xFu) + ((code + cx) & 0xF) + 0x10 * cy : code + cx + 0x10 * cy;
                }
                const uint32_t at = (tile * 128 + static_cast<uint32_t>(flipY ? 15 - line : line) * 8);
                if (at + 8 > graphicsSize) {
                    continue;
                }
                uint8_t pixels[16];
                Unpack(&graphics[at], pixels);
                Unpack(&graphics[at + 4], pixels + 8);
                for (int k = 0; k < 16; k++) {
                    const int px = sx + k;
                    if (px < 0 || px >= width) {
                        continue;
                    }
                    const uint8_t pen = pixels[flipX ? 15 - k : k];
                    if (pen != 15) {
                        const bool shown = !((mask >> (priority[px] & 0x1F)) & 1);
                        if (shown && liveHp)
                            xmvsf_vs::NoteCombatHudInk(static_cast<unsigned>(nativeHudPhase), hudShockFrame);
                        // Suppressed HUD ink leaves the real scroll/fighter ink
                        // and depth untouched, exactly FBNeo's per-tile restore.
                        if (shown && shockTile &&
                            !XM87392::visible(px, rasterLine - kFirstLine, hudStep) &&
                            !XM87392::shineCore(px, rasterLine - kFirstLine, hudStep))
                            continue;
                        if (shown) {
                            pens[px] = static_cast<uint16_t>(colour + pen);
                            if (m_inkReady)
                                RecordInk(static_cast<unsigned>(i / 4),
                                          px, rasterLine - kFirstLine);
                            if (hudInk && shockTile) hudInk[px] = 1;
                        }
                        if (marks && priority[px] != 31) {
                            marks[px] = static_cast<uint8_t>(mark & (shown ? 0xFF : ~kFrontPixel));
                        }
                        priority[px] = 31;
                    }
                }
            }
        }
    }
}

void Video::LatchObjectsCps1() {
    m_priorityControl = 0;
    const uint32_t base = (static_cast<uint32_t>(m_src.cpsA[0]) * 256) & ~0x7FFu & 0x3FFFF;
    std::fill(std::begin(m_objects), std::end(m_objects), 0);
    for (int n = 0; n < 0x400 && base + n * 2 + 1 < 0x30000; n++) {
        m_objects[n] = Word(&m_src.videoRam[base + n * 2]);
    }
    // The list ends at the first entry whose attributes' high byte is FF
    m_lastObject = 0x400 - 4;
    for (int n = 0; n < 0x400; n += 4) {
        if ((m_objects[n + 3] & 0xFF00) == 0xFF00) {
            m_lastObject = n - 4;
            break;
        }
    }
    if (objectsTaken) {
        objectsTaken(m_objects, m_lastObject);
    }
}

// A CPS-1's objects on a raster line: entries of x, y, tile and attributes (colour, flips, a block of up to 16 x 16
// tiles whose numbers go on within their row of sixteen), the first entry in front. Each is drawn in front of the
// layers below it but where `high` is set (the high pens of the scroll screen just below the objects).
void Video::DrawObjectsCps1(int rasterLine, uint16_t *pens, const uint8_t *high, uint16_t *drawn) {
    const int width = Width();
    uint8_t claimed[kMaxWidth] = {};
    for (int i = 0; i <= m_lastObject; i += 4) {
        const int x = m_objects[i], y = m_objects[i + 1];
        const uint32_t code = m_objects[i + 2];
        const uint16_t attr = m_objects[i + 3];
        if (hideObject && hideObject(code, attr)) {
            continue;
        }
        const bool blocks = (attr & 0xFF00) != 0;
        const int across = blocks ? ((attr >> 8) & 0x0F) + 1 : 1, down = blocks ? ((attr >> 12) & 0x0F) + 1 : 1;
        const bool flipX = attr & 0x20, flipY = attr & 0x40;
        const uint16_t colour = static_cast<uint16_t>((attr & 0x1F) * 16);
        const int shift = wide.side - kFirstColumn - (wide.object ? wide.object(code, attr, i / 4) : 0);
        for (int ty = 0; ty < down; ty++) {
            const int sy = (y + ty * 16) & 0x1FF;
            if (rasterLine < sy || rasterLine >= sy + 16) {
                continue;
            }
            const int line = rasterLine - sy;
            for (int tx = 0; tx < across; tx++) {
                const int sx = ((x + tx * 16) & 0x1FF) + shift;
                if (sx <= -16 || sx >= width) {
                    continue;
                }
                const int cx = flipX ? across - 1 - tx : tx, cy = flipY ? down - 1 - ty : ty;
                const uint32_t tile = (code & ~0xFu) + ((code + cx) & 0xF) + 0x10 * cy;
                uint32_t at = tile * 128 + static_cast<uint32_t>(flipY ? 15 - line : line) * 8;
                if (!m_src.board.gfxMap.empty()) {
                    // (through the graphics mapper, in units of 8x8 tiles as the scroll screens; a tile it has no
                    // range for is not drawn)
                    const int64_t mapped = MapTile(m_src.board, GfxRange::kObjects, tile << 1);
                    if (mapped < 0) {
                        continue;
                    }
                    at = static_cast<uint32_t>(mapped) * 64 + static_cast<uint32_t>(flipY ? 15 - line : line) * 8;
                }
                if (at + 8 > m_src.graphicsSize) {
                    continue;
                }
                uint8_t pixels[16];
                Unpack(&m_src.graphics[at], pixels);
                Unpack(&m_src.graphics[at + 4], pixels + 8);
                for (int k = 0; k < 16; k++) {
                    const int px = sx + k;
                    if (px < 0 || px >= width || claimed[px]) {
                        continue;
                    }
                    const uint8_t pen = pixels[flipX ? 15 - k : k];
                    if (pen != 15) {
                        claimed[px] = 1;
                        if (!high[px]) {
                            pens[px] = static_cast<uint16_t>(colour + pen);
                            if (drawn) {
                                drawn[px] = pens[px];
                            }
                        }
                    }
                }
            }
        }
    }
}

// A CPS-1's line: the layer control orders the four layers back to front (0: the objects; 1-3: the scroll screens,
// each on as its enable bit and the video control say); the layer just below the objects keeps its high pens in front
// of them (MAME's cps1_render_high_layer)
void Video::RenderLineCps1(int line, uint32_t *out) {
    const int rasterLine = line + kFirstLine;
    const int width = Width();
    uint16_t pens[kMaxWidth];
    uint8_t priority[kMaxWidth], high[kMaxWidth];
    for (int x = 0; x < width; x++) {
        pens[x] = 0xBFF;
    }
    memset(priority, 0, sizeof(priority));
    memset(high, 0, sizeof(high));
    uint16_t drawn[kMaxWidth]; // (markObjects: the pen an object drew at each pixel, 0xFFFF: none)
    if (markObjects) {
        memset(drawn, 0xFF, sizeof(drawn));
    }
    const Board &board = m_src.board;
    const uint16_t control = m_src.cpsB[board.layerControl / 2];
    const uint16_t videoControl = m_src.cpsA[kVideoControl];
    const unsigned order[4] = {(control >> 6) & 3u, (control >> 8) & 3u, (control >> 10) & 3u, (control >> 12) & 3u};
    for (int n = 0; n < 4; n++) {
        const unsigned layer = order[n];
        if (layer == 0) {
            if (layerMask & 1) {
                DrawObjectsCps1(rasterLine, pens, high, markObjects ? drawn : nullptr);
            }
            continue;
        }
        const bool on = (layerMask & (1u << layer)) && (control & board.enable[layer - 1]) &&
                        (layer == 1 || (videoControl & (layer == 2 ? 0x04 : 0x08)));
        if (on) {
            const bool belowObjects = n < 3 && order[n + 1] == 0;
            if (belowObjects) {
                memset(high, 0, sizeof(high));
            }
            if (wide.side || wide.scroll[0] || wide.scroll[1] || wide.scroll[2] || wide.rows || wide.tile) {
                DrawScroll<true>(static_cast<int>(layer) - 1, rasterLine, pens, priority, static_cast<uint8_t>(1u << n), belowObjects ? high : nullptr);
            } else {
                DrawScroll<false>(static_cast<int>(layer) - 1, rasterLine, pens, priority, static_cast<uint8_t>(1u << n), belowObjects ? high : nullptr);
            }
        }
    }
    for (int x = 0; x < width; x++) {
        out[x] = m_palette[pens[x]];
    }
    if (markObjects) {
        // (an object's pixel where nothing in front of it has drawn since: as RenderLineAs marks them; and a marked
        // scroll screen's, told by its pen's page: page n is scroll screen n's)
        for (int x = 0; x < width; x++) {
            const unsigned page = pens[x] >> 9;
            const bool layer = drawn[x] != pens[x] && pens[x] != 0xBFF && page >= 1 && page <= 3 && ((markLayers >> page) & 1);
            out[x] |= drawn[x] == pens[x] || layer ? 0x80000000u | (static_cast<uint32_t>(pens[x] >> 4) & 0x1F) << 24 : 0;
        }
    }
}

void Video::RenderLine(int line, uint32_t *out) {
    if (m_src.board.cps1) {
        RenderLineCps1(line, out);
        return;
    }
    if (wide.side || wide.scroll[0] || wide.scroll[1] || wide.scroll[2] || wide.rows || wide.object || wide.tile) {
        RenderLineAs<true>(line, out);
    } else {
        RenderLineAs<false>(line, out);
    }
}

// Pixels that nothing was drawn at (the backdrop's pen), in runs no wider than the added side, take the line's
// own picture mirrored at the nearest drawn pixel (or that pixel where the mirror finds nothing). Wider runs are
// left: a stage that shows the backdrop on purpose shows more of it than that.
void Video::FillHoles(uint16_t *pens, int width) const {
    constexpr uint16_t kBackdrop = 0xBFF;
    uint16_t drawn[kMaxWidth]; // (the line as drawn: what is mirrored)
    memcpy(drawn, pens, static_cast<size_t>(width) * sizeof(uint16_t));
    for (int start = 0; start < width;) {
        if (drawn[start] != kBackdrop) {
            start++;
            continue;
        }
        int end = start;
        while (end < width && drawn[end] == kBackdrop) {
            end++;
        }
        const bool left = start > 0, right = end < width;
        if (end - start <= wide.side && (left || right)) {
            for (int x = start; x < end; x++) {
                // (mirrored at the nearer drawn pixel; at the line's ends, the one there is)
                const bool fromLeft = left && (!right || x - (start - 1) <= end - x);
                const int edge = fromLeft ? start - 1 : end;
                const int mirror = 2 * edge - x;
                pens[x] = mirror >= 0 && mirror < width && drawn[mirror] != kBackdrop ? drawn[mirror] : drawn[edge];
            }
        }
        start = end;
    }
}

template <bool kWide>
void Video::RenderLineAs(int line, uint32_t *out) {
    const int rasterLine = line + kFirstLine;
    const int width = kWide ? Width() : +kWidth;
    uint16_t pens[kMaxWidth];
    uint8_t priority[kMaxWidth];
    for (int x = 0; x < width; x++) {
        pens[x] = 0xBFF; // the backdrop colour
    }
    memset(priority, 0, sizeof(priority));

    // The layer control register orders four layers back to front (0: the CPS-1's objects, which the
    // CPS-2 does not have; 1-3: the scroll screens) and switches the scroll screens on
    const uint16_t control = m_src.cpsB[m_src.board.layerControl / 2];
    const uint16_t videoControl = m_src.cpsA[kVideoControl];
    unsigned l0 = (control >> 6) & 3, l1 = (control >> 8) & 3, l2 = (control >> 10) & 3, l3 = (control >> 12) & 3;
    unsigned p0 = (m_priorityControl >> (4 * l0)) & 0xF, p1 = (m_priorityControl >> (4 * l1)) & 0xF;
    unsigned p2 = (m_priorityControl >> (4 * l2)) & 0xF, p3 = (m_priorityControl >> (4 * l3)) & 0xF;
    if (l0 == 0) {
        l0 = l1;
        l1 = 0;
        p0 = p1;
    }
    if (l1 == 0) {
        l1 = l2;
        l2 = 0;
        p1 = p2;
    }
    if (l2 == 0) {
        l2 = l3;
        l3 = 0;
        p2 = p3;
    }
    // Which objects each scroll screen hides: an object of level n is in front of the screens whose
    // priority is below n. masks[n] has a bit for every combination of screens (bit 0: the back one ...)
    // behind which such an object is not drawn.
    uint32_t masks[8] = {};
    {
        uint32_t mask0 = 0xAA, mask1 = 0xCC;
        if (p0 > p1) {
            mask0 &= ~0x88u;
        }
        if (p0 > p2) {
            mask0 &= ~0xA0u;
        }
        if (p1 > p2) {
            mask1 &= ~0xC0u;
        }
        masks[0] = 0xFF;
        for (unsigned n = 1; n < 8; n++) {
            if (n <= p0 && n <= p1 && n <= p2) {
                masks[n] = 0xFE;
                continue;
            }
            masks[n] = 0;
            if (n <= p0) {
                masks[n] |= mask0;
            }
            if (n <= p1) {
                masks[n] |= mask1;
            }
            if (n <= p2) {
                masks[n] |= 0xF0;
            }
        }
    }
    auto enabled = [&](unsigned layer) {
        if (!(layerMask & (1u << layer))) {
            return false;
        }
        switch (layer) {
        case 1: return (control & 0x02) != 0;
        case 2: return (control & 0x04) != 0 && (videoControl & 0x04) != 0;
        case 3: return (control & 0x08) != 0 && (videoControl & 0x08) != 0;
        default: return false;
        }
    };
    const unsigned order[3] = {l0, l1, l2};
    if (layerProbe) {
        for (int which = 0; which < 3; which++) {
            layerProbe[which][line] = LayerLine{};
        }
    }
    for (int n = 0; n < 3; n++) {
        if (order[n] && enabled(order[n])) {
            DrawScroll<kWide>(static_cast<int>(order[n]) - 1, rasterLine, pens, priority, static_cast<uint8_t>(1u << n));
        }
    }
    if (layerProbe) {
        // (the columns where each is seen: it is the front one painted there, or none is and it is drawn on the line)
        for (int x = 0; x < width; x++) {
            const unsigned marks = priority[x] & 7;
            for (int n = 0; n < 3; n++) {
                if (!order[n] || layerProbe[order[n] - 1][line].row < 0 || (marks ? (marks >> n) != 1 : false)) {
                    continue;
                }
                LayerLine &probe = layerProbe[order[n] - 1][line];
                probe.first = probe.first < 0 ? static_cast<int16_t>(x) : probe.first;
                probe.last = static_cast<int16_t>(x);
            }
        }
    }
    // (the holes filled from the scroll screens alone, before the objects: a fighter or the panel beside a hole is not
    // mirrored into it, and the objects are drawn over what fills it as over the stage)
    if (kWide && wide.fillHoles && wide.side > 0) {
        FillHoles(pens, width);
    }
    // (the marks of the objects' pixels: the panel's, those in front of the fighters)
    uint8_t marks[kMaxWidth];
    const bool marking = (markObjects && panelObject) || markFront;
    if (marking) {
        memset(marks, 0, sizeof(marks));
    }
    uint8_t hudInk[kMaxWidth] = {};
    if (layerMask & 1) {
        DrawObjects<kWide>(rasterLine, pens, priority, masks,
                           marking ? marks : nullptr, hudInk);
    }
    for (int x = 0; x < width; x++) {
        out[x] = m_palette[pens[x]];
        if (hudInk[x]) {
            // FBNeo's shineRGB uses 00RRGGBB; recomp's output is
            // 00BBGGRR. Swap endpoints, run the IDENTICAL color math,
            // then swap back. Only actual HUD ink is tinted.
            const uint32_t source = out[x];
            const uint32_t rgb = ((source & 0xFFu) << 16) |
                                 (source & 0xFF00u) |
                                ((source >> 16) & 0xFFu);
            const uint32_t light = XM87392::shineRGB(rgb, x, line,
                xmvsf_vs::HudStep(static_cast<unsigned>(xmvsf_vs::HudObjectPhase(hudShockPhase, hudShockGameStep)), hudShockFrame));
            out[x] = (source & 0xFF000000u) |
                     ((light & 0xFFu) << 16) |
                      (light & 0xFF00u) |
                     ((light >> 16) & 0xFFu);
        }
    }
    if (markFront) {
        // (in front: an object marked so, or a scroll screen's pixel an object of the fighters' level is behind)
        const uint32_t behind = frontLevel ? masks[frontLevel & 7] : 0;
        for (int x = 0; x < width; x++) {
            const bool front = priority[x] == 31 ? (marks[x] & kFrontPixel) != 0 : ((behind >> (priority[x] & 7)) & 1) != 0;
            out[x] |= front ? 0x40000000u : 0;
        }
    }
    if (markObjects) {
        // (the marks of the scroll screens marked too, by where they are in the order)
        unsigned marked = 0;
        for (int n = 0; n < 3; n++) {
            if (order[n] && (markLayers & (1u << order[n]))) {
                marked |= 1u << n;
            }
        }
        for (int x = 0; x < width; x++) {
            const bool layer = priority[x] != 31 && (priority[x] & marked) != 0;
            const bool mark = priority[x] == 31 || layer;
            const bool panel = layer || (priority[x] == 31 && marking && (marks[x] & kPanelPixel));
            out[x] |= mark ? 0x80000000u | (static_cast<uint32_t>(pens[x] >> 4) & 0x1F) << 24 | (panel && panelObject ? 0x40000000u : 0) : 0;
        }
    }
}

void Video::Serialize(Archive &ar) {
    ar.Mark(0x56494445);
    ar.Pod(m_palette);
    ar.Pod(m_objects);
    ar.Pod(m_priorityControl);
    ar.Pod(m_lastObject);
    if (m_extraGraphics) {
        ar.Pod(m_objectsFromExtra);
    }
}

} // namespace cps2recomp
