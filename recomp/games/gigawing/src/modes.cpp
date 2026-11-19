// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Giga Wing: the menu's "Alternative modes" and training mode (the toolkit's shmup/shmup.h): what they need of the game.
//
// A mode gets into a game the way a player does: a credit, Start, the choice of fighter (the fighter put in the
// choice's variable and the shot button pressed, or the player's own choice in the scored modes), Start again to skip
// the lesson, and the stage begins (mode 06). The stage it begins at is put in where a game's start sets it (game.cpp).
#include "gigawing.h"
#include "shmup.h"
#include "state.h"

#include <algorithm>
#include <cstdio>

namespace cps2recomp {

namespace {
    using namespace gigawing;

    std::string Bcd(uint32_t bcd) {
        char text[16];
        std::snprintf(text, sizeof text, "%X", bcd);
        // (grouped by thousands, as the scores are)
        std::string digits = text, out;
        for (size_t n = 0; n < digits.size(); n++) {
            if (n && (digits.size() - n) % 3 == 0) {
                out += ',';
            }
            out += digits[n];
        }
        return out;
    }
    uint64_t FromBcd(uint32_t bcd) {
        uint64_t value = 0;
        for (int shift = 28; shift >= 0; shift -= 4) {
            value = value * 10 + ((bcd >> shift) & 0xF);
        }
        return value;
    }

    // The picture's row for a height the game counts up from the picture's foot
    int Row(int up) {
        return Video::kHeight - up;
    }

    bool ToPlay(Machine &machine, const shmup::Begin &begin, int32_t frames) {
        const uint16_t mode = Ram16(kMode);
        if (mode == kModePlay) {
            return true;
        }
        if (mode == kModeChoose && begin.fighter >= 0) {
            // (the fighter put where Down steps it, then the shot button to take it)
            SetRam16(kFighter, static_cast<uint16_t>(begin.fighter));
            machine.players[0] = (frames & 8) ? kPunch1 : 0;
        } else if (mode == kModeLesson || mode == kModePilot) {
            machine.players[0] = (frames & 4) ? kStart : 0; // (Start skips the pilot's story and the lesson)
        }
        return false;
    }

    void ShipBoxes(std::vector<HudBox> &boxes) {
        for (int n = 0; n < kPlayers; n++) {
            const uint32_t ship = Ship(n);
            if (!((Ram8(kPlayersIn) >> n) & 1) || !(Ram16(ship + kShipFlags) & 0x8000)) {
                continue;
            }
            const int x = static_cast<int16_t>(Ram16(ship + kShipX)), y = static_cast<int16_t>(Ram16(ship + kShipY));
            // (signed: a ship's box can sit off its middle, up 7 and down -1, say)
            const int left = static_cast<int16_t>(Ram16(ship + kShipBoxLeft)), right = static_cast<int16_t>(Ram16(ship + kShipBoxRight)),
                      up = static_cast<int16_t>(Ram16(ship + kShipBoxUp)), down = static_cast<int16_t>(Ram16(ship + kShipBoxDown));
            // (bright green in a black edge, to stand out from any ship)
            boxes.push_back({x - left - 1, Row(y + up) - 1, left + right + 2, up + down + 2, 0, 0, 0, 255});
            boxes.push_back({x - left, Row(y + up), left + right, up + down, 0, 255, 0, 255});
        }
    }

    void BodyBoxes(std::vector<HudBox> &boxes) {
        // The enemies' bodies, each a box the ships' middles are tested against
        for (int n = 0; n < kBodies; n++) {
            const uint32_t body = kBody + kBodySize * static_cast<uint32_t>(n);
            if ((Ram16(body) & 0xE480) != 0x8000) {
                continue;
            }
            const int x = static_cast<int16_t>(Ram16(body + kBodyX)), y = static_cast<int16_t>(Ram16(body + kBodyY));
            const int right = static_cast<int16_t>(Ram16(body + kBodyRight)), left = static_cast<int16_t>(Ram16(body + kBodyLeft)),
                      up = static_cast<int16_t>(Ram16(body + kBodyUp)), down = static_cast<int16_t>(Ram16(body + kBodyDown));
            boxes.push_back({x - left, Row(y + up), left + right, up + down, 255, 140, 0, 40});
        }
    }

