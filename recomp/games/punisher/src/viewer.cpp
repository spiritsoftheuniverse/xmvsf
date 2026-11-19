// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The Punisher: the stage and model viewers (the toolkit's beat/beat.h). A stage of the viewer is a stretch of a stage the camera
// goes along in one piece: the viewer begins a game at that stage, unseen, walks player 1 on until the camera is
// there, and then has the camera.
//
// The camera is an object at FF7356 (A5-CAA; kCamera): its place across at +20 (the picture's left edge) and up and
// down at +24 (a long, the word first), the limits it keeps to at +5E, +60 (up and down) and +62, +64 (across), its
// step at +08 (0 setting up, drawing the whole picture; 4 moving). Its routine (0A84CC, called by each stage's own
// code every frame) moves it towards the players at most 4 pixels a frame, and draws the scroll screens' new columns
// and rows as it crosses them, from where it was (+40, +42, kept before it moves) to where it is (0A9006 and 0A9032 the columns at its left and right, 0A8FAE and 0A8FDA the rows below
// and above: each draws only when the place has crossed a column or row since the last). While viewing, the viewer
// moves it, as far a frame, and has it draw as it would. A stage goes on in parts the stage's own code sets up (a
// door, a lift: 00E540 is the first of stage 1's), which move the camera elsewhere at once; the viewer has the stretch
// a stage begins with (or, where its start is an entrance of a moment, the one after it). The stretches were found by
// tracing the camera through each stage played to its end.
#include "beat.h"
#include "punisher.h"
#include "state.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>

namespace cps2recomp {

namespace {
    using namespace punisher;

    constexpr uint32_t kCamera = kVars - 0xCAA;
    constexpr uint32_t kCameraStep = kCamera + 0x08, kCameraX = kCamera + 0x20, kCameraY = kCamera + 0x24;
    constexpr uint32_t kCameraMoves = 0x0A84CC;
    constexpr uint32_t kDrawLeft = 0x0A9006, kDrawRight = 0x0A9032, kDrawBelow = 0x0A8FAE, kDrawAbove = 0x0A8FDA;
    constexpr int kMiddle = 0xC0; // (from the camera's place, the picture's left edge)
    constexpr int kMove = 4; // (pixels a frame, as the game moves it)
    constexpr uint32_t kX = 0x20; // a player's place across (word)

    struct Place {
        int16_t x, y;
    };
    struct Stage {
        const char *name;
        uint8_t stage;
        Place way[4];
    };
    constexpr Place kEnd = {-1, -1};
    constexpr Stage kViewed[] = {
        {"Stage 1: Back to the War (the bar)", 0, {{0x120, 0x230}, {0x200, 0x230}, kEnd}},
        {"Stage 2: Raid on Pantaberde", 1, {{0x100, 0xE3}, {0x380, 0xE3}, kEnd}},
        {"Stage 3: Waterfront Warfare", 2, {{0x100, 0xD7}, {0x940, 0xD7}, kEnd}},
        {"Stage 4: Death on Rails", 3, {{0x180, 0x90}, {0x380, 0x90}, kEnd}},
        {"Stage 5: Kingpin's Wrath", 4, {{0x104, 0xD4}, {0x750, 0xD4}, kEnd}},
        {"Stage 6: Final Punishment", 5, {{0x000, 0x2C9}, {0xCF2, 0x2C9}, kEnd}},
    };

    // (kept with the state: online play runs frames again from it)
    struct View {
        int32_t x = -1, y = -1; // where the viewer has the camera (-1: not viewing)
        int32_t playing = 0; // frames the game has been played at the wanted stage
    } g_view;

    // The player the viewers' game is played by: the model viewer's character's (0 the Punisher, 1 Nick Fury)
    int Who() {
        return beat::ModelViewer() ? beat::ViewedCharacter() : 0;
    }

    int Toward(int from, int to) {
        return from + std::clamp(to - from, -kMove, kMove);
    }

