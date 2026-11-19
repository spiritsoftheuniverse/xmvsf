// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha 2: the court (the toolkit's ball game, fight/court.h; docs/COURT.md): where the game keeps what
// the ball needs (the fighters' boxes and their attacks, how strong each is), and what is done to a fighter (frozen by a
// hit on the ball, knocked down by the ball, its health kept full). The fighters are kept on their halves where the game
// pushes them apart (collision.cpp, PushApart), nobody is thrown (throws.cpp, ThrowTest), and what the ball does to a
// fighter is made at its step (player.cpp, PlayerStepOne: CourtStep).
#include "court.h"
#include "state.h"
#include "modes.h"
#include "sfa2.h"

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

namespace cps2recomp {

int WideStage(); // widescreen.cpp

namespace {
    using namespace sfa2;

    // In an attack's entry: bit 7 of +08 set for a special move's (hit.cpp's kAttackStrength); +09's low four bits the
    // button's strength, 0 light, 1 medium, 2 hard (HitLands keeps them at the victim's +56; seen with SFA2_COURT_LOG as
    // each button was pressed); and its number (bit 7: one more on the same attack)
    constexpr uint32_t kAttackSpecial = 0x08, kAttackStrength = 0x09, kAttackNumber = 0x12;
    constexpr uint8_t kReactionGuarded = 0xFF, kReactionKnockDown = 4, kStruckStop = 8;
    // From A5: set by the round's first hit (player.cpp, ReelBegins: "First attack")
    constexpr uint32_t kFirstAttack = 0xFF8000 + AtA5(0x135);
    // Where a reeling player is pushed: the way it is turned (HitLands' +66: 0 pushed left, 1 right)
    constexpr uint32_t kPushedWay = 0x66;
    // The hit's strength as HitLands keeps it (the attack's +09): how far the reel pushes (023C0C) and how it looks
    constexpr uint32_t kStrength = 0x56;
    constexpr uint32_t kHalfWidth = 0x12A; // (widescreen.cpp's)
    // A gauge spent in one go (a super, an Alpha counter): a level of it at least
    constexpr int kGaugeLevel = 0x30;

    // (for finding the facts: SFA2_COURT_LOG=1 writes each attack's entry as it comes out)
    void Log(Machine &machine, uint32_t object, uint32_t entry) {
        static const bool on = getenv("SFA2_COURT_LOG") != nullptr;
        static uint32_t last[64] = {};
        const int slot = object >= kThrown ? 4 + static_cast<int>((object - kThrown) / kThrownSize) : static_cast<int>((object - kPlayer1) / kPlayerSize);
        if (!on || slot < 0 || slot >= 64 || last[slot] == entry) {
            return;
        }
        last[slot] = entry;
        fprintf(stderr, "court: obj %06X side %d char %02X entry %06X:", object, Ram8(object + kSide), Ram8(Player(Ram8(object + kSide) & 1) + kCharacter), entry);
        for (uint32_t i = 8; i < kAttackBoxSize; i++) {
            fprintf(stderr, " %02X", machine.Read8(entry + i));
        }
        fprintf(stderr, "\n");
    }

    // A gauge spent (a super, a custom combo): the move is a super's to the ball until the fighter can act again
    struct Spent {
        uint16_t gauge[2] = {0, 0};
        bool spending[2] = {false, false};
    } g_spent;
    void WatchGauges() {
        for (int side = 0; side < 2; side++) {
            const uint32_t player = Player(side);
            const uint16_t gauge = Ram16(player + kSpecialGauge);
            if (gauge + kGaugeLevel / 2 <= g_spent.gauge[side] || Ram8(player + kCustomCombo)) {
                g_spent.spending[side] = true;
            } else if (CanAct(side)) {
                g_spent.spending[side] = false;
            }
            g_spent.gauge[side] = gauge;
        }
    }

    bool IsTable(uint32_t address) {
        return address >= 0x1000 && address < 0x400000 && !(address & 1);
    }

