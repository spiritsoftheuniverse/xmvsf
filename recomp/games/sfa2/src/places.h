// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The places these sources name in the game's program and memory are Street Fighter Alpha 2's (sfa2u). Capcom made
// Street Fighter Zero 2 Alpha (games/sfz2al) from the same program, and it builds these sources too: in it, most
// routines, tables and some variables are somewhere else. At() gives where the one named is in the game being
// built, from that game's own list (its family.h; tools/places.py makes it). For Alpha 2 there is no list: a place
// is where it is said to be (runtime/places_in.h).
#pragma once

#include "family.h"
#include "places_in.h"

#include <cstdint>

namespace sfa2 {

// A place in the ROM (code or a table) or in work RAM (FFxxxx), as Alpha 2 has it
consteval uint32_t At(uint32_t place) {
    return cps2recomp::PlaceIn(family::kPlaces, place);
}

// A variable of the game's, as a distance from A5 (FF8000), as Alpha 2 has it
consteval int32_t AtA5(int32_t offset) {
    return cps2recomp::VariableIn(family::kPlaces, offset);
}

} // namespace sfa2
