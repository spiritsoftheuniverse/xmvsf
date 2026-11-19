// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Street Fighter Alpha: the court (the toolkit's ball game, fight/court.h; docs/COURT.md): where the game keeps what the
// ball needs (the fighters' boxes and their attacks, how strong each is), and what is done to a fighter (frozen by a hit
// on the ball, knocked down by the ball, its health kept full). The game's own code is used as it is, hooked: the push
// that keeps the two apart (00F2FC) is the court's while it runs (each kept on its half), a throw's catch (020F80,
// 021032) goes to its test's "no", and what the ball does to a fighter is made at its step (training.cpp, PlayerStep:
// CourtStep).
#include "court.h"
#include "state.h"
#include "alternative.h"
#include "sfa.h"

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

namespace cps2recomp {

namespace {
    using namespace sfa;

    // In an attack's entry (sixteen bytes: the box's four, then what the attack does; the hit at 00F646 reads them): bit
    // 7 of +04 set for a special move's, +05's low four bits the button's strength (0 light, 1 medium, 2 hard: kept at
    // the victim's +46), +0E its number (bit 7: one more on the same attack)
    constexpr uint32_t kAttackSpecial = 0x04, kAttackStrength = 0x05, kAttackNumber = 0x0E;
    // In a player's record, as the hit leaves them (00F646): the state (02 02 00 00: in the reeling's charge), the
    // reaction (+45: the attack's kind, FF guarded), the strength (+46), the way it is pushed (+4D: 0 left, 1 right)
    constexpr uint32_t kReaction = kGuardedHit, kStrength = 0x46, kPushedWay = 0x4D;
    constexpr uint8_t kReactionGuarded = 0xFF, kReactionKnockDown = 4, kStruckStop = 8;
    constexpr uint32_t kHeld = 0x127;      // byte: bit 0 set while the player holds away from the other (01EC4A)
    constexpr uint32_t kAttackOut = 0x132; // not zero while an attack of the player's is out
    // A gauge spent in one go (a super): half a level of it at least
    constexpr int kGaugeLevel = 0x90;

    // The throw test's end that says no (the catches hooked: RegisterCourt)
    constexpr uint32_t kCatchFails = 0x020FBE; // (moveq #0,d0; rts: the test's "no")