    void ShotBoxes(std::vector<HudBox> &boxes) {
        // The enemies' shots, each a point tested against the ships' boxes
        uint32_t shot = Ram32(kShots);
        for (int n = 0; shot >= 0xFF0000 && n < 1024; n++) {
            const int x = static_cast<int16_t>(Ram16(shot + kShotX)), y = static_cast<int16_t>(Ram16(shot + kShotY));
            // (a point: a bright dot in a black edge)
            boxes.push_back({x - 2, Row(y) - 2, 5, 5, 0, 0, 0, 255});
            boxes.push_back({x - 1, Row(y) - 1, 3, 3, 255, 255, 0, 255});
            shot = Ram32(shot);
        }
    }

    void Readout(int player, std::vector<std::string> &lines) {
        const uint32_t p = static_cast<uint32_t>(player);
        lines.push_back("Multiplier x" + Bcd(Ram32(kMultiplier + 4 * p)));
        lines.push_back("Medal chain +" + Bcd(Ram32(kChain + 4 * p)));
        lines.push_back("Power " + std::to_string(Ram16(kPower + 2 * p) + 1) + "/" + std::to_string(kMostPower + 1));
        const int reflect = Ram16(Ship(player) + kShipReflect);
        if (reflect > 0) {
            lines.push_back("Reflect " + std::to_string(std::min(reflect, kReflectFull) * 100 / kReflectFull) + "%");
        }
        if ((Ram8(kBossFlags) & (kBossClock | kBossOver)) == kBossClock) {
            lines.push_back("Boss " + std::to_string(Ram16(kBossSeconds)) + " s");
        }
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
    d.stages = {"Stage 1", "Stage 2", "Stage 3", "Stage 4", "Stage 5", "Stage 6", "Stage 7"};
    d.fighters = {"Sinnosuke (Raijin)", "Ruby (Carmine)", "Isha (Porchka)", "Stuck (Widerstand)", "The hidden, unfinished fifth"};
    d.canStart = [](const Machine &) { return Ram16(kMode) != 0; };
    d.credit = [] { SetRam8(kCredits, 1); };
    d.credits = [] { return static_cast<int>(Ram8(kCredits)); };
    d.toPlay = ToPlay;
    d.inGame = [] { return (Ram8(kPlayersIn) & 3) != 0; };
    d.playing = [] { return Ram16(kMode) == kModePlay; };
    d.stage = [] { return static_cast<int>(Ram16(kStage)); };
    d.bossOn = [] { return (Ram8(kBossFlags) & (kBossClock | kBossOver)) == kBossClock && Ram16(kMode) == kModePlay; };
    d.bossStage = [](int stage) { return stage == kLastStage; };
    d.score = [](int player) { return Score(player); };
    d.setShips = [](int player, int ships) { SetRam16(kShips + 2 * static_cast<uint32_t>(player), static_cast<uint16_t>(ships)); };
    d.setBombs = [](int player, int bombs) { SetRam16(kBombs + 2 * static_cast<uint32_t>(player), static_cast<uint16_t>(bombs)); };
    d.setPower = [](int player, int power) { SetRam16(kPower + 2 * static_cast<uint32_t>(player), static_cast<uint16_t>(power)); };
    d.maxShips = 9;
    d.maxBombs = kMostBombs;
    d.maxPower = kMostPower;
    d.shipBoxes = ShipBoxes;
    d.bodyBoxes = BodyBoxes;
    d.shotBoxes = ShotBoxes;
    // (the game moves everything every frame, but makes its object list every other frame, from the places of the frame
    // before: found from the list and an enemy shot's place, frame by frame)
    d.listLag = 1;
    d.readout = Readout;
    d.multiplier = [](int player) { return FromBcd(Ram32(kMultiplier + 4 * static_cast<uint32_t>(player))); };
    d.ships = [](int player) { return static_cast<int>(Ram16(kShips + 2 * static_cast<uint32_t>(player))); };
    d.bombs = [](int player) { return static_cast<int>(Ram16(kBombs + 2 * static_cast<uint32_t>(player))); };
    d.shows = {"The demonstrations only", "The story only"};
    d.showsWords = "What attract mode plays: everything the board does, or only the demonstrations of play, or only the story "
                   "(the Medallion), over and over.";
    d.showPart = [] {
        const uint16_t mode = Ram16(kMode);
        return mode == kModeDemo ? 1 : mode == kModeStory ? 2 : 0;
    };
    d.showBegun = [](const Machine &) { return Ram16(kMode) != 0; };
    d.viewLayers = kStageLayers;
    shmup::Register(d);
}

} // namespace cps2recomp
