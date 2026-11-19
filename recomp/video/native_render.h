// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The picture drawn from the native interface's records alone (docs/NATIVE_VIDEO.md, check b): the palette as the
// game's uploads made it, the object list the game handed over, and on each line the layers' registers as the game
// set them; with the game's own data (the scroll screens' maps and the row scroll's table in video RAM) and the
// graphics ROMs. No register of the board's is read. What the game's sources set on the picture (a wider picture, its
// screens and objects moved, objects hidden, marks: PictureOptions) is followed as video.cpp follows it.
//
// It is made the way a GPU would draw it: once a frame the object list is turned into tiles (`ObjectTile`: one 16x16
// tile each, front to back), which a GPU would draw as instances; on each line the scroll screens' tiles and the
// objects' are put down in the layers' order, the objects behind or in front of each screen's pens by their level.
// This one draws them on the CPU, a line at a time, as the reference the GPU's is to equal; --video check compares
// its lines with video.cpp's.
#pragma once

#include "records.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace cps2recomp {
class Archive;
}

namespace cps2recomp::native_video {

// A 16x16 tile of an object, as a GPU would have it in an instance: where (before the object registers' offsets,
// which an object marked so takes and which can change part way down the picture), which tile, its colour set, flips
// and level (0-7, against the scroll screens); and the entry it is of (its number in the list, its first tile and its
// attributes: what a game's sources are asked of it)
struct ObjectTile {
    int16_t x = 0, y = 0;
    uint32_t tile = 0;
    uint16_t colour = 0; // (the first pen of its colour set)
    bool flipX = false, flipY = false, relative = false;
    uint8_t level = 0;
    uint16_t entry = 0;
    uint32_t entryTile = 0;
    uint16_t attributes = 0;
    // (for the GPU, which places each tile once a frame: what the game's sources said of its entry when the list was
    // taken, PictureOptions::object and hideObject)
    int16_t shift = 0;
    bool hidden = false;
    bool extra = false; // (drawn from another set's graphics: ObjectList::fromExtra)
    uint8_t marks = 0;  // (its pixels' marks: the panel's, in front of the fighters; Video::kPanelPixel, kFrontPixel)
};

// A tile of a scroll screen beyond the board's own columns that the game's sources give in the map's place
// (PictureOptions::tile), on a line: for the GPU, which cannot ask them
struct TileOverride {
    int16_t at = 0; // (the picture's column its left edge is at)
    uint8_t which = 0;
    uint16_t code = 0, attributes = 0;
};

class Raster {
public:
    static constexpr int kWidth = 384, kWideSide = 64, kMaxSide = 192, kMaxWidth = kWidth + 2 * kMaxSide;

    void SetGraphics(const uint8_t *graphics, size_t size) {
        m_graphics = graphics;
        m_graphicsSize = size;
    }
    // A CPS-1 (its objects' list, its layers' order and their enable bits: Board::enable, the bits of the layer control
    // that switch scroll screens 1-3 on)
    void SetCps1(const uint8_t *enable) {
        m_cps1 = true;
        for (int n = 0; n < 3; n++) {
            m_enable[n] = enable[n];
        }
    }
    bool Cps1() const {
        return m_cps1;
    }
    // (a CPS-1 whose graphics mapper does not keep the tiles in order, Street Fighter II's: machine/romset.h, GfxTable)
    void SetGfx(const GfxTable *gfx) {
        m_gfx = gfx && gfx->on ? gfx : nullptr;
    }
    const GfxTable *Gfx() const {
        return m_gfx;
    }
    // (an object's tile the mapper has nowhere: past every graphics' end, so not drawn)
    static constexpr uint32_t kNoTile = 0x00FFFFFF;
    const uint8_t *Enable() const {
        return m_enable;
    }
    // Another set's graphics, which the objects and tiles marked so are drawn from (Vampire Savior Definitive's)
    void SetExtraGraphics(const uint8_t *graphics, size_t size) {
        m_extraGraphics = graphics;
        m_extraGraphicsSize = size;
    }
    // The palette: the colours an upload takes (its pages), as the board's palette memory keeps them
    void Upload(const PaletteUpload &upload);
    // The frame's objects: the list the game handed over, as the board takes it at the frame's start (and as the
    // game's sources change it then: PictureOptions::objectsTaken)
    void Objects(const ObjectList &list, const PictureOptions &options);
    // A line of the picture (0-223), from the layers' registers it is drawn with and video RAM (the game's maps and
    // its row scroll's table): kWidth + 2 x options.side pixels, red in the low byte, then green, then blue, and
    // the marks above (Video::RenderLine's)
    // (`scrollExtra`: with another set's graphics, which of the maps' tiles are drawn from them, a byte for each four of
    // video RAM)
    void Line(int line, const Layers &layers, const uint8_t *videoRam, const PictureOptions &options, uint32_t *out,
              const uint8_t *scrollExtra = nullptr) const;
    // The tiles the game's sources give beyond the board's columns on a line (options.tile asked, as Line asks it)
    void Overrides(int line, const Layers &layers, const uint8_t *videoRam, const PictureOptions &options,
                   std::vector<TileOverride> &out) const;

    // (for the GPU, which draws the same: the colours, the frame's tiles and the priority control)
    const uint32_t *Colours() const {
        return m_colours;
    }
    const std::vector<ObjectTile> &Tiles() const {
        return m_tiles;
    }
    uint16_t Priority() const {
        return m_priority;
    }
    // (the scene export, port/scene.h: only the objects of these levels kept, bit n level n, to draw them alone)
    void KeepLevels(unsigned levels) {
        std::erase_if(m_tiles, [levels](const ObjectTile &t) { return !((levels >> t.level) & 1); });
    }
    // (in a save state, as the game's side's: native.h, NativeVideo::Serialize)
    void Serialize(Archive &ar);
    // (as the board's picture starts: no colours, no objects, Video::Reset)
    void Reset() {
        memset(m_colours, 0, sizeof(m_colours));
        m_tiles.clear();
        m_priority = 0;
    }

private:
    void DrawScroll(int which, int rasterLine, const Layers &layers, const uint8_t *videoRam, const PictureOptions &options, int width,
                    uint16_t *pens, uint8_t *priority, uint8_t mark, const uint8_t *scrollExtra, uint8_t *high = nullptr) const;
    // (a CPS-1's line: Video::RenderLineCps1)
    void LineCps1(int line, const Layers &layers, const uint8_t *videoRam, const PictureOptions &options, uint32_t *out) const;
    void DrawObjects(int rasterLine, const Layers &layers, const PictureOptions &options, int width, uint16_t *pens, uint8_t *priority,
                     const uint32_t *masks, uint8_t *marks = nullptr) const;

    const uint8_t *m_graphics = nullptr, *m_extraGraphics = nullptr;
    size_t m_graphicsSize = 0, m_extraGraphicsSize = 0;
    uint32_t m_colours[kPages * kPageColours] = {};
    std::vector<ObjectTile> m_tiles; // (the frame's, front to back)
    uint16_t m_priority = 0;         // (the object registers' priority control, as the list was taken)
    bool m_cps1 = false;
    const GfxTable *m_gfx = nullptr;
    uint8_t m_enable[3] = {2, 4, 8};
};

} // namespace cps2recomp::native_video
