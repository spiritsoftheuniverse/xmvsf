// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: what the game's own sources add to the port (see PROGRESS.md).
#include "alternative.h"
#include "battle.h"
#include "colours.h"
#include "game.h"
#include "marvel.h"
#include "session.h"
#include "xmcota.h"

namespace cps2recomp {

void DescribeEngine(); // engine.cpp
void RegisterCoins();  // coins.cpp
void CoinsFrame(Machine &machine);
void RegisterCheats(); // cheats.cpp
void RegisterTitle();  // title.cpp
void TitleFrame(Machine &machine);
void RegisterBoard(); // board.cpp
void BoardBeforeFrame(Machine &machine);
void BoardState(Archive &ar);
void RegisterMusic(); // music.cpp
void RegisterModes(); // modes.cpp
void ModesState(Archive &ar);
void ModesBeforeFrame(Machine &machine);
void RegisterTraining(); // training.cpp
void RegisterChoosing(); // choosing.cpp
void ChoosingBeforeFrame(Machine &machine);
void RegisterColours(); // colours.cpp
void RegisterBattle();  // battle.cpp

// 001332, the game's random number: a long divs (00133E), then the number kept (001344). The board takes an interrupt
// between any two instructions, and the frame's interrupt, which asks for a random number of its own, can come during
// the divs: it then reads the number before this one is kept. Translated code takes interrupts only where a block
// starts; a block starting after the divs (where nothing is done) lets it take this one where the board does, as
// Marvel Super Heroes' has to (the same routine there, 00134E).
static void RandomNumberAsTheBoardDoes() {
    AddHook(HookAt(0x001342), "AfterRandomDivide", [](M68K &) { return false; });
}

void RegisterStages(); // stages.cpp

void RegisterGame() {
    DescribeEngine();
    RandomNumberAsTheBoardDoes();
    RegisterModes();
    RegisterTraining();
    RegisterChoosing();
    // (the model viewer: the camera's way chosen at 06EB84 across and 06EC84 down; the characters are about twice as
    // tall as the Alpha games')
    marvel::RegisterModel(HookAt(0x06EB84), 80, HookAt(0x06EC84));
    RegisterColours();
    RegisterBattle();
    // (widescreen: the camera kept between its ends by each stage's routine, xmcota.h's kCameraKept; a character kept
    // inside the picture at 00A302)
    marvel::RegisterWidescreen(0, HookAt(0x00A302));
    RegisterCoins();
    RegisterCheats();
    RegisterTitle();
    RegisterBoard();
    RegisterMusic();
    TheGame().beforeFrame = [](Machine &machine) {
        BoardBeforeFrame(machine);
        ChoosingBeforeFrame(machine);
        marvel::WidescreenBeforeFrame(machine);
        session::BeforeFrame(machine);
        modes::BeforeFrame(machine);
        ModesBeforeFrame(machine);
        BattleBeforeFrame(machine);
        colours::BeforeFrame(machine);
        marvel::ModelBeforeFrame(machine);
    };
    // (the training aids see what the frame did before the cheats put things back)
    TheGame().frame = [](Machine &machine) {
        session::Frame(machine);
        modes::Frame(machine);
        BattleFrame(machine);
        marvel::ModelFrame(machine);
        TitleFrame(machine);
        CoinsFrame(machine);
    };
    // (what the sources keep between frames, with save states and run-ahead's frames taken back)
    TheGame().state = [](Archive &ar) {
        BoardState(ar);
        session::State(ar);
        modes::State(ar);
        ModesState(ar);
        BattleState(ar);
        marvel::ModelState(ar);
        marvel::WidescreenState(ar);
    };
    // (run-ahead: not in the modes, whose workings a save state does not wholly hold)
    TheGame().noRunAhead = [] {
        return modes::AttractMode() || modes::StageViewer() || BattleMode() || marvel::ModelViewer();
    };
    // With the arcade timing, the game's own code runs in place of the rewrites but where they carry what the port adds
    TheGame().rewritesNeeded = [] {
        return session::InSession() || modes::AttractMode() || modes::StageViewer() || BattleMode() || marvel::ModelViewer() ||
               marvel::WidePlay() || FeatureOn("widescreen");
    };
    // Its own, never set for the other games by the collection's launcher: these have other choices in the other games;
    // the best scores and the animations are this game's
    OwnFeatures({"board_difficulty", "board_damage", "board_timer", "board_rounds", "board_event", "survival_best", "ringout_best",
                 "model_animation", "colours_animation"});
    // Other games' stages: last, as it goes around the frame's callbacks and the state's (stages.cpp)
    RegisterStages();
}

} // namespace cps2recomp
