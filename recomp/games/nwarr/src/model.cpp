// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Night Warriors: the model viewer (the toolkit's model/model.h), a mode of the menu's "Alternative modes" page: one
// character alone, on a stage or on plain white or black, at any size, standing as it does in a fight (and moved with
// player 1's controls), or going through any one of its animations. As Pocket Fighter's.
//
// It is a round of a two-player match, reached unseen as the stage viewer's is (fight/alternative.h), with the second
// player put out of the way: the camera follows player 1 alone (0FF6D6, where it takes the two players' places), the
// second stands far off and is never drawn.
//
// A character's animations are in lists (006B02: its table of them, a long a list, from the table at 006B70 by the
// character's number, kept at +1C8; 0051B8 starts one: the list's word of how far on the animation is, by its number).
// The lists follow their table, so it is as long as the gap to the first. An animation is frames of 0x20 bytes: a word
// of how many frames of the game it is shown for (the low 13 bits), whose top bit says what follows it (with the next
// bit too: nothing, it stays; else a jump, to the frame the long 0x20 on names; 005344), a word to +26, the long of its
// picture (to +2C), and the numbers of its boxes (+10 on). Showing one is putting its frame in the player's record each
// frame, as 005364 does, with the time left kept high so that the game's own stepping never comes round.
#include "colours.h"
#include "model.h"
#include "alternative.h"
#include "nwarr.h"
#include "session.h"
#include "state.h"

#include <iterator>

namespace cps2recomp {

namespace {
    using namespace nwarr;

    constexpr const char *kModel = "Model viewer";
    constexpr int kStartMost = 9000; // frames after which getting to a round is given up

    // The characters in the menu's order (the game's own, 1-14), by their numbers in the game
    constexpr int kCharacterNumbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    constexpr uint32_t kListTables = At(0x006B70); // a long a character, by its number
    constexpr int kMostLists = 80, kMostAnimations = 32;
    constexpr uint32_t kFrameBytes = 0x20, kFramePicture = 0x04, kFrameWord = 0x02;
    constexpr uint16_t kFollowed = 0x8000, kHolds = 0x4000;
    constexpr int kHeldShown = 30; // frames a picture the game holds is shown before the animation starts again
    constexpr uint32_t kTimer = 0x0E, kWord = 0x26, kPicture = 0x2C;

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

    // (the ROM as the 68000 reads its data)
    uint16_t Read16(Machine &machine, uint32_t address) {
        return static_cast<uint16_t>(machine.cpu.Read(address, 1));
    }
    uint32_t Read32(Machine &machine, uint32_t address) {
        return (static_cast<uint32_t>(Read16(machine, address)) << 16) | Read16(machine, address + 2);
    }
    // (the table of the characters' lists is read pc-relative, 006B02, which on the board sees the deciphered program)
    uint32_t TableOf(Machine &machine, int character) {
        const uint32_t at = kListTables + static_cast<uint32_t>(character) * 4;
        return (static_cast<uint32_t>(machine.Fetch16(at)) << 16) | machine.Fetch16(at + 2);
    }
    bool InRom(uint32_t address) {
        return address >= 0x100000 && address < 0x400000 && !(address & 1);
    }
    // Is this an animation's frame? (Its picture is somewhere in the ROM, and it is shown for a while.)
    bool IsFrame(Machine &machine, uint32_t frame) {
        return InRom(frame) && (Read16(machine, frame) & 0x1FFF) != 0 && InRom(Read32(machine, frame + kFramePicture));
    }
    // Where an animation of the character's starts (0: there is none of that number)
    uint32_t AnimationAt(Machine &machine, int character, int list, int number) {
        const uint32_t table = TableOf(machine, character);
        if (!InRom(table)) {
            return 0;
        }
        const uint32_t first = Read32(machine, table);
        if (!InRom(first) || first <= table || static_cast<uint32_t>(list) >= (first - table) / 4) {
            return 0;
        }
        const uint32_t at = Read32(machine, table + static_cast<uint32_t>(list) * 4);
        const uint32_t next = static_cast<uint32_t>(list) + 1 < (first - table) / 4 ? Read32(machine, table + static_cast<uint32_t>(list + 1) * 4) : 0;
        if (!InRom(at) || (next > at && static_cast<uint32_t>(number) >= (next - at) / 2)) {
            return 0;
        }
        const uint32_t frame = at + Read16(machine, at + static_cast<uint32_t>(number) * 2);
        return IsFrame(machine, frame) ? frame : 0;
    }
    int FrameTime(Machine &machine, uint32_t frame) {
        const uint16_t word = Read16(machine, frame);
        return (word & (kFollowed | kHolds)) == (kFollowed | kHolds) ? kHeldShown : word & 0x1FFF;
    }
    // The frame after (0: the end, or the frame going on to itself, how the game holds an animation's last picture)
    uint32_t NextFrame(Machine &machine, uint32_t frame) {
        const uint16_t word = Read16(machine, frame);
        if ((word & (kFollowed | kHolds)) == (kFollowed | kHolds)) {
            return 0;
        }
        const uint32_t next = word & kFollowed ? Read32(machine, frame + kFrameBytes) : frame + kFrameBytes;
        return next == frame || !IsFrame(machine, next) ? 0 : next;
    }
    void ShowFrame(Machine &machine, uint32_t frame) {
        if (!frame) {
            // (back to standing: the game's own standing state from its start sets the animation again)
            SetRam16(Player(0) + kState, 0);
            SetRam16(Player(0) + kTimer, 1);
            return;
        }
        SetRam32(Player(0) + kFrame, frame);
        SetRam16(Player(0) + kWord, Read16(machine, frame + kFrameWord));
        SetRam32(Player(0) + kPicture, Read32(machine, frame + kFramePicture));
        SetRam16(Player(0) + kTimer, 0x1FFF);
    }

