// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Vampire Savior: the court (the toolkit's ball game, fight/court.h; docs/COURT.md): where the game keeps what the
// ball needs (the fighters' boxes and their attacks, how strong each is), and what is done to a fighter (frozen by a
// hit on the ball, knocked down by the ball, its gauge filled). The fighters are kept on their halves where the game
// pushes them apart (collision.cpp, PushApart), nobody is caught (CourtNoCatch), and what the ball does to a fighter
// is made at its step (frames.cpp, PlayerStep: CourtStep).
#include "boxes.h"
#include "court.h"
#include "state.h"
#include "modes.h"
#include "widescreen.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace cps2recomp {

namespace {
    using namespace vsav;

    // (for finding the facts: VSAV_COURT_LOG=1 writes each attack's entry as it comes out)
    void Log(Machine &machine, uint32_t object, uint32_t entry, uint8_t number) {
        static const bool on = getenv("VSAV_COURT_LOG") != nullptr;
        static uint32_t last[64] = {};
        const int slot = object >= kThrown ? 4 + static_cast<int>((object - kThrown) / kThrownSize) : static_cast<int>((object - kPlayer1) / kPlayerSize);
        if (!on || slot < 0 || slot >= 64 || last[slot] == entry) {
            return;
        }
        last[slot] = entry;
        const uint32_t frame = Ram32(object + kFrame);
        fprintf(stderr, "court: obj %06X side %d char %02X box %02X frame %06X str %02X entry %06X:", object, Ram8(object + kSide),
                Ram8(Player(Ram8(object + kSide) & 1) + kCharacter), number, frame, machine.Read8(frame + kFrameStrength), entry);
        for (uint32_t i = 8; i < kAttackBoxSize; i++) {
            fprintf(stderr, " %02X", machine.Read8(entry + i));
        }
        fprintf(stderr, "\n");
    }

    // In an attack's entry: its strength, 1 light to 3 hard for a button's attack (as the buttons are), more for a
    // special move
    constexpr uint32_t kAttackStrength = 0x12;
    // A hit guarded leaves +54 so; and frames a fighter the ball has hit stands frozen for
    constexpr uint8_t kReactionGuarded = 0xFF, kStruckStop = 8;
    // A byte: not zero once a round's first hit has landed (its "FIRST ATTACK" said)
    constexpr uint32_t kFirstAttackMade = At(0xFF8135);
    constexpr uint8_t kStrengthSpecial = 4;

    // A gauge spent: the move it was spent on (an ES or EX move, a super) is a super's to the ball, until the fighter
    // can act again
    struct Spent {
        uint8_t stocks[2] = {0, 0};
        bool spending[2] = {false, false};
    } g_spent;
    bool Spending(int side) {
        return g_spent.spending[side & 1];
    }
    void WatchGauges() {
        for (int side = 0; side < 2; side++) {
            const uint8_t stocks = Ram8(Player(side) + kStocks);
            if (stocks < g_spent.stocks[side]) {
                g_spent.spending[side] = true;
                if (getenv("VSAV_COURT_LOG")) {
                    fprintf(stderr, "court: side %d spent %d -> %d\n", side, g_spent.stocks[side], stocks);
                }
            } else if (CanAct(side)) {
                g_spent.spending[side] = false;
            }
            g_spent.stocks[side] = stocks;
        }
    }

    bool IsTable(uint32_t address) {
        return ((address >= 0x1000 && address < 0x400000) || (address >= 0x400000 && InProgram(address))) && !(address & 1);
    }

    void Attack(Machine &machine, uint32_t object, bool thrown, std::vector<court::Shape> &out) {
        if (!Ram8(object + kActive)) {
            return;
        }
        const uint32_t frame = Ram32(object + kFrame), table = Ram32(object + kAttackBoxes);
        if (!IsTable(frame) || !IsTable(table)) {
            return;
        }
        const uint8_t number = static_cast<uint8_t>(machine.Read8(frame + kFrameAttack));
        if (!number || (machine.Read8(frame + kFrameHarmless) & 0x80)) {
            return;
        }
        const uint32_t entry = table + number * kAttackBoxSize;
        Log(machine, object, entry, number);
        const Box box = ReadBox(machine, entry);
        if (box.halfWidth <= 0 || box.halfHeight <= 0) {
            return;
        }
        court::Shape shape;
        shape.kind = court::Shape::kAttack;
        shape.side = Ram8(object + kSide) & 1;
        shape.x = static_cast<int16_t>(Ram16(object + kX)) + (Ram8(object + kFacesRight) ? -box.x : box.x);
        shape.y = static_cast<int16_t>(Ram16(object + kY)) + box.y;
        shape.halfWidth = box.halfWidth;
        shape.halfHeight = box.halfHeight;
        shape.thrown = thrown;
        shape.id = (object & 0xFFFF) << 8 | machine.Read8(entry + kAttackNumber);
        // How hard: the attack's strength (1 light, 2 medium, 3 hard); in a move a gauge was spent on, a super's
        const uint8_t strength = static_cast<uint8_t>(machine.Read8(entry + kAttackStrength));
        // (a special move's are higher, one of its own for each: Demitri's fireball 5, its ES move 9, his uppercut 7, Lilith's
        // rising blade 0F; so a super's is told by the gauge spent)
        shape.power = Spending(shape.side) ? court::kSuper
                    : strength >= kStrengthSpecial ? court::kSpecial
                    : strength == 3 ? court::kHard : strength == 2 ? court::kMedium : court::kLight;
        out.push_back(shape);
    }

