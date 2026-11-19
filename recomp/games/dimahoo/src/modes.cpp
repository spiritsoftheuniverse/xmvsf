// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Dimahoo: the menu's "Alternative modes" and training mode (the toolkit's shmup/shmup.h): what they need of the game.
//
// A mode gets into a game the way a player does: a credit, Start, the choice of character (the cursor put on it and the
// shot button pressed, or the player's own choice in the scored modes), and the stage begins (scene 0C). The stage it
// begins at is put in as the choice ends (game.cpp).
#include "dimahoo.h"
#include "shmup.h"
#include "state.h"

namespace cps2recomp {

namespace {
    using namespace dimahoo;

    bool InGame() {
        return Ram16(kStep) == kStepGame && Ram8(kGameOn) == 1;
    }

    bool Playing() {
        return InGame() && Ram16(kScene) == kScenePlay;
    }

    bool ShipInPlay(int n) {
        return Ram8(Player(n) + kPlayerState) == 1 && (Ram16(Ship(n) + 2) & 0x8000);
    }

    bool ToPlay(Machine &machine, const shmup::Begin &begin, int32_t frames) {
        if (!InGame()) {
            return false;
        }
        const uint16_t scene = Ram16(kScene);
        // (played once player 1's ship is there: a ship's start puts its bombs back, so what training mode sets is put in
        // after it)
        if (scene == kScenePlay) {
            return ShipInPlay(0);
        }
        if (scene == kSceneChoose && begin.fighter >= 0) {
            // (the cursor put on the character, then the shot button to take it)
            SetRam8(Player(0) + kPlayerCharacter, static_cast<uint8_t>(begin.fighter));
            machine.players[0] = (frames & 8) ? kPunch1 : 0;
        }
        return false;
    }

    // The picture's row for a place across (dimahoo.h: the stage pans across with the ships)
    int Row(int across) {
        return across - static_cast<int16_t>(Ram16(kPan)) - kPanOffset;
    }

    // A box in the game's places (across and up, dimahoo.h) as the picture's: its column is up less 64, its row across
    // less the pan
    HudBox Box(int left, int right, int low, int high, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return {low - kUpOffset, Row(left), high - low, right - left, r, g, b, a};
    }

    void ShipBoxes(std::vector<HudBox> &boxes) {
        // (no ship can be hit while a bomb goes off: the box the shots are tested against is then the bomb's, which
        // takes them in; dimahoo.h)
        if (Ram8(kHitsLetBy) || Ram8(kHitsLetBy2)) {
            return;
        }
        for (int n = 0; n < kPlayers2; n++) {
            const uint32_t ship = Ship(n);
            if (!InGame() || !ShipInPlay(n)) {
                continue;
            }
            const int left = static_cast<int16_t>(Ram16(ship + kShipBoxLeft)), right = static_cast<int16_t>(Ram16(ship + kShipBoxRight));
            const int low = static_cast<int16_t>(Ram16(ship + kShipBoxLow)), high = static_cast<int16_t>(Ram16(ship + kShipBoxHigh));
            if (right <= left || high <= low) {
                continue;
            }
            // (bright green in a black edge, to stand out from any ship)
            boxes.push_back(Box(left - 1, right + 1, low - 1, high + 1, 0, 0, 0, 255));
            boxes.push_back(Box(left, right, low, high, 0, 255, 0, 255));
        }
    }

    // The enemies' shots and bodies as the hit tests last saw them: the game makes its grids during a frame and they are
    // gone before it ends, so they are read at the tests (hooks that only look). For drawing only, so kept out of the
    // state: `frame` is the count of frames they were read at, for leaving out what no test read this frame. Their rows
    // are places across, made the picture's as they are drawn (the pan as the frame ends, as the ships').
    struct Seen {
        std::vector<HudBox> shots, bodies;
        uint64_t shotsFrame = 0, bodiesFrame = 0;
    } g_seen;
    uint64_t g_frame = 0;

    bool ShotsSeen(M68K &) {
        if (g_seen.shotsFrame == g_frame) {
            return false; // (the second ship's test: the same grid)
        }
        g_seen.shots.clear();
        g_seen.shotsFrame = g_frame;
        const uint32_t grid = 0xFF0000 | Ram16(kShotGrid);
        for (uint32_t cell = 0; cell < kShotGridSize; cell += 2) {
            uint16_t node = Ram16(grid + cell);
            for (int guard = 0; (node & 0x8000) && guard < 256; guard++) {
                const uint32_t shot = 0xFF0000 | Ram16(0xFF0000 | static_cast<uint16_t>(node + 2));
                if (!(Ram16(shot + 0x48) & 0x80)) {
                    const int across = static_cast<int16_t>(Ram16(shot + 0x32)), up = static_cast<int16_t>(Ram16(shot + 0x30));
                    // (a point: a bright dot in a black edge)
                    g_seen.shots.push_back({up - kUpOffset - 2, across - 2, 5, 5, 0, 0, 0, 255});
                    g_seen.shots.push_back({up - kUpOffset - 1, across - 1, 3, 3, 255, 255, 0, 255});
                }
                node = Ram16(0xFF0000 | node);
            }
        }
        return false;
    }