    // Objects by what they are: player 1's own five sets; and for the picture as the board would show it, everything
    // but the panel and the second player
    unsigned g_firstSet = 0x09, g_secondSet = 0x0E;
    bool IsModel(uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return set >= g_firstSet && set < g_firstSet + 5 && !ThePanel(tile, attributes);
    }
    bool InAFight(uint32_t tile, uint16_t attributes) {
        const unsigned set = attributes & 0x1F;
        return !(ThePanel(tile, attributes) || (set >= g_secondSet && set < g_secondSet + 5));
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

    // 0FF6D6: the camera takes the two players' places across (D2, D3; A1, A2 their records) to follow them; in the
    // viewer it takes player 1's twice, and goes on from where it would have sorted them (0FF6F6)
    bool CameraPlayers(M68K &c) {
        if (!g.on || g.starting) {
            return false;
        }
        const uint16_t x = Ram16(Player(0) + kX);
        c.d[2] = (c.d[2] & 0xFFFF0000) | x;
        c.d[3] = (c.d[3] & 0xFFFF0000) | x;
        c.a[1] = c.a[2] = Player(0);
        c.pc = At(0x0FF6F6);
        return true;
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
    // (the second player far off to the right, never drawn)
    SetRam16(Player(1) + kX, static_cast<uint16_t>(Ram16(Player(0) + kX) + 0x300));
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
    if (!g.starting && Ram16(kMode) == 0) {
        End();
        return;
    }
    if (g.starting) {
        return;
    }
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
    if (!ar.Saving() && !RunningAhead()) {
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
                       "animations over and over. They are named by the game's list of them and a number (A3); the ones a "
                       "character has not got are passed over. While one is showing, left and right go to the one before and the "
                       "one after, and its name is shown.";
    d.foregroundWords = "What the stage has in front of the fighters: in front of the character too, as in a fight, or behind it so "
                        "that all of it is seen. In front only at the character's own size, and not in the colour editor.";
    d.characters = {std::begin(kCharacterNames) + 1, std::end(kCharacterNames)};
    d.numbers = {std::begin(kCharacterNumbers), std::end(kCharacterNumbers)};
    d.stages = {std::begin(kStageNames), std::end(kStageNames)};
    d.start = [](Machine &machine) {
        colours::EndEditor();
        StartModel(machine);
    };
    d.lists = kMostLists;
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
        y = 196 - (static_cast<int16_t>(Ram16(Player(0) + kY)) - static_cast<int16_t>(kGround) - static_cast<int16_t>(Ram16(kCameraY)));
    };
    d.shift = colours::EditorShift;
    d.linesInSet = 5;
    d.firstLine = [](Machine &) { return static_cast<int>(Ram8(Player(0) + kColourSet) & 0x1F); };
    d.spareFrame = [](Machine &) { SetRam8(kRoundClock, 0x99); };
    model::Register(d);
    AddHook(HookAt(At(0x0FF6D6)), "CameraPlayers", CameraPlayers);
}

} // namespace cps2recomp
