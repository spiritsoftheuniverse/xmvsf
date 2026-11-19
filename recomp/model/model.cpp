// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#include "model.h"

#include "image_files.h"
#include "spare.h"
#include "state.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <map>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

namespace cps2recomp::model {

namespace {
    // The "Size" choice, in quarters: the character's own size first (a choice's first is what it is when nothing is
    // chosen), then larger, and round to the smaller ones
    constexpr int kSizes[] = {4, 6, 8, 12, 16, 24, 32, 48, 64, 100, 1, 2, 3};
    // The "Speed" choice: a frame of the game in this many pictures of the screen (as the menu's own speeds)
    constexpr int kSlowBy[] = {1, 2, 4, 8};
    constexpr int kFramesASecond = 60;
    constexpr int kNamedFor = 120;                 // frames an animation's name is shown for after it changes
    // Frames a frame is shown for at most: a game holds some for good (the Marvel games' 7FFF, a fall that lies until
    // the code stands the character up), and the animation, or "Play all", would stay on it
    constexpr int kLongestFrame = 3 * kFramesASecond;
    constexpr int kHeldFirst = 20, kHeldEvery = 8; // frames a direction is held before it repeats, and between repeats
    // The colour editor's scan: frames of the copy run in a frame of the viewer's, and of an animation looked at
    constexpr int kScanFramesEach = 8, kScanFramesMost = 48;
    constexpr int kMostLines = 8;
    // The exports: frames of the game each frame of an animation is shown for before its picture is taken (the board
    // draws the list the game made the frame before: by then the picture is the frame's), the most frames an animation
    // is taken to, the most a sheet has across, how long what was saved is said for, and the sizes
    constexpr int kSettleFrames = 3, kMostExportFrames = 256, kSheetAcross = 8, kSaidFor = 4 * kFramesASecond;
    constexpr int kExportSizes[] = {1, 2, 3, 4, 6, 8, 12, 16};
    // ... and their page of the menu, under the viewer's
    constexpr const char *kExportPage = "Export";
    constexpr const char *kExportWords = "Save the character shown, or every frame of the animation shown, as pictures and a GIF.";

    Description g_d;
    int g_choices = 0; // the "Animation" choice's count: standing and every list's

    struct State {
        bool on = false;
        int32_t character = 0, background = 0; // the choices the viewer began with
        // The animation being shown (0: none, the character stands): which list (from 1) and which of it, where its
        // first frame and the frame shown are, and how long that one has left
        int32_t table = 0, number = 0;
        uint32_t first = 0, frame = 0;
        int32_t left = 0;
        int32_t named = 0;                // frames the animation's name is still shown for
        int32_t stick = 0, stickHeld = 0; // player 1's left or right a frame ago, and for how many frames
        int32_t held = 0;                 // frames its last frame is still held for before it starts again
        // Where the character stood in the picture a frame ago (the picture a frame shows is the game's of the frame
        // before), and whether its feet are to be found from its picture
        int32_t anchorX = 0, anchorY = 0;
        bool feetFound = false, feetKnown = false;
        bool seen = false; // the character was in the last picture made
        // An export asked for in the menu, made after the next frame (0: none; 1: the picture; 2: the animation), and
        // what was saved, said over the picture for a while
        int32_t exportWanted = 0;
        std::string said;
        int32_t saidFor = 0;
    } g;
    // The character's picture as it was last made (the board's objects alone, its own only, marked), for the export of
    // the picture shown
    std::vector<uint32_t> g_model;
    int g_modelWidth = 0;

    int Characters() {
        return static_cast<int>(g_d.characters.size());
    }
    int Stages() {
        return static_cast<int>(g_d.stages.size());
    }
    const char *AnimationChoice() {
        const char *choice = g_d.animationChoice ? g_d.animationChoice() : nullptr;
        return choice ? choice : "model_animation";
    }
    int Shift() {
        return g_d.shift ? g_d.shift() : 0;
    }
    bool IsFrame(Machine &machine, uint32_t frame) {
        return !g_d.isFrame || g_d.isFrame(machine, frame);
    }

    // How many animations a list has for the character: as many as start with something that is a frame
    int Count(Machine &machine, int character, int list) {
        static std::map<std::pair<int, int>, int> counts;
        const auto at = counts.find({character, list});
        if (at != counts.end()) {
            return at->second;
        }
        int count = 0;
        while (count < g_d.perList && g_d.animationAt(machine, character, list, count)) {
            count++;
        }
        counts[{character, list}] = count;
        return count;
    }
    // (one that does not start with a frame is passed over: the character has not got it)
    uint32_t AnimationAt(Machine &machine, int character, int choice) {
        const uint32_t first = choice ? g_d.animationAt(machine, character, (choice - 1) / g_d.perList, (choice - 1) % g_d.perList) : 0;
        return first && IsFrame(machine, first) ? first : 0;
    }
    bool Has(Machine &machine, int character, int choice) {
        return choice >= 1 && choice < g_choices && (choice - 1) % g_d.perList < Count(machine, character, (choice - 1) / g_d.perList) &&
               AnimationAt(machine, character, choice);
    }
    // The animation after `choice` (or before it) that the character has, going round from the last to the first
    int Step(Machine &machine, int character, int choice, int by) {
        for (int n = 0; n < g_choices; n++) {
            choice = 1 + (choice - 1 + by + (g_choices - 1)) % (g_choices - 1);
            if (Has(machine, character, choice)) {
                return choice;
            }
        }
        return 0;
    }

