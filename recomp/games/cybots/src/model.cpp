// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the model viewer (the toolkit's model/model.h), a mode of the menu's "Alternative modes" page: one machine
// alone, on a stage or on plain white or black, at any size, standing as it does in a fight (and moved with player 1's
// controls), or going through any one of its animations. As Pocket Fighter's.
//
// It is a round of a two-player match, reached unseen as the stage viewer's is (fight/alternative.h); the second
// player is never drawn.
//
// A machine's animations (01F3F2 starts one: D1 a group, D0 a number in it): the table at 01F40E (read pc-relative,
// through the program's cipher: its sixteen longs are kAnimationTables) has for each machine a table of words, one a
// group, each how far on the group's list is; a list is words, each how far on from the list an animation's first
// frame is (0014B6). A frame is 12, 24 or 34 bytes (its byte +0B 0, 1 or more: 0014E4): a word of how many frames of
// the game it is shown for, its top bit saying the next is a jump (to the frame the long after this one names), a word
// the record keeps at +38, where its picture is (+04), and from +0C the numbers of its boxes (to +62 on). Showing one
// is putting it in the player's record each frame (+3C, as 001508 on does), with the time left (+3A) kept high so that
// the game's own stepping (0014CC) never comes round.
#include "colours.h"
#include "model.h"
#include "alternative.h"
#include "cybots.h"
#include "session.h"
#include "state.h"

#include <algorithm>
#include <iterator>

namespace cps2recomp {

namespace {
    using namespace cybots;

    constexpr const char *kModel = "Model viewer";
    constexpr int kStartMost = 9000; // frames after which getting to a round is given up

