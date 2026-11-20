// 87.476 XMVSF F10 M68K capture: native P1 selection phase transitions.
// Frame 705: 01 -> 02 (first team pick, 00B1E8 runs).
// Frame 737: 02 -> 03 (second pick phase begins).
// Frame 762: 03 -> 04 (second team pick confirmed, NO 00B1E8).
// Frame 803: 04 -> 06 (native tail); frame 848: 06 -> 07 (ready).
#pragma once
#include <cstdint>
namespace cps2recomp::xmvsf {
inline constexpr bool NativeP1SecondPickCommitted(std::uint8_t phase) {
    return phase == 4u || phase == 6u;
}
}