    // Which animations show which of the character's palettes, for the colour editor's Line row: found by showing
    // each frame of every animation on a copy of the viewer's machine (a spare: runtime/spare.h), a few frames each
    // frame of the viewer's, and counting the objects the game puts in the list in each of the character's palettes.
    // For each line, the animation with the frame that has the most objects in it.
    struct LineScan {
        int character = -1;
        Spare spare;
        int choice = 0; // the animation being looked at (0: not begun)
        uint32_t first = 0, frame = 0;
        int frames = 0;
        bool done = false;
        int best[kMostLines] = {}; // for each line, the animation found, and its objects in that line
        int most[kMostLines] = {};
    } g_scan;

    // (in the copy: the objects in each of the character's lines, from the list the game has just made)
    void CountLines(Machine &machine, int counts[kMostLines]) {
        const uint8_t *list = machine.ObjectsToCome();
        const int slot = g_d.firstLine(machine);
        for (int n = 0; n < 0x1000; n += 4) {
            const uint16_t y = static_cast<uint16_t>((list[n * 2 + 2] << 8) | list[n * 2 + 3]);
            const uint16_t attributes = static_cast<uint16_t>((list[n * 2 + 6] << 8) | list[n * 2 + 7]);
            if (y >= 0x8000 || attributes >= 0xFF00) {
                break; // (the list's end)
            }
            const int line = (attributes & 0x1F) - slot;
            if (line >= 0 && line < g_d.linesInSet) {
                counts[line]++;
            }
        }
    }
    int FrameTime(Machine &machine, uint32_t frame) {
        return std::min(g_d.frameTime(machine, frame), kLongestFrame);
    }
    void ScanAt(Machine &machine, int character, int choice) {
        g_scan.choice = choice;
        g_scan.first = g_scan.frame = AnimationAt(machine, character, choice);
        g_scan.frames = 0;
    }

    // The menu's "Reset options": every choice of the viewer's page back to what it is when nothing is chosen
    void ResetChoices(Machine &) {
        for (const char *choice : {"model_character", "model_background", "model_size", "model_animation", "model_speed", "model_repeat",
                                   "model_all", "model_export_size"}) {
            SetFeatureValue(choice, 0);
        }
        if (g_d.inAFight) {
            SetFeatureValue("model_foreground", 0);
        }
    }

    // The animation chosen in the menu, shown: a frame of it put in the character's record
    void ShowAnimation(Machine &machine) {
        const int character = CharacterNumber();
        int choice = std::clamp(FeatureValue(AnimationChoice()), 0, g_choices - 1);
        // (one the character has not got: the next it has)
        if (choice && !Has(machine, character, choice)) {
            choice = Step(machine, character, choice, 1);
            SetFeatureValue(AnimationChoice(), choice);
        }
        // ("Play all" with nothing chosen: the first animation the character has; the colour editor's own choice is
        // one animation)
        const bool all = FeatureOn("model_all") && !(g_d.animationChoice && g_d.animationChoice());
        if (all && !choice) {
            choice = Step(machine, character, 0, 1);
            SetFeatureValue("model_animation", choice);
        }
        const uint16_t stick = machine.players[0] & (kLeft | kRight);
        if (choice) {
            // Left and right go to the animation before and the one after: at a press, and while held, after a
            // third of a second, eight a second
            if (stick == kLeft || stick == kRight) {
                g.stickHeld = g.stick == stick ? g.stickHeld + 1 : 0;
                if (g.stickHeld == 0 || (g.stickHeld >= kHeldFirst && (g.stickHeld - kHeldFirst) % kHeldEvery == 0)) {
                    choice = Step(machine, character, choice, stick == kRight ? 1 : -1);
                    SetFeatureValue(AnimationChoice(), choice);
                }
            }
        }
        g.stick = stick;
        const int table = choice ? (choice - 1) / g_d.perList + 1 : 0, number = choice ? (choice - 1) % g_d.perList : 0;
        const uint32_t first = AnimationAt(machine, character, choice);
        if (!first) {
            if (g.table) {
                g_d.showFrame(machine, 0); // (back to standing)
            }
            g.table = 0;
            g.named = 0;
            return;
        }
        if (table != g.table || number != g.number || first != g.first) {
            g.table = table;
            g.number = number;
            g.first = g.frame = first;
            g.left = FrameTime(machine, first);
            g.held = 0;
            g.named = kNamedFor;
        } else if (g.held > 0) {
            g.held--; // (between repeats: the last frame stays)
        } else if (--g.left <= 0) {
            // (the frame's time is up: the next, by what the frame says follows it; the end is the start again)
            uint32_t next = g_d.nextFrame(machine, g.frame);
            if (!next || !IsFrame(machine, next)) {
                next = g.first;
            }
            // (going back, to the start or to an earlier frame, is a repeat: after the wait chosen, if any. The wait
            // is counted once: when it is over the frame's time is up still, and this comes again.) With "Play all",
            // where it would start again the next animation starts instead (the last frame stays for this frame;
            // the next one sees the choice changed and starts it)
            const int wait = std::clamp(FeatureValue("model_repeat"), 0, 10) * kFramesASecond;
            if (next <= g.frame && wait && g.left == 0) {
                g.held = wait;
                g.left = -1;
            } else if (next <= g.frame && all) {
                SetFeatureValue("model_animation", Step(machine, character, choice, 1));
                g.left = 0;
            } else {
                g.frame = next;
                g.left = FrameTime(machine, next);
            }
        }
        g_d.showFrame(machine, g.frame);
        machine.players[0] = 0; // (the controls would start moves of the game's own)
    }

