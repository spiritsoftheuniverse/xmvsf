// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The picture at the game's level (docs/NATIVE_VIDEO.md): what the games' drawing code decides each frame, handed
// over as it decides it, in place of the board's registers and memories that video.cpp reads.
//
// Two sides make the same records. The game's side: Capcom's video library, rewritten once (video/lifts.cpp), says
// what the game's code tells the board, and the records are made from that and from the game's own memory. The
// board's side: a decoder makes them from what the board model was given, as video.cpp reads it. With --video check,
// every record from the board is compared with the one the game's side gave for it; a difference, or a record only
// one side made, is reported.
//
// Native so far: the palette's uploads, the object list the board takes at each frame, and the layers' registers each
// line is drawn with.
#pragma once

#include "native_render.h"
#include "records.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace cps2recomp {

class M68K;
class Archive;

// The two sides compared (--video check)
class NativeVideo {
public:
    // Recording: the game's side keeps its records (for the GPU renderer, port/gpu_picture.h) without comparing them
    bool recording = false;
    // --video native: the machine's picture drawn from the records, a line at a time (DrawLine), in place of video.cpp
    bool drawsPicture = false;
    // A line of the picture drawn from the game's side's records (its registers as they are now, its list, its palette;
    // the game's video RAM): false, and nothing drawn, before the game's first object list
    bool DrawLine(int line, uint32_t *out, const native_video::PictureOptions &options) const;
    bool Active() const {
        return checking || recording;
    }
    // The game's side's own state (what the game has said to each register, its list, the palette and objects as it
    // has them), with the machine's in a save state: online play's rollback puts it back with the machine
    void Serialize(Archive &ar);
    // A frame as the GPU draws it (docs/NATIVE_VIDEO.md, the frame drawn at once): video RAM and the palette as they were
    // at its first line, the writes and uploads after that with the line they came before, each line's layers, the
    // frame's objects; and the board's lines, for a check
    struct FrameEvent {
        int16_t row;      // (the first line it is seen on)
        uint32_t address; // (video RAM's; or, for a palette upload, its number in the uploads, with the top bit set)
        uint16_t value;
        uint8_t extra = 0; // (a tile word's: drawn from another set's graphics, Machine::scrollFromExtra)
    };
    struct FrameData {
        uint64_t frame = 0;
        const uint8_t *video = nullptr;                          // (0x30000 bytes)
        const native_video::Raster *raster = nullptr;            // (its colours, tiles and priority control)
        const std::vector<FrameEvent> *events = nullptr;
        const std::vector<native_video::PaletteUpload> *uploads = nullptr;
        const native_video::Layers *layers = nullptr;            // (224)
        const uint8_t *scrollExtra = nullptr; // (another set's graphics: which tiles of the maps are drawn from them, at the start)
        const std::vector<native_video::TileOverride> *overrides = nullptr; // (224: each line's, PictureOptions::tile)
        native_video::PictureOptions options;
        const uint32_t *board = nullptr;                         // (224 lines of Raster::kMaxWidth)
        bool whole = true; // (false: a frame the records do not make, before the game's first list, or of another set's
                           // graphics: the CPU's picture is to be shown, and nothing compared)
    };
    // The last frame recorded whole (FrameEnd: its 224 lines' records, after the game's first list), valid until the next
    // one starts; false if there is none. And a frame drawn from its records on the CPU, at once, as the GPU draws it
    // (its lines `pitch` pixels apart; the records not changed): what another machine's picture is made of when this one
    // does not draw its own (Machine::ownPicture, a guest board's: fight/guest_stages.cpp)
    bool LastFrame(FrameData &data) const;
    static void DrawFrame(const FrameData &frame, uint32_t *picture, int pitch);
    // Told each frame drawn at once (the GPU's check, port/gpu_picture.h), before the CPU draws it
    void (*frameDrawn)(const FrameData &frame, void *user) = nullptr;
    void *frameUser = nullptr;

    bool checking = false; // (--video check: the records compared with the board's)
    // While set, a state put back keeps the last frame recorded whole (LastFrame): run-ahead's, whose frames ahead are put
    // back every frame while their last is the picture shown (the scene export draws that frame again, port/scene.h)
    static inline bool keepLastFrame = false;
    std::string checkName = "video check"; // (what its reports begin with: a guest board's says whose)
    uint64_t frame = 0;                // (for the reports: the machine's frame)
    const uint8_t *videoRam = nullptr; // (the game's side reads the buffers the game fills there)
    uint8_t paletteControl = 0x30;     // (where the CPS-B's palette control is in its registers: Board::paletteControl)
    uint8_t layerControl = 0x26;       // (... its layer control: Board::layerControl)
    const uint8_t *cps1Masks = nullptr;
    // (a game with another set's graphics: which object entries and which tiles are drawn from them, the machine's
    // marks: Machine::objectsFromExtra, scrollFromExtra)
    const uint8_t (*objectsFromExtra)[0x400] = nullptr;
    const uint8_t *scrollFromExtra = nullptr;
    void SetExtraGraphics(const uint8_t *graphics, size_t size) {
        m_raster.SetExtraGraphics(graphics, size);
    }
    // (a CPS-1: its layers' enable bits, Board::enable)
    void SetCps1(const uint8_t *enable) {
        m_raster.SetCps1(enable);
    }
    void SetGfx(const GfxTable *gfx) {
        m_raster.SetGfx(gfx);
    } // (a CPS-1's: where its priority masks are, Board::priority; a CPS-2's draw without them)

