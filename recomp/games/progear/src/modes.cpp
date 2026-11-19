// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Progear: the menu's "Alternative modes" and training mode (the toolkit's shmup/shmup.h): what they need of the game.
//
// A mode gets into a game the way a player does: a credit, Start, the choice of pilot and then of gunner (each put in the
// choice's variable and its cursor and the shot button pressed, or the player's own choices in the scored modes), and
// the stage begins (mode 07). The stage it begins at is put in where Start is taken (game.cpp).
#include "progear.h"
#include "shmup.h"
#include "state.h"

#include <cstdio>

namespace cps2recomp {

namespace {
    using namespace progear;

    // The picture's row for a height the game counts up from the picture's foot, in its 64ths
    int Row(int up) {
        return Video::kHeight - up / kUnit;
    }
    int Column(int across) {
        return across / kUnit;
    }

    bool InGame() {
        const uint16_t mode = Ram16(kMode);
        return (mode == kModeChoose || mode == kModePlay || mode == kModeContinue) && Ram16(kPlayers) != 0;
    }

    bool PlayerIn(int n) {
        const uint16_t players = Ram16(kPlayers);
        return InGame() && !(players & (n == 0 ? kPlayer1Out : kPlayer2Out));
    }

    bool ToPlay(Machine &machine, const shmup::Begin &begin, int32_t frames) {
        const uint16_t mode = Ram16(kMode);
        // (played once player 1's ship is there and the take-off over: a ship's start puts its power and bombs back,
        // so what training mode sets is put in after it)
        if (mode == kModePlay) {
            return (Ram16(Ship(0) + kShipFlags) & 0x8000) && Ram16(kTakeOff) == 0;
        }
        if (mode == kModeChoose && begin.fighter >= 0) {
            // (the pilot and the gunner put where Down steps them, the variables and the cursors both, then the shot button
            // to take each)
            const int pilot = begin.fighter / kGunners, gunner = begin.fighter % kGunners;
            SetRam16(kPilot, static_cast<uint16_t>(pilot));
            SetRam16(kGunner, static_cast<uint16_t>(gunner));
            SetRam8(kPilotCursor, static_cast<uint8_t>(pilot));
            SetRam8(kGunnerCursor, static_cast<uint8_t>(gunner));
            machine.players[0] = (frames & 8) ? kPunch1 : 0;
        }
        return false;
    }

    void ShipBoxes(std::vector<HudBox> &boxes) {
        for (int n = 0; n < kPlayers2; n++) {
            const uint32_t ship = Ship(n);
            if (!PlayerIn(n) || !(Ram16(ship + kShipFlags) & 0x8000)) {
                continue;
            }
            const int y = static_cast<int16_t>(Ram16(ship + kShipY)), x = static_cast<int16_t>(Ram16(ship + kShipX));
            const int up = static_cast<int16_t>(Ram16(ship + kShipBoxUp)), down = static_cast<int16_t>(Ram16(ship + kShipBoxDown)),
                      right = static_cast<int16_t>(Ram16(ship + kShipBoxRight)), left = static_cast<int16_t>(Ram16(ship + kShipBoxLeft));
            const int x0 = Column(x - left), x1 = Column(x + right), y0 = Row(y + up), y1 = Row(y - down);
            // (bright green in a black edge, to stand out from any ship)
            boxes.push_back({x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, 0, 0, 0, 255});
            boxes.push_back({x0, y0, x1 - x0, y1 - y0, 0, 255, 0, 255});
        }
    }

    // The enemies' bodies as the hit test last saw them: the game makes the list during a frame and clears it before the
    // frame ends, so it is read at the hit test (04DF7A, a hook that only looks). For drawing only, so kept out of the
    // state: `frame` is the count of frames it was read at, for leaving out a list no hit test read this frame.
    struct Seen {
        std::vector<HudBox> bodies;
        uint64_t frame = 0;
    } g_seen;
    uint64_t g_frame = 0;

    bool BodiesSeen(M68K &) {
        g_seen.bodies.clear();
        g_seen.frame = g_frame;
        const int bodies = Ram16(kBodyCount1) + Ram16(kBodyCount2);
        for (int n = 0, seen = 0; seen < bodies && n < kBodyRecords; n++) {
            const uint32_t body = kBodies + kBodySize * static_cast<uint32_t>(n);
            const uint16_t flags = Ram16(body);
            if (!(flags & 0x8000)) {
                continue;
            }
            seen++;
            if (!(flags & 0x4000)) {
                continue;
            }
            const int y = static_cast<int16_t>(Ram16(body + 2)), x = static_cast<int16_t>(Ram16(body + 4));
            const int up = static_cast<int16_t>(Ram16(body + 0x10)), down = static_cast<int16_t>(Ram16(body + 0x12)),
                      right = static_cast<int16_t>(Ram16(body + 0x14)), left = static_cast<int16_t>(Ram16(body + 0x16));
            const int x0 = Column(x - left), x1 = Column(x + right), y0 = Row(y + up), y1 = Row(y - down);
            g_seen.bodies.push_back({x0, y0, x1 - x0, y1 - y0, 255, 140, 0, 40});
        }
        return false;
    }

    void BodyBoxes(std::vector<HudBox> &boxes) {
        // The enemies' bodies, each a box the ships' boxes are tested against (04DFC4), as the hit test saw them
        if (g_seen.frame + 1 >= g_frame) {
            boxes.insert(boxes.end(), g_seen.bodies.begin(), g_seen.bodies.end());
        }
    }

