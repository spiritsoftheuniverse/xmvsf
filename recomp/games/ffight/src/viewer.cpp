// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Final Fight: the Stage viewer and Model viewer (beat/beat.h).
#include "beat.h"
#include "ffight.h"
#include "lift.h"
#include "machine.h"

#include <cstdint>
#include <vector>

namespace cps2recomp {

namespace {
    using namespace ffight;

    struct StageInfo {
        const char *name;
        uint8_t stage;
        uint8_t area;
        uint8_t progression;
        uint8_t bonus;
        std::vector<beat::Description::Place> way;
    };

    const StageInfo kViewed[] = {
        {"1-1 Slum (Street)", 0, 0, 0, 0, {{0, 0}, {1600, 0}}},
        {"1-2 Slum (Subway Entrance)", 0, 1, 0, 0, {{0, 0}, {1200, 0}}},
        {"1-3 Slum (Boss: Damnd)", 0, 2, 0, 0, {{0, 0}, {800, 0}}},
        {"2-1 Subway (Train)", 1, 0, 1, 0, {{0, 0}, {1200, 0}}},
        {"2-2 Subway (Alley)", 1, 1, 1, 0, {{0, 0}, {1200, 0}}},
        {"2-3 Subway (Park)", 1, 2, 1, 0, {{0, 0}, {1200, 0}}},
        {"2-4 Subway (Boss: Sodom)", 1, 3, 1, 0, {{0, 0}, {600, 0}}},
        {"Bonus 1: Smash the Car", 6, 0, 2, 1, {{0, 0}}},
        {"3-1 West Side (Street)", 2, 0, 3, 0, {{0, 0}, {1200, 0}}},
        {"3-2 West Side (Bar)", 2, 1, 3, 0, {{0, 0}, {1200, 0}}},
        {"3-3 West Side (Boss: Edi.E)", 2, 2, 3, 0, {{0, 0}, {800, 0}}},
        {"4-1 Industrial Area", 3, 0, 4, 0, {{0, 0}, {1600, 0}}},
        {"4-2 Industrial Area (Boss: Rolento)", 3, 1, 4, 0, {{0, 0}, {800, 0}}},
        {"Bonus 2: Break the Glass", 7, 0, 5, 1, {{0, 0}}},
        {"5-1 Bay Area (Boss: Abigail)", 4, 0, 6, 0, {{0, 0}, {2400, 0}}},
        {"6-1 Up Town (Hotel)", 5, 0, 7, 0, {{0, 0}, {1200, 0}}},
        {"6-2 Up Town (Elevator)", 5, 1, 7, 0, {{0, 0}, {800, 0}}},
        {"6-3 Up Town (Boss: Belger)", 5, 2, 7, 0, {{0, 0}, {800, 0}}},
    };

    int g_x = -1;
    int g_y = -1;
    int g_playing = 0;

    bool StandStill(M68K &c) {
        if (!beat::Viewing() || beat::Starting()) {
            return false;
        }
        Return(c);
        return true;
    }

    bool ToStage(Machine &machine, int stage, int32_t &frames) {
        const StageInfo &want = kViewed[stage];
        if (Ram16(kStep) == 0) {
            g_playing = 0;
            if (Ram16(kShow) >= kShowStory1 && Ram8(kCredits) == 0 && frames < 2000 && !(frames & 0x100)) {
                SetRam8(kCredits, 1);
            }
            machine.players[0] = (frames & 4) ? ((frames & 8) ? kStart : kPunch1) : 0;
            return false;
        }
        const uint16_t step = Ram16(kStep);
        if ((step != 6 && step != 0xE) || Ram8(kStage) != want.stage || Ram8(kArea) != want.area) {
            g_playing = 0;
            return false;
        }
        return ++g_playing > 60;
    }

    void View(Machine &, int, int x, int y) {
        g_x = x;
        g_y = y;
        if (beat::StageViewer() && g_x >= 0) {
            SetRam16(kCameraX, static_cast<uint16_t>(g_x));
            if (g_y >= 0) {
                SetRam16(kCameraY, static_cast<uint16_t>(g_y));
            }
        }
    }
} // namespace

bool ViewerGameBegins() {
    const StageInfo &want = kViewed[beat::ViewedStage()];
    SetRam8(kStage, want.stage);
    SetRam8(kArea, want.area);
    SetRam8(kProgression, want.progression);
    SetRam8(kBonusFlag, want.bonus);
    return false;
}

void DescribeViewer(beat::Description &d) {
    AddHook(0x0057B4, "StandStillEnemies", StandStill);
    AddHook(0x0059DC, "StandStillItems", StandStill);

    for (const StageInfo &s : kViewed) {
        d.stages.push_back(s.name);
    }
    d.toStage = ToStage;
    d.stagePath = [](int stage, std::vector<beat::Description::Place> &way) { way = kViewed[stage].way; };
    d.stageCamera = [](int &x, int &y) {
        x = static_cast<int16_t>(Ram16(kCameraX));
        y = static_cast<int16_t>(Ram16(kCameraY));
    };
    d.view = View;
    d.viewLayers = 0xD;
}

} // namespace cps2recomp
