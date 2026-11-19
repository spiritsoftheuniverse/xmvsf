// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men vs. Street Fighter: what the game's own sources add to the port (see PROGRESS.md).
#include "alternative.h"
#include "battle.h"
#include "colours.h"
#include "game.h"
#include "marvel.h"
#include "session.h"
#include "tag.h"
#include "xmvsf.h"

namespace cps2recomp {

void DescribeEngine(); // engine.cpp
void RegisterCoins(); // coins.cpp
void CoinsFrame(Machine &machine);
void RegisterCheats(); // cheats.cpp
void RegisterTitle(); // title.cpp
void TitleFrame(Machine &machine);
void RegisterBoard(); // board.cpp
void BoardBeforeFrame(Machine &machine);
void BoardState(Archive &ar);
void RegisterMusic(); // music.cpp
void RegisterModes(); // modes.cpp
void ModesState(Archive &ar);
void RegisterTraining(); // training.cpp
void RegisterChoosing(); // choosing.cpp
void RegisterColours(); // colours.cpp
void RegisterBattle(); // battle.cpp
void ChoosingBeforeFrame(Machine &machine);
void RegisterTag(); // tag.cpp

// 001608, the game's random number: a long divs (001614, about 150 cycles), then the number kept (00161A). The board
// takes an interrupt between any two instructions, and the frame's interrupt, which asks for a random number of its own
// (00069A), often comes during the divs: it then reads the number before this one is kept. Translated code takes
// interrupts only where a block starts; a block starting after the divs (where nothing is done) lets it take this one
// where the board does, or the game's numbers part from the board's (seen at frame 5,227 of the ladder movie).
static void RandomNumberAsTheBoardDoes() {
    AddHook(HookAt(0x001618), "AfterRandomDivide", [](M68K &) { return false; });
}

void RegisterStages(); // stages.cpp

void RegisterGame() {
    DescribeEngine();
    RandomNumberAsTheBoardDoes();
    RegisterModes();
    RegisterTraining();
    RegisterChoosing();
    // (the model viewer: the camera's way chosen at 070232; the characters are about twice as tall as the Alpha games')
    marvel::RegisterModel(HookAt(0x070232), 80);
    RegisterColours();
    RegisterBattle();
    // (widescreen: the camera kept between its ends at 074488, a character kept inside the picture at 010806)
    marvel::RegisterWidescreen(HookAt(0x074488), HookAt(0x010806));
    // (the routine that says whether an object is in the picture, at its test across: played wide, the wide picture's)
    AddHook(HookAt(0x001ED6), "InPicture", marvel::InPicture);
    AddHook(HookAt(0x001F40), "InPicture", marvel::InPicture);
    RegisterTag();
    RegisterCoins();
    RegisterCheats();
    RegisterTitle();
    RegisterBoard();
    RegisterMusic();
    TheGame().beforeFrame = [](Machine &machine) {
        tag::BeforeFrame(machine);
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
        tag::Frame(machine);
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
    TheGame().noRunAhead = [] { return modes::AttractMode() || modes::StageViewer() || BattleMode() || marvel::ModelViewer(); };
    // With the arcade timing, the game's own code runs in place of the rewrites but where they carry what the port adds
    TheGame().rewritesNeeded = [] {
        return session::InSession() || modes::AttractMode() || modes::StageViewer() || BattleMode() || marvel::ModelViewer() || marvel::WidePlay() ||
               FeatureOn("widescreen");
    };
    // Its own, never set for the other games by the collection's launcher: Damage level has other choices in the Alpha
    // games (Low, Medium, Normal, High); the best scores and the animations are this game's
    OwnFeatures({"board_damage", "survival_best", "ringout_best", "model_animation", "colours_animation"});
    // Other games' stages: last, as it goes around the frame's callbacks and the state's (stages.cpp)
    RegisterStages();
}

} // namespace cps2recomp
