// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Hyper Street Fighter II: the model viewer (the toolkit's model/model.h), a mode of the menu's "Alternative
// modes" page: one character alone, on a stage or on plain white or black, at any size, standing as it does in a fight
// (and moved with player 1's controls), or going through any one of its animations.
//
// It is a round of a two-player match, reached unseen as the stage viewer's is (fight/alternative.h), with the second
// player put out of the way: the camera kept on the first, and the second never drawn.
//
// A character's animations: the game starts one through a table of the versions (0056EE, a word a version: Super
// Turbo, Super, World Warrior, Champion Edition, Hyper Fighting), each a table of the characters' lists of animations
// (a long a character; the Super Turbo version's seventeenth is Akuma's, its eighteenth the computer's Akuma), as
// 005462 reads them. A list is a word an animation (its first word saying how many), each naming a short list of the
// animation's variants by the way the player faces, and each of those its first frame, all read through address
// registers (the data view). Super Turbo kept each list 0x34 into a routine of the character's; the frames are its: 0x18
// bytes, a word of how many frames of the game it is shown for, a word whose top bit says the next is a jump (to the
// frame the long after this one names), where its picture is, and the numbers of its boxes. Showing one is putting its
// frame in the player's record each frame, as the game's own 0032B0 does, with the time left kept high so that the
// game's own stepping (0032C0) never comes round; the game goes on believing the player stands. The viewer shows the
// Super Turbo version of each character (the modes' fights are of it: modes.cpp).
#include "colours.h"
#include "model.h"
#include "alternative.h"
#include "hsf2.h"
#include "session.h"
#include "state.h"

#include <algorithm>
#include <iterator>

namespace cps2recomp {

namespace {
    using namespace hsf2;

    constexpr int kPalettesInSet = 5; // (colours.cpp)
    constexpr int kPictureFloor = 239; // the picture's line that height 0 in the stage is on, with the camera at its bottom

    constexpr const char *kModel = "Model viewer";
    constexpr int kStartMost = 9000; // frames after which getting to a round is given up

