// Native CPS2 object pixels, used only to suppress the actual P2 selector.
// Matches Video::Unpack / Raster::Unpack: four bitplanes, two 8px groups per row.
// No guessed tile IDs, region masking, or changes to game RAM.
#pragma once
#include <cstddef>
#include <cstdint>
namespace cps2recomp::native_video {
inline bool IsP2SelectorTile(const std::uint8_t* graphics, std::size_t bytes,
                             std::uint32_t tile, std::uint16_t attributes) {
    if ((attributes & 0x1fu) != 27u || !graphics ||
        tile >= bytes / 128u) return false;
    const std::size_t base = static_cast<std::size_t>(tile) * 128u;
    unsigned mask = 0;
    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t group = 0; group < 2; ++group) {
            const std::uint8_t* p = graphics + base + row * 8u + group * 4u;
            for (unsigned bit = 0; bit < 8; ++bit) {
                const unsigned pen = ((p[0] >> bit) & 1u) |
                                     (((p[1] >> bit) & 1u) << 1) |
                                     (((p[2] >> bit) & 1u) << 2) |
                                     (((p[3] >> bit) & 1u) << 3);
                if (pen != 15u) mask |= 1u << pen;
            }
        }
    }
    // Actual XMVSF palette 27 P2 outline pens from native OAM capture.
    // P1 is 13/14; other visible pixels are never suppressed.
    constexpr unsigned kP2 = (1u << 10) | (1u << 11) | (1u << 12);
    return (mask & kP2) != 0 && (mask & ~kP2) == 0;
}
} // namespace cps2recomp::native_video
