// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Progear: what the game's own sources add to the port (see PROGRESS.md): free play, straight to the title, the
// cheats, start at stage, and the modes (modes.cpp).
#include "progear.h"
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
    using namespace progear;

    // "Straight to the title": the board's start, its warning and the logos run unseen. (Kept with the state, as
    // everything that changes while the game runs: online play runs frames again from it.)
    struct Title {
        bool skipping = false, fast = false;
        int32_t frames = 0;
    } g_title;

    // A game begun at a later stage: its ships are to be started as its play begins (kept with the state)
    bool g_shipsWanted = false;

    void TitleFrame(Machine &machine) {
        // (a mode of the menu started: its start is its own, and so is running unseen)
        if (ChoicesLeftOut()) {
            g_title.skipping = false;
        } else {
            if (machine.frame <= 1 && !g_title.skipping) {
                g_title.skipping = FeatureOn("skip_intro");
                g_title.frames = 0;
            }
            const uint16_t mode = Ram16(kMode);
            const bool there = mode != kModeWarning && mode != kModeLogos;
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

    bool PlayerIn(int n) {
        const uint16_t players = Ram16(kPlayers);
        return players != 0 && !(players & (n == 0 ? kPlayer1Out : kPlayer2Out));
    }

    const char *const kInvincible[kPlayers2] = {"invincible", "invincible_2"};

    // A ship is out of harm: the cheat, or a mode running the game unseen
    bool Invincible(int n) {
        return (FeatureOn(kInvincible[n]) && CheatsWork()) || shmup::Protected();
    }

    // 0393DC: `bclr #4,(a6)`, a ship (A6) hit this frame (by a body, 04E03C, or a shot, 040B48), then its blast: with
    // invincibility the hit is cleared and the ship goes on unhurt (the shot or the enemy is spent as the game had it)
    bool ShipHit(M68K &c) {
        const uint32_t ship = c.a[6];
        for (int n = 0; n < kPlayers2; n++) {
            if (ship == Ship(n) && Invincible(n)) {
                c.bus.Write8(ship, static_cast<uint8_t>(c.bus.Read8(ship) & ~0x10));
                c.pc = kShipUnhurt;
                return true;
            }
        }
        return false;
    }

    // 0396FA: a bomb is taken (A6 the ship): not with infinite bombs (the bomb goes off all the same)
    bool BombTaken(M68K &c) {
        if (FeatureOn("infinite_bombs") && CheatsWork()) {
            c.pc = kBombTaken + 4;
            return true;
        }
        return false;
    }

    // 00E9C2: Start is taken on the title, the stage still 0; the stage's tables are read with it (and its double and
    // quadruple) when it begins, after the choice of pilot and gunner
    bool GameBegins(M68K &) {
        const int stage = shmup::WantedStage() >= 0 ? shmup::WantedStage() : ChoicesLeftOut() ? 0 : FeatureValue("start_stage");
        if (stage > 0 && stage < kStages) {
            SetRam16(kStage, static_cast<uint16_t>(stage));
            SetRam16(kStage2, static_cast<uint16_t>(stage * 2));
            SetRam16(kStage4, static_cast<uint16_t>(stage * 4));
            g_shipsWanted = true;
        }
        return false;
    }

    // The first stage's take-off starts player 1's ship (05C5BA) and later stages keep the ships they have: a game begun
    // at a later stage has none, and its take-off is never over. As its play begins, each player's ship task is started as the stage's change starts it
    // (06C202: task 2 for player 1, 3 for player 2, a slot of 0x40 bytes each from FF2912, the second word the player),
    // and the ship put where that leaves it across.
    void StartShips() {
        if (!g_shipsWanted || Ram16(kMode) != kModePlay) {
            return;
        }
        g_shipsWanted = false;
        // (the take-off over: the panel, Start and the ships' hit test wait for it, 046350, 039338)
        SetRam16(kTakeOff, 0);
        for (int n = 0; n < kPlayers2; n++) {
            const uint32_t slot = kTasks + kTaskSize * static_cast<uint32_t>(kShipTask + n);
            if (!PlayerIn(n) || (Ram16(slot) & 0x8000) || (Ram16(Ship(n)) & 0x8000)) {
                continue;
            }
            SetRam16(slot, static_cast<uint16_t>(0x8000 | (kShipTask + n)));
            SetRam16(slot + 2, 0);
            SetRam32(slot + 4, static_cast<uint32_t>(n));
            SetRam32(slot + 8, 0);
            SetRam32(slot + 12, 0);
            SetRam16(Ship(n) + kShipX, kShipStartX);
        }
    }

    void Cheats() {
        if (Ram16(kMode) != kModePlay) {
            return;
        }
        for (int n = 0; n < kPlayers2; n++) {
            if (FeatureOn("infinite_ships") && PlayerIn(n) && Ram16(kShips + 2 * n) < 2) {
                SetRam16(kShips + 2 * n, 2);
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
        StartShips();
        TitleFrame(machine);
    }

    void State(Archive &ar) {
        ModesState(ar);
        ar.Mark(0x50524F47); // PROG
        ar.Pod(g_title);
        ar.Pod(g_shipsWanted);
        BoardState(ar);
    }
} // namespace

void RegisterGame() {
    Game &game = TheGame();
    RegisterMusic();
    RegisterBoard();
    game.features.push_back({"free_play", "Game", "Free play", "Start works without a coin (the board's own free play).", kFeatureEnhanced});
    game.features.push_back({"skip_intro", "Game", "Straight to the title",
                             "When the game starts, and when it is reset, goes straight to the title: the board's start, its warning and "
                             "the logos are run unseen.",
                             0});
    game.features.push_back({"infinite_ships", "Cheats", "Infinite ships", "A player who loses a ship always has another."});
    game.features.push_back({"infinite_bombs", "Cheats", "Infinite bombs", "A bomb goes off without being used up."});
    game.features.push_back({"invincible", "Cheats", "Invincibility", "Player 1's ship is never hit."});
    game.features.push_back({"invincible_2", "Cheats", "Invincibility, player 2", "Player 2's ship is never hit."});
    game.features.push_back({"start_stage", "Game", "Start at stage", "A new game starts at the beginning of the stage chosen.", 0,
                             {"Stage 1", "Stage 2", "Stage 3", "Stage 4", "Stage 5"}});
    RegisterModes(); // (after the cheats: the hit boxes come after them on the Cheats page)
    AddHook(HookAt(kGameBegins), "GameBegins", GameBegins);
    AddHook(HookAt(kShipHitTaken), "ShipHit", ShipHit);
    AddHook(HookAt(kBombTaken), "BombTaken", BombTaken);
    game.beforeFrame = ModesBeforeFrame;
    game.frame = Frame;
    game.state = State;
}

} // namespace cps2recomp