    void Shapes(Machine &machine, std::vector<court::Shape> &out) {
        WatchGauges();
        for (int n = 0; n < kPlayerRecords; n++) {
            Attack(machine, Player(n), false, out);
        }
        for (int n = 0; n < kThrownRecords; n++) {
            const uint32_t object = kThrown + static_cast<uint32_t>(n) * kThrownSize;
            if (Ram8(object + kActive) == 1 && Ram8(object + kSide) <= 1) {
                Attack(machine, object, true, out);
            }
        }
        // The fighters' bodies: the boxes they can be hit in
        for (int n = 0; n < 2; n++) {
            std::vector<PlacedBox> boxes;
            ObjectBoxes(machine, Player(n), false, boxes);
            for (const PlacedBox &box : boxes) {
                if (box.kind == BoxKind::Head || box.kind == BoxKind::Body || box.kind == BoxKind::Legs) {
                    out.push_back({court::Shape::kBody, n, box.x, box.y, box.halfWidth, box.halfHeight});
                }
            }
        }
    }
} // namespace

namespace {
    // 029406: whether the fighter at A6 can catch the other now (a throw, a command grab), D0 and the flags 1 for yes. On
    // the court, never: a grab that reaches across it (Anakaris's coffin) would take the other from its half
    bool NoCatch(M68K &c) {
        if (!court::On()) {
            return false;
        }
        c.d[0] = 0;
        c.flagZ = true;
        c.flagN = c.flagV = c.flagC = false;
        Return(c);
        return true;
    }

    int g_knocked[2] = {0, 0}; // a side knocked down by the ball: the way it is thrown (-1 left, 1 right), while it flies
} // namespace

// What the ball has done to a fighter (court.h, ReactionWanted), at the fighter's step (A6; frames.cpp's PlayerStep): the
// fighter put as a hit leaves it (a hit has just landed: 02383C reads its reaction, +54, and sets the reeling up), and the
// game's own code does the rest from this step on: a reel on its feet, a knock-down (thrown back, falling, lying, getting
// up), or a guard (+54 FF, as a hit guarded leaves it).
bool CourtStep(M68K &c) {
    const uint32_t player = c.a[6] & 0xFFFFFF;
    const int side = player == Player(0) ? 0 : player == Player(1) ? 1 : -1;
    if (side < 0) {
        return false;
    }
    // (pushed or thrown back as the game does it, but away from the ball: the game's way is the attacker's, and there is
    // none; its speed across turned about while it reels)
    if (g_knocked[side] && (Ram32(player + kState) & 0xFFFF0000) == kReeling) {
        const int32_t speed = static_cast<int32_t>(Ram32(player + kSpeedX));
        if ((speed < 0 ? -1 : 1) != g_knocked[side] && speed != 0) {
            SetRam32(player + kSpeedX, static_cast<uint32_t>(-speed));
            SetRam32(player + kPullX, static_cast<uint32_t>(-static_cast<int32_t>(Ram32(player + kPullX))));
        }
    } else if (g_knocked[side]) {
        g_knocked[side] = 0;
    }
    int away = 0;
    const court::Reaction reaction = court::ReactionWanted(side, away);
    if (reaction == court::kNoReaction) {
        return false;
    }
    static constexpr uint8_t kReactions[] = {0, kReactionGuarded, 0x00, 0x01, 0x02, kReactionKnockDown};
    SetRam32(player + kState, kReeling);
    SetRam8(player + kGuarded, kReactions[reaction]);
    SetRam8(player + kLiesDown, reaction == court::kKnockDown ? 1 : 0); // (a knock-down lands lying and gets up)
    SetRam8(player + kHitStop, reaction == court::kKnockDown ? 0 : kStruckStop);
    SetRam8(player + kAttacking, 0);
    g_knocked[side] = away;
    return false; // (the game's routine for the player runs on: its reeling begins)
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
        return back && (CanAct(side) || Ram8(player + kAction) == kActionJump || Ram8(player + kGuarded) == kReactionGuarded);
    };
    // (reeling from a hit, not a guard: the state a hit leaves, its reaction not a guard's)
    d.stunned = [](int side) {
        const uint32_t player = Player(side);
        return (Ram32(player + kState) & 0xFFFF0000) == kReeling && Ram8(player + kGuarded) != kReactionGuarded;
    };
    d.floor = kGround;
    d.pictureFloor = 239;
    d.pictureTop = 40; // (under where the health bars and the clock are, which the zoom by distance leaves where they are)
    d.panel = ThePanel; // (the bars, names and clock not shown: court.h)
    d.wideStage = WideStage;
    d.freeze = [](int side, int steps) { SetRam8(Player(side) + kHitStop, static_cast<uint8_t>(steps)); };
    d.playSound = PlaySound;
    // (the sounds a hit of each strength makes on a fighter: light, medium, hard; specials and supers the hard one's)
    d.hitSounds[court::kLight] = 0x0100;
    d.hitSounds[court::kMedium] = 0x0101;
    d.hitSounds[court::kHard] = d.hitSounds[court::kSpecial] = d.hitSounds[court::kSuper] = 0x0102;
    // (each frame: the round's first hit taken as made, or the game says "FIRST ATTACK" when the ball knocks one down;
    // and the fighters, who may hit each other, never lose health)
    d.everyFrame = [] {
        SetRam8(kFirstAttackMade, 1);
        for (int side = 0; side < 2; side++) {
            SetRam16(Player(side) + kHealth, kHealthFull);
            SetRam16(Player(side) + kHealthCeiling, kHealthFull);
        }
    };
    d.state = [](Archive &ar) {
        ar.Pod(g_spent);
        ar.Pod(g_knocked);
    };
    court::Register(d);
    AddHook(At(0x029406), "CourtNoCatch", NoCatch);
}

} // namespace cps2recomp
