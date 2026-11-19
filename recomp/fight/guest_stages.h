// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// Other games' stages behind a fighting game's fighters, as those games show them. Each other game ("a guest") is a
// board of its own run beside the game's (runtime/guest.h; game.cfg's guests), held in a two-player match on the
// stage, its fighters left standing and never drawn, its clock held, its camera put where the game's is every frame.
// The picture is the guest's stage with the game's fighters and panel over it (drawn alone, marked:
// Video::markObjects); the music is the guest stage's own, in place of the game's stage tune (Soundtrack::Take), with
// the game's fades.
//
// A guest that says where its main loop is (Guest::StageAlone, Vampire Savior's) runs its stage alone: no match, its
// stage's own code only (below). For the others, each stage's round is made once, on a spare board of the guest
// (GuestSpare) and a thread of its own, from the
// program's start: the board played with made-up controls (credits, both Starts, a punch at each screen that waits for
// one) into a two-player match on the stage, to the round. That is the same board, frame for frame, every time, so a
// match on a stage always starts from it, however long ago it was made: the same in a run played straight, its frames
// run again (online play's rollback), a save state loaded or the other player's game. A match that wants a round not
// made yet waits for it. The guest's board is kept with the game's state.
//
// What a game says of itself (Host) and of each guest (Guest) is all of it: a guest is described once, by the facts
// its own port found (its steps, its credits, where it sets a match's stage, its clock, its stage's objects, its
// camera), and can then be shown by any game. Vampire Savior Definitive shows Darkstalkers' and Street Fighter Alpha
// 2's (games/vsav_definitive/src/stages.cpp); Alpha 2 and Zero 2 Alpha show Darkstalkers' and Vampire Savior's
// (games/sfa2/src/stages.cpp). The guests' facts: games/dstlk/guest.h, games/sfa2/guest.h, games/vampire_savior/guest.h.
#pragma once

#include "../machine/m68k.h"

#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

namespace cps2recomp {
class Machine;
}

namespace cps2recomp::guest_stages {

// A word (or a byte) of the guest's memory and the value it has
struct Is {
    uint32_t address;
    uint16_t value;
    bool byte = false;
};

struct Guest {
    const char *id;                       // its game's id (game.cfg's guests)
    std::vector<const char *> stageNames; // its stages, by its own numbers from 0, as the lists name them
    // Its way to a round: its character choice is on, both players in; its match is on (the made-up controls stop);
    // a round of it is fought (each: all of these)
    std::vector<Is> choosing, match, fought;
    uint32_t credits = 0;  // a byte, kept at 2 on the way
    uint32_t playersIn = 0; // a byte: 3 with both players in
    // Where it sets a match's stage: a hook there, at which `setStage` gives it the stage (0 the first)
    uint32_t stageAt = 0;
    void (*setStage)(M68K &c, int stage) = nullptr;
    // Its round's clock (a byte), held at `clockFull`
    uint32_t clock = 0;
    uint8_t clockFull = 0x99;
    // Its objects that are not its stage's (its fighters, its panel): not drawn. And the scroll screens drawn (layerMask)
    bool (*notTheStage)(uint32_t tile, uint16_t attributes) = nullptr;
    unsigned layers = 0xF;
    // Its camera's ends across: the game's camera's place goes to the same place between them; and how much lower
    // (in pixels) its camera is put than the game's
    int cameraLeft = 256, cameraRight = 640, lower = 0;
    // How its camera is put where it is wanted, by one of:
    // - Steps: the routines that step its camera (an object, A6) across and up, done here: the camera's place
    //   (words), the step it took (words: the scroll screens behind move by it) and which way (bytes)
    // - Extents: the routines its camera asks where the players are (D0 and D1: their outer edges across, their
    //   heights), answered so that its own camera goes where it is wanted (as far as it goes in a frame)
    // - Place: the routines that move its camera (A6 its record), at the point where they move it: its place set there
    //   (placeX, placeY), the routine returned from; with `endLeft` its record's own ends (words at these offsets, by
    //   stage), the place wanted put as far between them as the game's is between its own; with `placeDown` its place
    //   up and down counts down the picture (the top's), from its lowest
    enum class Camera { Steps, Extents, Place } camera = Camera::Steps;
    uint32_t across = 0, up = 0;
    uint32_t placeX = 0x10, placeY = 0x14, stepX = 0x34, stepY = 0x36, wayX = 0x2C, wayY = 0x2D;
    uint8_t wayRight = 4, wayLeft = 0, wayUp = 0x0C, wayDown = 0x08;
    int middle = 0xC0, halfApart = 0x60, above = 0x60; // (Extents: the picture's middle; the players' edges; height)
    // Where its camera's place is kept, across and up (words), for the check (--stage-check): that it goes where wanted
    uint32_t cameraAtX = 0, cameraAtY = 0;
    // ... or, when its record is not the same on every stage, where it is (its memory, from FF0000), the place at
    // placeX and placeY in it
    uint32_t (*cameraRecord)(const uint8_t *ram) = nullptr;
    uint32_t endLeft = 0, endRight = 0, endTop = 0, endBottom = 0;
    bool placeDown = false;

