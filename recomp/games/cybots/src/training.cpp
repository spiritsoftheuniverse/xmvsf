// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Cyberbots: the training session is the toolkit's (fight/session.h); this says where the game keeps what it needs,
// and tells it from the game's own code what the players do: each player's step of a fight (frame data), a hit landing
// or guarded, and the boxes as the FBNeo training script reads them. As Night Warriors'.
#include "fight.h"
#include "alternative.h"
#include "battle.h"
#include "cybots.h"
#include "session.h"

#include <iterator>

namespace cps2recomp {

bool RingOutStep(M68K &c); // battle.cpp

namespace {
    using namespace cybots;

    int PlayerOf(uint32_t record) {
        record &= 0xFFFFFF;
        return record == Player(0) ? 0 : record == Player(1) ? 1 : -1;
    }

    // The player is in the fight (the record's first byte, 1 while it is in use: 003A52 sets it as a round is made)
    bool InFight(uint32_t player) {
        return Ram8(player) != 0 && Ram8(kInGame) != 0;
    }

    // A box of the record's (cybots.h): kind `which` (0 attacks, 1-3 head, body and legs, 4 push), placed in the stage
    void AddBox(M68K &c, std::vector<fight::StageBox> &boxes, fight::BoxKind kind, uint32_t record, int which, bool thrown) {
        const uint8_t box = Ram8(record + kBoxIds + static_cast<uint32_t>(which));
        if (!box) {
            return;
        }
        const uint32_t tables = Ram32(record + kBoxTables);
        if (tables < 0x1000 || tables >= 0x400000) {
            return;
        }
        const uint32_t at = tables + static_cast<uint32_t>(static_cast<int16_t>(c.Read(tables + static_cast<uint32_t>(which) * 2, 1))) +
                            box * (which == 0 ? 16u : 8u);
        const int16_t across = static_cast<int16_t>(c.Read(at, 1));
        const int x = static_cast<int16_t>(Ram16(record + kX)) + (Ram8(record + kFacesRight) ? -across : across);
        const int y = static_cast<int16_t>(Ram16(record + kY)) + static_cast<int16_t>(c.Read(at + 2, 1)) - kGround;
        const int halfWidth = static_cast<int16_t>(c.Read(at + 4, 1)), halfHeight = static_cast<int16_t>(c.Read(at + 6, 1));
        if (halfWidth > 0 && halfHeight > 0) {
            boxes.push_back({kind, x, y, halfWidth, halfHeight, thrown});
        }
    }

