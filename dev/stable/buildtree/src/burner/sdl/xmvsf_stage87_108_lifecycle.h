#ifndef XMVSF_STAGE87_108_LIFECYCLE_H
#define XMVSF_STAGE87_108_LIFECYCLE_H
// Stage 87.108: cosmetic lifecycle. ROM gameplay and palette bank unmodified.
namespace XM87108 {
// Verified match phases: 02..08 intro/combat, 0A knockout, 0C results.
// Keep an already selected custom stage visible until the native stage is
// cleared or selection begins. The caller still validates stage 05 & opt-in.
static inline bool StagePhase(unsigned status) {
    return status>=0x02u && status<=0x0cu;
}
// Source-owned priority-7 health tiles, not screen pixels, determine HP ink.
// Never lose blue just because the game moves from fight to KO/tag/entry.
static inline bool HudPhase(int status) {
    return status>=0x02;
}
}
#endif