    // A list of animations by its letter: A to Z, then AA, AB... (a game with more than 26 of them)
    std::string ListName(int list) {
        return list < 26 ? std::string(1, static_cast<char>('A' + list)) : std::string("A") + static_cast<char>('A' + list - 26);
    }

    // The animation's name over the picture for a while after it changes (set after the frame, when the other
    // modes' own readouts have been cleared)
    void TakeSaid(); // (below, with the exports)

    void NameAnimation() {
        TakeSaid();
        std::vector<HudLine> lines;
        if (g.table && g.named > 0) {
            g.named--;
            char text[32];
            snprintf(text, sizeof(text), "%s%d", ListName(g.table - 1).c_str(), g.number);
            lines.push_back({0.5f, 0.04f, 0, text});
        }
        if (g.saidFor > 0) {
            g.saidFor--;
            lines.push_back({0.5f, 0.92f, 0, g.said});
        }
        SetHud(lines);
    }

    // The frame's picture made: the stage (or a plain colour), and the character over it at the size wanted
    // Of the character's picture, only what touches it where it stands: the pixels in 4x4 blocks, the blocks joined
    // to those about its feet and middle (with a block's gap between), the others left out (Description::nearestOnly)
    void KeepNearest(uint32_t *picture, int width, int height, int anchorX, int anchorY) {
        constexpr int kBlock = 4;
        const int across = (width + kBlock - 1) / kBlock, down = (height + kBlock - 1) / kBlock;
        std::vector<uint8_t> blocks(static_cast<size_t>(across) * down); // 1: drawn, 2: kept
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (picture[static_cast<size_t>(y) * width + x] & 0x80000000u) {
                    blocks[static_cast<size_t>(y / kBlock) * across + x / kBlock] = 1;
                }
            }
        }
        std::vector<int> todo;
        for (int by = std::max(0, (anchorY - 96) / kBlock); by <= std::min(down - 1, anchorY / kBlock); by++) {
            for (int bx = std::max(0, (anchorX - 12) / kBlock); bx <= std::min(across - 1, (anchorX + 12) / kBlock); bx++) {
                if (blocks[static_cast<size_t>(by) * across + bx] == 1) {
                    blocks[static_cast<size_t>(by) * across + bx] = 2;
                    todo.push_back(by * across + bx);
                }
            }
        }
        if (todo.empty()) {
            return; // (nothing about where it stands: all of it kept)
        }
        while (!todo.empty()) {
            const int at = todo.back(), bx = at % across, by = at / across;
            todo.pop_back();
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    const int x = bx + dx, y = by + dy;
                    if (x >= 0 && x < across && y >= 0 && y < down && blocks[static_cast<size_t>(y) * across + x] == 1) {
                        blocks[static_cast<size_t>(y) * across + x] = 2;
                        todo.push_back(y * across + x);
                    }
                }
            }
        }
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (blocks[static_cast<size_t>(y / kBlock) * across + x / kBlock] != 2) {
                    picture[static_cast<size_t>(y) * width + x] &= ~0x80000000u;
                }
            }
        }
    }

    // The character by itself: the objects alone, its own only, and marked so that their pixels are told from what is
    // behind them (the top bit set). True: something of it was drawn.
    bool DrawModel(Machine &machine, std::vector<uint32_t> &model) {
        const int width = machine.PictureWidth(), height = Video::kHeight;
        model.resize(static_cast<size_t>(width) * height);
        if (g_d.beforeCompose) {
            g_d.beforeCompose(machine);
        }
        Video &video = machine.video;
        const unsigned layers = video.layerMask;
        const bool marked = video.markObjects;
        const auto hidden = video.hideObject;
        video.layerMask = 1;
        video.markObjects = true;
        video.hideObject = [](uint32_t tile, uint16_t attributes) { return !g_d.isModel(tile, attributes); };
        bool seen = false;
        for (int line = 0; line < height; line++) {
            uint32_t *row = &model[static_cast<size_t>(line) * width];
            video.RenderLine(line, row);
            seen = seen || std::any_of(row, row + width, [](uint32_t pixel) { return (pixel & 0x80000000u) != 0; });
        }
        video.layerMask = layers;
        video.markObjects = marked;
        video.hideObject = hidden;
        return seen;
    }

    void Compose(Machine &machine) {
        static std::vector<uint32_t> fight;
        std::vector<uint32_t> &model = g_model;
        const int width = machine.PictureWidth(), height = Video::kHeight;
        uint32_t *picture = machine.Picture();
        Video &video = machine.video;
        g.seen = DrawModel(machine, model);
        g_modelWidth = width;
        const unsigned layers = video.layerMask;
        const bool marked = video.markObjects;
        const auto hidden = video.hideObject;
        // (the stage's pieces in front of the character: at its own size, on a stage, where it is not moved aside
        // for the colour editor's panel; the picture as the board would draw it)
        const int size = kSizes[std::clamp(FeatureValue("model_size"), 0, static_cast<int>(std::size(kSizes)) - 1)];
        const bool front = g_d.inAFight && size == 4 && g.background < Stages() && !Shift() && FeatureValue("model_foreground") == 0;
        if (front) {
            fight.resize(model.size());
            video.layerMask = 0xF;
            video.markObjects = false;
            video.hideObject = [](uint32_t tile, uint16_t attributes) { return !g_d.inAFight(tile, attributes); };
            for (int line = 0; line < height; line++) {
                video.RenderLine(line, &fight[static_cast<size_t>(line) * width]);
            }
        }
        video.layerMask = layers;
        video.markObjects = marked;
        video.hideObject = hidden;
        int x = 0, y = -1;
        if (g_d.anchorNow) {
            g_d.anchor(machine, x, y);
            if (x >= 0) {
                g.anchorX = x;
            }
            if (y >= 0) {
                g.anchorY = y;
            } else if (!g.feetKnown) {
                g.feetFound = true;
            }
        }
        // (its feet, where the game's sources do not say: the bottom of its picture as it first stands, or near the
        // bottom of the picture until it shows)
        if (g.feetFound) {
            g.anchorY = height - 32;
            for (int line = height - 1; line >= 0 && g.feetFound; line--) {
                for (int column = 0; column < width; column++) {
                    if (model[static_cast<size_t>(line) * width + column] & 0x80000000u) {
                        g.anchorY = line;
                        g.feetFound = false;
                        g.feetKnown = true;
                        break;
                    }
                }
            }
        }

        if (g_d.nearestOnly) {
            KeepNearest(model.data(), width, height, g.anchorX, g.anchorY);
        }
        if (Plain(g.background)) {
            std::fill(picture, picture + static_cast<size_t>(width) * height, g.background == White() ? 0xFFFFFFu : 0u);
        }
        // Each pixel of the picture looks at the character's picture, so many quarters to a pixel, about where the
        // character stands (its feet for the smaller sizes; for the larger ones a place in its middle, put in the
        // middle of the picture, so that it is not all legs)
        const bool large = size > 8;
        const int fromX = g.anchorX, fromY = g.anchorY - (large ? g_d.largeLift : 0);
        const int toX = (large || g_d.centred ? width / 2 : g.anchorX) - Shift(), toY = large ? height / 2 : g.anchorY;
        auto back = [size](int to, int about, int from) { // (rounded down, for places left of or above `about` too)
            const int quarters = (to - about) * 4 + 2;
            return from + (quarters >= 0 ? quarters / size : -((-quarters + size - 1) / size));
        };
        for (int y = 0; y < height; y++) {
            const int sourceY = back(y, toY, fromY);
            if (sourceY < 0 || sourceY >= height) {
                continue;
            }
            for (int x = 0; x < width; x++) {
                const int sourceX = back(x, toX, fromX);
                if (sourceX >= 0 && sourceX < width) {
                    const uint32_t pixel = model[static_cast<size_t>(sourceY) * width + sourceX];
                    if (pixel & 0x80000000u) {
                        const uint32_t shown = front ? fight[static_cast<size_t>(sourceY) * width + sourceX] : pixel;
                        picture[static_cast<size_t>(y) * width + x] = (shown & 0xFFFFFF) == (pixel & 0xFFFFFF) ? pixel & 0xFFFFFF : shown;
                    }
                }
            }
        }
    }

    // --- The exports: the picture shown, or every frame of the animation shown, as files in the player's folder ---

    // A taken frame: the character's marked pixels, and where it stood in them
    struct Taken {
        std::vector<uint32_t> pixels;
        int anchorX = 0, anchorY = 0;
        int time = 0;       // sixtieths of a second it is shown for
        uint32_t frame = 0; // its address
    };

    // A UTF-8 name as a path (a folder with an accent, on Windows)
    std::filesystem::path PathOf(const std::string &text) {
        return std::filesystem::path(std::u8string(text.begin(), text.end()));
    }

    int ExportSize() {
        return kExportSizes[std::clamp(FeatureValue("model_export_size"), 0, static_cast<int>(std::size(kExportSizes)) - 1)];
    }

    // A name that is a file's name on every system
    std::string FileName(std::string text) {
        for (char &c : text) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '-';
            }
        }
        while (!text.empty() && (text.back() == '.' || text.back() == ' ')) {
            text.pop_back(); // (Windows will not have them at the end)
        }
        return text;
    }
    std::string CharacterName() {
        return g.character >= 0 && g.character < Characters() ? FileName(g_d.characters[static_cast<size_t>(g.character)]) : "Character";
    }

    // A folder of its own under exports/ ("name", or "name 2" and on if there is one already), made; "" if it could not
    // be
    std::string NewFolder(const std::string &name) {
        std::error_code error;
        const std::string under = PlayerFolder() + "exports/";
        std::string folder = under + name;
        for (int n = 2; std::filesystem::exists(PathOf(folder), error); n++) {
            folder = under + name + " " + std::to_string(n);
        }
        std::filesystem::create_directories(PathOf(folder), error);
        return error ? "" : folder + "/";
    }

    // What was saved, said over the picture: from the frame, or from the thread writing the files (taken up by the frame)
    std::mutex g_sayLock;
    std::string g_toSay;
    bool g_saying = false;
    std::atomic<bool> g_writing{false}; // (an export's files are being written)
    void Say(const std::string &text) {
        std::lock_guard<std::mutex> lock(g_sayLock);
        g_toSay = text;
        g_saying = true;
        fprintf(stderr, "model viewer: %s\n", text.c_str());
    }
    void TakeSaid() {
        std::lock_guard<std::mutex> lock(g_sayLock);
        if (g_saying) {
            g.said = g_toSay;
            g.saidFor = g_writing ? 1 << 30 : kSaidFor; // ("saving" stays until it is done)
            g_saying = false;
        }
    }

    // Frames taken as pictures of one size, the character standing in the same place in each: as small as holds all of
    // them, at the board's own size (made larger as they are written). (anchorX and anchorY: where it stands in them.)
    std::vector<image_files::Picture> Cut(const std::vector<Taken> &taken, int width, int height, int &anchorX, int &anchorY) {
        constexpr int size = 1;
        int left = 1 << 20, top = 1 << 20, right = -(1 << 20), bottom = -(1 << 20); // (from the anchor)
        for (const Taken &one : taken) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    if (one.pixels[static_cast<size_t>(y) * width + x] & 0x80000000u) {
                        left = std::min(left, x - one.anchorX);
                        right = std::max(right, x - one.anchorX);
                        top = std::min(top, y - one.anchorY);
                        bottom = std::max(bottom, y - one.anchorY);
                    }
                }
            }
        }
        std::vector<image_files::Picture> pictures;
        if (right < left) {
            return pictures; // (nothing drawn in any of them)
        }
        const int across = right - left + 1, down = bottom - top + 1;
        anchorX = -left * size;
        anchorY = -top * size;
        for (const Taken &one : taken) {
            image_files::Picture picture;
            picture.width = across * size;
            picture.height = down * size;
            picture.pixels.assign(static_cast<size_t>(picture.width) * picture.height, 0);
            for (int y = 0; y < down; y++) {
                const int fromY = y + top + one.anchorY;
                for (int x = 0; x < across; x++) {
                    const int fromX = x + left + one.anchorX;
                    if (fromX < 0 || fromX >= width || fromY < 0 || fromY >= height) {
                        continue;
                    }
                    const uint32_t pixel = one.pixels[static_cast<size_t>(fromY) * width + fromX];
                    if (!(pixel & 0x80000000u)) {
                        continue;
                    }
                    for (int dy = 0; dy < size; dy++) {
                        uint32_t *row = &picture.pixels[static_cast<size_t>(y * size + dy) * picture.width + x * size];
                        std::fill(row, row + size, (pixel & 0xFFFFFFu) | 0xFF000000u);
                    }
                }
            }
            pictures.push_back(std::move(picture));
        }
        return pictures;
    }

    // The picture shown: the character as it is now, by itself
    void ExportPicture(Machine &machine) {
        if (!g.seen || g_model.empty()) {
            Say("NOTHING TO SAVE: THE CHARACTER IS NOT SHOWN");
            return;
        }
        Taken one;
        one.pixels = g_model;
        one.anchorX = g.anchorX;
        one.anchorY = g.anchorY;
        int anchorX = 0, anchorY = 0;
        const std::vector<image_files::Picture> pictures = Cut({one}, g_modelWidth, Video::kHeight, anchorX, anchorY);
        std::string name = CharacterName();
        if (g.table) {
            name += " " + ListName(g.table - 1) + std::to_string(g.number);
        }
        const std::string folder = PlayerFolder() + "exports/";
        std::error_code error;
        std::filesystem::create_directories(PathOf(folder), error);
        std::string file = name;
        for (int n = 2; std::filesystem::exists(PathOf(folder + file + ".png"), error); n++) {
            file = name + " " + std::to_string(n);
        }
        if (pictures.empty() || !image_files::WritePng(folder + file + ".png", pictures[0], ExportSize())) {
            Say("NOT SAVED");
            return;
        }
        (void)machine;
        Say("SAVED  exports/" + file + ".png");
    }

    // Every frame of the animation shown, each put in the character's record and the game run until the picture is
    // its, on a copy of the viewer's machine (the viewer's own goes on as it was): from its first frame to its end, or
    // to where it goes back to an earlier frame (where the viewer repeats it)
    void ExportAnimation(Machine &machine) {
        if (!g.table || !g.first) {
            Say("CHOOSE AN ANIMATION TO SAVE (STANDING IS THE GAME'S OWN)");
            return;
        }
        const uint32_t first = g.first;
        const int fallbackY = g.anchorY;
        std::vector<Taken> taken;
        Spare spare;
        {
            Archive ar(spare.state, true);
            machine.Serialize(ar);
        }
        spare.ready = true;
        RunSpare(
            machine, spare,
            [](Machine &m) {
                if (g_d.spareFrame) {
                    g_d.spareFrame(m);
                }
            },
            nullptr,
            [&](Machine &m) {
                uint32_t frame = first;
                while (frame && static_cast<int>(taken.size()) < kMostExportFrames) {
                    for (int n = 0; n < kSettleFrames; n++) {
                        if (g_d.spareFrame) {
                            g_d.spareFrame(m);
                        }
                        g_d.showFrame(m, frame);
                        m.players[0] = m.players[1] = 0;
                        m.RunFrame();
                    }
                    Taken one;
                    DrawModel(m, one.pixels);
                    int x = 0, y = -1;
                    g_d.anchor(m, x, y);
                    one.anchorX = x;
                    one.anchorY = y >= 0 ? y : fallbackY;
                    if (g_d.nearestOnly) {
                        KeepNearest(one.pixels.data(), m.PictureWidth(), Video::kHeight, one.anchorX, one.anchorY);
                    }
                    one.time = FrameTime(m, frame);
                    one.frame = frame;
                    taken.push_back(std::move(one));
                    const uint32_t next = g_d.nextFrame(m, frame);
                    frame = next > frame && IsFrame(m, next) ? next : 0;
                }
            });
        const int width = machine.PictureWidth(), size = ExportSize();
        int anchorX = 0, anchorY = 0;
        std::vector<image_files::Picture> pictures = Cut(taken, width, Video::kHeight, anchorX, anchorY);
        if (pictures.empty()) {
            Say("NOTHING TO SAVE: THE CHARACTER IS NOT DRAWN IN IT");
            return;
        }
        const std::string animation = ListName(g.table - 1) + std::to_string(g.number);
        const std::string name = CharacterName() + " " + animation;
        const std::string folder = NewFolder(name);
        if (folder.empty()) {
            Say("NOT SAVED: NO FOLDER");
            return;
        }
        // (the animated files play at the viewer's speed: each frame's time made that many times longer; the frames' own
        // times, in animation.json, stay the game's)
        const int slowBy = kSlowBy[std::clamp(FeatureValue("model_speed"), 0, static_cast<int>(std::size(kSlowBy)) - 1)];
        std::vector<int> times;
        char file[32];
        std::string frames;
        for (size_t n = 0; n < pictures.size(); n++) {
            snprintf(file, sizeof(file), "frame %02d.png", static_cast<int>(n));
            times.push_back(taken[n].time * slowBy);
            char line[160];
            snprintf(line, sizeof(line), "%s    {\"file\": \"%s\", \"time\": %d, \"address\": \"0x%06X\"}", n ? ",\n" : "", file,
                     taken[n].time, taken[n].frame);
            frames += line;
        }
        // (the sheet: the frames in rows of up to eight, left to right)
        const int across = std::min(static_cast<int>(pictures.size()), kSheetAcross);
        const int down = (static_cast<int>(pictures.size()) + kSheetAcross - 1) / kSheetAcross;
        image_files::Picture sheet;
        sheet.width = pictures[0].width * across;
        sheet.height = pictures[0].height * down;
        sheet.pixels.assign(static_cast<size_t>(sheet.width) * sheet.height, 0);
        for (size_t n = 0; n < pictures.size(); n++) {
            const int atX = static_cast<int>(n % kSheetAcross) * pictures[0].width, atY = static_cast<int>(n / kSheetAcross) * pictures[0].height;
            for (int y = 0; y < pictures[n].height; y++) {
                std::copy_n(&pictures[n].pixels[static_cast<size_t>(y) * pictures[n].width], pictures[n].width,
                            &sheet.pixels[static_cast<size_t>(atY + y) * sheet.width + atX]);
            }
        }
        // (what the frames are, for a program reading them: their times in sixtieths of a second, and where the character
        // stands in each, the same in all)
        std::string json = "{\n  \"character\": \"" + CharacterName() + "\",\n  \"animation\": \"" + animation + "\",\n";
        char head[384];
        snprintf(head, sizeof(head),
                 "  \"frames per second\": 60,\n  \"animated files slowed by\": %d,\n  \"scale\": %d,\n  \"width\": %d,\n  \"height\": %d,\n  \"anchor\": {\"x\": %d, \"y\": %d},\n"
                 "  \"sheet columns\": %d,\n  \"frames\": [\n",
                 slowBy, size, pictures[0].width * size, pictures[0].height * size, anchorX * size, anchorY * size, across);
        json += head + frames + "\n  ]\n}\n";
        // (the files written on a thread of their own: at the larger sizes they take a while, and the viewer goes on)
        const std::string shown = folder.substr(PlayerFolder().size());
        const size_t count = pictures.size();
        g_writing = true;
        Say("SAVING " + std::to_string(count) + " FRAMES...");
        std::thread([folder, name, shown, size, json, times, count, pictures = std::move(pictures), sheet = std::move(sheet)] {
            bool ok = true;
            char file[32];
            for (size_t n = 0; n < count; n++) {
                snprintf(file, sizeof(file), "frame %02d.png", static_cast<int>(n));
                ok = image_files::WritePng(folder + file, pictures[n], size) && ok;
            }
            ok = image_files::WritePng(folder + "sheet.png", sheet, size) && ok;
            ok = image_files::WriteApng(folder + name + ".png", pictures, times, size) && ok;
            ok = image_files::WriteGif(folder + name + ".gif", pictures, times, size) && ok;
            ok = image_files::WriteAll(folder + "animation.json", std::vector<uint8_t>(json.begin(), json.end())) && ok;
            g_writing = false;
            Say(ok ? "SAVED " + std::to_string(count) + " FRAMES  " + shown : "NOT ALL SAVED  " + shown);
        }).detach();
    }

    std::vector<const char *> Backgrounds() {
        std::vector<const char *> names = g_d.stages;
        names.push_back("White");
        names.push_back("Black");
        return names;
    }
} // namespace

