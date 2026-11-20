// XMVSF 87.477: canonical, ROM-backed P1 team-selection milestone.
//
// FBNeo's working Stage17 (87.68) reads FF5183, the P1 ROM-accepted
// character count. It routes P1's physical controller to P2 when it
// reaches 2, not on the second 00B1E8 hook or an inferred phase.
//
// The recomp's F10 capture independently confirms FF5183:
// native frame 706: 0 -> 1 (first team pick)
// native frame 763: 1 -> 2 (second team pick)
//
// This helper receives that byte from native RAM. Only Training uses
// it for P1->P2 controller ownership; no RAM write or ROM patch.
#pragma once
namespace cps2recomp::xmvsf {
inline constexpr bool NativeP1TeamAccepted(unsigned acceptedCount) {
    return acceptedCount == 2u;
}
} // namespace cps2recomp::xmvsf
