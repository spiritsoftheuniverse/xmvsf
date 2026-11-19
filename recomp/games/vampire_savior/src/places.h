// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The places these sources name in the game's program and memory are Vampire Savior's. Capcom made two more
// games from the same program, Vampire Savior 2 and Vampire Hunter 2 (games/vsav2, games/vhunt2), which build
// these sources too: in them, most routines, tables and variables are somewhere else. At() gives where the one
// named is in the game being built, from that game's own list (its family.h; tools/places.py makes it). For
// Vampire Savior there is no list: a place is where it is said to be (runtime/places.h).
#pragma once

#include "family.h"
#include "places_in.h"

#include <cstdint>

namespace vsav {

// A place in the ROM (code or a table) or in work RAM (FFxxxx), as Vampire Savior has it
consteval uint32_t At(uint32_t place) {
    return cps2recomp::PlaceIn(family::kPlaces, place);
}

// A variable of the game's, as a distance from A5 (FF8000), as Vampire Savior has it
consteval int32_t AtA5(int32_t offset) {
    return cps2recomp::VariableIn(family::kPlaces, offset);
}

} // namespace vsav
