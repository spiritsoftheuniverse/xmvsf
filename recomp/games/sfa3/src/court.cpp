// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 3: the court (the toolkit's ball game, fight/court.h; docs/COURT.md): where the game keeps what
// the ball needs (the fighters' boxes and their attacks, how strong each is), and what is done to a fighter (frozen by a
// hit on the ball, knocked down by the ball, its health kept full). The game's own code is used as it is, hooked: the
// players' push (0249E0) is the court's while it runs (each kept on its half), the throw test (02E3FE, training.cpp's
// ThrowTested) says no, and what the ball does to a fighter is made at its step (training.cpp, PlayerStep: CourtStep).
#include "alternative.h"
#include "court.h"
#include "state.h"
#include "sfa3.h"

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

namespace cps2recomp {

int WideStage(); // widescreen.cpp

namespace {
    using namespace sfa3;

    // In an attack's entry (thirty-two bytes: the box's eight, then what the attack does; the hit from 023278 reads them):
    // +09 the button's strength (0 light, 1 medium, 2 hard; seen with SFA3_COURT_LOG as each button was pressed), +0E its
    // number (bit 7: one more on the same attack)
    constexpr uint32_t kAttackStrength = 0x09, kAttackNumber = 0x0E;
    // In a player's record, as the hit leaves them: the state (02 02 00 00: in the reeling's charge), the reaction +55
    // (00 a reel, 0D a sweep's knock-down, FF guarded), how long it reels (+26A: 0C, 12, 18 by strength), the way it is
    // pushed (+66: 0 left, 1 right: 023C8E)
    constexpr uint32_t kReelTime = 0x26A, kPushedWay = 0x66;
    constexpr uint8_t kReactionGuarded = 0xFF, kReactionKnockDown = 0x0D, kStruckStop = 8;
    // A gauge spent in one go (a super): a third of the gauge at least, or a Custom Combo running
    constexpr int kGaugeSpent = kSpecialGaugeFull / 3;

    // (for finding the facts: SFA3_COURT_LOG=1 writes each attack's entry as it comes out)
    void Log(Machine &machine, uint32_t object, uint32_t entry) {
        static const bool on = getenv("SFA3_COURT_LOG") != nullptr;
        static uint32_t last[64] = {};
        const int slot = object >= kThrown ? 4 + static_cast<int>((object - kThrown) / kThrownSize) : static_cast<int>((object - kPlayer1) / kPlayerSize);
        if (!on || slot < 0 || slot >= 64 || last[slot] == entry) {
            return;
        }
        last[slot] = entry;
        fprintf(stderr, "court: obj %06X entry %06X:", object, entry);
        for (uint32_t i = 8; i < kAttackBoxSize; i++) {
            fprintf(stderr, " %02X", machine.Read8(entry + i));
        }
        fprintf(stderr, "\n");
    }

    struct Spent {
        uint16_t gauge[2] = {0, 0};
        bool spending[2] = {false, false};
    } g_spent;
    void WatchGauges() {
        for (int side = 0; side < 2; side++) {
            const uint32_t player = Player(side);
            const uint16_t gauge = Ram16(player + kSpecialGauge);
            if (gauge + kGaugeSpent <= g_spent.gauge[side] || Ram8(player + kCustomCombo)) {
                g_spent.spending[side] = true;
            } else if (CanAct(player)) {
                g_spent.spending[side] = false;
            }
            g_spent.gauge[side] = gauge;
        }
    }

    bool IsTable(uint32_t address) {
        return address >= 0x1000 && address < 0x400000;
    }

    // A box (words: its middle from where the object stands, half its width and height)
    bool Place(Machine &machine, uint32_t object, uint32_t box, court::Shape &shape) {
        const int16_t across = static_cast<int16_t>(machine.Read16(box + kBoxX)), up = static_cast<int16_t>(machine.Read16(box + kBoxY));
        shape.halfWidth = static_cast<int16_t>(machine.Read16(box + kBoxHalfWidth));
        shape.halfHeight = static_cast<int16_t>(machine.Read16(box + kBoxHalfHeight));
        shape.x = static_cast<int16_t>(Ram16(object + kX)) + (Ram8(object + kFacesRight) ? -across : across);
        shape.y = static_cast<int16_t>(Ram16(object + kY)) + up;
        return shape.halfWidth > 0 && shape.halfHeight > 0;
    }