    // Its stage alone, from its own stage code (a guest that says these): no game is played to a round and nothing of
    // the game but its stage runs. Its board is started and kept at the top of its main loop (`mainLoop`, where its
    // modes are run each frame), where the loop goes on to its wait for the frame (`loopGoesOn`) and nothing else
    // runs. A stage is built there once (the routines `build` are called, `setStage` before each, then `afterBuild`), and
    // each frame after the routines `frame` are called (the scroll screens and the camera, the stage's pieces, the
    // object list); the board's frame interrupt hands the frame over as the game's own does. `a5` is A5 as its code
    // has it. Its stage's music is asked for as the game asks for a sound: an entry of 16 bytes (the tune's number, a
    // long) put in its ring at `soundQueueEnd` (a word, bytes into the ring at `soundQueue`), each stage's tune.
    uint32_t mainLoop = 0, loopGoesOn = 0, a5 = 0xFF8000;
    // ... for a game whose main loop is a game's own (its task started when a game is begun, Darkstalkers'): once its
    // start-up has run `beginAfter` frames, a credit (`credits`) and Start, so that a game begins and its loop is
    // reached (at its character choice: nothing is chosen or played)
    int beginAfter = -1;
    std::vector<uint32_t> build, frame;
    void (*afterBuild)(M68K &c) = nullptr;
    // ... and when the game's camera is put somewhere else at once (a round begun): the guest's own round's start, its
    // camera put at the round's place and its stage painted there (its pieces and music go on)
    void (*roundStart)(M68K &c) = nullptr;
    // ... the routine of `frame` that places its scroll screens and its camera: called again, once a stage is built or
    // its round started, until its camera is where wanted (as far as it goes in a call each time, painting as it goes),
    // so that the stage is not seen to come there
    uint32_t placing = 0;
    // ... and hooks of its own in its code while its stage is alone (a camera let go as far in a frame as the game's
    // goes, say): each told the CPU before the instruction there, as a game's hook is
    std::vector<std::pair<uint32_t, bool (*)(M68K &c)>> hooks;
    // (a 0 in `build`: what the game's own build does there in its own lines, not in a routine of its own; `step`
    // the 0's number, from 0)
    void (*buildStep)(M68K &c, int step) = nullptr;
    // The fight's frame counter (a byte its stage's pieces go by: a searchlight's sweep, colours that cycle), a step each
    // frame as the fight's frame steps it (0: none)
    uint32_t frameCounter = 0;
    // What of its stage is in front of the fighters: the objects whose entries its object list's routine puts in after
    // the fighters' (`fightersDrawn`, the places in that routine where the fighters' are in: the next entry's place in the
    // address register `listRegister`), and, with `frontLevel`, the scroll screens' pixels an object of that level (the
    // fighters') is behind (0: none). They are drawn over the game's fighters, and under its panel (Host::panel).
    std::vector<uint32_t> fightersDrawn;
    int listRegister = 2;
    unsigned frontLevel = 0;
    uint32_t soundQueue = 0, soundQueueEnd = 0;
    uint16_t soundQueueMask = 0xFFF;
    std::vector<uint16_t> stageTunes;
    bool StageAlone() const {
        return mainLoop != 0;
    }
};

struct Host {
    uint32_t cameraX = 0, cameraY = 0; // the game's camera's place (words)
    int cameraLeft = 256, cameraRight = 640;
    // (or, where its camera's place is not in words of its own: its place across, the stage's ends, and how high it is
    // above its lowest (up from 0), each frame)
    void (*camera)(int &x, int &left, int &right, int &up) = nullptr;
    bool (*shown)() = nullptr;  // a frame in which the stage is shown (a match's, not its versus screen; the viewer's)
    bool (*viewer)() = nullptr; // the stage viewer: nothing of the game's own is drawn
    bool (*notTheStage)(uint32_t tile, uint16_t attributes) = nullptr; // the game's objects that are not its stage's
    bool (*panel)(uint32_t tile, uint16_t attributes) = nullptr;       // ... and of those, its panel's: over the stage's front
    unsigned panelLayers = 0; // its scroll screens that are its panel, not its stage's (layerMask's bits): laid over too
    std::vector<uint16_t> stageTunes; // its stages' tunes: the guest's music plays in their place
    // The stage wanted now: 1 the first guest's first, on through each guest's in turn (0: the game's own)
    int (*wanted)() = nullptr;
    // A feature with which the guests' stages are made ready from the start (else from the first wanted)
    const char *feature = nullptr;
};

// --stage-look FILE: each frame of the stage viewer that shows its stage (another game's or the game's own), the stage's
// picture written to FILE for tools/stage_look.py, which compares a guest's stage in another game with the same stage in
// its own game's viewer: a record a frame of "SLK1", the frame's number, the picture's width and height (every other
// column and line of the picture's), the six words of the stage's scroll screens' places (where its camera is, as the
// stage's own code put them), the picture's pixels (red, green, blue) and a byte for each, 1 where it is in front of the
// fighters (Guest::fightersDrawn)
void Look(const char *file);
void LookFrame(Machine &machine);

// The stages of all the guests, in turn (whether their sets are there or not), and whether one can be shown
int Stages();
const char *StageName(int stage); // (1 the first)
bool Available(int stage);
// The game's sources describe themselves and their guests, after the rest of their registering (this goes around the
// frame's callbacks and the state's)
void Register(const Host &host, std::vector<Guest> guests);
// The check (--stage-check): each frame another game's stage is shown, its camera against the place wanted for it (the
// game's camera's, between the guest's ends), and whether its picture is blank (one colour: the stage not drawn); at the
// end, Report says how often and how far it was off, and false when it stayed off longer than the camera takes to catch
// up, or blank for more than half a second
void Check(bool on);
bool Report(FILE *out);

// The board whose stage is under the game's picture this frame (Machine::guestBehind; none: the game's own stage), for
// the scene export (port/scene.h), which draws its layers too
Machine *ShownBoard();

} // namespace cps2recomp::guest_stages
