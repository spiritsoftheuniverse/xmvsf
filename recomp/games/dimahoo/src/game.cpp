// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Dimahoo: what the game's own sources add to the port (see PROGRESS.md): free play, straight to the title, the
// cheats, start at stage, and the modes (modes.cpp).
#include "dimahoo.h"
#include "shmup.h"
#include "state.h"

namespace cps2recomp {

void RegisterBoard(); // board.cpp
void BoardFrame(Machine &machine);
void BoardState(Archive &ar);
void RegisterMusic(); // music.cpp
void RegisterModes(); // modes.cpp
bool ChoicesLeftOut();
void ModesBeforeFrame(Machine &machine);
void ModesFrame(Machine &machine);
void ModesState(Archive &ar);

namespace {
    using namespace dimahoo;

    // "Straight to the title": the board's start, its warning, the logos and the story run unseen, until the show's
    // title (038E7E) or a game. (Kept with the state, as everything that changes while the game runs: online play runs
    // frames again from it.)
    struct Title {
        bool skipping = false, fast = false, shown = false;
        int32_t frames = 0;
    } g_title;

    bool TitleShown(M68K &) {
        g_title.shown = true;
        return false;
    }

    void TitleFrame(Machine &machine) {
        // (a mode of the menu started: its start is its own, and so is running unseen)
        if (ChoicesLeftOut()) {
            g_title.skipping = false;
        } else {
            if (machine.frame <= 1 && !g_title.skipping) {
                g_title.skipping = FeatureOn("skip_intro");
                g_title.shown = false;
                g_title.frames = 0;
            }
            const uint16_t step = Ram16(kStep);
            const bool there = g_title.shown || step == kStepTitle || step == kStepGame;
            if (g_title.skipping && (there || ++g_title.frames > 6000)) {
                g_title.skipping = false;
            }
        }
        if (g_title.skipping != g_title.fast) {
            g_title.fast = g_title.skipping;
            SetFastForward(g_title.fast);
        }
    }

    void FreePlay() {
        if (FeatureOn("free_play") && !ChoicesLeftOut()) {
            SetRam8(kFreePlay, 1);
            return;
        }
        // (off: the game's own, from the coin mode, as its settings routine has it)
        const uint8_t own = Ram8(kCoinMode) == kCoinModeFreePlay ? 1 : 0;
        if ((Ram8(kFreePlay) != 0) != (own != 0)) {
            SetRam8(kFreePlay, own);
        }
    }

    // The cheats work in the board's own game and in training mode; the other modes leave them out (their scores are kept)
    bool CheatsWork() {
        return !ChoicesLeftOut() || shmup::Practice();
    }

    bool InGame() {
        return Ram16(kStep) == kStepGame && Ram8(kGameOn) == 1;
    }

    bool PlayerIn(int n) {
        return InGame() && Ram8(Player(n) + kPlayerState) != 0;
    }

    const char *const kInvincible[kPlayers2] = {"invincible", "invincible_2"};

    // A ship is out of harm: the cheat, or a mode running the game unseen
    bool Invincible(int n) {
        return (FeatureOn(kInvincible[n]) && CheatsWork()) || shmup::Protected();
    }

    // 0292E4: a ship (A6) hit with no barrier: with invincibility it goes on unhurt, as with the board's own (the shot
    // or the enemy is spent as the game had it)
    bool ShipHit(M68K &c) {
        for (int n = 0; n < kPlayers2; n++) {
            if (c.a[6] == Ship(n) && Invincible(n)) {
                c.pc = kShipUnhurt;
                return true;
            }
        }
        return false;
    }

    // 029FDE: a bomb is taken: not with infinite bombs (the bomb goes off all the same)
    bool BombTaken(M68K &c) {
        if (FeatureOn("infinite_bombs") && CheatsWork()) {
            c.pc = kBombTaken + 4;
            return true;
        }
        return false;
    }

    // 02EDBE: the choice of character is over and the stage is about to be looked up from its count (0): a later count
    // starts the game there
    bool GameBegins(M68K &) {
        const int stage = shmup::WantedStage() >= 0 ? shmup::WantedStage() : ChoicesLeftOut() ? 0 : FeatureValue("start_stage");
        if (stage > 0 && stage < kStages) {
            SetRam16(kStageCount, static_cast<uint16_t>(stage));
        }
        return false;
    }

    void Cheats() {
        if (!InGame() || Ram16(kScene) != kScenePlay) {
            return;
        }
        for (int n = 0; n < kPlayers2; n++) {
            const uint32_t ships = Player(n) + kPlayerShips;
            if (FeatureOn("infinite_ships") && PlayerIn(n) && Ram8(ships) < 2) {
                SetRam8(ships, 2);
            }
        }
    }

    void Frame(Machine &machine) {
        BoardFrame(machine);
        ModesFrame(machine);
        FreePlay();
        if (CheatsWork()) {
            Cheats(); // (free play and the cheats are not the board's own game)
        }
        TitleFrame(machine);
    }

    void State(Archive &ar) {
        ModesState(ar);
        ar.Mark(0x44494D41); // DIMA
        ar.Pod(g_title);
        BoardState(ar);
    }
} // namespace

void RegisterGame() {
    Game &game = TheGame();
    game.vertical = true; // (the monitor stood on end: the port shows the picture upright)
    RegisterMusic();
    RegisterBoard();
    game.features.push_back({"free_play", "Game", "Free play", "Start works without a coin (the board's own free play).", kFeatureEnhanced});
    game.features.push_back({"skip_intro", "Game", "Straight to the title",
                             "When the game starts, and when it is reset, goes straight to the title: the board's start, its warning, "
                             "the logos and the story are run unseen.",
                             0});
    game.features.push_back({"infinite_ships", "Cheats", "Infinite ships", "A player who loses a ship always has another."});
    game.features.push_back({"infinite_bombs", "Cheats", "Infinite bombs", "A bomb goes off without being used up."});
    game.features.push_back({"invincible", "Cheats", "Invincibility", "Player 1 is never hit."});
    game.features.push_back({"invincible_2", "Cheats", "Invincibility, player 2", "Player 2 is never hit."});
    game.features.push_back({"start_stage", "Game", "Start at stage", "A new game starts at the beginning of the stage chosen.", 0,
                             {"Stage 1", "Stage 2", "Stage 3", "Stage 4", "Stage 5", "Stage 6"}});
    RegisterModes(); // (after the cheats: the hit boxes come after them on the Cheats page)
    AddHook(HookAt(kTitleShown), "TitleShown", TitleShown);
    AddHook(HookAt(kGameBegins), "GameBegins", GameBegins);
    AddHook(HookAt(kShipHit), "ShipHit", ShipHit);
    AddHook(HookAt(kBombTaken), "BombTaken", BombTaken);
    game.beforeFrame = ModesBeforeFrame;
    game.frame = Frame;
    game.state = State;
}

} // namespace cps2recomp