    // The characters in the menu's order, by their numbers in the game
    constexpr int kCharacterNumbers[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    constexpr uint32_t kVersionTables = 0x0056EE; // a word a version (by kVersion): where its table of lists is
    constexpr int kMostAnimations = 64;
    constexpr uint32_t kFrameBytes = 0x18, kFramePicture = 0x04, kFrameJump = 0x18, kFrameFollows = 0x02;
    constexpr uint16_t kForGood = 0x0FFF;
    constexpr int kForGoodShown = 30; // frames a picture the game shows for good is shown before the animation starts again
    constexpr uint32_t kTimer = 0x18, kFrameWord = 0x14; // a player's words: the frame's time left, and its second word

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
    bool InRom(uint32_t address) {
        return address >= 0x000400 && address < 0x400000 && !(address & 1);
    }
    // Is this an animation's frame? (Its picture is somewhere in the ROM, and it is shown for a while.)
    bool IsFrame(Machine &machine, uint32_t frame) {
        return InRom(frame) && machine.Read16(frame) != 0 && InRom(Read32(machine, frame + kFramePicture));
    }
    // Where an animation of the character's starts (0: there is none of that number)
    uint32_t AnimationAt(Machine &machine, int character, int, int number) {
        const uint32_t table = kVersionTables + machine.Read16(kVersionTables + kSuperTurbo);
        const uint32_t list = Read32(machine, table + static_cast<uint32_t>(character) * 4);
        if (!InRom(list)) {
            return 0;
        }
        if (number >= machine.Read16(list) / 2) {
            return 0;
        }
        const uint32_t variants = list + machine.Read16(list + static_cast<uint32_t>(number) * 2);
        const uint32_t frame = variants + machine.Read16(variants);
        return IsFrame(machine, frame) ? frame : 0;
    }
    // Frames an animation's frame is shown for (the game's time for it, but not for good); and the frame after it
    // (0: the end, or the frame going on to itself, how the game holds an animation's last picture)
    int FrameTime(Machine &machine, uint32_t frame) {
        const uint16_t time = machine.Read16(frame);
        return time == kForGood ? kForGoodShown : time;
    }
    uint32_t NextFrame(Machine &machine, uint32_t frame) {
        if (machine.Read16(frame) == kForGood) {
            return 0;
        }
        const uint32_t next = machine.Read16(frame + kFrameFollows) & 0x8000 ? Read32(machine, frame + kFrameJump) : frame + kFrameBytes;
        return next == frame ? 0 : next;
    }
    void ShowFrame(Machine &machine, uint32_t frame) {
        if (!frame) {
            // (back to standing: the game's own standing from its start sets the animation again)
            SetRam8(Player(0) + kState, kStand);
            SetRam8(Player(0) + kStateStep, 0);
            SetRam16(Player(0) + kTimer, 1);
            return;
        }
        SetRam32(Player(0) + kFrame, frame);
        SetRam16(Player(0) + kTimer, 0x7FFF);
        SetRam16(Player(0) + kFrameWord, machine.Read16(frame + kFrameFollows));
    }

    // Objects by what they are: for the character's picture, player 1's own and what it throws; and for the picture as
    // the board would show it, everything but the panel and the second player
    bool IsModel(uint32_t, uint16_t attributes) {
        return ((kPlayerSets[0] >> (attributes & 0x1F)) & 1) != 0;
    }
    bool InAFight(uint32_t tile, uint16_t attributes) {
        return !(ThePanel(tile, attributes) || ((kPlayerSets[1] >> (attributes & 0x1F)) & 1));
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
        modes::Wanted().characters[0] = kCharacterNumbers[model::Character()];
        modes::Wanted().characters[1] = kCharacterNumbers[(model::Character() + 1) % std::size(kCharacterNumbers)];
        machine.Reset();
    }

    // The second player out of the way, far off to the right (it is never drawn), and the camera on the first
    void PutAway() {
        SetRam16(Player(1) + kX, static_cast<uint16_t>(Ram16(Player(0) + kX) + 0x300));
        modes::CameraAt().x = std::clamp(static_cast<int16_t>(Ram16(Player(0) + kX)) - 0xC0, kCameraLeft, kCameraRight);
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
            // (the character in the middle of the stage)
            SetRam16(Player(0) + kX, static_cast<uint16_t>((kStageLeft + kStageRight) / 2));
        } else if (g.frames > kStartMost) {
            End();
        }
        return;
    }
    machine.players[1] = 0;
    PutAway();
    if (colours::EditorAnimation()) {
        model::Scan(machine);
    }
    colours::EditorBeforeFrame(machine); // (takes player 1's controls, when it is on)
    SetRam8(kRoundClock, 0x99);
    model::BeforeFrame(machine);
}

void ModelFrame(Machine &machine) {
    if (!g.on) {
        return;
    }
    // (the board reset, by the menu or by another mode starting: the viewer is over)
    if (!g.starting && !InGame()) {
        End();
        return;
    }
    if (g.starting) {
        return;
    }
    // Another character or background chosen in the menu: the viewer starts again with it
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
    d.pageWords = "One character by itself, on a stage or on white or black, at any size, standing or going through any of its "
                  "animations.";
    d.resetWords = "Puts every choice of this page back: the first character and stage, its own size, standing, full speed, "
                   "repeating at once, one animation only. (While viewing, the viewer goes to that character and stage.)";
    d.animationWords = "What the character does: stands as in a fight, moved with player 1's controls, or goes through one of its "
                       "animations over and over. They are named by the game's list of them and a number (A12); the ones a "
                       "character has not got are passed over. While one is showing, left and right go to the one before and the "
                       "one after, and its name is shown.";
    d.foregroundWords = "What the stage has in front of the fighters: in front of the character too, as in a fight, or behind it so "
                        "that all of it is seen. In front only at the character's own size, and not in the colour editor.";
    d.characters = {std::begin(kCharacterNames), std::end(kCharacterNames)};
    d.numbers = {std::begin(kCharacterNumbers), std::end(kCharacterNumbers)};
    d.stages = {std::begin(kStageNames), std::end(kStageNames)};
    d.start = [](Machine &machine) {
        colours::EndEditor();
        StartModel(machine);
    };
    d.lists = 1;
    d.perList = kMostAnimations;
    d.animationAt = AnimationAt;
    d.isFrame = IsFrame;
    d.frameTime = FrameTime;
    d.nextFrame = NextFrame;
    d.showFrame = ShowFrame;
    d.animationChoice = colours::EditorAnimation;
    d.isModel = IsModel;
    d.inAFight = InAFight;
    d.anchor = [](Machine &, int &x, int &y) {
        x = static_cast<int16_t>(Ram16(Player(0) + kX)) - static_cast<int16_t>(Ram16(kCameraX));
        y = kPictureFloor - (static_cast<int16_t>(Ram16(Player(0) + kY)) - static_cast<int16_t>(Ram16(kCameraY)));
    };
    d.shift = colours::EditorShift;
    d.linesInSet = kPalettesInSet;
    d.firstLine = [](Machine &) { return 1; };
    d.spareFrame = [](Machine &) { SetRam8(kRoundClock, 0x99); };
    model::Register(d);
}

} // namespace cps2recomp