int ChosenCharacter() {
    return std::clamp(FeatureValue("model_character"), 0, Characters() - 1);
}
int ChosenBackground() {
    return std::clamp(FeatureValue("model_background"), 0, Black());
}
int Number(int choice) {
    if (choice < 0 || choice >= Characters()) {
        return -1;
    }
    return g_d.numbers.empty() ? choice : g_d.numbers[static_cast<size_t>(choice)];
}
bool Plain(int background) {
    return background >= Stages();
}
int White() {
    return Stages();
}
int Black() {
    return Stages() + 1;
}

void Begin() {
    g = State();
    g.on = true;
    g.character = ChosenCharacter();
    g.background = ChosenBackground();
}
void End() {
    if (g.on) {
        g = State();
        SetSlowMotion(1);
    }
}
bool On() {
    return g.on;
}
int Character() {
    return g.character;
}
int Background() {
    return g.background;
}
int CharacterNumber() {
    return Number(g.character);
}
bool Seen() {
    return g.seen;
}
bool Changed() {
    return ChosenCharacter() != g.character || ChosenBackground() != g.background;
}

void BeforeFrame(Machine &machine) {
    SetSlowMotion(kSlowBy[std::clamp(FeatureValue("model_speed"), 0, static_cast<int>(std::size(kSlowBy)) - 1)]);
    ShowAnimation(machine);
}

