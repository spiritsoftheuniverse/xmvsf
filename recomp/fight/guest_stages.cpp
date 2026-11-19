// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// Other games' stages behind a fighting game's fighters (guest_stages.h).
#include "guest_stages.h"

#include "../machine/machine.h"
#include "../runtime/game.h"
#include "../runtime/guest.h"
#include "../runtime/lift.h"
#include "../runtime/state.h"
#include "alternative.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <string>
#include <mutex>
#include <thread>
#include <vector>

namespace cps2recomp::guest_stages {

namespace {
    Host h;
    std::vector<Guest> &Guests() {
        static auto &guests = *new std::vector<Guest>; // (never freed: the threads read them)
        return guests;
    }
    constexpr int kMostGuests = 16; // (Definitive shows nine games' stages)

    struct State {
        int8_t guest = 0;  // the guest whose stage is shown, 1 the first (0: the game's own); its board is in its round
        int8_t stage = 0;  // ... its stage, 1 its first
        int8_t follow = 0; // its camera goes where the game's is (while its stage is shown)
        int8_t built = 0;  // (a stage alone, Guest::StageAlone: its stage has been built on its board; 2: its round to start)
        int16_t cameraX = 0, cameraY = 0; // where its camera is to be (the game's, as its last frame left it)
        int16_t hostX = 0;                // (the game's camera's place across, between its ends)
    } g;
    std::vector<uint8_t> g_guestState; // (the guest's machine, for a save state: kept so a peer without the set can read past it)
    size_t g_soundBefore = 0;

    uint8_t G8(Machine &m, uint32_t address) {
        return m.ram[address & 0xFFFF];
    }
    uint16_t G16(Machine &m, uint32_t address) {
        const uint8_t *p = &m.ram[address & 0xFFFF];
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }
    void SetG8(Machine &m, uint32_t address, uint8_t value) {
        m.ram[address & 0xFFFF] = value;
    }
    uint16_t C16(M68K &c, uint32_t address) {
        const uint8_t *p = &c.ram[address & 0xFFFF];
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }
    uint16_t R16(const uint8_t *ram, uint32_t address) {
        const uint8_t *p = &ram[address & 0xFFFF];
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }
    // Where a guest's camera's place is kept, across and up (0: not known)
    void CameraAt(const Guest &guest, const uint8_t *ram, uint32_t &x, uint32_t &y) {
        if (guest.cameraRecord) {
            const uint32_t record = guest.cameraRecord(ram);
            x = record + guest.placeX;
            y = record + guest.placeY;
            return;
        }
        x = guest.cameraAtX;
        y = guest.cameraAtY;
    }
    // A camera's place between one pair of ends put between another: as far from the middle as it is (a stage wider than
    // the other goes no faster than it; its ends beyond the other's are not reached), or, between ends closer together,
    // as far between them as it is between its own
    int Across(int x, int fromLeft, int fromRight, int toLeft, int toRight) {
        const int from = std::max(1, fromRight - fromLeft), to = toRight - toLeft;
        const int off = x - (fromLeft + fromRight) / 2;
        const int at = (toLeft + toRight) / 2 + (to >= from ? off : off * to / from);
        return std::clamp(at, std::min(toLeft, toRight), std::max(toLeft, toRight));
    }
    // Where its camera is wanted (between its ends): the game's camera's place, put between the guest's ends; a Place
    // camera's by its record's own ends
    void WantedAt(const Guest &guest, const uint8_t *ram, int &x, int &y) {
        x = std::clamp<int>(g.cameraX, guest.cameraLeft, guest.cameraRight);
        y = g.cameraY;
        if (guest.camera != Guest::Camera::Place || !guest.endLeft) {
            return;
        }
        const uint32_t record = guest.cameraRecord ? guest.cameraRecord(ram) : guest.cameraAtX - guest.placeX;
        const int left = static_cast<int16_t>(R16(ram, record + guest.endLeft));
        const int right = static_cast<int16_t>(R16(ram, record + guest.endRight));
        const int top = static_cast<int16_t>(R16(ram, record + guest.endTop));
        const int bottom = static_cast<int16_t>(R16(ram, record + guest.endBottom));
        x = Across(g.hostX, h.cameraLeft, h.cameraRight, left, right);
        y = guest.placeDown ? std::clamp<int>(bottom - g.cameraY, std::min(top, bottom), bottom) : std::clamp<int>(g.cameraY, top, bottom);
    }
    void SetC16(M68K &c, uint32_t address, uint16_t value) {
        uint8_t *p = &c.ram[address & 0xFFFF];
        p[0] = static_cast<uint8_t>(value >> 8);
        p[1] = static_cast<uint8_t>(value);
    }
    void SetC8(M68K &c, uint32_t address, uint8_t value) {
        c.ram[address & 0xFFFF] = value;
    }
    void ReturnGuest(M68K &c) {
        const uint8_t *p = &c.ram[c.a[7] & 0xFFFF];
        c.pc = static_cast<uint32_t>((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]) & 0xFFFFFF;
        c.a[7] += 4;
    }
    bool All(Machine &m, const std::vector<Is> &all) {
        for (const Is &is : all) {
            if ((is.byte ? G8(m, is.address) : G16(m, is.address)) != is.value) {
                return false;
            }
        }
        return true;
    }