    // 0A84CC: the camera's routine. While viewing, once it is moving, the viewer moves it instead, and it draws what
    // that brings into the picture as its own moves do.
    bool CameraMoves(M68K &c) {
        if (g_view.x < 0 || !beat::Viewing() || Ram16(kCameraStep) != 4) {
            return false;
        }
        uint32_t d[8], a[8];
        std::copy(std::begin(c.d), std::end(c.d), d);
        std::copy(std::begin(c.a), std::end(c.a), a);
        c.a[0] = kCamera;
        const int x = static_cast<int16_t>(Ram16(kCameraX)), y = static_cast<int16_t>(Ram16(kCameraY));
        const int nx = Toward(x, g_view.x), ny = g_view.y >= 0 ? Toward(y, g_view.y) : y;
        // (the place before the move, which the drawing compares with: 0A858A)
        SetRam16(kCamera + 0x40, static_cast<uint16_t>(x));
        SetRam16(kCamera + 0x42, static_cast<uint16_t>(y));
        SetRam16(kCameraX, static_cast<uint16_t>(nx));
        SetRam16(kCameraY, static_cast<uint16_t>(ny));
        if (nx > x) {
            Call(c, kDrawRight);
        } else if (nx < x) {
            Call(c, kDrawLeft);
        }
        if (ny > y) {
            Call(c, kDrawBelow);
        } else if (ny < y) {
            Call(c, kDrawAbove);
        }
        // (the registers as they were, and back to the routine's caller)
        std::copy(d, d + 8, std::begin(c.d));
        std::copy(a, a + 8, std::begin(c.a));
        Return(c);
        return true;
    }

    // The way to a stage: a credit and Start once the show has begun (free play is off in the modes), and once the
    // stage is played, player 1 walking on until the camera is at the stretch's start
    bool ToStage(Machine &machine, int stage, int32_t &frames) {
        const Stage &want = kViewed[stage];
        if (!Ram8(kPlayersIn)) {
            g_view.playing = 0;
            // (player 2's Start takes two credits when player 1 is not in: "only 1 player" with one)
            const uint16_t credits = static_cast<uint16_t>(Who() + 1);
            if (Ram16(kCredits) < credits && frames < 4000 && !(frames & 0x100)) {
                SetRam16(kCredits, credits);
            }
            // (the Punisher is player 1's, Nick Fury player 2's: the model viewer's begins the game with that one's Start)
            machine.players[Who()] = (frames & 0x10) ? ((frames & 0x20) ? kStart : kPunch1) : 0; // (held 16 frames)
            return false;
        }
        if (Ram16(kStep) != kStepPlay || Ram8(kStage) != want.stage || Ram16(kCameraStep) != 4) {
            g_view.playing = 0;
            return false;
        }
        const int x = static_cast<int16_t>(Ram16(kCameraX)), y = static_cast<int16_t>(Ram16(kCameraY));
        const bool there = std::abs(x - want.way[0].x) < 0x100 && std::abs(y - want.way[0].y) < 0x80;
        if (!there && g_view.playing < 6000) {
            machine.players[Who()] = kRight;
        }
        return ++g_view.playing > 120 && (there || g_view.playing > 6000);
    }

    void View(Machine &, int, int x, int y) {
        g_view.x = x;
        g_view.y = y;
        // (the players out of harm, in the middle of the picture, and the clock still)
        for (int n = 0; n < kPlayers; n++) {
            const uint32_t p = Player(n);
            if (Ram8(p + kInPlay) == 1) {
                SetRam16(p + kHealth, kHealthFull);
                if (x >= 0 && !beat::ModelViewer()) {
                    SetRam16(p + kX, static_cast<uint16_t>(Ram16(kCameraX) + kMiddle));
                }
            }
        }
        SetRam16(kClockFrames, kClockTick);
    }
} // namespace


// --- the model viewer ---
namespace {
    // The characters: a player's animations are by its number (+02: 0 the Punisher, 1 Nick Fury; 020824 and 02083C
    // take them from two tables each, of word offsets, as many as the first is twice), and its objects are in colours
    // 0 and 1 (found from the object list)
    constexpr const char *kCharacters[] = {"The Punisher", "Nick Fury"};
    constexpr uint32_t kTables[2][2] = {{0x108C30, 0x109C10}, {0x111384, 0x1123FA}};
    constexpr uint16_t kColours[] = {0, 1};

    uint16_t Word(uint32_t address) {
        const uint8_t *p = Program(address);
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }

    // A frame of an animation (as 00117C steps them, for the players): four bytes, a word that says how far on its
    // picture's words are, a byte for the record's +11 and its time; the time's top bit says the next word is how far
    // on (from that word) the frame after it is
    int FrameTime(uint32_t frame) {
        return Program(frame)[3] & 0x7F;
    }
    uint32_t NextFrame(uint32_t frame) {
        const uint32_t next = frame + 4;
        return Program(frame)[3] & 0x80 ? next + static_cast<int16_t>(Word(next)) : next;
    }
    // A frame's picture starts with its kind, which the objects' drawing (014FF0) takes from a table of 14 (0, 4 ...
    // 0x34): an address whose picture has another is not a frame (some of the tables' entries are not animations, and
    // forced on a player they crash the game)
    constexpr uint32_t kProgramEnd = 0x180000;
    bool IsFrame(uint32_t frame) {
        if ((frame & 1) || frame + 8 > kProgramEnd) {
            return false;
        }
        const uint32_t picture = frame + Word(frame);
        if ((picture & 1) || picture < 8 || picture >= kProgramEnd || (Program(picture)[0] & 3) || Program(picture)[0] > 0x34) {
            return false;
        }
        const int16_t on = static_cast<int16_t>(Word(frame + 4));
        return !(Program(frame)[3] & 0x80) || (on <= 0 && !(on & 1));
    }

    // A frame put in the character's record as 00117C would (+16 the next, +12 its picture's words and the four words
    // before them at +1C, +1A, +3C, +3E, +11 and its time at +10), with its time kept high so that the game's own
    // stepping never comes round (0: back to standing, the first animation of the first table)
    void ShowFrame(uint32_t frame) {
        const int who = beat::ViewedCharacter();
        const uint32_t p = Player(who);
        const bool standing = frame == 0;
        if (standing) {
            frame = kTables[who][0] + Word(kTables[who][0]);
        }
        const uint32_t picture = frame + Word(frame);
        SetRam32(p + 0x16, NextFrame(frame));
        SetRam32(p + 0x12, picture);
        SetRam16(p + 0x1C, Word(picture - 2));
        SetRam16(p + 0x1A, Word(picture - 4));
        SetRam16(p + 0x3C, Word(picture - 6));
        SetRam16(p + 0x3E, Word(picture - 8));
        SetRam8(p + 0x11, Program(frame)[2]);
        SetRam8(p + 0x10, standing ? static_cast<uint8_t>(std::max(FrameTime(frame), 1)) : 0x7F);
    }
} // namespace

// The stage a game the viewers begin starts at (-1: not the viewers'), and the character it is played with
int ViewerStage() {
    return beat::Viewing() ? kViewed[beat::ViewedStage()].stage : -1;
}
int ViewerCharacter() {
    return -1;
}
void ViewerGameBegins() {}

void ViewerFrame(Machine &) {
    if (!beat::Viewing() || beat::Starting()) {
        g_view.x = g_view.y = -1;
    }
}

void ViewerState(Archive &ar) {
    ar.Pod(g_view);
}

void DescribeViewer(beat::Description &d) {
    for (const Stage &stage : kViewed) {
        d.stages.push_back(stage.name);
    }
    d.toStage = ToStage;
    d.stagePath = [](int stage, std::vector<beat::Description::Place> &path) {
        for (const Place &place : kViewed[stage].way) {
            if (place.x < 0) {
                break;
            }
            path.push_back({place.x, place.y});
        }
    };
    d.stageCamera = [](int &x, int &y) {
        x = static_cast<int16_t>(Ram16(kCameraX));
        y = static_cast<int16_t>(Ram16(kCameraY));
    };
    d.view = View;
    d.viewLayers = 0xD; // (not the first scroll screen: the panel)
    for (const char *character : kCharacters) {
        d.characters.push_back(character);
    }
    d.animations = [](int character, std::vector<std::vector<uint32_t>> &lists) {
        for (uint32_t table : kTables[character]) {
            std::vector<uint32_t> list;
            for (uint32_t k = 0; k < Word(table) / 2u; k++) {
                if (Word(table + 2 * k)) {
                    list.push_back(table + Word(table + 2 * k));
                }
            }
            lists.push_back(std::move(list));
        }
    };
    d.frameTime = FrameTime;
    d.nextFrame = NextFrame;
    d.isFrame = IsFrame;
    d.showFrame = ShowFrame;
    d.characterObject = [](uint32_t, uint16_t attributes) { return (attributes & 0x1F) == kColours[beat::ViewedCharacter()]; };
    d.standing = [](int &x, int &y) {
        x = static_cast<int16_t>(Ram16(Player(beat::ViewedCharacter()) + kX) - Ram16(kCameraX));
        y = -1;
    };
    d.centre = [] { SetRam16(Player(beat::ViewedCharacter()) + kX, static_cast<uint16_t>(Ram16(kCameraX) + Video::kWidth / 2)); };
    AddHook(HookAt(kCameraMoves), "CameraMoves", CameraMoves);
}

} // namespace cps2recomp