void Scan(Machine &machine) {
    if (!g_d.firstLine) {
        return;
    }
    const int character = CharacterNumber();
    if (g_scan.character != character) {
        g_scan = LineScan();
        g_scan.character = character;
        g_scan.done = g_d.noScan && g_d.noScan(character);
        if (!g_scan.done) {
            Archive ar(g_scan.spare.state, true); // (the copy: the viewer's machine as it is)
            machine.Serialize(ar);
            g_scan.spare.ready = true;
        }
    }
    if (g_scan.done) {
        return;
    }
    RunSpare(
        machine, g_scan.spare,
        [](Machine &m) {
            if (g_d.spareFrame) {
                g_d.spareFrame(m);
            }
        },
        nullptr,
        [character](Machine &m) {
            for (int n = 0; n < kScanFramesEach && !g_scan.done; n++) {
                // (what the frame shown last made, counted; then the next frame, or the next animation)
                if (g_scan.choice) {
                    int counts[kMostLines] = {};
                    CountLines(m, counts);
                    for (int line = 0; line < g_d.linesInSet; line++) {
                        if (counts[line] > g_scan.most[line]) {
                            g_scan.most[line] = counts[line];
                            g_scan.best[line] = g_scan.choice;
                        }
                    }
                    const uint32_t next = g_d.nextFrame(m, g_scan.frame);
                    if (next > g_scan.frame && IsFrame(m, next) && ++g_scan.frames < kScanFramesMost) {
                        g_scan.frame = next;
                    } else {
                        const int choice = Step(m, character, g_scan.choice, 1);
                        if (choice <= g_scan.choice) { // (gone round: every animation looked at)
                            g_scan.done = true;
                            break;
                        }
                        ScanAt(m, character, choice);
                    }
                } else {
                    ScanAt(m, character, Step(m, character, 0, 1));
                }
                if (!g_scan.frame) {
                    g_scan.done = true;
                    break;
                }
                g_d.showFrame(m, g_scan.frame);
                m.players[0] = m.players[1] = 0;
                m.RunFrame();
            }
        });
}