    void ShotBoxes(std::vector<HudBox> &boxes) {
        // The enemies' shots, each a point tested against the ships' boxes (040B02)
        for (int n = 0; n < kShotCount; n++) {
            const uint32_t shot = kShots + kShotSize * static_cast<uint32_t>(n);
            const uint16_t flags = Ram16(shot);
            if (!(flags & 0x8000) || (flags & 0x5180)) {
                continue;
            }
            const int y = Row(static_cast<int16_t>(Ram16(shot + 2))), x = Column(static_cast<int16_t>(Ram16(shot + 4)));
            // (a point: a bright dot in a black edge)
            boxes.push_back({x - 2, y - 2, 5, 5, 0, 0, 0, 255});
            boxes.push_back({x - 1, y - 1, 3, 3, 255, 255, 0, 255});
        }
    }

    void Readout(int player, std::vector<std::string> &lines) {
        const uint32_t ship = Ship(player);
        lines.push_back("Power " + std::to_string(Ram16(ship + kShipPower) / 2 + 1) + "/" + std::to_string(kMostPower + 1));
        const int safe = Ram8(ship + kShipSafe);
        if (safe > 0 && safe < 0xFF) {
            lines.push_back("Out of harm " + std::to_string(safe));
        }
    }

    uint64_t FromBcd(uint32_t bcd) {
        uint64_t value = 0;
        for (int shift = 28; shift >= 0; shift -= 4) {
            value = value * 10 + ((bcd >> shift) & 0xF);
        }
        return value;
    }
} // namespace

// Whether the player's own choices (free play, the cheats, where a game starts, the board's continue and demo sound)
// are left out now
bool ChoicesLeftOut() {
    return shmup::AnyMode();
}
bool AttractModeOn() {
    return shmup::AttractMode();
}
bool ScoreModeOn() {
    return shmup::NoContinue();
}

void ModesBeforeFrame(Machine &machine) {
    g_frame++;
    shmup::BeforeFrame(machine);
}
void ModesFrame(Machine &machine) {
    shmup::Frame(machine);
}
void ModesState(Archive &ar) {
    shmup::State(ar);
}

void RegisterModes() {
    shmup::Description d;
    d.stages = {"Stage 1", "Stage 2", "Stage 3", "Stage 4", "Stage 5"};
    // (the game's numbers: the pilot times three and the gunner)
    d.fighters = {"Ring with Chain", "Ring with Nail", "Ring with Rivet", "Bolt with Chain", "Bolt with Nail", "Bolt with Rivet"};
    d.canStart = [](const Machine &) { return Ram16(kMode) != kModeWarning; };
    d.credit = [] { SetRam8(kCredits, 1); };
    d.credits = [] { return static_cast<int>(Ram8(kCredits)); };
    d.toPlay = ToPlay;
    d.inGame = InGame;
    d.playing = [] { return Ram16(kMode) == kModePlay; };
    d.stage = [] { return static_cast<int>(Ram16(kStage)); };
    d.bossOn = [] { return Ram8(kBossOn) != 0 && Ram16(kMode) == kModePlay; };
    d.score = [](int player) { return FromBcd(Ram32(kScore1 + 4 * static_cast<uint32_t>(player))) * 10; };
    d.setShips = [](int player, int ships) { SetRam16(kShips + 2 * static_cast<uint32_t>(player), static_cast<uint16_t>(ships)); };
    d.setBombs = [](int player, int bombs) { SetRam16(Ship(player) + kShipBombs, static_cast<uint16_t>(bombs)); };
    d.setPower = [](int player, int power) {
        SetRam16(Ship(player) + kShipPower, static_cast<uint16_t>(power * 2));
        SetRam16(Ship(player) + kShipPowerHighest, static_cast<uint16_t>(power * 2));
    };
    d.maxShips = 9;
    d.maxPower = kMostPower;
    d.readout = Readout;
    d.readoutWords = "At the left: the shot's power (1 to 5) and, after a ship's start, the frames it is still out of harm.";
    d.maxBombs = kMostBombs;
    d.shipBoxes = ShipBoxes;
    d.bodyBoxes = BodyBoxes;
    d.shotBoxes = ShotBoxes;
    // (the object list is made from the places of the frame before: the shots' points sit on their shots so, not a frame
    // ahead of them; found by comparing pictures both ways, over thousands of shots)
    d.listLag = 1;
    d.summaryWords = "When a boss comes and at a stage's end: how long the way to the boss took, and the boss, the points scored, the ships "
                     "lost and the bombs used.";
    d.ships = [](int player) { return static_cast<int>(Ram16(kShips + 2 * static_cast<uint32_t>(player))); };
    d.bombs = [](int player) { return static_cast<int>(Ram16(Ship(player) + kShipBombs)); };
    d.shows = {"The demonstrations only"};
    d.showsWords = "What attract mode plays: everything the board does, or only the demonstrations of play, over and over.";
    d.showPart = [] { return Ram16(kMode) == kModeDemo ? 1 : 0; };
    d.showBegun = [](const Machine &) { return Ram16(kMode) != kModeWarning; };
    d.viewLayers = kStageLayers;
    shmup::Register(d);
    AddHook(HookAt(kHitTest), "BodiesSeen", BodiesSeen);
}

} // namespace cps2recomp