    // --- each stage's round, made ready on a thread of its own (guest_stages.h) ---

    struct Ready {
        std::mutex lock;
        std::condition_variable done;
        std::vector<std::vector<uint8_t>> round; // (each written once, by the thread; read once `made` says so)
        std::unique_ptr<std::atomic<bool>[]> made;
        std::atomic<int> wanted{0};
        bool started = false;
        Machine *ahead = nullptr;       // (the thread's board)
        std::atomic<int> aheadStage{0}; // (the stage it is taking to its round, 1 the first: 0 none yet)
    };
    Ready &TheReady(int guest) { // (0 the first)
        static Ready *ready = new Ready[kMostGuests]; // (never freed: the threads may run until the program is gone)
        return ready[guest];
    }

    // The guest whose board this 68000 is (0 the first; -1 none), and whether it is the thread's
    int GuestOf(M68K &c, bool &ahead) {
        for (int n = 0; n < static_cast<int>(Guests().size()); n++) {
            if (TheReady(n).ahead && &c == &TheReady(n).ahead->cpu) {
                ahead = true;
                return n;
            }
            if (Machine *m = cps2recomp::Guest(Guests()[n].id); m && &c == &m->cpu) {
                ahead = false;
                return n;
            }
        }
        return -1;
    }

    // The guest's controls on its way to a round: credits kept, each player's Start until both are in, then a punch
    // now and then (the character choice, anything else that waits for a button)
    void Controls(const Guest &guest, Machine &m, uint8_t &pressing) {
        memset(m.players, 0, sizeof(m.players));
        SetG8(m, guest.credits, 2);
        const bool down = (pressing & 7) < 3;
        pressing++;
        if (!down || All(m, guest.match)) {
            return;
        }
        if (G8(m, guest.playersIn) != 3) {
            m.players[(pressing >> 3) & 1] = kStart;
            return;
        }
        m.players[(pressing >> 3) & 1] = kPunch1;
    }
    std::vector<uint8_t> Saved(Machine &m) {
        std::vector<uint8_t> data;
        Archive ar(data, true);
        m.Serialize(ar);
        return data;
    }
    void Load(Machine &m, std::vector<uint8_t> data) {
        Archive ar(data, false);
        m.Serialize(ar);
    }

    void MakeRounds(int index) {
        const Guest &guest = Guests()[index];
        Ready &ready = TheReady(index);
        Machine &m = *ready.ahead;
        const int stages = static_cast<int>(guest.stageNames.size());
        m.drawing = false;
        // To the character choice, kept
        uint8_t pressing = 0;
        constexpr int kMostFrames = 20000; // (never more than about 1,000: a board that does not get there is given up)
        m.Reset();
        for (int n = 0; n < kMostFrames && !(All(m, guest.choosing) && G8(m, guest.playersIn) == 3); n++) {
            Controls(guest, m, pressing);
            m.RunFrame();
            m.audio.clear();
        }
        const std::vector<uint8_t> choosing = Saved(m);
        const uint8_t choosingPressing = pressing;
        for (int left = stages; left > 0; left--) {
            // (the stage wanted first, else the next not made)
            int stage = ready.wanted.load();
            if (stage < 1 || stage > stages || ready.made[stage - 1]) {
                stage = 0;
                for (int s = 1; s <= stages && !stage; s++) {
                    stage = ready.made[s - 1] ? 0 : s;
                }
            }
            if (!stage) {
                break;
            }
            Load(m, choosing);
            pressing = choosingPressing;
            ready.aheadStage = stage;
            for (int n = 0; n < kMostFrames && !All(m, guest.fought); n++) {
                Controls(guest, m, pressing);
                m.RunFrame();
                m.audio.clear();
            }
            memset(m.players, 0, sizeof(m.players));
            {
                std::lock_guard<std::mutex> hold(ready.lock);
                ready.round[stage - 1] = Saved(m);
                ready.made[stage - 1] = true;
            }
            ready.done.notify_all();
        }
    }
    void StartMaking(int index) {
        Ready &ready = TheReady(index);
        if (ready.started || Guests()[index].StageAlone() || !cps2recomp::Guest(Guests()[index].id)) {
            return;
        }
        ready.started = true;
        ready.ahead = GuestSpare(Guests()[index].id);
        std::thread(MakeRounds, index).detach();
    }
    // A guest's stage's round (1 its first), into its board (waiting for it, if it is not made yet)
    void ToStage(int index, int stage);
    void ToRound(int index, int stage) {
        if (Guests()[index].StageAlone()) {
            ToStage(index, stage);
            return;
        }
        Ready &ready = TheReady(index);
        StartMaking(index);
        ready.wanted = stage;
        std::unique_lock<std::mutex> hold(ready.lock);
        ready.done.wait(hold, [&] { return ready.made[stage - 1].load(); });
        Load(*cps2recomp::Guest(Guests()[index].id), ready.round[stage - 1]);
    }

    // --- a stage alone (Guest::StageAlone): the guest's board at the top of its main loop, its stage's code run there ---

