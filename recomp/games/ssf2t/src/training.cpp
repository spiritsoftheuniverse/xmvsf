// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Super Street Fighter II Turbo: the training session is the toolkit's (fight/session.h); this says where the game keeps
// what it needs, and tells it from the game's own code what the players do: each player's step of a fight (frame data),
// a hit landing or guarded, and the hit boxes as the game's hit tests read them. The game's own aid beside the
// toolkit's: the super combo gauge kept full, and each player's stun against how much stuns them.
#include "fight.h"
#include "alternative.h"
#include "session.h"
#include "ssf2t.h"

#include <cstdio>
#include <iterator>

namespace cps2recomp {

bool RingOutStep(M68K &c); // battle.cpp

namespace {
    using namespace ssf2t;

    constexpr uint8_t kGaugeFull = 0x30; // (FBNeo's script; a super is ready at it)
    // The stun: a word of what has built up (each hit adds to it: 07F1EE), and the frames until it goes away (+5C, the
    // word before it, counted down at 077EE2); the player is stunned when it reaches a word of the table at 07F1C6, by a
    // byte that steps by two each time the player has been stunned (30, then 34, then 35)
    constexpr uint32_t kStun = 0x5E, kStunTimes = 0x2B1, kStunMost = At(0x07F1C6);

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
            if (static_cast<int16_t>(Ram16(Player(n) + kHealth)) > kHealthFull || Ram8(Player(n) + kFighting) != 2) {
                return false;
            }
        }
        return true;
    }

    // 0782D4 (a person's) and 07BC12 (the computer's): a player's fight step, where it goes on to whatever is in charge
    // of the player (kState); once a step for each player. Its record is as the last step left it, which is what the
    // picture showed and the hit tests tested.
    bool PlayerStep(M68K &c) {
        if (RingOutStep(c)) { // (the ring out's own movements: battle.cpp)
            Return(c);
            return true;
        }
        const uint32_t player = c.a[6] & 0xFFFFFF;
        const int n = PlayerOf(player);
        if (n < 0 || Ram8(player + kFighting) != 2 || !modes::RoundFought()) {
            return false;
        }
        fight::StepView view;
        const uint8_t state = Ram8(player + kState);
        const bool guarded = state == kGuard && Ram8(player + kGuarded);
        view.control = state == kReeling || guarded                                          ? fight::StepView::Reeling :
                       state <= kJump || state == kGuard || state == kNormal || state == kSpecial ? fight::StepView::Own :
                                                                                                   fight::StepView::Held;
        if (state == kGettingUp) {
            view.control = fight::StepView::Held;
        }
        view.frozen = Ram8(player + kHitStop) != 0;
        view.canAct = CanAct(player);
        view.attacking = Ram8(player + kAttacking) != 0;
        view.jumping = state == kJump;
        const uint32_t frame = Ram32(player + kFrame);
        view.attackOut = frame < 0x400000 && c.Read(frame + kFrameAttack, 0) != 0;
        session::Step(n, view);
        return false;
    }

    // 0BE678 and 0BEBDA: an attack has reached the other player (A2), and the game has made it a hit or a guarded one
    bool HitTaken(M68K &c) {
        const int victim = PlayerOf(c.a[2]);
        if (victim >= 0) {
            session::Contact(victim, Ram8((c.a[2] & 0xFFFFFF) + kGuarded) != 0);
        }
        return false;
    }

    // What the game read off the panel for a player this frame, as Button bits
    uint16_t PanelHeld(int n) {
        const uint16_t word = Ram16(kControls[n]);
        uint16_t held = word & 0x0F;
        held |= static_cast<uint16_t>(((word >> 4) & 0x07) << 4); // punches
        held |= static_cast<uint16_t>(((word >> 8) & 0x07) << 7); // kicks
        return held;
    }

    fight::MoveLog::Start MoveStart(int n) {
        const uint32_t player = Player(n);
        const uint8_t who = Ram8(player + kCharacter);
        fight::MoveLog::Start start;
        start.character = IsAkuma(player) ? "Akuma" : who < kCharacters ? kCharacterNames[who] : "?";
        start.input = fight::Notation(PanelHeld(n), Ram8(player + kFacesRight) == 0);
        start.air = Ram8(player + kState) == kJump || Ram8(player + kInAir) != 0;
        start.id = Ram32(player + kFrame); // (the move's first animation frame)
        return start;
    }

    // A box of a record's (a player's or a thrown thing's), placed in the stage as the hit test 0BF5AE places it
    void AddBox(M68K &c, std::vector<fight::StageBox> &boxes, fight::BoxKind kind, uint32_t record, uint32_t box, bool thrown = false) {
        int across = static_cast<int8_t>(c.Read(box, 0));
        const int up = static_cast<int8_t>(c.Read(box + 1, 0));
        const int halfWidth = static_cast<int8_t>(c.Read(box + 2, 0)), halfHeight = static_cast<int8_t>(c.Read(box + 3, 0));
        if (halfWidth <= 0 || halfHeight <= 0) {
            return;
        }
        if (kind == fight::BoxKind::Attack && (c.Read(box + 5, 0) & 0x80)) {
            across = -static_cast<int>(c.Read(box + 5, 0)); // (a long reach: 0BF5B6)
        }
        const int x = static_cast<int16_t>(Ram16(record + kX)) + (Ram8(record + kFacesRight) ? -across : across);
        const int y = static_cast<int16_t>(Ram16(record + kY)) + up;
        boxes.push_back({kind, x, y, halfWidth, halfHeight, thrown});
    }

    // The box of a kind named in a record's frame (0: none): its table, from the record's box tables
    uint32_t BoxOf(M68K &c, uint32_t record, uint32_t number, uint32_t table, uint32_t size) {
        const uint32_t frame = Ram32(record + kFrame), tables = Ram32(record + kBoxTables);
        if (frame >= 0x400000 || tables >= 0x400000) {
            return 0;
        }
        const uint8_t which = static_cast<uint8_t>(c.Read(frame + number, 0));
        if (!which) {
            return 0;
        }
        return tables + static_cast<uint32_t>(static_cast<int16_t>(c.Read(tables + table, 1))) + which * size;
    }

    std::vector<fight::StageBox> Boxes(Machine &machine) {
        M68K &c = machine.cpu;
        std::vector<fight::StageBox> boxes;
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            if (Ram8(player + kFighting) != 2) {
                continue;
            }
            if (const uint32_t box = BoxOf(c, player, kFramePush, kTablePush, kBoxSize)) {
                AddBox(c, boxes, fight::BoxKind::Push, player, box);
            }
            constexpr struct {
                uint32_t number, table;
            } kHurt[] = {{kFrameHead, kTableHead}, {kFrameBody, kTableBody}, {kFrameLegs, kTableLegs}};
            for (const auto &hurt : kHurt) {
                if (const uint32_t box = BoxOf(c, player, hurt.number, hurt.table, kBoxSize)) {
                    AddBox(c, boxes, fight::BoxKind::Hurt, player, box);
                }
            }
            if (const uint32_t box = BoxOf(c, player, kFrameAttack, kTableAttack, kAttackBoxSize)) {
                AddBox(c, boxes, fight::BoxKind::Attack, player, box);
            }
        }
        // What the players throw (the pool from FF97A2, records of a player's layout): their attacks (the thrown things'
        // hit test, 0BEB0E, takes them as a player's)
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t thrown = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            if (Ram8(thrown) != 1) {
                continue;
            }
            if (const uint32_t box = BoxOf(c, thrown, kFrameAttack, kTableAttack, kAttackBoxSize)) {
                AddBox(c, boxes, fight::BoxKind::Attack, thrown, box, true);
            }
        }
        return boxes;
    }

    // The game's own aids: the gauge kept full, and each player's stun against how much stuns it
    void Hud(Machine &machine, std::vector<HudLine> &lines, std::vector<HudBox> &) {
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            if (family::kTurbo && FeatureOn("gauge_refill") && Ram8(player + kSuperGauge) < kGaugeFull) {
                SetRam8(player + kSuperGauge, kGaugeFull);
            }
            if (FeatureOn("stun_readout")) {
                char text[32];
                const uint32_t most = kStunMost + (family::kTurbo ? Ram8(player + kStunTimes) & 0x1E : 0);
                snprintf(text, sizeof text, "stun %d/%d", Ram16(player + kStun), machine.Read16(most));
                lines.push_back({n == 0 ? 0.02f : 0.98f, 0.12f, n == 0 ? -1 : 1, text});
            }
        }
    }

    // The dummy: the game knows when it is knocked down
    void DummyView(Machine &, fight::DummyView &view) {
        view.knowsDown = true;
        view.down = Ram8(Player(1) + kState) == kDown;
    }
} // namespace

