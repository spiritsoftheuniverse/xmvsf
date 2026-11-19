// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the training session is the toolkit's (fight/session.h); this says where the game keeps what
// it needs, and tells it from the game's own code what the players do: each player's step of a fight (frame data), a
// hit landing or guarded, and the hit boxes as the game's hit tests read them. The game's own aids beside the toolkit's:
// the super combo gauge kept full, and each player's stun against how much stuns them.
#include "fight.h"
#include "alternative.h"
#include "session.h"
#include "sfa.h"

#include <cstdio>
#include <cstdlib>
#include <iterator>

namespace cps2recomp {

void CourtStep(uint32_t player); // court.cpp

bool RingOutStep(M68K &c); // battle.cpp

namespace {
    using namespace sfa;

    constexpr uint16_t kGaugeFull = 0x90; // three levels of 0x30 (0217A8: the most the gauge takes)
    constexpr uint32_t kStunMost = 0x13A; // byte: the stun at which the player is stunned (00F922)

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // A round's fighting, for the aids: both players in it and nobody over the most health
    bool Fighting() {
        if (!modes::RoundFought()) {
            return false;
        }
        for (int n = 0; n < 2; n++) {
            if (static_cast<int16_t>(Ram16(Player(n) + kHealth)) > kHealthFull || Ram8(Player(n) + kState) != 2) {
                return false;
            }
        }
        return true;
    }

    // 01CE48 (a person's) and 021DF4 (the computer's): a player's fight step, where it goes on to whatever is in charge
    // of the player; once a step for each player. Its record is as the last step left it, which is what the picture
    // showed and the hit tests tested.
    bool PlayerStep(M68K &c) {
        if (RingOutStep(c)) { // (the ring out's own movements: battle.cpp)
            Return(c);
            return true;
        }
        CourtStep(c.a[6] & 0xFFFFFF); // (what the court's ball has done to it: court.cpp)
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n < 0 || Ram8(player + kState) != 2 || !modes::RoundFought()) {
            return false;
        }
        fight::StepView view;
        const uint8_t control = Ram8(player + kControl);
        view.control = control == kControlOwn       ? fight::StepView::Own :
                       control == kControlReeling ? fight::StepView::Reeling :
                                                    fight::StepView::Held;
        view.frozen = Ram8(player + kHitStop) != 0;
        view.canAct = CanAct(player);
        view.attacking = Ram8(player + kAction) == kActionAttack;
        view.jumping = Ram8(player + kAction) == kActionJump;
        const uint32_t frame = Ram32(player + kFrame);
        view.attackOut = frame < 0x400000 && c.Read(frame + kFrameAttack, 0) != 0;
        session::Step(n, view);
        return false;
    }

    // 00F638: an attack has reached the other player (A6 the attacker, A1 the one reached), and 00F646 has made it a hit
    // or a guarded one (00F792: kGuardedHit FF)
    bool HitTaken(M68K &c) {
        const int victim = PlayerOf(c.a[1]);
        if (victim >= 0) {
            session::Contact(victim, Ram8((c.a[1] & 0xFFFFFF) + kGuardedHit) == 0xFF);
        }
        return false;
    }

    // What the game read off the panel for a player this frame, as Button bits
    uint16_t PanelHeld(int n) {
        const uint16_t word = Ram16(kControls[n]);
        uint16_t held = word & 0x0F;
        held |= static_cast<uint16_t>(((word >> 8) & 0x07) << 4);  // punches
        held |= static_cast<uint16_t>(((word >> 12) & 0x07) << 7); // kicks
        return held;
    }

    fight::MoveLog::Start MoveStart(int n) {
        const uint32_t player = Player(n);
        const uint8_t who = Ram8(player + kCharacter);
        fight::MoveLog::Start start;
        start.character = who < kCharacters ? kCharacterNames[who] : "?";
        start.input = fight::Notation(PanelHeld(n), Ram8(player + kFacesRight) == 0);
        start.air = Ram8(player + kAction) == kActionJump || Ram16(player + kY) != kGround;
        start.id = Ram32(player + kFrame); // (the move's first animation frame)
        return start;
    }

    // A box of a player's (four bytes: its middle's offsets across and up, half its width and height), placed in the
    // stage as 0104B8 places it
    void AddBox(M68K &c, std::vector<fight::StageBox> &boxes, fight::BoxKind kind, uint32_t player, uint32_t box, bool thrown = false) {
        const int8_t across = static_cast<int8_t>(c.Read(box, 0)), up = static_cast<int8_t>(c.Read(box + 1, 0));
        const int halfWidth = static_cast<int>(c.Read(box + 2, 0)), halfHeight = static_cast<int>(c.Read(box + 3, 0));
        if (halfWidth == 0 || halfHeight == 0) {
            return;
        }
        const int x = static_cast<int16_t>(Ram16(player + kX)) + (Ram8(player + kFacesRight) ? -across : across);
        const int y = static_cast<int16_t>(Ram16(player + kY)) + up;
        boxes.push_back({kind, x, y, halfWidth, halfHeight, thrown});
    }

