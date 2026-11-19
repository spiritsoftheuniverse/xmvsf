// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The picture drawn on the GPU from the native interface's records (docs/NATIVE_VIDEO.md, the GPU renderer): a frame's
// record (NativeVideo::FrameData) into a texture, in three passes (video/shaders/): the scroll screens a pixel at a time,
// in bands where the game wrote its maps or the palette while the board drew; the objects as instanced tiles, the front
// one at each pixel kept by depth; the colours, with each object's level against the screens' pens. With the CPU's
// rasteriser (video/native_render.h) as the reference: --video gpu-check draws each frame here too, reads it back and
// compares it with the board's lines.
//
// So far: the board's picture and what a game's sources set on it (a wider picture, its screens and objects moved, tiles
// given beyond the board's columns, holes filled, objects hidden), with the objects' offsets the same on every line; not
// the marks of pictures put together from two, nor layers left out. Other frames are not drawn here and are counted.
#pragma once

#include "../video/native.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace port {

class GpuPicture {
public:
    ~GpuPicture();
    // On `device` (the presenter's), or a device of its own when none (headless: no window); the graphics ROMs are put
    // on the GPU once
    bool Open(SDL_GPUDevice *device, const uint8_t *graphics, size_t size, std::string &error);
    // Another set's graphics, which the objects and tiles marked so are drawn from (Vampire Savior Definitive's), after Open
    bool SetExtraGraphics(const uint8_t *graphics, size_t size, std::string &error);
    void Close();
    SDL_GPUDevice *Device() const {
        return m_device;
    }

    // Whether a frame is one drawn here
    static bool Drawable(const cps2recomp::NativeVideo::FrameData &frame);
    // The frame drawn into Picture() (Raster::kWidth x 224, red, green, blue, alpha), in `commands`
    void Draw(SDL_GPUCommandBuffer *commands, const cps2recomp::NativeVideo::FrameData &frame);
    SDL_GPUTexture *Picture() const {
        return m_picture;
    }

    // The game (--video gpu): the last frame finished kept (NativeVideo::frameDrawn, with NativeVideo::recording), and
    // drawn when the presenter shows a frame
    void Keep(const cps2recomp::NativeVideo::FrameData &frame);
    bool CanDrawKept() const {
        return m_hasKept && m_kept.drawable;
    }
    // The kept frame drawn, with the readouts over it (`readouts`: Raster::kWidth x 224, red, green, blue and alpha bytes,
    // the colours times the alpha; none: nothing over it); zoomed, two pictures stacked, the game's above and with the
    // readouts below (present.h, Zoomed). The texture to show.
    SDL_GPUTexture *DrawKept(SDL_GPUCommandBuffer *commands, const uint32_t *readouts, bool zoomed);
    // (the kept frame's width: Raster::kWidth and the added sides)
    int KeptWidth() const {
        return cps2recomp::native_video::Raster::kWidth + 2 * m_kept.options.side;
    }

    // Another game's board beside this one (runtime/guest.h) whose stage this game's objects are drawn over: a frame of
    // this one's with its objects alone (PictureOptions: layerMask 1 or 0, the objects marked) is drawn over the guest's
    // last frame (its own GpuPicture, on the same device)
    void AddGuest(GpuPicture *guest) {
        m_guests.push_back(guest);
    }
    // (whether the frame was put together over a guest's: Machine::guestBehind; drawn over one only when it was)
    const bool *guestBehind = nullptr;
    const int *guestShift = nullptr; // (Machine::guestShift)
    // (what its reports begin with)
    std::string checkName = "video gpu-check";

    // --video gpu-check: the frame drawn, read back and compared with the board's lines
    void Check(const cps2recomp::NativeVideo::FrameData &frame);
    // ... and the frame kept, drawn with the readouts over it (DrawKept), against the CPU's picture with them (`shown`:
    // Port::Shown), within a step of a colour (the readouts' blend rounds)
    void CheckShown(const uint32_t *readouts, const uint32_t *shown, int width);
    bool Report(FILE *out) const;

private:
    bool MakePipelines(std::string &error);
    SDL_GPUDevice *m_device = nullptr;
    bool m_ownDevice = false;
    size_t m_graphicsSize = 0;
    SDL_GPUBuffer *m_graphics = nullptr, *m_video = nullptr, *m_lines = nullptr, *m_tiles = nullptr, *m_frame = nullptr,
                  *m_overrides = nullptr, *m_extraGraphics = nullptr, *m_scrollExtra = nullptr;
    size_t m_extraGraphicsSize = 0;
    std::vector<uint8_t> m_scrollExtraNow; // (which tiles are another set's, as the frame's bands have it)
    size_t m_tilesCapacity = 0;
    SDL_GPUTexture *m_screens = nullptr, *m_holes = nullptr, *m_objects = nullptr, *m_depth = nullptr, *m_lineTexture = nullptr, *m_palette = nullptr,
                   *m_picture = nullptr;
    SDL_GPUSampler *m_sampler = nullptr;
    SDL_GPUGraphicsPipeline *m_layersPipeline = nullptr, *m_holesPipeline = nullptr, *m_objectsPipeline = nullptr,
                            *m_coloursPipeline = nullptr;
    SDL_GPUTransferBuffer *m_upload = nullptr, *m_download = nullptr;
    size_t m_uploadSize = 0;
    std::vector<uint8_t> m_videoNow; // (video RAM as the frame's bands have it, put in as they go)
    struct Kept {
        uint64_t frame = 0;
        std::vector<uint8_t> video;
        cps2recomp::native_video::Raster raster;
        std::vector<cps2recomp::NativeVideo::FrameEvent> events;
        std::vector<cps2recomp::native_video::PaletteUpload> uploads;
        cps2recomp::native_video::Layers layers[224];
        std::vector<uint8_t> scrollExtra;
        std::vector<cps2recomp::native_video::TileOverride> overrides[224];
        cps2recomp::native_video::PictureOptions options;
        bool drawable = false;
    } m_kept;
    bool m_hasKept = false;
    uint64_t m_keptSerial = 0; // (when it was kept, among all GpuPictures: the guest's last frame is the one kept latest)
    std::vector<GpuPicture *> m_guests;
    // (the frame being drawn is this game's objects alone over a guest's stage, this picture of it: the last pass puts the
    // two together; and the kept frame drawn into m_picture, no readouts)
    SDL_GPUTexture *m_stage = nullptr;
    bool DrawKeptPicture(SDL_GPUCommandBuffer *commands);
    static bool Over(const cps2recomp::native_video::PictureOptions &o) {
        return (o.layerMask & ~1u) == 0 && !o.markLayers && (o.markObjects || o.layerMask == 0);
    }
    GpuPicture *LatestGuest() const;
    SDL_GPUTexture *m_readouts = nullptr, *m_clear = nullptr, *m_shown = nullptr, *m_stack = nullptr;
    int m_shownWidth = 0; // (the width m_shown and m_stack were made at: the frame's, which the presenter takes as theirs)
    SDL_GPUGraphicsPipeline *m_shownPipeline = nullptr;
    SDL_GPUTransferBuffer *m_readoutsUpload = nullptr;
    uint64_t m_same = 0, m_differ = 0, m_notDrawn = 0, m_linesDiffer = 0, m_wide = 0, m_over = 0;
    uint64_t m_shownSame = 0, m_shownDiffer = 0, m_shownWithReadouts = 0, m_shownCpu = 0;
};

} // namespace port
