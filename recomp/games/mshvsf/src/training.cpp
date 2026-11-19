// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Marvel Super Heroes vs. Street Fighter: the training session is the toolkit's (fight/session.h); this says where the game keeps
// what it needs. What the players do (each player's step of a fight, for the frame data; a hit landing; the boxes as
// the game's hit tests read them) is told from the game's own code by the engine's shared code (fight/marvel.h), at
// the character's step (00BE56, once a step for each of the four, the first characters first). A side's
// two characters keep records of their own, and the game says which is on point (mshvsf.h), so the session follows the
// character on point; the one resting is looked after here (its health refilled with the point character's).
#include "alternative.h"
#include "fight.h"
#include "marvel.h"
#include "session.h"
#include "mshvsf.h"

namespace cps2recomp {

namespace {
    using namespace mshvsf;

    // The game's own aids: the gauge kept full, and the partner's health refilled with the point character's
    void Hud(Machine &, std::vector<HudLine> &, std::vector<HudBox> &) {
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            if (FeatureOn("gauge_refill")) {
                SetRam8(player + kGaugeLevel, kGaugeFull);
                SetRam16(player + kGaugeBar, kGaugeBarFull);
            }
            if (FeatureOn("health_refill")) {
                const uint32_t partner = PartnerOf(n);
                if (Ram16(partner + kHealth) != 0 && Ram16(partner + kHealth) < kHealthFull) {
                    SetRam16(partner + kHealth, kHealthFull);
                    SetRam16(partner + kRedHealth, kHealthFull);
                }
            }
        }
    }
} // namespace

void RegisterTraining() {
    session::Description d;
    d.player1 = kPlayer1;
    d.playerAt = Player;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.xLong = true;
    d.health = kHealth;
    d.healthCeiling = kRedHealth;
    d.healthFull = kHealthFull;
    d.clock = kRoundClock;
    d.clockFull = 0x99; // (in BCD: a round is being fought while it is 1 to 99)
    d.cameraXAt = marvel::CameraXAt;
    d.cameraYAt = marvel::CameraYAt;
    d.ends = marvel::Ends;
    // (no start places: "Start over" takes the round's own, which are not the same on every stage)
    d.pictureFloor = marvel::kFloorLine;
    d.fighting = [] {
        const uint8_t clock = Ram8(kRoundClock);
        return Ram8(kFight) == kFighting && clock > 0 && clock <= 0x99;
    };
    d.chosen = [](int n) { return Ram8(kChoosing + static_cast<uint32_t>(n) * kChoosingSize) >= kChoosingDone; };
    d.panelHeld = marvel::PanelHeld;
    d.facingLeft = marvel::FacingLeft;
    d.moveStart = marvel::MoveStart;
    d.boxes = marvel::Boxes;
    d.wideStage = marvel::WideStage;
    d.wideSide = marvel::WideSide;
    d.wideMargin = marvel::WideMargin;
    d.wideEnds = marvel::WideEnds;
    d.stageChoices = modes::StageChoices({"The game's choice"});
    d.hud = Hud;
    d.ownAids = {"gauge_refill"};
    d.dummy.downAfter = 34;
    Game &game = TheGame();
    game.features.push_back({"gauge_refill", modes::kTraining, "Gauge refills", "Both players' hyper combo gauge kept full."});
    session::Register(d);
    AddHook(HookAt(0x00BE56), "PlayerStep", marvel::PlayerStep);
}

} // namespace cps2recomp