    // The board as its own start-up leaves it at its main loop's top, made once (nothing played: no coin, no control)
    std::vector<std::vector<uint8_t>> &Booted() {
        static auto &booted = *new std::vector<std::vector<uint8_t>>(kMostGuests);
        return booted;
    }
    int g_booting = -1;     // (the guest whose board is starting up now: at its main loop it waits, and is there)
    bool g_reached = false;
    const std::vector<uint8_t> &BootedState(int index) {
        std::vector<uint8_t> &booted = Booted()[index];
        if (booted.empty()) {
            Machine &m = *cps2recomp::Guest(Guests()[index].id);
            const bool drawing = m.drawing;
            m.drawing = false;
            m.Reset();
            g_booting = index;
            g_reached = false;
            constexpr int kMostFrames = 3000; // (a board's start-up takes a few hundred at most)
            for (int n = 0; n < kMostFrames && !g_reached; n++) {
                memset(m.players, 0, sizeof(m.players));
                const Guest &guest = Guests()[index];
                if (guest.beginAfter >= 0 && n >= guest.beginAfter) {
                    SetG8(m, guest.credits, 2);
                    m.players[0] = (n / 8) % 2 ? kStart : 0;
                }
                m.RunFrame();
                m.audio.clear();
            }
            g_booting = -1;
            m.drawing = drawing;
            booted = Saved(m);
        }
        return booted;
    }
    // The guest's stage (1 its first) to be built on its board, from the board's start (at its next frame: the main
    // loop's hook builds it, State::built)
    void ToStage(int index, int stage) {
        Load(*cps2recomp::Guest(Guests()[index].id), BootedState(index));
        g.built = 0;
    }
    // The object list's entries put in after the fighters' would be (Guest::fightersDrawn, where the routine making it had
    // got to then): marked in front of them, bit 12 of the entry's first word (Video::markFront), to the list's end
    uint32_t g_fightersDrawn = 0;
    bool FightersDrawn(M68K &c) {
        bool ahead = false;
        const int index = GuestOf(c, ahead);
        if (index >= 0) {
            g_fightersDrawn = c.a[Guests()[index].listRegister & 7] & 0xFFFFFF;
        }
        return false;
    }
    void MarkFront(M68K &c) {
        if (!g_fightersDrawn) {
            return;
        }
        const uint32_t end = (g_fightersDrawn & ~0x1FFFu) + 0x2000; // (a bank of the object memory: 1,024 entries)
        for (uint32_t at = g_fightersDrawn; at < end; at += 8) {
            if (c.bus.Read16(at + 2) >= 0x8000 || c.bus.Read16(at + 6) >= 0xFF00) {
                break;
            }
            c.bus.Write16(at, static_cast<uint16_t>(c.bus.Read16(at) | 0x1000));
        }
        g_fightersDrawn = 0;
    }

    // The main loop's top: the stage's frame in place of the game's modes (its build first, once), then on to the loop's
    // wait for the frame
    bool StageAlone(M68K &c) {
        bool ahead = false;
        const int index = GuestOf(c, ahead);
        if (index < 0) {
            return false;
        }
        const Guest &guest = Guests()[index];
        // (what is called here is the port's doing, not the program's own loop: it takes none of the board's frame, or a
        // heavy frame, a stage's build, would leave the program's loop short of time, its next frame missed)
        const uint64_t cycles = c.cycles;
        if (g_booting == index) {
            g_reached = true;
        } else if (g.guest == index + 1 && g.stage > 0) {
            c.a[5] = guest.a5;
            const bool justBuilt = !g.built;
            if (!g.built) {
                g.built = 1;
                // (the stage put in place before each routine: the first may clear the words it is kept in, as Vampire
                // Savior's does, 016C4E)
                int gap = 0;
                for (const uint32_t at : guest.build) {
                    guest.setStage(c, g.stage - 1);
                    if (!at) {
                        if (guest.buildStep) {
                            guest.buildStep(c, gap);
                        }
                        gap++;
                        continue;
                    }
                    c.d[0] = 0;
                    Call(c, at);
                }
                if (guest.afterBuild) {
                    guest.afterBuild(c);
                }
                if (guest.soundQueue && g.stage <= static_cast<int>(guest.stageTunes.size())) {
                    const uint16_t at = C16(c, guest.soundQueueEnd) & guest.soundQueueMask;
                    const uint32_t entry = guest.soundQueue + at;
                    SetC16(c, entry, 0);
                    SetC16(c, entry + 2, guest.stageTunes[g.stage - 1]);
                    for (uint32_t n = 4; n < 0x10; n += 2) {
                        SetC16(c, entry + n, 0);
                    }
                    SetC16(c, guest.soundQueueEnd, static_cast<uint16_t>((at + 0x10) & guest.soundQueueMask));
                }
            }
            bool placed = false;
            if (g.built == 2) {
                g.built = 1;
                if (guest.roundStart) {
                    guest.roundStart(c);
                }
                placed = true;
            }
            g_fightersDrawn = 0;
            if (guest.frameCounter) {
                SetC8(c, guest.frameCounter, static_cast<uint8_t>(c.ram[guest.frameCounter & 0xFFFF] + 1));
            }
            for (const uint32_t at : guest.frame) {
                Call(c, at);
            }
            MarkFront(c);
            // (a stage just made, or its round just started: its camera brought to the place wanted at once)
            uint32_t atX = 0, atY = 0;
            CameraAt(guest, c.ram, atX, atY);
            if ((placed || justBuilt) && guest.placing && atX) {
                constexpr int kMostCalls = 128;
                for (int n = 0; n < kMostCalls; n++) {
                    int wantedX, wantedY;
                    WantedAt(guest, c.ram, wantedX, wantedY);
                    const int x = static_cast<int16_t>(C16(c, atX));
                    const int y = atY ? static_cast<int16_t>(C16(c, atY)) : wantedY;
                    if (std::abs(x - wantedX) <= 1 && std::abs(y - wantedY) <= 1) {
                        break;
                    }
                    Call(c, guest.placing);
                }
            }
        }
        c.cycles = cycles;
        c.pc = guest.loopGoesOn;
        return true;
    }