    void AddAttack(Machine &machine, uint32_t object, uint32_t entry, bool thrown, std::vector<court::Shape> &out) {
        Log(machine, object, entry);
        court::Shape shape;
        if (!Place(machine, object, entry, shape)) {
            return;
        }
        shape.kind = court::Shape::kAttack;
        shape.side = Ram8(object + kSide) & 1;
        shape.thrown = thrown;
        shape.id = (object & 0xFFFF) << 8 | (machine.Read8(entry + kAttackNumber) & 0x7F);
        const uint8_t strength = machine.Read8(entry + kAttackStrength) & 0x0F;
        // (a special move's attacks are told by the move: the player not in a button's attack on the floor or in a jump,
        // kActionAttack or kActionJump; specials and supers are 0E and 10)
        const uint8_t action = Ram8(object + kAction);
        const bool special = thrown || (action != kActionAttack && action != kActionJump);
        shape.power = g_spent.spending[shape.side] ? court::kSuper
                    : special                     ? court::kSpecial
                    : strength >= 2               ? court::kHard
                    : strength == 1               ? court::kMedium
                                                  : court::kLight;
        out.push_back(shape);
    }

    void Shapes(Machine &machine, std::vector<court::Shape> &out) {
        WatchGauges();
        constexpr uint32_t kTables[3] = {kHeadBoxes, kBodyBoxes, kLegBoxes};
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            const uint32_t frame = Ram32(player + kFrame);
            if (Ram8(player + kState) != 2 || !IsTable(frame)) {
                continue;
            }
            const uint8_t attack = machine.Read8(frame + kFrameAttack);
            if (attack && IsTable(Ram32(player + kAttackBoxes))) {
                AddAttack(machine, player, Ram32(player + kAttackBoxes) + attack * kAttackBoxSize, false, out);
            }
            for (int part = 0; part < 3; part++) {
                const uint8_t box = Ram8(player + kBoxNumbers + static_cast<uint32_t>(part));
                court::Shape shape;
                if (box && IsTable(Ram32(player + kTables[part])) && Place(machine, player, Ram32(player + kTables[part]) + box * kBoxSize, shape)) {
                    shape.kind = court::Shape::kBody;
                    shape.side = n;
                    out.push_back(shape);
                }
            }
        }
        // What they threw (the pool from FF9400: records laid out as a player's, its side kSide)
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t thrown = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            const uint32_t frame = Ram32(thrown + kFrame);
            if (Ram8(thrown + kActive) != 1 || Ram8(thrown + kSide) > 1 || !IsTable(frame) || !IsTable(Ram32(thrown + kAttackBoxes))) {
                continue;
            }
            if (const uint8_t attack = machine.Read8(frame + kFrameAttack)) {
                AddAttack(machine, thrown, Ram32(thrown + kAttackBoxes) + attack * kAttackBoxSize, true, out);
            }
        }
    }

    // 0249E0: the two players pushed apart. On the court each is kept on its half instead (they never meet)
    bool PushApart(M68K &c) {
        if (!court::On()) {
            return false;
        }
        for (int side = 0; side < 2; side++) {
            const uint32_t player = Player(side);
            const int x = static_cast<int16_t>(Ram16(player + kX));
            const int kept = court::KeepToHalf(side, x, static_cast<int16_t>(Ram16(player + kHalfWidth)));
            if (kept != x) {
                SetRam32(player + kX, static_cast<uint32_t>(kept) << 16);
            }
        }
        Return(c);
        return true;
    }
} // namespace

