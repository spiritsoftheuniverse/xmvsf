// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Other games' stages behind the fighters of a game on the Marvel engine (fight/marvel.h: Marvel Super Heroes, X-Men:
// Children of the Atom, X-Men vs. Street Fighter, Marvel Super Heroes vs. Street Fighter, Marvel vs. Capcom), as those
// games show them (fight/guest_stages.h): every other fighting game's in the collection, in fight/all_guests.h's order
// (game.cfg's guests). Each game's src/stages.cpp gives its facts; its camera is read as its own guest.h reads it (the
// record by the stage, its place and ends), its height the record's lowest place less its place down.
//
// Which: a stage chosen by name (the menu's Stage page's, the stage viewer's: the guests'
// after the game's own), or, with the "Other games' stages" feature, an arcade match's stage is one of theirs, the same
// one for the same match. Under one of theirs the game fights on its first (modes.cpp, where it takes the stage).
#pragma once

#include "alternative.h"
#include "guest_stages.h"
#include "session.h"

#include "all_guests.h"

#include <algorithm>
#include <cstring>

namespace cps2recomp {

bool BattleMode(); // (fight/battle.h)
int BattleStage();

namespace marvel_stages {

struct Facts {
    const char *id = nullptr; // (its guest's, fight/all_guests.h)
    int stages = 0;           // its own
    uint32_t stage = 0;       // the stage, a byte
    uint32_t playing = 0, gameStep = 0;
    uint16_t gameMatch = 8;
    uint32_t player1 = 0, playerSize = 0x400, character = 0;
    bool (*notTheStage)(uint32_t tile, uint16_t attributes) = nullptr;
    bool (*panel)(uint32_t tile, uint16_t attributes) = nullptr;
};

inline Facts &TheFacts() {
    static Facts facts;
    return facts;
}
// (its own stages as a guest: its camera's record and places)
inline guest_stages::Guest &Own() {
    static guest_stages::Guest own;
    return own;
}

// A match's stage: from a round's start to its end (its versus screen is a step of the game's before)
inline bool InMatch() {
    const Facts &f = TheFacts();
    return Ram8(f.playing) != 0 && Ram16(f.gameStep) == f.gameMatch;
}

// Its camera: across, its ends, and how high above its lowest it is
inline void Camera(int &x, int &left, int &right, int &up) {
    const guest_stages::Guest &own = Own();
    const uint32_t record = own.cameraRecord(Ram(0xFF0000));
    const auto word = [](uint32_t address) { return static_cast<int>(static_cast<int16_t>(Ram16(address))); };
    x = word(record + own.placeX);
    left = word(record + own.endLeft);
    right = word(record + own.endRight);
    // (never below its lowest: as a round is put together, its record not yet the stage's)
    up = std::max(0, word(record + own.endBottom) - word(record + own.placeY));
}

// The stage wanted now, 1 the guests' first (fight/all_guests.h's order); 0 the game's own
inline int Wanted() {
    const Facts &f = TheFacts();
    const int stages = guest_stages::Stages();
    if (BattleMode()) {
        // (a mode's stages are the game's own, but the player's stage when it is one of the guests': BattleStage)
        const int chosen = BattleStage();
        return chosen > f.stages && chosen <= f.stages + stages ? chosen - f.stages : 0;
    }
    const int wanted = session::WantedStage();
    if (wanted > f.stages && wanted <= f.stages + stages) {
        return wanted - f.stages;
    }
    if (wanted >= 1 && wanted <= f.stages) {
        return 0;
    }
    if (!FeatureOn("other_stages") || !InMatch() || stages == 0) {
        return 0;
    }
    // One of theirs, the same for the same match: from the stage the game chose and who is fought there
    const int from = (Ram8(f.stage) * 7 + Ram16(f.player1 + f.character) * 3 + Ram16(f.player1 + f.playerSize + f.character)) % stages;
    for (int n = 0; n < stages; n++) {
        const int stage = (from + n) % stages + 1;
        if (guest_stages::Available(stage)) {
            return stage;
        }
    }
    return 0;
}

// The modes' stage lists: the game's own, then the other games' (each plays the game's first stage's tune: its own)
inline void AddStages(modes::Description &d, const char *id, int stages) {
    const std::vector<const char *> others = guests::NamesBut(id);
    d.stageNames.insert(d.stageNames.end(), others.begin(), others.end());
    d.infiniteStages = stages;
    if (!d.stageTunes.empty()) {
        d.stageTunes.resize(d.stageNames.size(), d.stageTunes[0]);
    }
}

// The game's sources describe it (last of their registering: this goes around the frame's callbacks and the state's)
inline void Register(const Facts &facts) {
    TheFacts() = facts;
    for (const guests::Entry &e : guests::All()) {
        if (std::strcmp(e.id, facts.id) == 0) {
            Own() = e.description();
        }
    }
    Game &game = TheGame();
    // (Display: it changes nothing of the game's play; online, each player sees the stage of their own choice)
    game.features.push_back({"other_stages", "Display", "Other games' stages",
                             "An arcade match is fought on a stage of another game's, as that game shows it, its music too: "
                             "the other fighting games in the collection, those whose ROM sets are there (beside this "
                             "game's, or chosen on their pages in the launcher). The menu's Stage page and the stage "
                             "viewer's list have them all after this game's own.",
                             0});
    guest_stages::Host host;
    host.camera = Camera;
    host.shown = [] { return modes::StageViewer() || InMatch(); };
    host.viewer = modes::StageViewer;
    // (what is laid over another game's stage: the fighters, their shadows and the panel; not this game's own stage's
    // pieces; the panel over the other stage's pieces in front of the fighters)
    host.notTheStage = facts.notTheStage;
    host.panel = facts.panel;
    host.stageTunes = Own().stageTunes;
    host.wanted = Wanted;
    host.feature = "other_stages";
    guest_stages::Register(host, guests::AllBut(facts.id));
}

} // namespace marvel_stages
} // namespace cps2recomp