    // --- the guests' hooks ---

    // Where a guest sets a match's stage: the one its spare board is taking to its round
    bool StageSet(M68K &c) {
        bool ahead = false;
        const int index = GuestOf(c, ahead);
        const int stage = index >= 0 && ahead ? TheReady(index).aheadStage.load() : 0;
        if (stage > 0) {
            Guests()[index].setStage(c, stage - 1);
        }
        return false;
    }
    // The guest's camera, while its stage is shown, goes where the game's is, as far as it goes in a frame: the game
    // paints the stage's new columns and rows as its camera comes to them
    bool Following(M68K &c, int &index) {
        bool ahead = false;
        index = GuestOf(c, ahead);
        return index >= 0 && !ahead && g.follow && g.guest == index + 1;
    }
    // Steps: the camera (A6) steps across, and up or down, by no more than kMostStep (it catches up a camera put
    // somewhere else at once in a few frames)
    constexpr int kMostStep = 8;
    int16_t StepTo(int16_t was, int16_t to) {
        return static_cast<int16_t>(std::clamp(to - was, -kMostStep, kMostStep));
    }
    bool StepAcross(M68K &c) {
        int index;
        if (!Following(c, index)) {
            return false;
        }
        const Guest &guest = Guests()[index];
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        const uint16_t was = C16(c, camera + guest.placeX);
        const int16_t step = StepTo(static_cast<int16_t>(was), g.cameraX);
        SetC16(c, camera + guest.placeX, static_cast<uint16_t>(was + step));
        SetC16(c, camera + guest.stepX, static_cast<uint16_t>(step));
        SetC8(c, camera + guest.wayX, step >= 0 ? guest.wayRight : guest.wayLeft);
        ReturnGuest(c);
        return true;
    }
    bool StepUp(M68K &c) {
        int index;
        if (!Following(c, index)) {
            return false;
        }
        const Guest &guest = Guests()[index];
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        const uint16_t was = C16(c, camera + guest.placeY);
        const int16_t step = StepTo(static_cast<int16_t>(was), g.cameraY);
        SetC16(c, camera + guest.placeY, static_cast<uint16_t>(was + step));
        SetC16(c, camera + guest.stepY, static_cast<uint16_t>(step));
        if (step) {
            SetC8(c, camera + guest.wayY, step > 0 ? guest.wayUp : guest.wayDown);
        }
        ReturnGuest(c);
        return true;
    }
    // Place: the camera's record (A6) given the place wanted, as far as it goes in a frame (kMostPlace: as far as the
    // game's camera goes), and its routine returned from
    constexpr int kMostPlace = 16;
    bool PlaceAcross(M68K &c) {
        int index;
        if (!Following(c, index)) {
            return false;
        }
        const Guest &guest = Guests()[index];
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        int x, y;
        WantedAt(guest, c.ram, x, y);
        const int was = static_cast<int16_t>(C16(c, camera + guest.placeX));
        SetC16(c, camera + guest.placeX, static_cast<uint16_t>(was + std::clamp(x - was, -kMostPlace, kMostPlace)));
        ReturnGuest(c);
        return true;
    }
    bool PlaceUp(M68K &c) {
        int index;
        if (!Following(c, index)) {
            return false;
        }
        const Guest &guest = Guests()[index];
        const uint32_t camera = c.a[6] & 0xFFFFFF;
        int x, y;
        WantedAt(guest, c.ram, x, y);
        const int was = static_cast<int16_t>(C16(c, camera + guest.placeY));
        SetC16(c, camera + guest.placeY, static_cast<uint16_t>(was + std::clamp(y - was, -kMostPlace, kMostPlace)));
        ReturnGuest(c);
        return true;
    }
    // Extents: the players' outer edges across (D0 left, D1 right), as far apart as the camera then puts their middle
    // in the picture's: at the camera wanted; their heights (D0, D1), as high as the camera then follows them to the
    // place wanted
    bool ExtentsAcross(M68K &c) {
        int index;
        if (!Following(c, index)) {
            return false;
        }
        const Guest &guest = Guests()[index];
        const int middle = g.cameraX + guest.middle;
        c.d[0] = (c.d[0] & 0xFFFF0000) | static_cast<uint16_t>(middle - guest.halfApart);
        c.d[1] = (c.d[1] & 0xFFFF0000) | static_cast<uint16_t>(middle + guest.halfApart);
        ReturnGuest(c);
        return true;
    }
    bool ExtentsUp(M68K &c) {
        int index;
        if (!Following(c, index)) {
            return false;
        }
        const Guest &guest = Guests()[index];
        const uint16_t high = static_cast<uint16_t>(g.cameraY + guest.above);
        c.d[0] = (c.d[0] & 0xFFFF0000) | high;
        c.d[1] = (c.d[1] & 0xFFFF0000) | high;
        ReturnGuest(c);
        return true;
    }