    // (for finding the facts: SFA_COURT_LOG=1 writes each attack's entry as it comes out)
    void Log(Machine &machine, uint32_t object, uint32_t entry) {
        static const bool on = getenv("SFA_COURT_LOG") != nullptr;
        static uint32_t last[64] = {};
        const int slot = object >= kThrown ? 4 + static_cast<int>((object - kThrown) / kThrownSize) : static_cast<int>((object - kPlayer1) / kPlayerSize);
        if (!on || slot < 0 || slot >= 64 || last[slot] == entry) {
            return;
        }
        last[slot] = entry;
        fprintf(stderr, "court: obj %06X entry %06X:", object, entry);
        for (uint32_t i = 4; i < kAttackBoxSize; i++) {
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
            if (gauge + kGaugeLevel / 2 <= g_spent.gauge[side]) {
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

    // A box (four bytes: its middle's offsets across and up, half its width and height), placed as 0104B8 places it
    bool Place(Machine &machine, uint32_t object, uint32_t box, court::Shape &shape) {
        const int8_t across = static_cast<int8_t>(machine.Read8(box)), up = static_cast<int8_t>(machine.Read8(box + 1));
        shape.halfWidth = machine.Read8(box + 2);
        shape.halfHeight = machine.Read8(box + 3);
        shape.x = static_cast<int16_t>(Ram16(object + kX)) + (Ram8(object + kFacesRight) ? -across : across);
        shape.y = static_cast<int16_t>(Ram16(object + kY)) + up;
        return shape.halfWidth > 0 && shape.halfHeight > 0;
    }

    void AddAttack(Machine &machine, uint32_t object, int side, uint32_t entry, bool thrown, std::vector<court::Shape> &out) {
        Log(machine, object, entry);
        court::Shape shape;
        if (!Place(machine, object, entry, shape)) {
            return;
        }
        shape.kind = court::Shape::kAttack;
        shape.side = side;
        shape.thrown = thrown;
        shape.id = (object & 0xFFFF) << 8 | (machine.Read8(entry + kAttackNumber) & 0x7F);
        const uint8_t strength = static_cast<uint8_t>(machine.Read8(entry + kAttackStrength) & 0x0F);
        const bool special = thrown || (machine.Read8(entry + kAttackSpecial) & 0x80);
        shape.power = g_spent.spending[side] ? court::kSuper
                    : special               ? court::kSpecial
                    : strength >= 2         ? court::kHard
                    : strength == 1         ? court::kMedium
                                            : court::kLight;
        out.push_back(shape);
    }

    // Whose a thing thrown is: the player nearer it when it came out is taken as its thrower (it keeps no side of its
    // own that is found yet), remembered by its record while it is in use
    int g_thrownSide[kThrownRecords] = {};
    uint32_t g_thrownFrame[kThrownRecords] = {};

    void Shapes(Machine &machine, std::vector<court::Shape> &out) {
        WatchGauges();
        for (int n = 0; n < 2; n++) {
            const uint32_t player = Player(n);
            const uint32_t frame = Ram32(player + kFrame);
            if (Ram8(player + kState) != 2 || !IsTable(frame)) {
                continue;
            }
            const uint8_t attack = machine.Read8(frame + kFrameAttack);
            if (attack && attack < 0x80 && IsTable(Ram32(player + kAttackBoxes))) {
                AddAttack(machine, player, n, Ram32(player + kAttackBoxes) + attack * kAttackBoxSize, false, out);
            }
            for (const auto &[table, number] : {std::pair{kHeadBoxes, kFrameHead}, {kBodyBoxes, kFrameBody}, {kLegBoxes, kFrameLegs}}) {
                const uint8_t box = machine.Read8(frame + number);
                court::Shape shape;
                if (box && IsTable(Ram32(player + table)) && Place(machine, player, Ram32(player + table) + box * kBoxSize, shape)) {
                    shape.kind = court::Shape::kBody;
                    shape.side = n;
                    out.push_back(shape);
                }
            }
        }
        // What they threw (the pool from FF9000): the frame names the attack, in a table the record's long +50 points at,
        // from the word at its +2 on (01018A)
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t thrown = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            const uint32_t frame = Ram32(thrown + kFrame), table = Ram32(thrown + kThrownTables);
            if (Ram8(thrown) != 1 || !IsTable(frame) || !IsTable(table)) {
                g_thrownFrame[n] = 0;
                continue;
            }
            if (!g_thrownFrame[n]) {
                const int x = static_cast<int16_t>(Ram16(thrown + kX));
                g_thrownSide[n] = std::abs(x - static_cast<int16_t>(Ram16(Player(0) + kX))) <=
                                          std::abs(x - static_cast<int16_t>(Ram16(Player(1) + kX)))
                                      ? 0
                                      : 1;
            }
            g_thrownFrame[n] = frame;
            if (const uint8_t attack = machine.Read8(frame + kFrameAttack)) {
                const uint32_t attacks = table + machine.Read16(table + 2);
                AddAttack(machine, thrown, g_thrownSide[n], attacks + attack * kAttackBoxSize, true, out);
            }
        }
    }

    // 00F2FC: the two players pushed apart. On the court each is kept on its half instead (they never meet)
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

    // 020F80, 021032: a throw has reached the other and is about to catch it: on the court, the test says no
    bool Catch(M68K &c) {
        if (!court::On()) {
            return false;
        }
        c.pc = kCatchFails;
        return true;
    }
} // namespace

// What the ball has done to a fighter (court.h, ReactionWanted), at the fighter's step (training.cpp, PlayerStep): the
// fighter put as a hit leaves it (00F646), and the game's own reeling runs from this step on (a reel on its feet, a
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
    static constexpr uint8_t kStrengths[] = {0, 1, 0, 1, 2, 2};
    SetRam32(player + kState, 0x02020000);
    SetRam8(player + kReaction, kReactions[reaction]);
    SetRam8(player + kStrength, kStrengths[reaction]);
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
    // (the bars, names and clock are the first scroll screen's, which shows its map again past 64 a side: not drawn on
    // the court, where health stays full and there is no clock; the gauges are objects and stay)
    d.hiddenLayers = 0xF & ~kLayersWithoutPanel;
    d.panel = ThePanel; // (and the panel's objects at the top)
    d.wideStage = WideStage;
    d.freeze = [](int side, int steps) { SetRam8(Player(side) + kHitStop, static_cast<uint8_t>(steps)); };
    d.playSound = modes::PlaySound;
    // (the sounds a hit makes, queued (012BEE) on the frame it lands (00F646), in the endless battle: 0206 with medium
    // buttons, 0207 with hard ones, 0208 with specials; 0205 for light by that order. Each has a copy at 01xx)
    d.hitSounds[court::kLight] = 0x0205;
    d.hitSounds[court::kMedium] = 0x0206;
    d.hitSounds[court::kHard] = 0x0207;
    d.hitSounds[court::kSpecial] = d.hitSounds[court::kSuper] = 0x0208;
    d.everyFrame = [] {
        for (int side = 0; side < 2; side++) {
            SetRam16(Player(side) + kHealth, kHealthFull);
            SetRam16(Player(side) + kHealthShown, kHealthFull);
        }
    };
    d.state = [](Archive &ar) { // (in a save state, for rollback)
        ar.Pod(g_spent);
        ar.Pod(g_thrownSide);
        ar.Pod(g_thrownFrame);
    };
    court::Register(d);
    AddHook(HookAt(0x00F2FC), "CourtPushApart", PushApart);
    AddHook(HookAt(0x020F80), "CourtNoCatch", Catch);
    AddHook(HookAt(0x021032), "CourtNoCatch2", Catch);
}

} // namespace cps2recomp