    // The machines in the menu's order, by their numbers in the game
    constexpr int kMachineNumbers[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    constexpr uint32_t kAnimationTables[kMachines] = {0x31BE2A, 0x3278B4, 0x330134, 0x3403E2, 0x346AC4, 0x35326E, 0x35CE60, 0x367A8A,
                                                      0x373BF4, 0x37CAB8, 0x387730, 0x398E26, 0x3A3F72, 0x3B0650, 0x3BB7EE, 0x3C6848};
    constexpr int kLists = 52, kPerList = 16; // (52: the most the toolkit names, A to AZ; one machine has 56 groups)
    constexpr uint32_t kFrame = 0x3C, kTimer = 0x3A; // a player's: the animation's frame (a long) and its time left
    constexpr int kForGoodShown = 30;

    struct State {
        bool on = false;
        bool starting = false;
        int32_t frames = 0;
    } g;
    bool g_fast = false;

    void Fast(bool on) {
        if (on != g_fast) {
            g_fast = on;
            SetFastForward(on);
        }
    }
    void End() {
        if (g.on) {
            g = State();
            modes::Wanted() = MatchWanted();
            modes::CameraAt() = CameraWanted();
            Fast(false);
            model::End();
        }
    }

    uint32_t Read32(Machine &machine, uint32_t address) {
        return (static_cast<uint32_t>(machine.Read16(address)) << 16) | machine.Read16(address + 2);
    }
    bool InAnimations(uint32_t address) {
        return address >= 0x300000 && address < 0x400000 && !(address & 1);
    }
    uint32_t FrameBytes(Machine &machine, uint32_t frame) {
        const uint8_t size = static_cast<uint8_t>(machine.Read16(frame + 0x0A));
        return size == 0 ? 0x0C : size == 1 ? 0x18 : 0x22;
    }
    // Is this an animation's frame? (In the animations' part of the ROM, shown for a while, of a size the game knows.)
    bool IsFrame(Machine &machine, uint32_t frame) {
        if (!InAnimations(frame)) {
            return false;
        }
        const uint16_t time = machine.Read16(frame) & 0x7FFF;
        return time != 0 && time < 0x400 && (machine.Read16(frame + 0x0A) & 0xFF) <= 2;
    }
    // Where an animation of the machine's starts (0: none of that number). A group's list ends where the nearest list
    // above it begins (the lists follow the groups' words).
    uint32_t AnimationAt(Machine &machine, int character, int list, int number) {
        if (character < 0 || character >= kMachines) {
            return 0;
        }
        const uint32_t table = kAnimationTables[character];
        const int groups = std::min(static_cast<int>(machine.Read16(table) / 2), kLists);
        if (list >= groups) {
            return 0;
        }
        const uint16_t at = machine.Read16(table + static_cast<uint32_t>(list) * 2);
        uint16_t end = 0xFFFF;
        for (int k = 0; k < groups; k++) {
            const uint16_t other = machine.Read16(table + static_cast<uint32_t>(k) * 2);
            end = other > at && other < end ? other : end;
        }
        if (end == 0xFFFF || at + static_cast<uint32_t>(number) * 2 >= end) {
            return 0;
        }
        const uint32_t start = table + at;
        const uint32_t frame = start + static_cast<uint32_t>(static_cast<int16_t>(machine.Read16(start + static_cast<uint32_t>(number) * 2)));
        return IsFrame(machine, frame) ? frame : 0;
    }
    int FrameTime(Machine &machine, uint32_t frame) {
        const uint16_t time = machine.Read16(frame) & 0x7FFF;
        return time >= 0x100 ? kForGoodShown : time;
    }
    // The frame after (0: the end, or the frame going on to itself, how the game holds an animation's last picture)
    uint32_t NextFrame(Machine &machine, uint32_t frame) {
        const uint32_t after = frame + FrameBytes(machine, frame);
        const uint32_t next = (machine.Read16(frame) & 0x8000) ? Read32(machine, after) : after;
        return next == frame || !IsFrame(machine, next) ? 0 : next;
    }
    void ShowFrame(Machine &machine, uint32_t frame) {
        const uint32_t player = Player(0);
        if (!frame) {
            // (back to standing: the game's own stepping takes it up again)
            SetRam16(player + kTimer, 1);
            return;
        }
        SetRam32(player + kFrame, frame);
        SetRam16(player + kTimer, 0x7FFF);
        SetRam16(player + 0x38, machine.Read16(frame + 2));
        const uint16_t sizes = machine.Read16(frame + 0x0A);
        SetRam8(player + 0x57, static_cast<uint8_t>(sizes >> 8));
        SetRam8(player + 0x56, static_cast<uint8_t>(sizes));
    }

    // Objects by what they are: for the machine's own picture, player 1's sets (00-07: its palettes and what it
    // throws); for the picture as the board would show it, everything but the panel and the second player's (10-17)
    // (not the shadows: both players' are in one set, and only the object list's places would tell them apart)
    bool IsModel(uint32_t tile, uint16_t attributes) {
        return (attributes & 0x1F) < 0x08 && !ThePanel(tile, attributes) && !IsShadow(tile);
    }
    bool InAFight(uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return !(ThePanel(tile, attributes) || (set >= 0x10 && set < 0x18));
    }

    void StartModel(Machine &machine) {
        End();
        modes::End();
        EndBattle();
        SetFeature("training_mode", false);
        g.on = true;
        g.starting = true;
        model::Begin();
        modes::Wanted().stage = model::Plain(model::Background()) ? 1 : model::Background() + 1;
        modes::Wanted().characters[0] = kMachineNumbers[model::Character()];
        modes::Wanted().characters[1] = kMachineNumbers[(model::Character() + 1) % std::size(kMachineNumbers)];
        machine.Reset();
    }
} // namespace

bool ModelViewer() {
    return g.on;
}
void StartModelViewer(Machine &machine) {
    StartModel(machine);
}

void ModelBeforeFrame(Machine &machine) {
    if (!g.on) {
        return;
    }
    modes::Wanted().stage = model::Plain(model::Background()) ? 1 : model::Background() + 1;
    if (g.starting) {
        Fast(true);
        if (modes::ToRound(machine, g.frames)) {
            g.starting = false;
            Fast(false);
            // (the machine in the middle of the stage, the second player at its right end)
            SetRam16(Player(0) + kX, static_cast<uint16_t>((kStageLeft + kStageRight) / 2));
            SetRam16(Player(1) + kX, static_cast<uint16_t>(kStageRight));
        } else if (g.frames > kStartMost) {
            End();
        }
        return;
    }
    machine.players[1] = 0;
    // (the camera on the machine: it follows both players, and the second is far off at the stage's right end)
    modes::CameraAt().x = static_cast<int16_t>(Ram16(Player(0) + kX)) - Video::kWidth / 2;
    if (colours::EditorAnimation()) {
        model::Scan(machine);
    }
    colours::EditorBeforeFrame(machine); // (takes player 1's controls, when it is on)
    model::BeforeFrame(machine);
}

void ModelFrame(Machine &machine) {
    if (!g.on) {
        return;
    }
    // (the board reset, by the menu or by another mode starting: the viewer is over)
    if (!g.starting && Ram16(kMode) == 0) {
        End();
        return;
    }
    if (g.starting) {
        return;
    }
    // Another machine or background chosen in the menu: the viewer starts again with it
    if (model::Changed()) {
        StartModel(machine);
        return;
    }
    machine.video.hideObject = NotTheStage;
    model::Frame(machine);
    colours::EditorFrame(machine);
}

// (Nothing of the viewer is kept in a save state: a state loaded while viewing ends the viewer, and what is loaded is
// the fight it holds.)
void ModelState(Archive &ar) {
    if (!ar.Saving() && !RunningAhead()) { // (not when run-ahead puts the frame back: runtime/game.h)
        End();
    }
}

void RegisterModel() {
    model::Description d;
    d.page = kModel;
    d.pageWords = "One machine by itself, on a stage or on white or black, at any size, standing or going through any of its "
                  "animations.";
    d.resetWords = "Puts every choice of this page back: the first machine and stage, its own size, standing, full speed, "
                   "repeating at once, one animation only. (While viewing, the viewer goes to that machine and stage.)";
    d.animationWords = "What the machine does: stands as in a fight, moved with player 1's controls, or goes through one of its "
                       "animations over and over. They are named by the game's groups of them and a number (C4); the ones a "
                       "machine has not got are passed over. While one is showing, left and right go to the one before and the "
                       "one after, and its name is shown.";
    d.foregroundWords = "What the stage has in front of the fighters: in front of the machine too, as in a fight, or behind it so "
                        "that all of it is seen. In front only at the machine's own size, and not in the colour editor.";
    d.characters = {std::begin(kMachineNames), std::end(kMachineNames)};
    d.numbers = {std::begin(kMachineNumbers), std::end(kMachineNumbers)};
    d.stages = {std::begin(kStageNames), std::end(kStageNames)};
    d.start = [](Machine &machine) {
        colours::EndEditor();
        StartModel(machine);
    };
    d.lists = kLists;
    d.perList = kPerList;
    d.animationAt = AnimationAt;
    d.isFrame = IsFrame;
    d.frameTime = FrameTime;
    d.nextFrame = NextFrame;
    d.showFrame = ShowFrame;
    d.animationChoice = colours::EditorAnimation;
    d.isModel = IsModel;
    d.inAFight = InAFight;
    d.anchor = [](Machine &, int &x, int &y) {
        x = static_cast<int16_t>(Ram16(Player(0) + kX)) - CameraX();
        y = 240 - (static_cast<int16_t>(Ram16(Player(0) + kY)) - CameraY());
    };
    d.shift = colours::EditorShift;
    d.linesInSet = kPalettesInSet;
    d.firstLine = [](Machine &) { return 0; };
    d.spareFrame = [](Machine &) {};
    model::Register(d);
}

} // namespace cps2recomp
