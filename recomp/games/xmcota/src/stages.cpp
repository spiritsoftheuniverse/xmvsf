// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Other games' stages behind X-Men: Children of the Atom's fighters, as those games show them: every other fighting game's in the
// collection (fight/marvel_stages.h, as for each game on the Marvel engine). Here are this game's facts: its stage,
// its match, who is fought there, and its objects that are not its stage's (its fighters, their shadows, its panel).
#include "marvel_stages.h"
#include "xmcota.h"

namespace cps2recomp {

void RegisterStages() {
    using namespace xmcota;
    marvel_stages::Facts facts;
    facts.id = "xmcota";
    facts.stages = kStages;
    facts.stage = kStage;
    facts.playing = kPlaying;
    facts.gameStep = kGameStep;
    facts.gameMatch = kGameMatch;
    facts.player1 = kPlayer1;
    facts.playerSize = kPlayerSize;
    facts.character = kCharacter;
    facts.notTheStage = NotTheStage;
    facts.panel = ThePanel;
    marvel_stages::Register(facts);
}

} // namespace cps2recomp
