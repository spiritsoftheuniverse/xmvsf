// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The scene export (issue #366): the picture shown, and each of the board's layers by itself (its three scroll screens
// and its objects) as a PNG with nothing where the layer has nothing, so that the picture can be taken apart and put
// together again; and a scene.json that says what each file is and the layers' order, back to front. Works for every
// game with no knowledge of it: each layer is the frame drawn again from its records (video/native.h, LastFrame) with
// only that layer on, so what the game changed part way down the screen (its raster effects) is in it as it was.
//
// A fight on another game's stage (fight/guest_stages.h) has that board's layers too, behind the game's.
//
// A clip is the same for each frame of a few seconds, in a folder per layer, written by threads beside the game.
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace cps2recomp {
class Machine;
}

namespace port {

class StageStitch;

class SceneExport {
public:
    ~SceneExport();
    // The last frame's scene, in a folder of its own under `under` named `name`; what to say of it
    std::string Save(cps2recomp::Machine &machine, const std::string &under, const std::string &name);
    // A clip of the next `frames` frames, in a folder of its own; what to say of it (it is begun, or why not)
    std::string StartClip(const std::string &under, const std::string &name, int frames);
    // Ended before its length, the frames so far kept
    void StopClip(bool cutShort = false);
    bool Recording() const {
        return m_clip.on;
    }
    // Each frame the game ran and will show (the clip's frame taken)
    void Frame(cps2recomp::Machine &machine);
    // What is to be said now (a clip finished writing, or one could not be): true once for each
    bool TakeSaid(std::string &said);
    // A whole stage's files written on a thread of their own (StageStitch::Write); what it says is said as the above.
    // False: one is being written still.
    bool WriteStage(StageStitch &&stitch, const std::string &under, const std::string &name, const std::string &stage);

    // A layer of a scene: a board's scroll screen or its objects, its pixels red in the low byte, then green, blue, and
    // 0xFF in the top byte where it has something (0: nothing)
    struct Layer {
        std::string name;  // ("scroll1"..."scroll3", "objects" (or "objects-0"..."objects-3", a CPS-2's objects at more
                           // than one place: how many scroll screens are behind them); "stage-" before them for another game's stage)
        std::string board; // ("game", or "stage")
        int which = 0;            // (0 the objects, 1-3 the scroll screens)
        unsigned levels = 0;      // (objects: the levels drawn in it, bit n level n; 0xFF all)
        std::vector<uint32_t> pixels;
        bool empty = true;
        // (a scroll screen's: on each line, its map's column at the picture's left edge and its map's line there, not
        // wrapped: for the whole stage put together, StageStitch)
        std::vector<int> mapX, mapY;
    };
    struct Scene {
        int width = 0, height = 0;
        uint64_t frame = 0;
        uint32_t backdrop = 0;
        std::vector<uint32_t> picture; // (as shown, solid)
        std::vector<Layer> layers;     // (back to front)
    };
    // The last frame's scene (false: no records to draw it from: before the game's first object list, or --video board)
    static bool Take(cps2recomp::Machine &machine, Scene &scene);

private:
    struct Clip {
        std::atomic<bool> on{false};
        std::string folder;
        int wanted = 0, taken = 0;
        std::vector<std::string> names; // (the first frame's layers, in its order: the folders)
        std::string json;               // (each frame's line of scene.json)
    } m_clip;
    // The writers: frames waiting to be written as PNGs
    struct Job {
        std::string folder;
        int number = 0;
        Scene scene;
    };
    std::mutex m_lock;
    std::condition_variable m_wake;
    std::deque<Job> m_jobs;
    std::vector<std::thread> m_writers;
    int m_writing = 0;   // (jobs taken and not yet written)
    bool m_failed = false, m_stopping = false, m_finishing = false, m_cutShort = false;
    // (the clip's frames for scene.json as they are taken, and its size; then all of it, for when the last is written)
    std::string m_frameJson;
    int m_width = 0, m_height = 0;
    std::string m_finalJson, m_finalFolder;
    int m_finalFrames = 0;
    std::string m_said;
    bool m_saying = false;
    void Writer();
    void Finish();
    std::thread m_stageWriter;
    std::atomic<bool> m_stageWriting{false};
};

// The whole stage put together (issues #370, #371), from the stage viewer's frames as its camera goes from the stage's
// left end to its right (fight/alternative.h, StageSweep): each layer in full, in its own place (a scroll screen by
// where it is in its map on each line, so that its row scroll and its own pace are its; an objects layer by how far its
// picture moved since the frame before), the board's own columns preferred to a wide picture's added ones (which a game
// may not have painted), each pixel as it was first seen. With a stage's planes going at their own paces there is no
// one right picture of all of them: the put-together one has each layer where it is with the camera in the middle of
// its way, and stage.json says each layer's pace (its pixels moved for each of the camera's).
class StageStitch {
public:
    void Begin(int camera);
    bool Begun() const {
        return m_begun;
    }
    // A frame of the camera's way across (where the viewer's camera is, in pixels)
    void Add(cps2recomp::Machine &machine, int camera);
    // The stage's files in a folder of its own under `under` named `name`; what to say. The stitch is over.
    std::string Write(const std::string &under, const std::string &name, const std::string &stage);
    void End() {
        m_begun = false;
        m_layers.clear();
    }

private:
    static constexpr int kOrigin = 128, kWidth = 4096, kHeight = 224 + 2 * kOrigin;
    struct Canvas {
        std::string name, board;
        int which = 0;
        unsigned levels = 0;
        std::vector<uint32_t> pixels;
        std::vector<uint8_t> seen; // (0: not yet; 1: from a wide picture's added columns; 2: from the board's own)
        // (a scroll screen's, on each line: where it was in its map the frame before, and how far it has moved since the
        // first: not wrapped)
        std::vector<int> lastX, lastY, movedX, movedY;
        // (an objects layer's: how far it has moved, and its picture the frame before)
        int offX = 0, offY = 0;
        std::vector<uint32_t> last;
        int firstCamera = 0, lastCamera = 0;
        bool started = false;
    };
    std::vector<Canvas> m_layers;
    bool m_begun = false;
    int m_width = 0, m_frames = 0;
    uint32_t m_backdrop = 0;
};

} // namespace port