    std::vector<fight::StageBox> Boxes(Machine &machine) {
        M68K &c = machine.cpu;
        std::vector<fight::StageBox> boxes;
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            const uint32_t frame = Ram32(player + kFrame);
            if (Ram8(player + kState) != 2 || frame >= 0x400000) {
                continue;
            }
            if (const uint8_t push = static_cast<uint8_t>(c.Read(frame + kFramePush, 0))) {
                AddBox(c, boxes, fight::BoxKind::Push, player, Ram32(player + kPushBoxes) + push * kBoxSize);
            }
            constexpr struct {
                uint32_t number, table;
            } kHurt[] = {{kFrameHead, kHeadBoxes}, {kFrameBody, kBodyBoxes}, {kFrameLegs, kLegBoxes}};
            for (const auto &hurt : kHurt) {
                if (const uint8_t number = static_cast<uint8_t>(c.Read(frame + hurt.number, 0))) {
                    AddBox(c, boxes, fight::BoxKind::Hurt, player, Ram32(player + hurt.table) + number * kBoxSize);
                }
            }
            if (const uint8_t attack = static_cast<uint8_t>(c.Read(frame + kFrameAttack, 0))) {
                AddBox(c, boxes, fight::BoxKind::Attack, player, Ram32(player + kAttackBoxes) + attack * kAttackBoxSize);
            }
        }
        // What the players throw (the pool from FF9000, records of a player's layout but shorter): the frame names the
        // attack (+0B), in a table the record's long +50 points at, from the word at its +2 on (the thrown things' hit
        // test, 01018A)
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t thrown = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            const uint32_t frame = Ram32(thrown + kFrame), table = Ram32(thrown + kThrownTables);
            if (Ram8(thrown) != 1 || frame >= 0x200000 || table >= 0x200000) {
                continue;
            }
            if (const uint8_t attack = static_cast<uint8_t>(c.Read(frame + kFrameAttack, 0))) {
                const uint32_t attacks = table + static_cast<uint16_t>(c.Read(table + 2, 1));
                AddBox(c, boxes, fight::BoxKind::Attack, thrown, attacks + attack * kAttackBoxSize, true);
            }
        }
        return boxes;
    }

    // The game's own readouts and aids: the gauge kept full, and each player's stun against how much stuns it
    void Hud(Machine &, std::vector<HudLine> &lines, std::vector<HudBox> &) {
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            if (FeatureOn("gauge_refill") && Ram16(player + kSpecialGauge) < kGaugeFull) {
                SetRam16(player + kSpecialGauge, kGaugeFull);
            }
            if (FeatureOn("stun_readout")) {
                char text[32];
                snprintf(text, sizeof text, "stun %d/%d", Ram8(player + kStun), Ram8(player + kStunMost));
                lines.push_back({n == 0 ? 0.02f : 0.98f, 0.12f, n == 0 ? -1 : 1, text});
            }
        }
    }

    // The dummy: the game knows when it is knocked down (a hit that knocks down marks the one it knocks down)
    void DummyView(Machine &, fight::DummyView &view) {
        view.knowsDown = true;
        view.down = Ram8(Player(1) + kControl) == kControlReeling && Ram8(Player(1) + kKnocksDown) != 0;
    }
} // namespace

void RegisterTraining() {
    session::Description d;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.health = kHealth;
    d.healthFull = kHealthFull;
    d.healthShown = kHealthShown;
    d.clock = kRoundClock;
    d.clockFull = 99;
    d.cameraX = kCameraX;
    d.cameraY = kCameraY;
    d.cameraCopies.assign(std::begin(kCameraCopies), std::end(kCameraCopies));
    d.startX[0] = kStartX[0];
    d.startX[1] = kStartX[1];
    d.stageLeft = kStageLeft;
    d.stageRight = kStageRight;
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.cornerApart = 100;
    d.fighting = Fighting;
    d.mayRefill = [](int n) { return CanAct(Player(n)); };
    d.chosen = [](int n) { return (Ram8(kPlayersChosen) & (1 << n)) != 0; };
    d.panelHeld = PanelHeld;
    d.facingLeft = [](int n) { return Ram8(Player(n) + kFacesRight) == 0; };
    d.moveStart = MoveStart;
    d.boxes = Boxes;
    d.dummyView = DummyView;
    d.wideStage = WideStage;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideEnds = WideEnds;
    d.stageChoices = modes::StageChoices({"The game's choice"});
    d.stageWords = "Where the fights are, whatever is played (the game's own games, training, the other modes): the "
                   "game's own choice (the stage of the second player's character) or a stage named here. Takes "
                   "effect when the characters are next chosen.";
    d.inputWords = "What each player presses and for how many frames: the stick as a keypad number from the player's side (6 "
                   "forward, 4 back, 2 down), then the buttons.";
    d.hud = Hud;
    d.ownAids = {"gauge_refill", "stun_readout"};
    Game &game = TheGame();
    game.features.push_back({"gauge_refill", modes::kTraining, "Gauge refills", "Both players' super combo gauge kept full (three levels)."});
    game.features.push_back({"stun_readout", modes::kTraining, "Stun readout",
                             "Under each player's bar: the stun built up, against how much stuns the player (it drains when they "
                             "are left alone for a while)."});
    session::Register(d);
    AddHook(HookAt(kPlayerDispatch), "PlayerStep", PlayerStep);
    AddHook(HookAt(kComputerDispatch), "ComputerStep", PlayerStep);
    AddHook(HookAt(kHitTaken), "HitTaken", HitTaken);
}

} // namespace cps2recomp
