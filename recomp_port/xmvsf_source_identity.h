// XMVSF 87.490: native ROM object signatures established from F6/F9 OAM
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

// The 2026-11-20 F6 captures show 30 consecutive wall/logo objects
// at slots 4..39, all source code 1C60A, attr 2318 (palette 24,
// 4x3 object), level 7. This is a WHOLE OAM OBJECT exemption from
// HUD shock clipping, never a per-tile blacklist or visibility kill.
constexpr bool IsWallBlock(const OamIdentity &o) {
    return o.slot >= 4u && o.slot <= 39u &&
           o.tile == 0x1C60Au &&
           o.attributes == 0x2318u &&
           (o.attributes & 31u) == 24u &&
           o.priority == 7u &&
           o.across == 4u && o.down == 3u;
}
} // namespace cps2recomp::xmvsf_sources
