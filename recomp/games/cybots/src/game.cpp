// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: Fullmetal Madness: what the game's own sources add to the port (see PROGRESS.md).
#include "game.h"
#include "cybots.h"
#include "alternative.h"
#include "battle.h"
#include "colours.h"
#include "session.h"
#include "state.h"

namespace cps2recomp {

void RegisterCoins(); // coins.cpp
void CoinsFrame(Machine &machine);
void CoinsState(Archive &ar);
void RegisterCheats(); // cheats.cpp
void RegisterTitle(); // title.cpp
void TitleFrame(Machine &machine);
void RegisterBoard(); // board.cpp
void BoardBeforeFrame(Machine &machine);
void BoardState(Archive &ar);
void RegisterMusic(); // music.cpp
void RegisterModes(); // modes.cpp
void RegisterTraining(); // training.cpp
void RegisterBattle(); // battle.cpp
void RingOutState(Archive &ar);
void RegisterWidescreen(); // widescreen.cpp
void WidescreenBeforeFrame(Machine &machine);
void WidescreenState(Archive &ar);
bool WidePlay();
void RegisterModel(); // model.cpp
void ModelBeforeFrame(Machine &machine);
void ModelFrame(Machine &machine);
void ModelState(Archive &ar);
bool ModelViewer();
void RegisterColours(); // colours.cpp
void RegisterChoosing(); // choosing.cpp
void ChoosingBeforeFrame(Machine &machine);
void ChoosingState(Archive &ar);

void RegisterGame() {
    RegisterModes();
    RegisterTraining();
    RegisterBattle();
    RegisterWidescreen();
    RegisterModel();
    RegisterColours();
    RegisterChoosing();
    RegisterCoins();
    RegisterCheats();
    RegisterTitle();
    RegisterBoard();
    RegisterMusic();
    TheGame().beforeFrame = [](Machine &machine) {
        BoardBeforeFrame(machine);
        ChoosingBeforeFrame(machine);
        WidescreenBeforeFrame(machine);
        session::BeforeFrame(machine);
        modes::BeforeFrame(machine);
        BattleBeforeFrame(machine);
        colours::BeforeFrame(machine);
        ModelBeforeFrame(machine);
    };
    TheGame().frame = [](Machine &machine) {
        session::Frame(machine);
        modes::Frame(machine);
        BattleFrame(machine);
        ModelFrame(machine);
        TitleFrame(machine);
        CoinsFrame(machine);
    };
    // (what the sources keep between frames, with save states and run-ahead's frames taken back)
    TheGame().state = [](Archive &ar) {
        BoardState(ar);
        CoinsState(ar);
        session::State(ar);
        modes::State(ar);
        BattleState(ar);
        RingOutState(ar);
        WidescreenState(ar);
        ModelState(ar);
        ChoosingState(ar);
    };
    // (run-ahead: not in the modes, whose workings a save state does not wholly hold)
    TheGame().noRunAhead = [] { return modes::AttractMode() || modes::StageViewer() || BattleMode() || ModelViewer(); };
    // With the arcade timing, the game's own code runs in place of the rewrites but where they carry what the port adds
    TheGame().rewritesNeeded = [] { return session::InSession() || modes::AttractMode() || modes::StageViewer() || BattleMode() || ModelViewer() || WidePlay() ||
               FeatureOn("widescreen");
    };
    // Its own: the names and choices are this board's, never set for the other games by the collection's launcher
    OwnFeatures({"board_difficulty", "board_damage", "board_timer", "survival_best", "ringout_best", "model_animation",
                 "colours_animation"});
}

} // namespace cps2recomp
