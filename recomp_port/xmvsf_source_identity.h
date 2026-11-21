// XMVSF 87.491: native ROM object signatures established from F6/F9 OAM
// source records. Never classify a graphic by its screen pixels or colour.
// A hardware OAM bank can flip between frames; the bank itself is NOT
// part of the logical object identity.
#pragma once
#include <cstdint>

namespace cps2recomp::xmvsf_sources {
struct OamIdentity {
    unsigned slot = 0;       // hardware OAM object index, not ROM tile index
    unsigned tile = 0;       // full 20-bit source object graphics code
    unsigned attributes = 0; // unmodified 16-bit native OAM attr word
    unsigned priority = 0;   // x-word bits 13..15
    unsigned across = 0;     // native object block width, in 16px cells
    unsigned down = 0;       // native object block height, in 16px cells
};

// Native F6 proves the SAME 30 physical wall sources move between
// bank 0 slots 4..39 (frame 2014), and bank 1 slots 114..149
// (frame 816). Slot position is NOT part of their logical identity.
// Exempt the WHOLE verified 4x3 OAM object from HUD-shock clipping.
// Do not hide the source or blacklist component graphic tiles.
constexpr bool IsWallBlock(const OamIdentity &o) {
    return o.tile == 0x1C60Au &&
           o.attributes == 0x2318u &&
           (o.attributes & 31u) == 24u &&
           o.priority == 7u &&
           o.across == 4u && o.down == 3u;
}
} // namespace cps2recomp::xmvsf_sources
