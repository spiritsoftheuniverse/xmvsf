// Native M68K selection RAM write translation for the new 18-slot layout.
// Operates at the original write boundary, BEFORE native logic reads its
// selected ID again; not a post-frame cosmetic correction.
#pragma once
#include <array>
#include <cstdint>

namespace port::xmvsf_select {

inline constexpr uint16_t RewriteNativeSelectionWrite(
    uint32_t address, uint16_t value, unsigned bytes,
    const std::array<uint16_t, 2> &selected,
    const std::array<uint8_t, 2> &accepted) {
    if (bytes != 1u && bytes != 2u) return value;
    address &= 0xFFFFFFu;
    for (unsigned n = 0; n < 2u; ++n) {
        const uint32_t cursor = 0xFF518Cu + n * 0x20u;
        const uint32_t firstPick = 0xFF50B0u + n * 4u;
        const uint32_t pendingPick = firstPick + 2u * accepted[n];
        bool rewrite = (accepted[n] < 2u &&
                         (address == cursor || address == cursor + 1u));
        // P1 first/second and P2 first/second; never change a previously
        // committed partner again when the ROM refreshes its RAM.
        if (accepted[n] < 2u &&
            (address == pendingPick || address == pendingPick + 1u))
            rewrite = true;
        if (!rewrite) continue;
        if (bytes == 2u) return selected[n];
        return static_cast<uint16_t>((address & 1u) != 0u
            ? selected[n] & 0xFFu : selected[n] >> 8u);
    }
    return value;
}
} // namespace port::xmvsf_select
