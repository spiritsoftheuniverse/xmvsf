// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// A shoot-'em-up's "Alternative modes" page and its training mode, for every shooting game on the board (Giga Wing the
// first, then Progear and Dimahoo; 19XX and 1944 would describe themselves the same way). Each is started by a command
// of the in-game menu and ends when the board is reset, when another is started, or as said below.
//
//   Training mode  a stage, or straight at its boss, with the fighter, ships, bombs and power chosen. Backspace (a
//                  gamepad's left shoulder) puts it back as it began, at once: the start is kept as a save state in
//                  memory when it is first reached. Replay shows the last attempt again, from that start, with the
//                  controls as they were pressed. Its aids (a readout of the scoring, a summary at each boss and stage
//                  end) work only in training mode.
//   Attract mode   the game's own show on a loop, with no coin in and its sound on, or one part of it.
//   Score attack   one player, one credit, no continue and no cheats, from the first stage: the board's own game, for
//                  the best score, which is kept (shmup_scores.txt beside settings.ini).
//   Caravan        a score attack against the clock: two, three or five minutes from the first stage, as many ships as
//                  it takes; the best score in the time is kept for each length.
//   Boss rush      each boss in turn: the way between them is run unseen, the ships out of harm. The best score is
//                  kept, and the time it took.
//   Stage viewer   a stage scrolling by with nobody on it: no ships, no enemies, no shots, no panel. It scrolls at the
//                  speed chosen; the directions take it over (forward: faster; back: back the way it came, from the
//                  states it keeps every half second) and the shot button stops and starts it.
//
// Besides the modes, everywhere a stage is played (the board's own game too): the hit boxes (the Cheats page: the
// players' ships', the enemies' bodies', the enemies' shots'), and the slowdown (the Game page: as the board has it, or
// less or none, the 68000 run faster than the board's, Machine::cpuSpeed).
//
// A game's sources describe how it is got into and what it keeps where (Description), register it, and ask while the
// game runs: the stage a game is to start at, whether the ships are to be out of harm (Protected), whether their own
// choices are left out.
#pragma once

#include "game.h"
#include "machine.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cps2recomp {

class Archive;

namespace shmup {

// What a mode wants of a game it begins
struct Begin {
    int stage = 0;        // from 0
    int fighter = -1;     // the game's own number (-1: the player's choice, the game's first)
    bool twoPlayers = false;
};

struct Description {
    // --- the game's parts ---
    std::vector<const char *> stages;    // the stages' names, in order
    std::vector<const char *> fighters;  // the fighters' names, as the choice of fighter shows them (in the game's numbers)
    // The board's start is over: Start (with a credit, or free play) begins a game
    bool (*canStart)(const Machine &machine) = nullptr;
    // Puts a credit in for player 1, and player 1's credits (the game has taken it when they are 0 again)
    void (*credit)() = nullptr;
    int (*credits)() = nullptr;
    // The controls of a machine on its way from a game's Start to its first stage being played, with `begin`'s fighter
    // (it is run unseen meanwhile): true once the stage is played
    bool (*toPlay)(Machine &machine, const Begin &begin, int32_t frames) = nullptr;
    // A game is on, up to its game over (not the show's demonstrations), and a stage is being played (not the choice of
    // fighter, a story, the continue)
    bool (*inGame)() = nullptr;
    bool (*playing)() = nullptr;
    int (*stage)() = nullptr;  // the stage being played, from 0
    bool (*bossOn)() = nullptr; // a boss is being fought (its clock running)
    // A stage that is a boss from its start, with no clock (Giga Wing's last): its boss is met as the stage begins
    bool (*bossStage)(int stage) = nullptr;
    uint64_t (*score)(int player) = nullptr;
    // --- what training mode can set, as the ship's start is reached: each -1 leaves it as the game had it ---
    void (*setShips)(int player, int ships) = nullptr;
    void (*setBombs)(int player, int bombs) = nullptr;
    void (*setPower)(int player, int power) = nullptr; // (none: no Power row on the Training mode page)
    int maxShips = 9, maxBombs = 8, maxPower = 3;
    // --- the hit boxes ---
    // The players' ships' hit boxes, the enemies' bodies and the enemies' shots, in the board's picture's pixels, as the
    // game has them in the frame just run (they are shown a frame later, with the objects: the board shows the objects
    // the game listed in the frame before)
    void (*shipBoxes)(std::vector<HudBox> &boxes) = nullptr;
    void (*bodyBoxes)(std::vector<HudBox> &boxes) = nullptr;
    void (*shotBoxes)(std::vector<HudBox> &boxes) = nullptr;
    // The game makes its object list (Machine::ObjectsToCome) from the places of the frame before the one it makes it in
    // (1: Giga Wing, Progear), not of that frame (0: Dimahoo): the boxes shown with a list are those of the places it was
    // made from
    int listLag = 0;
    // --- the training aids ---
    // Lines of the readout (the multiplier and the like), each player's (none: no readout), and what its row on the
    // Training mode page says it shows (nullptr: Giga Wing's words)
    void (*readout)(int player, std::vector<std::string> &lines) = nullptr;
    const char *readoutWords = nullptr;
    // What the summary's row says it counts (nullptr: Giga Wing's words; a game with no multiplier counts its score)
    const char *summaryWords = nullptr;
    // The numbers a summary counts (a stage's or a boss's): the player's multiplier as a number, and the ships and bombs
    // the player has now
    uint64_t (*multiplier)(int player) = nullptr;
    int (*ships)(int player) = nullptr;
    int (*bombs)(int player) = nullptr;
    // --- attract mode ---
    // The parts of the show (after "Everything"), and which the show is in now (0: none of them, the rest of it)
    std::vector<const char *> shows;
    const char *showsWords = "";
    int (*showPart)() = nullptr;
    bool (*showBegun)(const Machine &machine) = nullptr;
    // --- the stage viewer ---
    // The scroll screens shown while viewing (Video::layerMask: the panel's left out; objects are never shown)
    unsigned viewLayers = 0xF;
};

void Register(const Description &description);

bool Practice();
bool AttractMode();
int AttractShow();
bool ScoreAttack();
bool Caravan();
bool BossRush();
bool StageViewer();
// Any mode of these is on: the game's sources leave out their own choices (free play, the cheats, the board's continue,
// "Start at stage"), and those of the Board page that a mode sets (the continue, the demo sound)
bool AnyMode();
// The modes that are a scored game of one player: no continue
bool NoContinue();
// The stage a game beginning now is to start at (-1: as the game would)
int WantedStage();
// The ships are to be out of harm now (a mode running the game unseen, the stage viewer): the game's sources make them
// invincible
bool Protected();
// A mode is getting the board to where it begins (the game's sources leave the board alone meanwhile)
bool Starting();
// Training mode's aids work (Game::inTraining)
bool InPractice();
void EndModes();

// The game's sources call these from their own (Game::beforeFrame, frame, state)
void BeforeFrame(Machine &machine);
void Frame(Machine &machine);
void State(Archive &ar);

} // namespace shmup
} // namespace cps2recomp
