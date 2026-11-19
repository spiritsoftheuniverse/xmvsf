// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: the model viewer (the toolkit's model/model.h), a mode of the menu's "Alternative modes" page:
// one character alone, on a stage or on plain white or black, at any size, standing as it does in a fight (and moved
// with player 1's controls), or going through any one of its animations.
//
// It is a round of a two-player match, reached unseen as the stage viewer's is (fight/alternative.h), with the second
// player put out of the way: not followed by the camera, out of the picture, and never drawn.
//
// A character's animations are four lists, by the character's number and its ism (the table at DD412: a long a
// character, to three blocks of four longs, A-ism, X-ism, V-ism; 02FC4A puts a block's lists in the player's record at
// +2B0, +2B8, +2B4, +2BC as the character is set up). Each list is of how far on from the list an animation is (02A762).
// An animation is frames of 0x14 bytes: a byte of how many frames of the game it is shown for (FF: for good), a byte
// whose top bit says the next is a jump (to the frame the long after this one names) and whose bit 6 says it is the
// move's end, where its picture is, and the numbers of its boxes. Showing one is putting its frame in the player's
// record each frame, as the game's own 02A770 does, with the time left kept high so that the game's own stepping
// (02A7EA) never comes round; the game goes on believing the player stands.
#include "colours.h"
#include "model.h"
#include "alternative.h"
#include "sfa3.h"
#include "session.h"
#include "state.h"

#include <algorithm>
#include <iterator>

namespace cps2recomp {

namespace {
    using namespace sfa3;

    constexpr const char *kModel = "Model viewer";
    constexpr int kStartMost = 9000; // frames after which getting to a round is given up
    constexpr int kPalettesInSet = 5;
    constexpr int kPictureFloor = 239; // the picture's line that height 0 in the stage is on, with the camera at its bottom

