// XMVSF 87.495: source-proven paired Training dummy-selector groups.
// F6 frame 1222 (2026-11-21 00:53:17 / 00:53:27) records OAM
// slots 229..241 as two adjacent sets: five palette-27 objects and
// eight palette-28 objects. The two user-selected anchors are
// 1BB30/121B (249 visible pixels) and 1BB25/121C (427 pixels).
//
// OAM slots, bank IDs, ink counts, and screen rectangles are NOT
// identities. Match native source code + FULL native attributes only.
// This is intentionally limited to the 13 verified records: nearby
// palette-28 objects 242..247 include unrelated character-select art.
#pragma once
#include <cstdint>

namespace cps2recomp::native_video {
inline constexpr bool HideUntilDummySelection(bool trainingChoosing,
                                               bool p1SecondPickCommitted) {
    return trainingChoosing && !p1SecondPickCommitted;
}
inline constexpr bool IsTrainingDummySelectorSource(std::uint32_t tile,
                                                    std::uint16_t attr) {
    switch (attr) {
    case 0x121Bu: return tile == 0x1BB30u;  // P2 set, 249 ink
    case 0x011Bu: return tile == 0x1BB28u;
    case 0x001Bu: return tile == 0x1BB23u || tile == 0x1BB33u ||
                           tile == 0x1BB34u;
    case 0x121Cu: return tile == 0x1BB25u;  // adjacent set, 427 ink
    case 0x011Cu: return tile == 0x1BB28u;
    case 0x003Cu: return tile == 0x1B819u;
    case 0x001Cu: return tile == 0x1BB23u || tile == 0x1BB2Cu ||
                           tile == 0x1BB2Du || tile == 0x1BB2Eu ||
                           tile == 0x1BB2Fu;
    default: return false;
    }
}
} // namespace cps2recomp::native_video