    // The board starts, and the game's side with it: the registers are the board's until the game says otherwise
    void Reset(const uint16_t *cpsA, const uint16_t *cpsB, const uint16_t *objectRegs);

    // The game's side: an instruction of the game's (a hook's), or a rewrite of the library, is about to write
    // `value` to the video register at `address` (a byte, or a word). The game's side keeps what the game said, and
    // makes the records it means: the palette's upload, the object list handed over.
    void GameWrites(M68K &c, uint32_t address, uint16_t value, bool byte);

    // The board's side: what the board did
    void BoardPalette(const native_video::PaletteUpload &upload);
    void BoardObjects(const native_video::ObjectList &list, const native_video::PictureOptions &options);
    void BoardLayers(const native_video::Layers &layers, int line);
    // ... a CPS-1's object list, from video RAM where its register says
    void BoardObjectsCps1(const native_video::ObjectList &list, const native_video::PictureOptions &options);
    // ... and a line it drew (Video::Width() pixels, as the game's sources set the picture: `options`), when it drew a
    // CPS-2's picture from its own graphics (Video::ComparablePicture): the line drawn from the game's side's records
    // alone (native_render.h) must be the same
    void BoardLine(int line, const uint32_t *pixels, const native_video::PictureOptions &options, bool comparable);
    void SetGraphics(const uint8_t *graphics, size_t size) {
        m_raster.SetGraphics(graphics, size);
    }
    // The game wrote a word of video RAM (its maps, its row scroll's table, its palettes' buffers): for the frame drawn
    // at once (below), with the line it came before
    void VideoWritten(uint32_t address);

    // At the end of a run: the counts, and false if anything differed or was made by code not native yet
    bool Report(FILE *out) const;

private:
    void Palette(const native_video::PaletteUpload &upload);

    // What the game has said (its words to the registers), its list handed over, and its upload not yet made
    uint16_t m_objectRegs[6] = {}, m_cpsA[32] = {}, m_cpsB[32] = {};
    int m_bank = -1; // (-1: not said yet)
    uint16_t m_list[native_video::kObjectWords] = {};
    uint8_t m_listExtra[native_video::kObjectWords / 4] = {};
    native_video::PaletteUpload m_pending;
    bool m_hasPending = false;

    uint64_t m_same = 0, m_differ = 0, m_boardOnly = 0, m_gameOnly = 0;
    uint64_t m_firstBoardOnly = 0;
    uint64_t m_listsSame = 0, m_listsDiffer = 0, m_listsBefore = 0, m_listsExtra = 0, m_framesExtra = 0;
    uint64_t m_linesSame = 0, m_linesDiffer = 0;
    native_video::Raster m_raster;
    uint64_t m_picturesSame = 0, m_picturesDiffer = 0, m_picturesNot = 0;

    // The frame drawn at once, as the GPU will draw it (docs/NATIVE_VIDEO.md): from video RAM and the palette as they
    // were when its first line was drawn, the writes and uploads after that put in at the line they came before, each
    // line's layers, and the frame's objects; its lines compared with the board's when the frame is over
    void FrameStart(const native_video::PictureOptions &options);
    void FrameEnd();
    FrameData FrameRecords() const;
    bool m_lastWhole = false; // (the last frame was recorded whole: LastFrame)
    bool m_inFrame = false, m_frameComparable = false;
    int m_frameRow = 0; // (the lines drawn so far)
    std::vector<uint8_t> m_frameVideo, m_frameScrollExtra;
    native_video::Raster m_frameRaster;
    native_video::PictureOptions m_frameOptions;
    std::vector<FrameEvent> m_frameEvents;
    std::vector<native_video::PaletteUpload> m_frameUploads;
    native_video::Layers m_frameLayers[224];
    std::vector<native_video::TileOverride> m_frameOverrides[224];
    std::vector<uint32_t> m_frameBoard; // (the board's lines)
    uint64_t m_framesSame = 0, m_framesDiffer = 0, m_frameLinesDiffer = 0;
    uint64_t m_bands = 0, m_bandFrames = 0; int m_mostBands = 0; // (the lines a frame has writes before: its bands, less one)
};

// The rewrites and hooks of Capcom's video library that the game's game.cfg asks for (video/lifts.cpp)
void RegisterVideoLifts();
// ... and a guest board's (runtime/guest.h): its game's video_writes, read from its program (opcodes, as the CPU reads it)
void RegisterGuestVideoLifts(const std::string &id, const char *writes, const uint8_t *opcodes, size_t size);

} // namespace cps2recomp