    // --- the stage wanted ---

    // The guest (0 the first) and its stage (1 its first) of the stages' count from 1 (false: none)
    bool Which(int stage, int &index, int &own) {
        for (index = 0; index < static_cast<int>(Guests().size()); index++) {
            const int stages = static_cast<int>(Guests()[index].stageNames.size());
            if (stage >= 1 && stage <= stages) {
                own = stage;
                return true;
            }
            stage -= stages;
        }
        return false;
    }

    void (*g_before)(Machine &) = nullptr;
    void (*g_frame)(Machine &) = nullptr;
    void (*g_state)(Archive &) = nullptr;

    bool Shown() {
        return g.guest > 0 && h.shown && h.shown();
    }

    bool g_drawnOver = false; // (the game's picture was drawn to go over a guest's last frame)

    // The guest's stage as this frame has it: drawn here, from its board's records (its board draws no picture of its
    // own: Machine::ownPicture), by the same renderer as the game's; or its board's own picture, when it has no records to
    // draw it from (a frame before its game's first list; --video board)
    std::vector<uint32_t> g_guestPicture;
    bool g_guestDrawn = false; // (g_guestPicture is this frame's)
    const uint32_t *GuestPicture(const Machine &board) {
        if (board.ownPicture) {
            return board.Picture();
        }
        if (!g_guestDrawn) {
            NativeVideo::FrameData records;
            if (!board.nativeVideo.LastFrame(records)) {
                return board.Picture();
            }
            g_guestPicture.resize(static_cast<size_t>(board.PictureWidth()) * Video::kHeight);
            NativeVideo::DrawFrame(records, g_guestPicture.data(), board.PictureWidth());
            g_guestDrawn = true;
        }
        return g_guestPicture.data();
    }
    void BeforeFrame(Machine &machine) {
        // (the game's picture as it was, before the game's own sources set theirs: what was changed here for a guest's
        // stage put back, so that a stage of its own shows again once no other game's is)
        if (g_drawnOver) {
            g_drawnOver = false;
            machine.video.layerMask = 0xF;
            machine.video.hideObject = nullptr;
            machine.video.markObjects = false;
            machine.video.markLayers = 0;
            machine.video.panelObject = nullptr;
        }
        if (g_before) {
            g_before(machine);
        }
        g_soundBefore = machine.audio.size();
        // The game's stage music is the guest's while a guest's stage is chosen (from before the round, so that the
        // game's never starts)
        std::vector<bool> taken;
        if (g.guest > 0) {
            for (const uint16_t tune : h.stageTunes) {
                if (taken.size() <= tune) {
                    taken.resize(tune + 1u);
                }
                taken[tune] = true;
            }
        }
        machine.soundtrack.Take(std::move(taken));
        if (!Shown()) {
            return;
        }
        g_drawnOver = true;
        // The game draws its fighters and its panel alone, marked, to be laid over the guest's stage (the stage
        // viewer: nothing of its own)
        if (h.viewer && h.viewer()) {
            machine.video.layerMask = 0;
        } else {
            machine.video.layerMask = 1 | h.panelLayers;
            machine.video.hideObject = [](uint32_t tile, uint16_t attributes) { return !h.notTheStage(tile, attributes); };
            machine.video.markObjects = true;
            machine.video.markLayers = h.panelLayers;
            machine.video.panelObject = h.panel;
        }
    }

