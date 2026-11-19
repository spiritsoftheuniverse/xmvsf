// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Final Fight: game-side features (free play, skip intro, stage select, cheats, custom soundtrack, and beat-'em-up
// modes).
#include "beat.h"
#include "ffight.h"
#include "game.h"
#include "lift.h"
#include "machine.h"
#include "state.h"

#include <algorithm>
#include <cstdint>

namespace cps2recomp {

void RegisterMusic(Game &g);
bool ChoicesLeftOut();
void ModesBeforeFrame(Machine &machine);
void ModesFrame(Machine &machine);
void ModesState(Archive &ar);
void RegisterModes();
bool ViewerGameBegins();

namespace {
    using namespace ffight;

    bool g_titleReached = false;
    constexpr uint32_t kBootFrames = 120; // (the board's start, which works out the switches)

    // Progression pairs matching 0x54FC (stage, progression, bonus flag)
    struct StartChoice {
        uint8_t stage;
        uint8_t progression;
        uint8_t bonus;
    };
    // (start_stage's choices: the first is the board's own start, Stage 1)
    constexpr StartChoice kStartChoices[] = {
        {0, 0, 0}, // Stage 1: Slum
        {1, 1, 0}, // Stage 2: Subway
        {2, 3, 0}, // Stage 3: West Side
        {3, 4, 0}, // Stage 4: Industrial Area
        {4, 6, 0}, // Stage 5: Bay Area
        {5, 7, 0}, // Stage 6: Up Town
        {6, 2, 1}, // Bonus 1: Car
        {7, 5, 1}, // Bonus 2: Glass
    };

    bool OnBootWarning(M68K &c) {
        if (!ChoicesLeftOut() && FeatureOn("skip_intro")) {
            Return(c);
            return true;
        }
        return false;
    }

    bool OnGameBegins(M68K &) {
        if (beat::Viewing()) {
            return ViewerGameBegins();
        }
        if (ChoicesLeftOut()) {
            return false;
        }
        const int choice = FeatureValue("start_stage");
        if (choice > 0 && choice < static_cast<int>(std::size(kStartChoices))) {
            const StartChoice &s = kStartChoices[choice];
            SetRam8(kStage, s.stage);
            SetRam8(kArea, 0);
            SetRam8(kProgression, s.progression);
            SetRam8(kBonusFlag, s.bonus);
        }
        return false;
    }

    void BeforeFrame(Machine &machine) {
        ModesBeforeFrame(machine);

        if (machine.testSwitch || ChoicesLeftOut()) {
            return;
        }

        if (FeatureOn("free_play")) {
            machine.dip[2] &= ~0x04;
        }
        // (the board works out its free play from the switch only as it starts: put in what the switch now says, so a
        // choice made later counts at once)
        if (machine.frame > kBootFrames) {
            SetRam8(kFreePlay, (machine.dip[2] & 0x04) ? 0 : 1);
        }

        const uint16_t step = Ram16(kStep);
        if (step == 6 || step == 0xE) {
            if (FeatureOn("infinite_health")) {
                for (int p = 0; p < 2; p++) {
                    const uint32_t base = Player(p);
                    if (Ram8(base + kInPlay)) {
                        const uint16_t maxHp = Ram16(base + kMaxHealth);
                        if (maxHp > 0) {
                            SetRam16(base + kHealth, maxHp);
                            SetRam16(base + kRedHealth, maxHp);
                        }
                    }
                }
            }

            if (FeatureOn("infinite_lives")) {
                const uint8_t minLives = std::max<uint8_t>(Ram8(kInitialLives), 1);
                for (int p = 0; p < 2; p++) {
                    const uint32_t base = Player(p);
                    if (Ram8(base + kInPlay) && Ram8(base + kLives) < minLives) {
                        SetRam8(base + kLives, minLives);
                    }
                }
            }

            if (FeatureOn("infinite_time")) {
                SetRam16(kClockTicks, 0);
            }
        }
    }

    void Frame(Machine &machine) {
        ModesFrame(machine);

        if (machine.frame <= 1) {
            g_titleReached = false; // (the board started again: its story is skipped again too)
        }
        if (!g_titleReached && Ram16(kStep) == 0) {
            const uint16_t show = Ram16(kShow);
            if (show == kShowTitle) {
                g_titleReached = true;
            } else if (!ChoicesLeftOut() && FeatureOn("skip_intro") && show == kShowStory1) {
                SetRam16(kShow, kShowTitle);
                SetRam16(kShowSub, 0);
                g_titleReached = true;
            }
        }
    }

    void State(Archive &ar) {
        ar.Pod(g_titleReached);
        ModesState(ar);
    }
} // namespace

void RegisterGame() {
    Game &g = TheGame();
    RegisterMusic(g);

    g.features.push_back({"free_play", "Game", "Free play", "Start works without a coin (the board's own free play).", kFeatureEnhanced});
    g.features.push_back({"skip_intro", "Game", "Straight to the title",
                          "When the game starts, and when it is reset, goes straight to the title: the board's warning and "
                          "the story are left out."});
    g.features.push_back({"start_stage", "Game", "Start at stage", "A new game starts at the beginning of the stage chosen.", 0,
                          {"Stage 1: Slum", "Stage 2: Subway", "Stage 3: West Side", "Stage 4: Industrial Area",
                           "Stage 5: Bay Area", "Stage 6: Up Town", "Bonus 1: Car", "Bonus 2: Glass"}});
    g.features.push_back({"infinite_health", "Cheats", "Infinite health", "No player's health goes down."});
    g.features.push_back({"infinite_lives", "Cheats", "Infinite lives", "A player who goes down always has another life."});
    g.features.push_back({"infinite_time", "Cheats", "Infinite time", "The stage's clock stands still."});

    AddHook(0x000E64, "OnBootWarning", OnBootWarning);
    AddHook(0x004C54, "OnGameBegins", OnGameBegins);
    RegisterModes();

    g.beforeFrame = BeforeFrame;
    g.frame = Frame;
    g.state = State;
    OwnFeatures({"start_stage"});
}

} // namespace cps2recomp
