// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy
//
// The places these sources name in the game's program and memory are Super Street Fighter II Turbo's (ssf2xjr1).
// Capcom made Super Turbo from Super Street Fighter II's program, and Super Street Fighter II (games/ssf2) builds these
// sources too: in it, most routines, tables and some variables are somewhere else. At() gives where the one named is in
// the game being built, from that game's own list (its family.h; tools/places.py makes it). For Super Turbo there is no
// list: a place is where it is said to be (runtime/places_in.h).
#pragma once

#include "family.h"
#include "places_in.h"

#include <cstdint>

namespace ssf2t {

// A place in the ROM (code or a table) or in work RAM (FFxxxx), as Super Turbo has it
consteval uint32_t At(uint32_t place) {
    return cps2recomp::PlaceIn(family::kPlaces, place);
}

// A variable of the game's, as a distance from A5 (FF8000), as Super Turbo has it
consteval int32_t AtA5(int32_t offset) {
    return cps2recomp::VariableIn(family::kPlaces, offset);
}

} // namespace ssf2t
