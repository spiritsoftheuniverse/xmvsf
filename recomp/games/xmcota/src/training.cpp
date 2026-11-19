// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// X-Men: Children of the Atom: the training session is the toolkit's (fight/session.h); this says where the game keeps
// what it needs. What the players do (each player's step of a fight, for the frame data; a hit landing; the boxes as the
// game's hit tests read them) is told from the game's own code by the engine's shared code (fight/marvel.h), at the
// player's step (006EB0). The game's own aid is here: the X-Power gauge kept full.
#include "alternative.h"
#include "fight.h"
#include "marvel.h"
#include "session.h"
#include "xmcota.h"

namespace cps2recomp {

namespace {
    using namespace xmcota;

    // The X-Power gauge: a word, 0x8E full (the FBNeo training script's meter and its most)
    constexpr uint32_t kGauge = 0x194;
    constexpr uint16_t kGaugeFull = 0x8E;

    void Hud(Machine &, std::vector<HudLine> &, std::vector<HudBox> &) {
        if (FeatureOn("gauge_refill")) {
            for (int n = 0; n < 2; n++) {
                SetRam16(Player(n) + kGauge, kGaugeFull);
            }
        }
    }
} // namespace

void RegisterTraining() {
    session::Description d;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.xLong = true;
    d.health = kHealth;
    d.healthCeiling = kHealthTop;
    d.healthFull = kHealthFull;
    d.clock = kRoundClock;
    d.clockFull = 0x99; // (in BCD: a round is being fought while it is 1 to 99)
    d.cameraXAt = marvel::CameraXAt;
    d.cameraYAt = marvel::CameraYAt;
    d.ends = marvel::Ends;
    // (no start places: "Start over" takes the round's own)
    d.pictureFloor = marvel::kFloorLine;
    d.fighting = [] {
        const uint8_t clock = Ram8(kRoundClock);
        return Ram8(kFight) == kFighting && clock > 0 && clock <= 0x99;
    };
    d.chosen = [](int n) { return (Ram8(kStillChoosing) & (1u << n)) == 0; };
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
    TheGame().features.push_back({"gauge_refill", modes::kTraining, "Gauge refills", "Both players' X-Power gauge kept full."});
    session::Register(d);
    AddHook(HookAt(0x006EB0), "PlayerStep", marvel::PlayerStep);
}

} // namespace cps2recomp