    void AddAttack(Machine &machine, uint32_t object, uint32_t entry, bool thrown, std::vector<court::Shape> &out) {
        Log(machine, object, entry);
        const int16_t x = static_cast<int16_t>(machine.Read16(entry + kBoxX)), y = static_cast<int16_t>(machine.Read16(entry + kBoxY));
        const int16_t halfWidth = static_cast<int16_t>(machine.Read16(entry + kBoxHalfWidth));
        const int16_t halfHeight = static_cast<int16_t>(machine.Read16(entry + kBoxHalfHeight));
        if (halfWidth <= 0 || halfHeight <= 0) {
            return;
        }
        court::Shape shape;
        shape.kind = court::Shape::kAttack;
        shape.side = Ram8(object + kSide) & 1;
        shape.x = static_cast<int16_t>(Ram16(object + kX)) + (Ram8(object + kFacesRight) ? -x : x);
        shape.y = static_cast<int16_t>(Ram16(object + kY)) + y;
        shape.halfWidth = halfWidth;
        shape.halfHeight = halfHeight;
        shape.thrown = thrown;
        shape.id = (object & 0xFFFF) << 8 | (machine.Read8(entry + kAttackNumber) & 0x7F);
        // How hard: a super's while a gauge is spent; a special move's or a thing thrown's; else the button's strength
        const uint8_t strength = static_cast<uint8_t>(machine.Read8(entry + kAttackStrength) & 0x0F);
        const bool special = thrown || (machine.Read8(entry + kAttackSpecial) & 0x80);
        shape.power = g_spent.spending[shape.side] ? court::kSuper
                    : special                     ? court::kSpecial
                    : strength >= 2               ? court::kHard
                    : strength == 1               ? court::kMedium
                                                  : court::kLight;
        out.push_back(shape);
    }

    void Shapes(Machine &machine, std::vector<court::Shape> &out) {
        WatchGauges();
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            if (!Ram8(player + kActive) || Ram8(player + kState) != 2) {
                continue;
            }
            const uint32_t frame = Ram32(player + kFrame);
            if (!IsTable(frame)) {
                continue;
            }
            // Its attack
            const uint32_t attacks = Ram32(player + kAttackBoxes);
            const int8_t attack = static_cast<int8_t>(machine.Read8(frame + kFrameAttack));
            if (attack > 0 && IsTable(attacks) && !(machine.Read8(frame + kFrameHarmless) & 0x80)) {
                AddAttack(machine, player, attacks + static_cast<uint32_t>(attack) * kAttackBoxSize, false, out);
            }
            // Its body: the boxes it can be hit in
            for (const auto &[table, number] : {std::pair{kHeadBoxes, kFrameHead}, {kBodyBoxes, kFrameBody}, {kLegBoxes, kFrameLegs}}) {
                const uint32_t boxes = Ram32(player + table);
                const int8_t box = static_cast<int8_t>(machine.Read8(frame + number));
                if (box <= 0 || !IsTable(boxes)) {
                    continue;
                }
                const uint32_t at = boxes + static_cast<uint32_t>(box) * 8;
                const int16_t x = static_cast<int16_t>(machine.Read16(at + kBoxX)), y = static_cast<int16_t>(machine.Read16(at + kBoxY));
                const int16_t halfWidth = static_cast<int16_t>(machine.Read16(at + kBoxHalfWidth));
                const int16_t halfHeight = static_cast<int16_t>(machine.Read16(at + kBoxHalfHeight));
                if (halfWidth > 0 && halfHeight > 0) {
                    out.push_back({court::Shape::kBody, n, static_cast<int16_t>(Ram16(player + kX)) + (Ram8(player + kFacesRight) ? -x : x),
                                   static_cast<int16_t>(Ram16(player + kY)) + y, halfWidth, halfHeight});
                }
            }
        }
        // What they threw: its tables a header of word offsets from its kThrownTables (+2 the attacks)
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t thrown = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            if (Ram8(thrown + kActive) != 1 || Ram8(thrown + kSide) > 1) {
                continue;
            }
            const uint32_t frame = Ram32(thrown + kFrame), tables = Ram32(thrown + kThrownTables);
            if (!IsTable(frame) || !IsTable(tables)) {
                continue;
            }
            const int8_t number = static_cast<int8_t>(machine.Read8(frame + kFrameAttack));
            if (number > 0) {
                const uint32_t attacks = tables + static_cast<uint32_t>(static_cast<int16_t>(machine.Read16(tables + 2)));
                AddAttack(machine, thrown, attacks + static_cast<uint32_t>(number) * kAttackBoxSize, true, out);
            }
        }
    }
} // namespace