    void CheckFrame(const Guest &guest, Machine &board, bool shown, int shift);
    // How far short its camera is of the place wanted up or down (up: its picture to be moved down; 0 where its camera's
    // height is not known): the game's camera's height, not put between the guest's ends, against the guest camera's.
    // No more than kMostShift (a fighter's jump above a stage that does not rise; not a game's camera coming down from
    // the sky at a round's start, its top line repeated down half the picture)
    constexpr int kMostShift = 48;
    int Shift(const Guest &guest, const uint8_t *ram) {
        uint32_t atX = 0, atY = 0;
        CameraAt(guest, ram, atX, atY);
        if (!atY) {
            return 0;
        }
        const int y = static_cast<int16_t>(R16(ram, atY));
        int shift = g.cameraY - y;
        if (guest.placeDown && guest.camera == Guest::Camera::Place && guest.endLeft) {
            const uint32_t record = guest.cameraRecord ? guest.cameraRecord(ram) : guest.cameraAtX - guest.placeX;
            const int bottom = static_cast<int16_t>(R16(ram, record + guest.endBottom));
            shift = g.cameraY - (bottom - y);
        }
        return std::clamp(shift, -kMostShift, kMostShift);
    }
    // The guest's frame, after the game's: its picture under the game's marked objects, its sound with the game's
    void Frame(Machine &machine) {
        machine.guestBehind = false;
        if (g_frame) {
            g_frame(machine);
        }
        // (with the feature on, the stages are made ready from the start)
        if (h.feature && FeatureOn(h.feature)) {
            for (int n = 0; n < static_cast<int>(Guests().size()); n++) {
                StartMaking(n);
            }
        }
        int index = -1, stage = 0;
        if (!Which(h.wanted ? h.wanted() : 0, index, stage) || !cps2recomp::Guest(Guests()[index].id)) {
            index = -1;
            stage = 0;
        }
        if (index + 1 != g.guest || stage != g.stage) {
            g = State();
            g.guest = static_cast<int8_t>(index + 1);
            g.stage = static_cast<int8_t>(stage);
            if (index >= 0) {
                ToRound(index, stage);
            }
        }
        if (g.guest == 0) {
            return;
        }
        const Guest &guest = Guests()[g.guest - 1];
        Machine &board = *cps2recomp::Guest(guest.id);
        // Its camera where the game's is (as far between its ends as the game's is between its own), while its stage
        // is shown; its clock held
        const bool shown = Shown();
        // (its stage just come into the picture, built before it was: its camera brought there at once, as after a jump)
        if (shown && !g.follow && guest.StageAlone() && g.built == 1 && guest.placing) {
            g.built = 2;
        }
        g.follow = shown;
        int hostX = static_cast<int16_t>(Ram16(h.cameraX)), hostUp = static_cast<int16_t>(Ram16(h.cameraY));
        if (h.camera) {
            h.camera(hostX, h.cameraLeft, h.cameraRight, hostUp);
        }
        const int x = std::clamp<int>(hostX, h.cameraLeft, h.cameraRight);
        const int16_t wasX = g.cameraX;
        g.hostX = static_cast<int16_t>(x);
        g.cameraX = static_cast<int16_t>(Across(x, h.cameraLeft, h.cameraRight, guest.cameraLeft, guest.cameraRight));
        // The game's camera put somewhere else at once (a round begun: back to the middle): the guest's own round's
        // start (Guest::roundStart: its camera put at the round's place and the stage painted there), rather than its
        // camera going there at its own pace across the picture
        constexpr int kJump = 24; // (more than a camera goes in a frame: the game's goes 16 at most)
        if (guest.StageAlone() && g.built == 1 && (guest.roundStart || guest.placing) && std::abs(g.cameraX - wasX) > kJump) {
            g.built = 2; // (at the board's next frame: Guest::roundStart)
        }
        g.cameraY = static_cast<int16_t>(hostUp - guest.lower);
        if (!guest.StageAlone()) {
            SetG8(board, guest.clock, guest.clockFull);
        }
        memset(board.players, 0, sizeof(board.players));
        board.drawing = machine.drawing;
        board.video.hideObject = guest.notTheStage;
        board.video.layerMask = guest.layers;
        board.video.markFront = !guest.fightersDrawn.empty() || guest.frontLevel;
        board.video.frontLevel = guest.frontLevel;
        board.video.wide.side = machine.video.wide.side;
        // (its picture drawn here, from its records, when it keeps them; else its own, as it was)
        NativeVideo::FrameData last;
        board.ownPicture = !board.nativeVideo.Active() || (board.ownPicture && !board.nativeVideo.LastFrame(last));
        g_guestDrawn = false;
        board.RunFrame();
        machine.guestShift = shown ? Shift(guest, board.ram.data()) : 0;
        CheckFrame(guest, board, shown, machine.guestShift);
        // The picture: the guest's, under the game's marked pixels; but its pixels in front of the fighters over them (and
        // under the game's panel). Moved down by the shift (its camera short of the game's, up or down)
        if (shown && machine.drawing && board.PictureWidth() == machine.PictureWidth()) {
            machine.guestBehind = true;
            uint32_t *picture = machine.Picture();
            const uint32_t *behind = GuestPicture(board);
            const int width = machine.PictureWidth();
            for (int line = 0; line < Video::kHeight; line++) {
                const int from = std::clamp(line - machine.guestShift, 0, Video::kHeight - 1);
                uint32_t *mine = picture + static_cast<size_t>(line) * width;
                const uint32_t *theirs = behind + static_cast<size_t>(from) * width;
                for (int i = 0; i < width; i++) {
                    const bool over = (mine[i] & 0x80000000u) && ((mine[i] & 0x40000000u) || !(theirs[i] & 0x40000000u));
                    mine[i] = over ? mine[i] & 0xFFFFFF : theirs[i] & 0xFFFFFF;
                }
            }
        }
        // The sound: the guest's music, as loud as the game's would be (its fades), while the game's stage tune is what
        // it plays
        const Soundtrack::State &music = machine.soundtrack.state;
        const bool playing = shown && music.song >= 0 && machine.soundtrack.Taken(static_cast<uint16_t>(music.song));
        const int64_t gain = playing ? static_cast<int64_t>(music.level) * music.allLevel / Soundtrack::kFull : 0;
        const size_t from = std::min(g_soundBefore, machine.audio.size());
        const size_t made = std::min(machine.audio.size() - from, board.audio.size());
        for (size_t i = 0; i < made && gain; i++) {
            const int64_t sample = machine.audio[from + i] + board.audio[i] * gain / Soundtrack::kFull;
            machine.audio[from + i] = static_cast<int16_t>(std::clamp<int64_t>(sample, -32768, 32767));
        }
        board.audio.clear();
    }

    // --- the check (guest_stages.h, Check) ---