// The throw test (02E3FE) on the court: its "no" (02E52A: D0 0), as nobody is thrown there
bool CourtNoThrow(M68K &c) {
    if (!court::On()) {
        return false;
    }
    c.pc = 0x02E52A;
    return true;
}

// What the ball has done to a fighter (court.h, ReactionWanted), at the fighter's step (training.cpp, PlayerStep): the
// fighter put as a hit leaves it, and the game's own reeling runs from this step on (028BE6: a reel on its feet, a
// knock-down, a guard), pushed away from the ball.
void CourtStep(uint32_t player) {
    const int side = player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
    if (side < 0 || !court::On()) {
        return;
    }
    int away = 0;
    const court::Reaction reaction = court::ReactionWanted(side, away);
    if (reaction == court::kNoReaction) {
        return;
    }
    static constexpr uint8_t kReactions[] = {0, kReactionGuarded, 0x00, 0x00, 0x00, kReactionKnockDown};
    static constexpr uint8_t kReelTimes[] = {0, 0x0C, 0x0C, 0x12, 0x18, 0x18};
    SetRam32(player + kState, 0x02020000);
    SetRam8(player + kReaction, kReactions[reaction]);
    SetRam8(player + kReelTime, kReelTimes[reaction]);
    SetRam8(player + kHitStop, reaction == court::kKnockDown ? 0 : kStruckStop);
    if (away) {
        SetRam8(player + kPushedWay, away > 0 ? 1 : 0);
    }
}

void RegisterCourt() {
    court::Description d;
    d.shapes = Shapes;
    d.fighterX = [](int side) { return static_cast<int>(static_cast<int16_t>(Ram16(Player(side) + kX))); };
    d.fighterY = [](int side) { return static_cast<int>(static_cast<int16_t>(Ram16(Player(side) + kY))); };
    d.place = [](int side, int x) { SetRam32(Player(side) + kX, static_cast<uint32_t>(x) << 16); };
    d.halfWidth = [](int side) { return static_cast<int>(static_cast<int16_t>(Ram16(Player(side) + kHalfWidth))); };
    d.canAct = [](int side) { return CanAct(Player(side)); };
    d.guarding = [](int side) {
        const uint32_t player = Player(side);
        const bool back = Ram8(player + kHeld) & 1;
        return back && (CanAct(player) || Ram8(player + kAction) == kActionJump || Ram8(player + kReaction) == kReactionGuarded);
    };
    d.stunned = [](int side) {
        const uint32_t player = Player(side);
        return Ram8(player + kControl) == kControlReeling && Ram8(player + kReaction) != kReactionGuarded;
    };
    d.floor = kGround;
    d.pictureFloor = 239;
    d.pictureTop = 40;
    d.panel = ThePanel; // (the bars, names and clock not shown: court.h)
    d.wideStage = WideStage;
    d.freeze = [](int side, int steps) { SetRam8(Player(side) + kHitStop, static_cast<uint8_t>(steps)); };
    d.playSound = modes::PlaySound;
    // (the sounds a hit makes, queued (003168) a frame or three after it lands (023278), in the endless battle: 0204 with
    // light buttons, 0205 medium, 0206 hard, 0203 with specials and supers; each has a copy at 07xx)
    d.hitSounds[court::kLight] = 0x0204;
    d.hitSounds[court::kMedium] = 0x0205;
    d.hitSounds[court::kHard] = 0x0206;
    d.hitSounds[court::kSpecial] = d.hitSounds[court::kSuper] = 0x0203;
    d.everyFrame = [] {
        for (int side = 0; side < 2; side++) {
            SetRam16(Player(side) + kHealth, kHealthFull);
            SetRam16(Player(side) + kHealthShown, kHealthFull);
        }
    };
    d.state = [](Archive &ar) { ar.Pod(g_spent); }; // (the gauges' last values: in a save state, for rollback)
    court::Register(d);
    AddHook(HookAt(0x0249E0), "CourtPushApart", PushApart);
}

} // namespace cps2recomp