void Frame(Machine &machine) {
    Compose(machine);
    if (g.exportWanted && !RunningAhead()) {
        const int wanted = g.exportWanted;
        g.exportWanted = 0;
        if (wanted == 2 && g_writing) {
            Say("STILL SAVING THE LAST ONE");
        } else if (wanted == 1) {
            ExportPicture(machine);
        } else {
            ExportAnimation(machine);
        }
    }
    NameAnimation();
    if (g_d.anchorNow) {
        return;
    }
    int x = 0, y = -1;
    g_d.anchor(machine, x, y);
    g.anchorX = x;
    if (y >= 0) {
        g.anchorY = y;
    } else if (!g.feetFound && g.anchorY == 0) {
        g.feetFound = true;
    }
}

int AnimationStep(Machine &machine, int choice, int by) {
    const int next = Step(machine, CharacterNumber(), choice, by);
    if (!next || (choice && (by > 0 ? next <= choice : next >= choice))) {
        return 0; // (gone round)
    }
    return next;
}
int AnimationForLine(int line) {
    if (line <= 0) {
        return 0; // (the body: standing shows all of it)
    }
    if (g_scan.character != CharacterNumber() || !g_scan.done) {
        return -2;
    }
    return line < g_d.linesInSet && line < kMostLines && g_scan.best[line] ? g_scan.best[line] : -1;
}
const std::vector<const char *> &AnimationNames() {
    static std::vector<std::string> text;
    static std::vector<const char *> names;
    if (names.empty()) {
        text.push_back("Standing (you control it)");
        for (int list = 0; list < g_d.lists; list++) {
            for (int n = 0; n < g_d.perList; n++) {
                text.push_back(ListName(list) + std::to_string(n));
            }
        }
        for (const std::string &one : text) {
            names.push_back(one.c_str());
        }
    }
    return names;
}