    bool BodiesSeen(M68K &c) {
        if (g_seen.bodiesFrame == g_frame) {
            return false;
        }
        g_seen.bodies.clear();
        g_seen.bodiesFrame = g_frame;
        const uint8_t kinds = Ram8(c.a[6] + 0x49);
        const uint32_t grid = 0xFF0000 | Ram16(kBodyGrid);
        for (uint32_t cell = 0; cell < kBodyGridSize; cell += 2) {
            uint16_t node = Ram16(grid + cell);
            for (int guard = 0; (node & 0x8000) && guard < 256; guard++) {
                const uint32_t at = 0xFF0000 | node;
                if (Ram16(at + 2) & kinds) {
                    const uint32_t body = Ram32(at + 4) & 0xFFFFFF;
                    const int left = static_cast<int16_t>(Ram16(body + 8)), right = static_cast<int16_t>(Ram16(body + 0xA));
                    const int low = static_cast<int16_t>(Ram16(body + 0xC)), high = static_cast<int16_t>(Ram16(body + 0xE));
                    if (right > left && high > low) {
                        g_seen.bodies.push_back({low - kUpOffset, left, high - low, right - left, 255, 140, 0, 40});
                    }
                }
                node = Ram16(at);
            }
        }
        return false;
    }

    void Seen(const std::vector<HudBox> &seen, uint64_t frame, std::vector<HudBox> &boxes) {
        if (frame + 1 >= g_frame) {
            for (HudBox box : seen) {
                box.y = Row(box.y);
                boxes.push_back(box);
            }
        }
    }
    void BodyBoxes(std::vector<HudBox> &boxes) {
        Seen(g_seen.bodies, g_seen.bodiesFrame, boxes);
    }
    void ShotBoxes(std::vector<HudBox> &boxes) {
        Seen(g_seen.shots, g_seen.shotsFrame, boxes);
    }

    void Readout(int player, std::vector<std::string> &lines) {
        lines.push_back("Magic " + std::to_string(Ram8(Player(player) + kPlayerMagic)) + "/" + std::to_string(kMagicLevel));
    }

    // The bosses' music (dimahoo.h): a boss is being fought from its "has appeared" to its end
    bool BossOn() {
        const uint16_t music = Ram16(kMusic);
        return Playing() && (music == 0xC9 || music == 0xCA || music == 0xCB || music == 0xCC);
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
    d.stages = {"Stage 1", "Stage 2", "Stage 3", "Stage 4", "Stage 5", "Stage 6"};
    // (the game's numbers)
    d.fighters = {"Solo-Bang", "Karte", "Miyamoto", "Grimlen"};
    d.canStart = [](const Machine &) {
        const uint16_t step = Ram16(kStep);
        return step != 0 && step != kStepBoot;
    };
    d.credit = [] { SetRam8(kCredits, 1); };
    d.credits = [] { return static_cast<int>(Ram8(kCredits)); };
    d.toPlay = ToPlay;
    d.inGame = InGame;
    d.playing = Playing;
    d.stage = [] { return static_cast<int>(Ram16(kStageCount)); };
    d.bossOn = BossOn;
    d.score = [](int player) { return static_cast<uint64_t>(Ram32(Player(player) + kPlayerScore)); };
    // (the ships in stock and the one in play)
    d.setShips = [](int player, int ships) { SetRam8(Player(player) + kPlayerShips, static_cast<uint8_t>(ships > 0 ? ships - 1 : 0)); };
    d.setBombs = [](int player, int bombs) { SetRam8(Player(player) + kPlayerBombs, static_cast<uint8_t>(bombs)); };
    d.maxShips = 9;
    d.maxBombs = kMostBombs;
    d.readout = Readout;
    d.readoutWords = "The magic gauge: the items taken toward the next magic level (48 make one).";
    d.shipBoxes = ShipBoxes;
    d.bodyBoxes = BodyBoxes;
    d.shotBoxes = ShotBoxes;
    d.summaryWords = "When a boss comes and at a stage's end: how long the way to the boss took, and the boss, the points scored, the ships "
                     "lost and the bombs used.";
    d.ships = [](int player) { return static_cast<int>(Ram8(Player(player) + kPlayerShips)) + 1; };
    d.bombs = [](int player) { return static_cast<int>(Ram8(Player(player) + kPlayerBombs)); };
    d.shows = {"The demonstrations only"};
    d.showsWords = "What attract mode plays: everything the board does, or only the demonstrations of play, over and over.";
    d.showPart = [] { return Ram16(kStep) == kStepDemo ? 1 : 0; };
    d.showBegun = [](const Machine &) {
        const uint16_t step = Ram16(kStep);
        return step != 0 && step != kStepBoot;
    };
    d.viewLayers = kStageLayers;
    shmup::Register(d);
    AddHook(HookAt(kShotTest), "ShotsSeen", ShotsSeen);
    AddHook(HookAt(kBodyTest), "BodiesSeen", BodiesSeen);
}

} // namespace cps2recomp