    // The "Animation" choice: standing as in a fight, or one of the four lists (A-D: the record's +2B0, +2B4, +2B8 the
    // reelings, +2BC), of the A-ism's block (the table at DD412, whose block has them as +2B0, +2B8, +2B4, +2BC)
    constexpr uint32_t kAnimationBlocks = 0x0DD412;
    constexpr uint32_t kListInBlock[4] = {0x0, 0x8, 0x4, 0xC};
    constexpr int kMostAnimations = 256;
    constexpr uint32_t kFrameBytes = 0x14, kFramePicture = 0x04, kFrameJump = 0x14, kFrameFlags = 0x01;
    constexpr uint8_t kForGood = 0xFF;
    constexpr int kForGoodShown = 30; // frames a picture the game shows for good is shown before the animation starts again
    constexpr uint32_t kNotFollowed = 0x263; // a player's byte: not zero, the camera does not follow it (018700)

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
            Fast(false);
            model::End();
        }
    }

    uint32_t Read32(Machine &machine, uint32_t address) {
        return (static_cast<uint32_t>(machine.Read16(address)) << 16) | machine.Read16(address + 2);
    }
    uint8_t Read8(Machine &machine, uint32_t address) {
        return static_cast<uint8_t>(machine.Read16(address & ~1u) >> ((address & 1) ? 0 : 8));
    }
    bool InRom(uint32_t address) {
        return address >= 0x100000 && address < 0x400000 && !(address & 1);
    }
    // Is this an animation's frame? (Its picture is somewhere in the ROM, and it is shown for a while.)
    bool IsFrame(Machine &machine, uint32_t frame) {
        return InRom(frame) && Read8(machine, frame) != 0 && InRom(Read32(machine, frame + kFramePicture));
    }
    // Where the character's list is, and how many animations it has: up to the next of its lists or the first of its
    // animations' frames, whichever is nearer
    uint32_t List(Machine &machine, int character, int table, int &count) {
        const uint32_t block = Read32(machine, kAnimationBlocks + static_cast<uint32_t>(character) * 4);
        count = 0;
        if (!InRom(block)) {
            return 0;
        }
        const uint32_t list = Read32(machine, block + kListInBlock[table]);
        if (!InRom(list)) {
            return 0;
        }
        // (the lists are side by side, the frames after the last of them)
        uint32_t end = list + 2 * kMostAnimations;
        for (const uint32_t at : kListInBlock) {
            const uint32_t other = Read32(machine, block + at);
            if (other > list) {
                end = std::min(end, other);
            }
            if (InRom(other) && other + machine.Read16(other) > list) {
                end = std::min(end, other + machine.Read16(other));
            }
        }
        count = static_cast<int>((end - list) / 2);
        return list;
    }
    // Where an animation of the character's starts (0: there is none of that number)
    uint32_t AnimationAt(Machine &machine, int character, int table, int number) {
        int count = 0;
        const uint32_t list = List(machine, character, table, count);
        if (!list || number >= count) {
            return 0;
        }
        const uint32_t frame = list + machine.Read16(list + static_cast<uint32_t>(number) * 2);
        return IsFrame(machine, frame) ? frame : 0;
    }
    // Frames an animation's frame is shown for (the game's time for it, but not for good); and the frame after it
    // (0: the end, or the frame going on to itself, how the game holds an animation's last picture)
    int FrameTime(Machine &machine, uint32_t frame) {
        const uint8_t time = Read8(machine, frame);
        return time == kForGood ? kForGoodShown : time;
    }
    uint32_t NextFrame(Machine &machine, uint32_t frame) {
        const uint8_t flags = Read8(machine, frame + kFrameFlags);
        if (Read8(machine, frame) == kForGood || (!(flags & 0x80) && (flags & 0x40))) {
            return 0;
        }
        const uint32_t next = flags & 0x80 ? Read32(machine, frame + kFrameJump) : frame + kFrameBytes;
        return next == frame ? 0 : next;
    }
    void ShowFrame(Machine &machine, uint32_t frame) {
        if (!frame) {
            // (back to standing: the game's own "nothing" action from its start sets the animation again)
            SetRam16(Player(0) + kAction, 0);
            SetRam8(Player(0) + kFrameTime, 1);
            return;
        }
        SetRam32(Player(0) + kFrame, frame);
        SetRam32(Player(0) + kFrameTime, Read32(machine, frame));
        SetRam8(Player(0) + kFrameTime, 0x7F);
        SetRam8(Player(0) + kFrameTime + 1, 0);
    }

    // Objects by what they are: for the frame's own picture, the stage's alone (NotTheStage); for the character's,
    // player 1's own and what it throws (a player's objects are in its own colour sets, from the one its record's +0D
    // names: player 1's from 0, player 2's from 5, five each); and for the picture as the board would show it,
    // everything but the panel and the second player
    unsigned g_firstSet = 0, g_secondSet = 5;
    bool IsModel(uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return !(set < g_firstSet || set >= g_firstSet + kPalettesInSet || ThePanel(tile, attributes));
    }
    bool InAFight(uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return !(ThePanel(tile, attributes) || (set >= g_secondSet && set < g_secondSet + kPalettesInSet));
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
        modes::Wanted().characters[0] = kFighterNumbers[model::Character()];
        modes::Wanted().characters[1] = kFighterNumbers[(model::Character() + 1) % kFighters];
        machine.Reset();
    }

    // The second player out of the way: the camera does not follow it (the game's own flag), and it stands far off
    // to the right
    void PutAway() {
        SetRam8(Player(1) + kNotFollowed, 1);
        SetRam16(Player(1) + kX, static_cast<uint16_t>(Ram16(Player(0) + kX) + 0x300));
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
    SetRam8(kClock, kClockFull);
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
                       "animations over and over. They are named by the game's four lists of them and a number (A34); the ones a "
                       "character has not got are passed over. While one is showing, left and right go to the one before and the "
                       "one after, and its name is shown.";
    d.foregroundWords = "What the stage has in front of the fighters: in front of the character too, as in a fight, or behind it so "
                        "that all of it is seen. In front only at the character's own size, and not in the colour editor.";
    for (const uint8_t number : kFighterNumbers) {
        d.characters.push_back(kCharacterNames[number]);
        d.numbers.push_back(number);
    }
    d.stages = {std::begin(kStageNames), std::end(kStageNames)};
    d.start = [](Machine &machine) {
        colours::EndEditor();
        StartModel(machine);
    };
    d.lists = 4;
    d.perList = kMostAnimations;
    d.animationAt = AnimationAt;
    d.isFrame = IsFrame;
    d.frameTime = FrameTime;
    d.nextFrame = NextFrame;
    d.showFrame = ShowFrame;
    d.animationChoice = colours::EditorAnimation;
    d.isModel = IsModel;
    d.inAFight = InAFight;
    d.beforeCompose = [](Machine &) {
        g_firstSet = Ram8(Player(0) + kColourSet) & 0x1F;
        g_secondSet = Ram8(Player(1) + kColourSet) & 0x1F;
    };
    d.anchor = [](Machine &, int &x, int &y) {
        x = static_cast<int16_t>(Ram16(Player(0) + kX)) - static_cast<int16_t>(Ram16(kCameraX));
        y = kPictureFloor - (static_cast<int16_t>(Ram16(Player(0) + kY)) - static_cast<int16_t>(Ram16(kCameraY)));
    };
    d.shift = colours::EditorShift;
    d.linesInSet = kPalettesInSet;
    d.firstLine = [](Machine &) { return static_cast<int>(Ram8(Player(0) + kColourSet) & 0x1F); };
    d.spareFrame = [](Machine &) { SetRam8(kClock, kClockFull); };
    model::Register(d);
}

} // namespace cps2recomp