void Register(const Description &description) {
    g_d = description;
    g_choices = 1 + g_d.lists * g_d.perList;
    Game &game = TheGame();
    game.pages.push_back({g_d.page, g_d.parent, g_d.pageWords});
    game.commands.push_back({"model_start", g_d.page, "Start", g_d.startWords, g_d.start});
    game.commands.push_back({"model_reset", g_d.page, "Reset options", g_d.resetWords, ResetChoices});
    // (the exports: a page of their own under the viewer's)
    game.pages.push_back({kExportPage, g_d.page, kExportWords});
    game.commands.push_back({"model_save_picture", kExportPage, "Save picture",
                             "Saves the character as it is shown now, by itself on nothing, as a PNG in the exports folder beside "
                             "your saves, at the export size chosen on this page.",
                             [](Machine &) {
                                 if (g.on) {
                                     g.exportWanted = 1;
                                 } else {
                                     Say("START THE VIEWER FIRST");
                                 }
                             }});
    game.commands.push_back({"model_save_animation", kExportPage, "Save animation",
                             "Saves every frame of the animation shown, by itself on nothing, in a folder of its own in the exports "
                             "folder beside your saves: each frame as a PNG, all of them on one sheet, a looping animated PNG and "
                             "GIF with the game's own timing (slowed as the viewer's Speed is), and a list of the frames' times for "
                             "other programs.",
                             [](Machine &) {
                                 if (g.on) {
                                     g.exportWanted = 2;
                                 } else {
                                     Say("START THE VIEWER FIRST");
                                 }
                             }});
    game.commands.push_back({"model_show_folder", kExportPage, "Show pictures folder",
                             "Opens the exports folder beside your saves (where Save picture and Save animation write) in your file "
                             "manager.",
                             [](Machine &) {
                                 const std::string folder = PlayerFolder() + "exports";
                                 std::error_code error;
                                 std::filesystem::create_directories(PathOf(folder), error);
                                 if (!OpenFolder(folder)) {
                                     Say("CAN'T OPEN IT HERE: IT IS " + folder);
                                 }
                             }});
    game.features.push_back({"model_export_size", kExportPage, "Export size",
                             "How large the saved pictures are: each of the board's pixels as a square of this many.", 0,
                             {"1x", "2x", "3x", "4x", "6x", "8x", "12x", "16x"}});
    game.features.push_back({"model_character", g_d.page, "Character", "Who is shown. Changing it while viewing goes to the new one.", 0,
                             g_d.characters});
    game.features.push_back({"model_background", g_d.page, "Background",
                             "What is behind the character: one of the stages, or plain white or black. Changing it while viewing "
                             "goes to the new one.",
                             0, Backgrounds()});
    if (g_d.inAFight) {
        game.features.push_back({"model_foreground", g_d.page, "Stage in front", g_d.foregroundWords, 0, {"In front", "Behind"}});
    }
    game.features.push_back({"model_size", g_d.page, "Size", "How large the character is drawn, from a quarter of its own size to 25 times.", 0,
                             {"1x", "1.5x", "2x", "3x", "4x", "6x", "8x", "12x", "16x", "25x", "0.25x", "0.5x", "0.75x"}});
    game.features.push_back({"model_animation", g_d.page, "Animation", g_d.animationWords, 0, AnimationNames()});
    game.features.push_back({"model_speed", g_d.page, "Speed",
                             "How fast the viewer runs: slow motion for looking at an animation frame by frame. (The menu's own "
                             "speed, and the - and = keys, slow it further.)",
                             0, {"Full", "1/2", "1/4", "1/8"}});
    game.features.push_back({"model_repeat", g_d.page, "Repeat after", "How long an animation's last frame is held before it starts again.",
                             0,
                             {"At once", "1 second", "2 seconds", "3 seconds", "4 seconds", "5 seconds", "6 seconds", "7 seconds",
                              "8 seconds", "9 seconds", "10 seconds"}});
    game.features.push_back({"model_all", g_d.page, "Play all",
                             "Goes through all of the character's animations, one after another: each is shown once (and its "
                             "last frame held for \"Repeat after\") and then the next starts, round from the last to the first. "
                             "Left and right still step through them.",
                             0});
}

} // namespace cps2recomp::model
