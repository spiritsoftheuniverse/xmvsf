// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes: what the game's own sources add to the port (see PROGRESS.md).
#include "alternative.h"
#include "battle.h"
#include "colours.h"
#include "game.h"
#include "marvel.h"
#include "msh.h"
#include "session.h"

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
void RegisterTraining(); // training.cpp
void RegisterChoosing(); // choosing.cpp
void ChoosingBeforeFrame(Machine &machine);
void RegisterColours(); // colours.cpp
void RegisterBattle();  // battle.cpp

// 00134E, the game's random number: a long divs (00135A), then the number kept (00135E). The board takes an interrupt
// between any two instructions, and the frame's interrupt, which asks for a random number of its own (000682), can come
// during the divs: it then reads the number before this one is kept. Translated code takes interrupts only where a
// block starts; a block starting after the divs (where nothing is done) lets it take this one where the board does, as
// X-Men vs. Street Fighter's has to (the same routine there, 001608).
static void RandomNumberAsTheBoardDoes() {
    AddHook(HookAt(0x00135E), "AfterRandomDivide", [](M68K &) { return false; });
}

void RegisterStages(); // stages.cpp

void RegisterGame() {
    DescribeEngine();
    RandomNumberAsTheBoardDoes();
    RegisterModes();
    RegisterTraining();
    RegisterChoosing();
    // (the model viewer: the camera's way chosen at 082440; the characters are about twice as tall as the Alpha games')
    marvel::RegisterModel(HookAt(0x082440), 80);
    RegisterColours();
    RegisterBattle();
    // (widescreen: the camera kept between its ends at 083A46, a character kept inside the picture at 00DFF2)
    marvel::RegisterWidescreen(HookAt(0x083A46), HookAt(0x00DFF2));
    // (the routine that says whether an object is in the picture, at its test across: played wide, the wide picture's)
    AddHook(HookAt(0x001C06), "InPicture", marvel::InPicture);
    AddHook(HookAt(0x001C70), "InPicture", marvel::InPicture);
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
    OwnFeatures({"board_difficulty", "board_damage", "board_timer", "board_rounds", "board_mode", "survival_best", "ringout_best",
                 "model_animation", "colours_animation"});
    // Other games' stages: last, as it goes around the frame's callbacks and the state's (stages.cpp)
    RegisterStages();
}

} // namespace cps2recomp