void RegisterTraining() {
    session::Description d;
    d.player1 = kPlayer1;
    d.playerSize = kPlayerSize;
    d.x = kX;
    d.xLong = true;
    d.health = kHealth;
    d.healthFull = kHealthFull;
    d.healthShown = kHealthBefore;
    d.clock = kRoundClock;
    d.clockFull = 0x99; // (BCD)
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
    d.chosen = [](int n) { return Ram8(Player(n) + kChosen) != 0; };
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
    d.stageWords = "Where the fights are, whatever is played (two-player games, training, the other modes): the "
                   "game's own choice or a stage named here. A one-player game's stage stays the opponent's own, "
                   "which the game goes by; another game's stage is shown there all the same. Takes effect when the "
                   "characters are next chosen.";
    d.inputWords = "What each player presses and for how many frames: the stick as a keypad number from the player's side (6 "
                   "forward, 4 back, 2 down), then the buttons.";
    d.hud = Hud;
    d.ownAids = {"stun_readout"};
    Game &game = TheGame();
    if (family::kTurbo) { // (Super Street Fighter II has no super combos)
        d.ownAids.insert(d.ownAids.begin(), "gauge_refill");
        game.features.push_back({"gauge_refill", modes::kTraining, "Gauge refills", "Both players' super combo gauge kept full."});
    }
    game.features.push_back({"stun_readout", modes::kTraining, "Stun readout",
                             family::kTurbo ? "Under each player's bar: the stun built up, against how much stuns the player (it goes away "
                                              "when they are left alone for a while, and takes more each time they have been stunned)." :
                                              "Under each player's bar: the stun built up, against how much stuns the player (it goes away "
                                              "when they are left alone for a while)."});
    session::Register(d);
    AddHook(HookAt(kPlayerStep), "PlayerStep", PlayerStep);
    AddHook(HookAt(kComputerPlayerStep), "ComputerPlayerStep", PlayerStep);
    AddHook(HookAt(kHitTaken), "HitTaken", HitTaken);
    AddHook(HookAt(kThrownHitTaken), "ThrownHitTaken", HitTaken);
}

} // namespace cps2recomp
