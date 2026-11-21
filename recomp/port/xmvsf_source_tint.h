// XMVSF 87.493: read-only F9 selected native OAM ink tint.
// Uses ONLY the renderer's per-pixel source ownership. No tile-ID,
// colour-key, screen rectangle or game/ROM/RAM modifications.
#pragma once
#include "../video/visible_oam_ink.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace port::xmvsf_home::source_tint {
constexpr std::uint32_t Magenta(std::uint32_t source) {
    // Recomp presentation pixels are 0xAABBGGRR. Preserve alpha and
    // enough original luminance/texture to identify letters/characters.
    const unsigned r = (source & 255u);
    const unsigned g = (source >> 8) & 255u;
    const unsigned b = (source >> 16) & 255u;
    const unsigned rr = (r * 2u + 255u * 3u) / 5u;
    const unsigned gg = (g * 2u + 16u * 3u) / 5u;
    const unsigned bb = (b * 2u + 255u * 3u) / 5u;
    return (source & 0xFF000000u) | (bb << 16) | (gg << 8) | rr;
}
inline unsigned Paint(std::vector<std::uint32_t> &pixels, int width, int height,
                      const cps2recomp::native_video::VisibleOamInk &owners,
                      unsigned slot, int left, int top, int right, int bottom) {
    if (!owners.Valid() || slot >= cps2recomp::native_video::VisibleOamInk::kObjects ||
        width <= 0 || height <= 0 ||
        pixels.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        return 0u;
    // Scan measured visible bounds only. OwnerAt is still the authority:
    // another object or scroll pixel within this rectangle is untouched.
    left = std::max(left, 0); top = std::max(top, 0);
    right = std::min(right, width - 1); bottom = std::min(bottom, height - 1);
    unsigned tinted = 0u;
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            if (owners.OwnerAt(x, y) != static_cast<int>(slot)) continue;
            auto &pixel = pixels[static_cast<std::size_t>(y) *
                                 static_cast<std::size_t>(width) +
                                 static_cast<std::size_t>(x)];
            pixel = Magenta(pixel);
            ++tinted;
        }
    }
    return tinted;
}
} // namespace port::xmvsf_home::source_tint