    struct Checked {
        bool on = false;
        uint64_t frames = 0, followed = 0, off = 0, unknown = 0, blank = 0;
        int run = 0, longestRun = 0, worst = 0, settling = 0, blankRun = 0, longestBlank = 0;
        int lastGuest = 0, lastStage = 0, mostShift = 0;
        const char *worstStage = nullptr; // (the stage of the longest time off)
    } g_check;
    constexpr int kNear = 8, kLongestOff = 4, kSettle = 60;
    // (a picture of one colour, or nearly: the stage not drawn at all. Longer than a fade's)
    constexpr int kLongestBlank = 30;
    bool Blank(const Machine &board) {
        const uint32_t *picture = GuestPicture(board);
        const size_t pixels = static_cast<size_t>(board.PictureWidth()) * Video::kHeight;
        size_t other = 0;
        for (size_t i = 0; i < pixels; i += 7) {
            other += (picture[i] & 0xFFFFFF) != (picture[0] & 0xFFFFFF);
        }
        return other * 7 < pixels / 100;
    }
    void CheckFrame(const Guest &guest, Machine &board, bool shown, int shift) {
        // (for finding things out: CPS2RECOMP_GUEST_DUMP=FRAME:FILE writes the guest's work RAM (and FILE.video, FILE.sound: the memory it shares with its sound CPU) after its frame FRAME)
        static const char *dump = getenv("CPS2RECOMP_GUEST_DUMP");
        static uint64_t guestFrames = 0;
        if (dump && strchr(dump, ':') && ++guestFrames == strtoull(dump, nullptr, 10)) {
            if (FILE *f = fopen(strchr(dump, ':') + 1, "wb")) {
                fwrite(board.ram.data(), 1, board.ram.size(), f);
                fclose(f);
            }
            if (FILE *f = fopen((std::string(strchr(dump, ':') + 1) + ".video").c_str(), "wb")) {
                fwrite(board.videoRam.data(), 1, board.videoRam.size(), f);
                fclose(f);
            }
            if (FILE *f = fopen((std::string(strchr(dump, ':') + 1) + ".sound").c_str(), "wb")) {
                fwrite(board.soundRam, 1, sizeof(board.soundRam), f);
                fclose(f);
            }
        }
        if (!g_check.on || !shown) {
            g_check.run = 0;
            g_check.blankRun = 0;
            return;
        }
        if (g.guest != g_check.lastGuest || g.stage != g_check.lastStage) {
            g_check.lastGuest = g.guest;
            g_check.lastStage = g.stage;
            g_check.settling = kSettle; // (a stage just made: its camera comes to the place at its own pace)
        }
        g_check.frames++;
        uint32_t atX = 0, atY = 0;
        CameraAt(guest, board.ram.data(), atX, atY);
        if (!atX) {
            g_check.unknown++;
            return;
        }
        if (g_check.settling > 0) {
            g_check.settling--;
            return;
        }
        if (board.drawing && Blank(board)) {
            g_check.blank++;
            g_check.blankRun++;
            g_check.longestBlank = std::max(g_check.longestBlank, g_check.blankRun);
        } else {
            g_check.blankRun = 0;
        }
        const int x = static_cast<int16_t>(G16(board, atX)), y = static_cast<int16_t>(G16(board, atY));
        int wantedX, wantedY;
        WantedAt(guest, board.ram.data(), wantedX, wantedY);
        // (up or down, the picture moved by the shift: as far off as it is short of that)
        const int distance = std::max(std::abs(x - wantedX), atY ? std::max(0, std::abs(y - wantedY) - std::abs(shift)) : 0);
        g_check.mostShift = std::max(g_check.mostShift, std::abs(shift));
        if (distance <= kNear) {
            g_check.followed++;
            g_check.run = 0;
        } else {
            g_check.off++;
            g_check.run++;
            if (g_check.run > g_check.longestRun) {
                g_check.longestRun = g_check.run;
                g_check.worstStage = guest.stageNames[g.stage - 1];
            }
            g_check.worst = std::max(g_check.worst, distance);
        }
    }