// What the ball has done to a fighter (court.h, ReactionWanted), at the fighter's step (player.cpp, PlayerStepOne): the
// fighter put as a hit leaves it (HitLands, hit.cpp: in the reeling's charge, its reaction +55), and the game's own
// reeling runs from this step on (ReelBegins: a reel on its feet, a knock-down, a guard). The way it is pushed is the
// game's +66, set as HitLands would from where the attacker stood: here away from the ball.
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
    static constexpr uint8_t kStrengths[] = {0, 1, 0, 1, 2, 2};
    SetRam8(player + kControl, kControlReeling);
    SetRam8(player + kAction, 0);
    SetRam8(player + kActionStep, 0);
    SetRam8(player + kReaction, kReactions[reaction]);
    SetRam8(player + kStrength, kStrengths[reaction]); // (how far it is pushed: 023C0C's table)
    SetRam8(player + kKnocksDown, reaction == court::kKnockDown ? 1 : 0);
    SetRam8(player + kHitStop, reaction == court::kKnockDown ? 0 : kStruckStop);
    SetRam8(player + kAttackOut, 0);
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
    d.canAct = [](int side) { return CanAct(side); };
    // (holding back, free to guard, or guarding already: the ball blocked)
    d.guarding = [](int side) {
        const uint32_t player = Player(side);
        const bool back = Ram8(player + kHeld) & 1;
        return back && (CanAct(side) || Ram8(player + kAction) == kActionJump || Ram8(player + kReaction) == kReactionGuarded);
    };
    // (reeling from a hit, not a guard)
    d.stunned = [](int side) {
        const uint32_t player = Player(side);
        return Ram8(player + kControl) == kControlReeling && Ram8(player + kReaction) != kReactionGuarded;
    };
    d.floor = kGround;
    d.pictureFloor = 239;
    d.pictureTop = 40; // (under where the health bars and the clock are)
    d.panel = ThePanel; // (the bars, names and clock not shown: court.h)
    d.wideStage = WideStage;
    d.freeze = [](int side, int steps) { SetRam8(Player(side) + kHitStop, static_cast<uint8_t>(steps)); };
    d.playSound = PlaySound;
    // (the sounds a hit makes on a fighter, queued on the frame it lands: SFA2_SOUND_LOG against the health going down
    // in the endless battle. 0200 goes with the lightest hits, 0204-0206 with kicks by their damage, 0203 with the
    // specials' and supers' hits)
    d.hitSounds[court::kLight] = 0x0200;
    d.hitSounds[court::kMedium] = 0x0205;
    d.hitSounds[court::kHard] = 0x0206;
    d.hitSounds[court::kSpecial] = d.hitSounds[court::kSuper] = 0x0203;
    // (each frame: the round's first hit taken as made, else "First attack" when the ball knocks one down; and the
    // fighters, who may hit each other, never lose health)
    d.everyFrame = [] {
        SetRam8(kFirstAttack, 0xFF);
        for (int side = 0; side < 2; side++) {
            SetRam16(Player(side) + kHealth, kHealthFull);
            SetRam16(Player(side) + kHealthShown, kHealthFull);
        }
    };
    d.state = [](Archive &ar) { ar.Pod(g_spent); }; // (the gauges' last values: in a save state, for rollback)
    court::Register(d);
}

} // namespace cps2recomp
