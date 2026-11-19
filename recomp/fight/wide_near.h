// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// Widescreen: a stage's pieces in the wide picture's added columns (issue #344). The fighting games draw an object only
// while it is near the board's picture: a routine of theirs takes its place across from the scroll screen it rides on,
// adds a margin (0x40) and compares with the board's 384 columns and twice the margin (0x200), and has it drawn (its
// +01 set, on to its kind's list) or not (+01 cleared). The wide picture adds `side` columns on each side, so the pieces
// there (bicycles, crates, a statue's foot, cats) were left out. Played wide, a game's sources hook its routine and ask
// this instead, with the margin that much more on each side; with invisible walls they leave the game's code alone.
#pragma once

#include "lift.h"

#include <cstdint>

namespace cps2recomp::wide {

// From the routine's test across: `across` the object's place less the scroll screen's, `margin` and `room` the
// routine's (0x40 and 0x200), `side` the columns the picture played in adds on each side. D0 is left as the routine's
// test leaves it, and the CPU at `drawn` or `hidden`, where the routine goes on (true: the hook has done the test)
inline bool NearPicture(M68K &c, int across, int margin, int room, int side, uint32_t drawn, uint32_t hidden) {
    const uint16_t place = static_cast<uint16_t>(across + margin + side);
    c.d[0] = (c.d[0] & 0xFFFF0000u) | place;
    c.pc = place > room + 2 * side ? hidden : drawn;
    return true;
}

} // namespace cps2recomp::wide
