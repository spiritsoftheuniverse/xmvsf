#ifndef XMVSF_STAGE87_329_MORPHUS_DHC_CATALOG_H
#define XMVSF_STAGE87_329_MORPHUS_DHC_CATALOG_H
// Stage 87.329: read-only DHC preflight catalog from Morphus56k
// XMvsSF-3v3-DHC-v1.1.1, source/engine/dhc_table.inc.
// The source targets EURO 961004. XMVSFU ROM compatibility is UNVERIFIED.
// These are incoming native +D6 Hyper IDs, NOT 2E action selectors.
// This catalog cannot activate a Hyper or stand in for a native 68000 hook.
// Opt-in: XM87329_DHC_PREFLIGHT=1 plus the 87.274 experimental Version-4 DHC.
// Original author: Morphus56k / Daniel Plas Rivera.
#include <cstdint>
#include <cstdlib>
#include <cstring>
namespace XM87329Morphus {
enum Review { UNKNOWN_MOTION=0, POSSIBLE=1, NOT_REGISTERED=2,
              NO_HYPER=3, AIR_ONLY=4, CONDITIONAL=5, POSSIBLE_THROW=6 };
struct Row { std::uint8_t hyper[7]; std::uint8_t air, conditional, throwing; };
static inline const Row* rows() {
    static const Row a[] = {
        {{0x08,0,0x06,0,0,0,0},0,0,0}, // 00 Wolverine
        {{0x04,0x02,0,0,0,0,0},0,0,0}, // 02 Cyclops
        {{0x02,0x0A,0,0,0,0,0},0,0,0}, // 04 Storm
        {{0x28,0x2E,0,0,0,0,0},0,0x02,0x02}, // 06 Rogue
        {{0x0A,0,0,0,0,0,0},0,0,0}, // 08 Gambit
        {{0x16,0,0x0E,0,0,0x08,0},0,0,0}, // 0A Sabretooth
        {{0x02,0,0,0,0,0,0},0,0,0}, // 0C Juggernaut
        {{0x04,0x06,0,0,0,0,0},0,0,0}, // 0E Magneto
        {{0,0,0,0,0,0,0},0,0,0}, // 10 Apocalypse (nonplayable)
        {{0x0A,0x0C,0,0,0,0,0},0,0,0}, // 12 Ryu
        {{0x0C,0x0E,0,0,0,0,0},0,0,0}, // 14 Ken
        {{0x06,0x0A,0x08,0,0,0,0},0,0,0}, // 16 Chun-Li
        {{0x0A,0x0C,0,0,0,0,0},0,0,0}, // 18 Dhalsim
        {{0x0A,0,0,0,0,0,0},0,0,0x01}, // 1A Zangief
        {{0x0C,0x0A,0,0,0,0,0},0,0,0}, // 1C M. Bison
        {{0x0A,0x1A,0x10,0x16,0,0,0},0x01,0,0x04}, // 1E Akuma
        {{0x04,0x06,0x08,0,0,0,0},0,0,0}, // 20 Charlie
        {{0x0E,0x16,0,0,0,0,0},0,0,0}, // 22 Cammy
        {{0x06,0x0A,0x08,0,0,0,0},0,0,0} // 24 alt Chun-Li
    };
    return a;
}
// Column order: QCF, QCB, DP, reverse-DP, HCF, HCB, double-down.
// Map XM87192Owner::Motion to columns; 2QCF / 2QCB != double-down.
static inline int columnFromOwnerMotion(unsigned motion) {
    switch(motion) {
    case 1: return 0; // QCF
    case 2: return 1; // QCB
    case 3: return 2; // DPF
    case 4: return 3; // DPB
    case 5: return 4; // HCF
    case 6: return 5; // HCB
    default: return -1;
    }
}
static inline Review review(unsigned fighter, unsigned ownerMotion) {
    const int col=columnFromOwnerMotion(ownerMotion);
    if (col<0) return UNKNOWN_MOTION;
    if (fighter>0x24u || (fighter&1u)) return NOT_REGISTERED;
    const Row& r=rows()[fighter/2u];
    if (!r.hyper[col]) return NO_HYPER;
    if (r.air & (1u<<col)) return AIR_ONLY;
    if (r.conditional & (1u<<col)) return CONDITIONAL;
    if (r.throwing & (1u<<col)) return POSSIBLE_THROW;
    return POSSIBLE;
}
static inline bool rejectBeforeTag(Review r) {
    // Fail open for unknown motions and nonregistered fighters.
    return r==NO_HYPER || r==AIR_ONLY || r==CONDITIONAL;
}
static inline bool enabled() {
    const char* v=std::getenv("XM87329_DHC_PREFLIGHT");
    return v && std::strcmp(v,"1")==0;
}
static inline const char* reason(Review r) {
    switch(r) {
    case UNKNOWN_MOTION: return "MORPHUS_MOTION_NOT_MAPPED";
    case POSSIBLE: return "MORPHUS_EURO_HYPER_CANDIDATE";
    case NOT_REGISTERED: return "MORPHUS_FIGHTER_NOT_REGISTERED";
    case NO_HYPER: return "MORPHUS_PRETAG_REJECT_NO_TARGET_HYPER";
    case AIR_ONLY: return "MORPHUS_PRETAG_REJECT_AIR_ONLY";
    case CONDITIONAL: return "MORPHUS_PRETAG_REJECT_CONDITIONAL";
    case POSSIBLE_THROW: return "MORPHUS_EURO_THROW_HYPER_CANDIDATE";
    }
    return "MORPHUS_UNKNOWN";
}
} // namespace XM87329Morphus
#endif