    std::vector<fight::StageBox> Boxes(Machine &machine) {
        M68K &c = machine.cpu;
        std::vector<fight::StageBox> boxes;
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            if (!InFight(player)) {
                continue;
            }
            AddBox(c, boxes, fight::BoxKind::Push, player, 4, false);
            for (int which = 1; which <= 3; which++) {
                AddBox(c, boxes, fight::BoxKind::Hurt, player, which, false);
            }
            AddBox(c, boxes, fight::BoxKind::Attack, player, 0, false);
        }
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t thrown = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            if (Ram8(thrown + 1) & 1) {
                AddBox(c, boxes, fight::BoxKind::Hurt, thrown, 1, true);
                AddBox(c, boxes, fight::BoxKind::Attack, thrown, 0, true);
            }
        }
        return boxes;
    }

    // The player is free to do anything: standing, walking or crouching
    bool CanAct(uint32_t player) {
        const uint8_t sub = Ram8(player + kSub);
        return Ram8(player + kState) == kFree && (sub == kSubStand || sub == kSubCrouch);
    }

    // 019D70: a player's step of a fight (A6), once a frame from 019CF8 (the return address one of its own: the step's
    // own states go back to 019D70 by jmp, which is not another step); seen as it starts, the record is as the last
    // step left it, which is what the picture showed
    bool PlayerStep(M68K &c) {
        if (RingOutStep(c)) { // (the ring out's own movements: battle.cpp)
            Return(c);
            return true;
        }
        const uint32_t back = c.Read(c.a[7], 2) & 0xFFFFFF;
        if (back < 0x019D28 || back > 0x019D58) {
            return false;
        }
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n < 0 || !InFight(player)) {
            return false;
        }
        fight::StepView view;
        const uint8_t state = Ram8(player + kState), sub = Ram8(player + kSub);
        const bool busy = state == kBusy;
        view.control = !busy || sub == kSubAttack || sub == kSubSpecial ? fight::StepView::Own :
                       sub == kSubReel || sub == kSubDown               ? fight::StepView::Reeling :
                                                                          fight::StepView::Held;
        view.frozen = Ram8(kHitFreeze) != 0;
        view.canAct = CanAct(player);
        view.attacking = busy && (sub == kSubAttack || sub == kSubSpecial);
        view.jumping = InAir(player);
        view.attackOut = Ram8(player + kBoxIds) != 0;
        session::Step(n, view);
        return false;
    }

    // 0197AE: the computer's play of a player (A6) runs this frame. A player handed to the computer in the middle of a
    // fight (the modes do: battle.h) has no script yet, which the game gives only as a round is set up; with none, the
    // computer does nothing. It is given one, as the game would have.
    // (only in those modes: the game's own show has players of its own with none, which it plays by other means)
    bool ComputerPlays(M68K &c) {
        const uint32_t player = c.a[6] & 0xFFFFFF;
        if (BattleMode() && PlayerOf(player) >= 0 && Ram8(player + kComputer) && !Ram32(player + kComputerScript)) {
            Call(c, kComputerStarts);
        }
        return false;
    }

    // 0463B8: a hit has reached the player (A6), its damage worked out: guarded when +130 is set
    bool HitTaken(M68K &c) {
        const int victim = PlayerOf(c.a[6]);
        if (victim >= 0) {
            session::Contact(victim, Ram8((c.a[6] & 0xFFFFFF) + kGuarded) != 0);
        }
        return false;
    }

    // What the game read off the panel for a player this frame, as Button bits (the four buttons as the first four)
    uint16_t PanelHeld(int n) {
        const uint16_t word = Ram16(kControls[n]);
        uint16_t held = word & 0x0F;
        held |= static_cast<uint16_t>(((word >> 4) & 0x07) << 4);
        held |= static_cast<uint16_t>(((word >> 8) & 0x01) << 7);
        return held;
    }

    fight::MoveLog::Start MoveStart(int n) {
        const uint32_t player = Player(n);
        const uint8_t who = Ram8(player + kMachine);
        fight::MoveLog::Start start;
        start.character = who < kMachines ? kMachineNames[who] : "?";
        start.input = fight::Notation(PanelHeld(n), Ram8(player + kFacesRight) == 0);
        start.air = InAir(player);
        start.id = Ram32(player + 0x1C);
        return start;
    }
} // namespace

void RegisterTraining() {
    // (the input display's and the frame data log's names for the four buttons: Low attack, High attack, Boost, Weapon)
    static const char *const kButtonNames[] = {"LA", "HA", "B", "W", nullptr, nullptr};
    fight::SetButtonNames(kButtonNames);
    session::Description d;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.health = kHealth;
    d.healthFull = kHealthFull;
    d.healthShown = kHealthShown;
    d.clock = kRoundClock;
    d.clockFull = 0x99; // (the clock is BCD: 99 is 0x99)
    d.fighting = [] {
        return Ram8(kInGame) != 0 && Ram16(kMode) == kGameRunning && Ram16(kStep) == kGameMatch && Ram16(kStep2) == kRoundFought;
    };
    d.cameraXAt = CameraXAt;
    d.cameraYAt = CameraYAt;
    d.startX[0] = kStartX[0];
    d.startX[1] = kStartX[1];
    d.stageLeft = kStageLeft;
    d.stageRight = kStageRight;
    d.cameraLeft = kCameraLeft;
    d.cameraRight = kCameraRight;
    d.chosen = [](int n) { return Ram8(Player(n) + kChosen) != 0; };
    d.panelHeld = PanelHeld;
    d.facingLeft = [](int n) { return Ram8(Player(n) + kFacesRight) == 0; };
    d.moveStart = MoveStart;
    d.boxes = Boxes;
    d.wideStage = WideStage;
    d.wideSide = WideSide;
    d.wideMargin = WideMargin;
    d.wideEnds = WideEnds;
    d.pictureFloor = 192; // (the players' feet with the camera at its lowest: 224 - 30 + 16)
    d.stageChoices = modes::StageChoices({"The game's choice"});
    d.inputWords = "What each player presses and for how many frames: the stick as a keypad number from the player's side (6 "
                   "forward, 4 back, 2 down), then the buttons (LA low attack, HA high attack, B boost, W weapon).";
    d.dummy.downAfter = 34;
    session::Register(d);
    AddHook(HookAt(kPlayerStep), "PlayerStep", PlayerStep);
    AddHook(HookAt(kHitTaken), "HitTaken", HitTaken);
    AddHook(HookAt(kComputerPlays), "ComputerPlays", ComputerPlays);
}

} // namespace cps2recomp