    void StagesState(Archive &ar) {
        if (g_state) {
            g_state(ar);
        }
        if (ar.version != 0 && ar.version < 9) { // (a save state from before has none)
            return;
        }
        ar.Mark(0x44535447);
        ar.Pod(g);
        Machine *board = g.guest > 0 && g.guest <= static_cast<int>(Guests().size()) ? cps2recomp::Guest(Guests()[g.guest - 1].id) : nullptr;
        if (ar.Saving()) {
            g_guestState.clear();
            if (board) {
                Archive inner(g_guestState, true);
                board->Serialize(inner);
            }
            ar.Vec(g_guestState);
        } else {
            ar.Vec(g_guestState);
            if (!board) {
                g = State(); // (no set here: the game's own stages)
            } else if (!g_guestState.empty()) {
                Archive inner(g_guestState, false);
                board->Serialize(inner);
            } else {
                ToRound(g.guest - 1, g.stage); // (a peer's state without its board: from its round)
            }
        }
    }
} // namespace

namespace {
    FILE *g_look = nullptr;
}
void Look(const char *file) {
    g_look = fopen(file, "wb");
    if (!g_look) {
        fprintf(stderr, "--stage-look: cannot write %s\n", file);
    }
}
void LookFrame(Machine &machine) {
    if (!g_look || !modes::StageViewerShowing() || !machine.drawing) {
        return;
    }
    // (another game's stage: its board's picture, as it was before the game's laid over it, with its marks; its board's
    // registers)
    const Machine *board = Shown() ? cps2recomp::Guest(Guests()[g.guest - 1].id) : nullptr;
    const Machine &from = board && board->PictureWidth() == machine.PictureWidth() ? *board : machine;
    const int width = from.PictureWidth() / 2, height = Video::kHeight / 2;
    std::vector<uint8_t> record(4 + 4 + 4 + 12 + static_cast<size_t>(width) * height * 4);
    uint8_t *p = record.data();
    memcpy(p, "SLK1", 4);
    const uint32_t frame = static_cast<uint32_t>(machine.frame);
    memcpy(p + 4, &frame, 4);
    const uint16_t size[2] = {static_cast<uint16_t>(width), static_cast<uint16_t>(height)};
    memcpy(p + 8, size, 4);
    memcpy(p + 12, &from.cpsA[6], 12); // (scroll screens 1-3, across and down)
    uint8_t *rgb = p + 24, *front = rgb + static_cast<size_t>(width) * height * 3;
    const uint32_t *picture = board && &from == board ? GuestPicture(*board) : from.Picture();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const uint32_t pixel = picture[static_cast<size_t>(y) * 2 * from.PictureWidth() + x * 2];
            *rgb++ = static_cast<uint8_t>(pixel);
            *rgb++ = static_cast<uint8_t>(pixel >> 8);
            *rgb++ = static_cast<uint8_t>(pixel >> 16);
            *front++ = (pixel & 0x40000000u) ? 1 : 0;
        }
    }
    fwrite(record.data(), 1, record.size(), g_look);
}

int Stages() {
    int stages = 0;
    for (const Guest &guest : Guests()) {
        stages += static_cast<int>(guest.stageNames.size());
    }
    return stages;
}

const char *StageName(int stage) {
    int index, own;
    return Which(stage, index, own) ? Guests()[index].stageNames[own - 1] : "";
}

void Check(bool on) {
    g_check.on = on;
}
bool Report(FILE *out) {
    if (!g_check.on) {
        return true;
    }
    // (a run that showed none has checked nothing: not passed)
    const bool ok = g_check.frames > 0 && g_check.longestRun <= kLongestOff && g_check.unknown == 0 &&
                    g_check.longestBlank <= kLongestBlank;
    fprintf(out, "stage check: %llu frames of other games' stages shown; the camera within %d of where wanted in %llu, off in %llu "
                 "(longest %d frames, at most %d off); the picture blank in %llu (longest %d)%s; %s\n",
            static_cast<unsigned long long>(g_check.frames), kNear, static_cast<unsigned long long>(g_check.followed),
            static_cast<unsigned long long>(g_check.off), g_check.longestRun, g_check.worst,
            static_cast<unsigned long long>(g_check.blank), g_check.longestBlank, g_check.unknown ? " (a guest's camera not known: not checked)" : "", ok ? "ok" : "FAILED");
    if (g_check.mostShift) {
        fprintf(out, "stage check: a guest's picture moved up or down by at most %d (its camera short of the game's)\n", g_check.mostShift);
    }
    if (g_check.worstStage) {
        fprintf(out, "stage check: the longest time off on %s\n", g_check.worstStage);
    }
    return ok;
}

bool Available(int stage) {
    int index, own;
    return Which(stage, index, own) && cps2recomp::Guest(Guests()[index].id);
}

void Register(const Host &host, std::vector<Guest> guests) {
    h = host;
    Guests() = std::move(guests);
    for (int n = 0; n < static_cast<int>(Guests().size()) && n < kMostGuests; n++) {
        const Guest &guest = Guests()[n];
        Ready &ready = TheReady(n);
        ready.round.resize(guest.stageNames.size());
        ready.made = std::make_unique<std::atomic<bool>[]>(guest.stageNames.size());
        if (guest.StageAlone()) {
            AddGuestHook(guest.id, guest.mainLoop, StageAlone);
            for (const auto &[at, fn] : guest.hooks) {
                AddGuestHook(guest.id, at, fn);
            }
            for (const uint32_t at : guest.fightersDrawn) {
                AddGuestHook(guest.id, at, FightersDrawn);
            }
        } else {
            AddGuestHook(guest.id, guest.stageAt, StageSet);
        }
        const bool steps = guest.camera == Guest::Camera::Steps, place = guest.camera == Guest::Camera::Place;
        AddGuestHook(guest.id, guest.across, place ? PlaceAcross : steps ? StepAcross : ExtentsAcross);
        AddGuestHook(guest.id, guest.up, place ? PlaceUp : steps ? StepUp : ExtentsUp);
    }
    if (Guests().size() > kMostGuests) {
        Guests().resize(kMostGuests);
    }
    Game &game = TheGame();
    g_before = game.beforeFrame;
    game.beforeFrame = BeforeFrame;
    g_frame = game.frame;
    game.frame = Frame;
    g_state = game.state;
    game.state = StagesState;
}

Machine *ShownBoard() {
    return g.guest > 0 && Shown() ? cps2recomp::Guest(Guests()[g.guest - 1].id) : nullptr;
}

} // namespace cps2recomp::guest_stages
